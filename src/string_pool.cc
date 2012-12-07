#include "string_pool.h"
#include "string.h"
#include "reachable.h"

namespace ajimu {
namespace values {

StringPool::StringPool()
	: slot_(nullptr)
	, large_list_(nullptr)
	, shift_(8)
	, used_(0)
	, allocated_(0) {
	slot_ = NewSlot(SlotSize());
}

StringPool::~StringPool() {
	// Delete pool strings
	Reachable **k = slot_ + SlotSize();
	shift_ = 0;
	for (Reachable **i = slot_; i != k; ++i) {
		if (*i) {
			Reachable *x = *i, *p = x;
			while (x) {
				p = x;
				x = x->next_;
				String::Delete(static_cast<String*>(p));
				--used_;
			}
		}
	}
	DCHECK_EQ(0, used_);
	delete[] slot_;

	// Delete large strings
	Reachable *i = large_list_, *p;
	while (i) {
		p = i;
		i = i->next_;
		String::Delete(static_cast<String*>(p));
	}
}

String *StringPool::NewString(const char *raw, size_t len,
		unsigned white) {
	if (len > String::MAX_POOL_STRING_LEN) {
		String *rv = String::New(raw, len, large_list_, white);
		allocated_ += rv->Allocated();
		large_list_ = rv;
		return rv;
	}
	DCHECK_NOTNULL(slot_);

	Reachable *list = slot_[String::ToHash(raw, len) % SlotSize()];
	for (Reachable *i = list; i != nullptr; i = i->next_) {
		String *x = static_cast<String *>(i);
		if (x->Length() == len && x->Equal(raw, len))
			return x;
	}
	return Append(raw, len, white);
}

String *StringPool::Append(const char *raw, size_t len, unsigned white) {
	if (used_ >= SlotSize())
		Resize(shift_ + 1);
	size_t hash = String::ToHash(raw, len);
	Reachable **list = slot_ + hash % SlotSize();
	String *o = String::New(raw, len, *list, white);
	allocated_ += o->Allocated();
	*list = o;
	++used_;
	return o;
}

void StringPool::Resize(int shift) {
	Reachable **bak = slot_;
	Reachable **k = bak + SlotSize();
	slot_ = NewSlot(1 << shift);
	for (Reachable **i = bak; i != k; ++i) {
		if (*i) {
			Reachable *x = *i;
			while (x) {
				Reachable *next = x->next_;
				Copy2(slot_, x, shift);
				x = next;
			}
		}
	}
	delete[] bak;
	shift_ = shift;
}

void StringPool::Copy2(Reachable **slot, Reachable *x, int shift) {
	int k = 1 << shift;
	String *o = static_cast<String*>(x);
	Reachable **list = slot + o->Hash() % k;
	x->next_ = *list;
	*list = x;
}

int StringPool::Sweep(unsigned white) {
	int sweeped = 0;

	// Sweep the large strings:
	if (large_list_) {
		Reachable prev(large_list_, white);
		sweeped += DoSweep(large_list_, &prev, white);
		large_list_ = prev.next_;
	}

	// Sweep the hashed strings:
	Reachable **k = slot_ + SlotSize();
	for (Reachable **i = slot_; i != k; ++i) {
		if (!*i)
			continue;
		Reachable prev(*i, white);
		int rv = DoSweep(*i, &prev, white);
		used_ -= rv;
		sweeped += rv;
		*i = prev.next_;
	}
	return sweeped;
}

//
// x: header node
// p: prev   node
int StringPool::DoSweep(Reachable *x, Reachable *p, unsigned white) {
	int sweeped = 0;
	while (x) {
		if (x->TestInvWhite(white)) {
			p->next_ = x->next_;
			allocated_ -= String::Delete(static_cast<String*>(x));
			++sweeped;
			x = p->next_;
		} else {
			x->ToWhite(white);
			p = x;
			x = x->next_;
		}
	}
	return sweeped;
}

} // namespace values
} // namespace ajimu

