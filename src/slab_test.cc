#include "nginx/ngx_slab.h"
#include "object.h"
#include "gmock/gmock.h"

namespace ajimu {
namespace values {

class NgxSlabTest : public ::testing::Test {
protected:
	virtual void SetUp() override {
		ngx_os_init();
		void *p = malloc(kPoolSize);
		memset(p, 0,  kPoolSize);

		pool_ = static_cast<ngx_slab_pool_t*>(p);
		pool_->end = static_cast<u_char*>(p) + kPoolSize;
		ngx_slab_init(pool_);
	}

	virtual void TearDown() override {
		free(pool_);
		pool_ = nullptr;
	}

	ngx_slab_pool_t *pool_;
	static const
	size_t kPoolSize = 1024 * 1024 + sizeof(ngx_slab_pool_t); // 1 MB
};

class FakeObject {
public:
	FakeObject() {
		memset(padding_, 0x11, sizeof(padding_));
	}

private:
	char padding_[sizeof(Object)];
};

static const int kNumAlloced = 40;
static const int kNumCount = 300000;

TEST_F(NgxSlabTest, Benchmark) {
	printf("sizeof(FakeObject): %zd\n", sizeof(FakeObject));
	for (int i = 0; i < kNumCount; ++i) {
		FakeObject *alloced[40];
		for (int j = 0; j < kNumAlloced; ++j) {
			alloced[j] = new (ngx_slab_alloc(pool_, sizeof(FakeObject)))
				FakeObject();
			ASSERT_NE(nullptr, alloced[j])
				<< "count: " << i << ", alloc: " << j;
		}
		for (auto elem : alloced) {
			elem->~FakeObject();
			ngx_slab_free(pool_, elem);
		}
	}
}

TEST(NewTest, Benchmark) {
	for (int i = 0; i < kNumCount; ++i) {
		FakeObject *alloced[40];
		for (int j = 0; j < kNumAlloced; ++j) {
			alloced[j] = new FakeObject();
			ASSERT_NE(nullptr, alloced[j])
				<< "count: " << i << ", alloc: " << j;
		}
		for (auto elem : alloced) {
			delete elem;
		}
	}
}

static const uintptr_t kFullMask = static_cast<uintptr_t>(-1);
static const int kBitCount = 600000;

TEST(ForLoopBit, BitOp) {
	for (int n = 0; n < kBitCount; ++n) {
		uintptr_t slab = 0, m, i;
		while (slab != kFullMask) {
			for (m = 1, i = 0; m; m <<= 1, i++) {
				if (slab & m)
					continue;
				slab |= m;
			}
		}
		ASSERT_EQ(slab, kFullMask);
	}
}

/*
static inline intptr_t fast_1st_zero(uintptr_t slab) {
	intptr_t rv;
	slab ^= (slab + 1);
	__asm__ __volatile__(
		"bsr %1, %0":
		"=r"(rv) : "r"(slab));	
	return rv;
}*/

TEST(FastBit, BitOp) {
	for (int n = 0; n < kBitCount; ++n) {
		uintptr_t slab = 0, m;
		intptr_t i = 0;
		while (slab != kFullMask) {
			m = slab ^ (slab + 1);
			__asm__ __volatile__(
				"bsr %1, %0":
				"=r"(i) : "r"(m));
			m = 1 << i;
			slab |= m;
		}
		ASSERT_EQ(slab, kFullMask);
	}
}

static const int kLog2[256] = {
	0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};

TEST(FastBit2, BitOp) {
	for (int n = 0; n < kBitCount; ++n) {
		uintptr_t slab = 0, m;
		intptr_t i;
		while (slab != kFullMask) {
			int l = -1;
			m = slab ^ (slab + 1);
			while (m >= 256) { l += 8; m >>= 8; }
			i = l + kLog2[m];
			slab |= (1 << i);
		}
		ASSERT_EQ(slab, kFullMask);
	}
}

} // namespace values
} // namespace ajimu

