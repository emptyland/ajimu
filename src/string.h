#ifndef AJIMU_VALUES_STRING_H
#define AJIMU_VALUES_STRING_H

#include "glog/logging.h"
#include <stddef.h>
#include <string>

namespace ajimu {
namespace values {
//class ObjectManagement;
class Object;

struct String {
	int    ref;
	size_t hash;
	size_t len;
	char   land[1]; // Variable array
}; // Pure POD type

class StringHandle {
public:
	enum {
		MAX_POOL_STRING_LEN = 160,
	};

	StringHandle(String *core)
		: core_(DCHECK_NOTNULL(core)) {
	}

	const char *c_str() const {
		return core_->land;
	}

	std::string str() const {
		return std::move(std::string(core_->land, core_->len));
	}

	// Lazy hash compute
	size_t Hash() const {
		if (core_->hash == 0)
			core_->hash = ToHash(*core_);
		return core_->hash;
	}

	size_t Length() const {
		return core_->len;
	}

	bool InPool() const {
		return Length() <= MAX_POOL_STRING_LEN;
	}

	static String *New(const char *raw, size_t len) {
		size_t size = sizeof(String) + len;
		String *rv = reinterpret_cast<String *>(new char[size]);
		rv->ref  = 1;
		rv->hash = 0;
		rv->len  = len;
		memcpy(rv->land, raw, len); rv->land[len] = 0;
		return rv;
	}

	static size_t ToHash(const String &core) {
		long i = static_cast<long>(core.len);
		const char *z = core.land;
		size_t n = 1315423911U;
		while(i--)
			n ^= ((n << 5) + (*z++) + (n >> 2));
		return n | 1;
	}

	friend class Object;
private:
	void Free() {
		delete[] (reinterpret_cast<char *>(core_));
		core_ = nullptr;
	}

	void Drop() {
		DCHECK_GT(core_->ref, 0);
		if (--core_->ref == 0 && !InPool()) Free();
	}

	struct String *core_;
}; // class StringHandle

} // namespace values
} // namespace ajimu

#endif //AJIMU_VALUES_STRING_H

