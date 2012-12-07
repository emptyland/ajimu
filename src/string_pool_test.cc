#include "string_pool.h"
#include "string.h"
#include "gmock/gmock.h"
#include "reachable.h"

namespace ajimu {
namespace values {

class StringPoolTest : public ::testing::Test {
protected:
	virtual void SetUp() override {
		srand(time(0));
		pool_ = new StringPool();
	}

	virtual void TearDown() override {
		delete pool_;
		pool_ = nullptr;
	}

	StringPool *pool_;
};

class PoolSweepingTest : public StringPoolTest
                       , public ::testing::WithParamInterface<const char*> {
protected:
	static std::string RandomParam() {
		// character: 32, 126 - 94
		// length:    60, 260
		std::string param;
		param.resize(60 + rand() % 200);
		for (char &c : param)
			c = 32 + rand() % 94;
		return std::move(param);
	}
};

TEST_F(StringPoolTest, Sanity) {
	String *s = String::New("Hello", nullptr, 1);
	ASSERT_NE(0U, s->Hash());
	ASSERT_EQ(5U, s->Length());
	ASSERT_STREQ("Hello", s->c_str());

	String::Delete(s);
}

TEST_F(StringPoolTest, UniqueString) {
	static const char *k = "World!";

	int i = 1000;
	const String *expected = pool_->NewString(k,
			Reachable::WHITE_BIT0);
	while (i--) {
		String *s = pool_->NewString(k, Reachable::WHITE_BIT0);
		ASSERT_EQ(expected, s);
		ASSERT_STREQ(k, s->c_str());
	}
}

TEST_F(StringPoolTest, UniqueString2) {
	static const char *kList[] = {
		"a",
		"aaaa",
		"aaaaa",
		"Hello",
		"World!",
		"",
		"b",
		"bb",
		"bbc",
		"bbbc",
	};
	for (auto k : kList) {
		String *s = pool_->NewString(k, Reachable::WHITE_BIT0);
		ASSERT_NE(nullptr, s);
		ASSERT_STREQ(k, s->c_str());
	}
	ASSERT_EQ(sizeof(kList)/sizeof(kList[0]), pool_->HashedCount());
}

TEST_F(StringPoolTest, LargeString) {
	std::unique_ptr<char[]> list[4];
	size_t len = String::MAX_POOL_STRING_LEN + 2;
	char c = 'a';
	for (std::unique_ptr<char[]> &elem : list) {
		elem.reset(new char[len]);
		memset(elem.get(), c++, len);
	}

	String *old[4] = {0};
	for (int i = 0; i < 4; ++i) {
		String *s = pool_->NewString(list[i].get(), len,
				Reachable::WHITE_BIT0);
		ASSERT_NE(nullptr, s);
		ASSERT_TRUE(s->Equal(list[i].get(), len));
		old[i] = s;
	}

	for (int i = 0; i < 4; ++i) {
		String *s = pool_->NewString(list[i].get(), len,
				Reachable::WHITE_BIT0);
		ASSERT_NE(nullptr, s);
		ASSERT_NE(old[i], s);
		ASSERT_TRUE(s->Equal(list[i].get(), len));
	}
	ASSERT_EQ(0U, pool_->HashedCount());
}

TEST_F(StringPoolTest, Sweeping) {
	size_t len = String::MAX_POOL_STRING_LEN + 2;
	std::unique_ptr<char[]> large(new char[len]);
	memset(large.get(), 'a', len);

	String *s = pool_->NewString(large.get(), len,
			Reachable::WHITE_BIT0);
	ASSERT_TRUE(s->Equal(large.get(), len));

	static const char *k = "Hello, World!";
	s = pool_->NewString(k, Reachable::WHITE_BIT0);
	ASSERT_STREQ(k, s->c_str());

	ASSERT_EQ(0, pool_->Sweep(Reachable::WHITE_BIT0));
	ASSERT_EQ(2, pool_->Sweep(Reachable::WHITE_BIT1));
}

const char *kSweepingValues[] = {
	"a",
	"b",
	"bb",
	"bbc",
	"Hello, world!",
	"",
};

INSTANTIATE_TEST_CASE_P(PoolSweeping,
		PoolSweepingTest,
		::testing::ValuesIn(kSweepingValues));

TEST_P(PoolSweepingTest, Sweeping) {
	String *s = pool_->NewString(GetParam(), Reachable::WHITE_BIT0);
	ASSERT_STREQ(GetParam(), s->c_str());

	ASSERT_EQ(0, pool_->Sweep(Reachable::WHITE_BIT0));
	ASSERT_EQ(1, pool_->Sweep(Reachable::WHITE_BIT1));
}

TEST_F(PoolSweepingTest, RandomizeSweeping) {
	int i = 100000;
	while (i--) {
		std::string param(std::move(RandomParam()));
		String *s = pool_->NewString(param.c_str(), param.size(),
				Reachable::WHITE_BIT0);
		ASSERT_TRUE(s->Equal(param.c_str(), param.size()));
	}
	printf("Hashed: %zd\n", pool_->HashedCount());
	ASSERT_EQ(0, pool_->Sweep(Reachable::WHITE_BIT0));
	ASSERT_LT(0, pool_->Sweep(Reachable::WHITE_BIT1));
}

} // namespace values
} // namespace ajimu

