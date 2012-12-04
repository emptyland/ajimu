#ifndef AJIMU_UTILS_UTILS_H
#define AJIMU_UTILS_UTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <string>

namespace ajimu {
namespace utils {

template<class T>
class HandleBase {
public:
	HandleBase()
		: naked_(nullptr) {
	}

	HandleBase(T *naked)
		: naked_(naked) {
	}

	T *Get() const {
		return DCHECK_NOTNULL(naked_);
	}

	T *Release() {
		T *rv = naked_; naked_ = nullptr;
		return DCHECK_NOTNULL(rv);
	}

	bool Valid() const {
		return naked_ != nullptr;
	}

private:
	T *naked_;
};

template<class T>
class Handle : public HandleBase<T> {};

template<>
class Handle<FILE> : public HandleBase<FILE> {
public:
	Handle(FILE *fp)
		: HandleBase<FILE>(fp) {
	}

	~Handle() {
		if (this->Valid())
			fclose(this->Release());
	}

private:
	Handle(const Handle &) = delete;
	void operator = (const Handle &) = delete;
};

// String utils:
std::string Formatf(const char *fmt, ... ) {
	char buf[1024] = {0};
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	return std::move(std::string(buf));
}

} // namespace utils
} // namespace ajimu

#endif //AJIMU_UTILS_UTILS_H

