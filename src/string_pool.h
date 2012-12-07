#ifndef AJIMU_VALUES_STRING_POOL_H
#define AJIMU_VALUES_STRING_POOL_H

#include <string>
#include <string.h>

namespace ajimu {
namespace values {
class String;
class Reachable;

class StringPool {
public:
	StringPool();

	~StringPool();

	String *NewString(const char *raw, size_t len, unsigned white);

	String *NewString(const char *raw, unsigned white) {
		return NewString(raw, strlen(raw), white);
	}

	String *Append(const char *raw, size_t len, unsigned white);

	void Resize(int shift);

	int SlotSize() const {
		return (1 << shift_);
	}

	size_t HashedCount() const {
		return static_cast<size_t>(used_);
	}

	size_t Allocated() const {
		return allocated_;
	}

	int Sweep(unsigned white);

private:
	StringPool(const StringPool &) = delete;
	void operator = (const StringPool &) = delete;

	void Copy2(Reachable **slot, Reachable *x, int shift);

	static Reachable **NewSlot(int n) {
		Reachable **rv = new Reachable*[n];
		memset(rv, 0, n * sizeof(*rv));
		return rv;
	}

	int DoSweep(Reachable *head, Reachable *prev, unsigned white);

	Reachable **slot_;
	Reachable *large_list_;
	int shift_;
	int used_;
	size_t allocated_;
};

} // namespace values
} // namespace ajimu

#endif //AJIMU_VALUES_STRING_POOL_H

