#ifndef AJIMU_VM_LOCAL_H
#define AJIMU_VM_LOCAL_H

#include "glog/logging.h"
#include <deque>

namespace ajimu {
namespace values {
class Object;
} // namespace values
namespace vm {
class Environment;

template<class T>
class Local {
public:
	class Persisted;

	Local() {}

	~Local() {
		DCHECK_EQ(0U, val_.size())
			<< "Stack is not balanced! Remain:"
			<< val_.size();
	}

	const std::deque<T*> &Values() const {
		return val_;
	}

	size_t Count() const {
		return val_.size();
	}

	void Push(T *o) {
		val_.push_back(o);
	}

	void Pop(size_t n) {
		DCHECK_GE(Count(), n);
		val_.resize(Count() - n);
	}

	T *Last(size_t i) const {
		DCHECK_GT(Count(), i);
		return val_.at(Count() - i - 1);
	}

private:
	Local(const Local &) = delete;
	void operator = (const Local &) = delete;

	std::deque<T*> val_;
}; // class Local

template<class T>
class Local<T>::Persisted {
public:
	Persisted(Local *local)
		: local_(local)
		, keeped_(local->Count()) {
	}

	~Persisted() {
		DCHECK_GE(local_->Count(), keeped_);
		local_->Pop(local_->Count() - keeped_);
	}

private:
	Persisted(const Persisted &) = delete;
	void operator = (const Persisted &) = delete;

	Local *local_;
	size_t keeped_;
}; // class Local::Persisted

} // namespace vm
} // namespace ajimu

#endif //AJIMU_VM_LOCAL_H

