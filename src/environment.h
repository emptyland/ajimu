#ifndef AJIMU_VM_ENVIRONMENT_H
#define AJIMU_VM_ENVIRONMENT_H

#include "glog/logging.h"
#include <string>
#include <unordered_map>

namespace ajimu {
namespace values {
class Object;
} // namespace values
namespace vm {

class Environment {
public:
	class Handle;

	Environment(Environment *top)
		: top_(top) {
	}

	~Environment() {}

	void Define(const std::string &name, values::Object *val) {
		var_[name] = DCHECK_NOTNULL(val);
	}

	values::Object *Lookup(const std::string &name) const {
		auto iter = var_.find(name);
		return iter != var_.end() ? iter->second : nullptr;
	}

	Environment *Next() const {
		return top_;
	}

private:
	Environment(const Environment &) = delete;
	void operator = (const Environment &) = delete;

	Environment *top_;
	std::unordered_map<std::string, values::Object*> var_;
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

