#ifndef AJIMU_VM_ENVIRONMENT_H
#define AJIMU_VM_ENVIRONMENT_H

#include "reachable.h"
#include "glog/logging.h"
#include <string>
#include <unordered_map>

namespace ajimu {
namespace values {
class Reachable;
class Object;
} // namespace values
namespace vm {

class Environment : public values::Reachable {
public:
	class Handle;

	Environment(Environment *top) // Only for test
		: values::Reachable(nullptr, 0)
		, top_(top) {
	}

	Environment(Environment *top, values::Reachable *next, unsigned white)
		: values::Reachable(next, white)
		, top_(top) {
	}

	~Environment() {}

	size_t Define(const std::string &name, values::Object *val) {
		auto iter = map_.find(name);
		if (iter == map_.end()) {
			var_.push_back(DCHECK_NOTNULL(val));
			map_.insert(std::make_pair(name, var_.size() - 1));
		} else {
			var_[iter->second] = DCHECK_NOTNULL(val);
			return iter->second;
		}
		return var_.size() - 1;
	}

	values::Object *Lookup(const std::string &name) const {
		auto iter = map_.find(name);
		if (iter == map_.end())
			return nullptr;
		return var_[iter->second];
	}

	Environment *Next() const {
		return top_;
	}

	size_t Count() const {
		return var_.size();
	}

	values::Object *At(size_t i) const {
		DCHECK_LE(i, var_.size());
		return var_.at(i);
	}

	const std::vector<values::Object*> &Variable() const {
		return var_;
	}

private:
	Environment(const Environment &) = delete;
	void operator = (const Environment &) = delete;

	Environment *top_;
	std::unordered_map<std::string, size_t> map_; // <name, index>
	std::vector<values::Object*> var_;
}; // class Environment

class Environment::Handle {
public:
	Handle(const std::string &name, Environment *env)
		: found_(nullptr)
		, cur_(env) {
		Lookup(name, env);
	}

	values::Object *Get() const {
		return found_;
	}

	bool Valid() const {
		return found_ != nullptr;
	}

	void Set(const std::string &name, values::Object *val) {
		cur_->Define(name, val);
	}

private:
	Handle(const Handle &) = delete;
	void operator = (const Handle &) = delete;

	void Lookup(const std::string &name, Environment *env) {
		while (!found_) {
			found_ = env->Lookup(name);
			cur_   = env;
			if (!found_)
				env = env->Next();
			if (!env)
				break;
		}
	}

	values::Object *found_;
	Environment *cur_;
}; // class Environment::Handle

} // namespace vm
} // namespace ajimu

#endif //AJIMU_VM_ENVIRONMENT_H

