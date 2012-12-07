#ifndef AJIMU_VALUES_STRING_H
#define AJIMU_VALUES_STRING_H

#include "reachable.h"
#include "glog/logging.h"
#include <stddef.h>
#include <string.h>
#include <string>
#include <memory>

namespace ajimu {
namespace values {

class String : public Reachable {
public:
	enum {
		MAX_POOL_STRING_LEN = 160,
	};

	const char *c_str() const {
		return Data();
	}

	std::string str() const {
		return std::move(std::string(Data(), Length()));
	}

	const char *Data() const {
		return land_;
	}

	size_t Hash() {
		if (hash_ == 0U)
			hash_ = ToHash(Data(), Length());
		return hash_;
	}

	size_t Length() const {
		return len_;
	}

	size_t Allocated() const {
		return Length() + sizeof(String);
	}

	bool Equal(const char *raw, size_t len) const {
		return memcmp(Data(), raw, len) == 0;
	}

	static String *New(const char *naked, size_t len,
			Reachable *next, unsigned white) {
		void *blob = new char[sizeof(String) + len + 1];
		return ::new (blob) String(naked, len, next, white);
	}

	static String *New(const char *naked,
			Reachable *next, unsigned white) {
		return New(naked, strlen(naked), next, white);
	}

	static size_t Delete(const String *o) {
		union {
			const char   *raw;
			const String *obj;
		};
		size_t rv = o->Allocated();
		obj = o;
		obj->~String();
		delete[] raw;
		return rv;
	}

	static size_t ToHash(const char *z, size_t len) {
		size_t n = 1315423911U;
		while(len--)
			n ^= ((n << 5) + (*z++) + (n >> 2));
		return n | 1;
	}

private:
	String(const String &) = delete;
	void operator = (const String &) = delete;

	String(const char *naked, size_t len, Reachable *next, unsigned white)
		: Reachable(next, white)
		, hash_(0)
		, len_(len) {
		memcpy(land_, naked, len);
		land_[len] = '\0';
	}

	size_t hash_;
	size_t len_;
	char land_[1]; // MUST to last one!
};

} // namespace values
} // namespace ajimu

#endif //AJIMU_VALUES_STRING_H

