#ifndef AJIMU_VM_MACH_H
#define AJIMU_VM_MACH_H

#include <memory>
#include <functional>
#include <vector>

namespace ajimu {
namespace values {
class ObjectManagement;
class Object;
} // namespace values
namespace vm {
class Lexer;
class Environment;

class Mach {
public:
	typedef std::function<void (const char *, Mach *)> Observer;

	Mach();

	~Mach();

	bool Init();

	void AddObserver(const Observer &callable) {
		observer_.push_back(callable);
	}

	int Error() {
		return error_;
	}

	Environment *GlobalEnvironment() {
		return global_env_;
	}

	values::Object *Feed(const char *input, size_t len);

	values::Object *Feed(const char *input);

	values::Object *Eval(values::Object *expr, Environment *env);

	values::Object *LookupVariable(values::Object *expr,
			Environment *env);
	values::Object *EvalDefinition(values::Object *expr,
			Environment *env);
	values::Object *EvalAssignment(values::Object *expr,
			Environment *env);
	values::Object *ListOfValues(values::Object *operand,
			Environment *env);

	Environment *ExtendEnvironment(values::Object *params,
			values::Object *args, Environment *base);
private:
	Mach(const Mach &) = delete;
	void operator = (const Mach &) = delete;

	void RaiseError(const char *err);

	void RaiseErrorf(const char *fmt, ...);

	//
	// Primitive Proc(s)
	//
	values::Object *Add(values::Object *args);
	values::Object *Dec(values::Object *args);
	values::Object *Mul(values::Object *args);
	values::Object *Div(values::Object *args);
	values::Object *NumberEqual(values::Object *args);
	values::Object *NumberGreat(values::Object *args);
	values::Object *NumberLess(values::Object *args);

	std::unique_ptr<values::ObjectManagement> obm_;
	std::unique_ptr<Lexer> lex_;
	std::vector<Observer> observer_;
	Environment *global_env_;
	int error_;
}; // class Mach

} // namespace vm
} // namespace ajimu

#endif //AJIMU_VM_MACH_H

