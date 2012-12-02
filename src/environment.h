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
	typedef std::unordered_map<std::string, values::Object*> Map;

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
	Map var_;
}; // class Environment

class Environment::Handle {
public:
	Handle(const std::string &name, Environment *env)
		: found_(nullptr) {
		Lookup(name, env);
	}

	values::Object *Get() const {
		return found_;
	}

	bool Valid() const {
		return found_ != nullptr;
	}

private:
	Handle(const Handle &) = delete;
	void operator = (const Handle &) = delete;

	void Lookup(const std::string &name, Environment *env) {
		while (!found_) {
			found_ = env->Lookup(name);
			if (!found_)
				env = env->Next();
			if (!env)
				break;
		}
	}

	values::Object *found_;
}; // class Environment::Handle

} // namespace vm
} // namespace ajimu

#endif //AJIMU_VM_ENVIRONMENT_H

