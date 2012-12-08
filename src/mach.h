#ifndef AJIMU_VM_MACH_H
#define AJIMU_VM_MACH_H

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <stack>

namespace ajimu {
namespace values {
class ObjectManagement;
class Object;
} // namespace values
namespace vm {
class Lexer;
class Environment;
template<class T> class Local;

class Mach {
public:
	typedef std::function<void (const char *, Mach *)> Observer;

	Mach();

	~Mach();

	bool Init();

	void AddObserver(const Observer &callable) {
		observer_.push_back(callable);
	}

	int Error() const {
		return error_;
	}

	int Line() const;

	const char *File() const {
		return file_level_.empty() ? "" : file_level_.top().c_str();
	}

	Environment *GlobalEnvironment() {
		return global_env_;
	}

	Lexer *Lex() const {
		return lex_.get();
	}

	values::ObjectManagement *Obm() {
		return obm_.get();
	}

	// Feed script for eval
	values::Object *Feed(const char *input, size_t len);

	values::Object *Feed(const char *input);

	// Eval a file
	values::Object *EvalFile(const char *filename);

	// Primitive eval
	values::Object *Eval(values::Object *expr, Environment *env);

private:
	Mach(const Mach &) = delete;
	void operator = (const Mach &) = delete;

	values::Object *LookupVariable(values::Object *expr,
			Environment *env);

	values::Object *EvalDefinition(values::Object *expr,
			Environment *env);

	values::Object *EvalAssignment(values::Object *expr,
			Environment *env);

	values::Object *ListOfValues(values::Object *operand,
			Environment *env);

	values::Object *ExpandClauses(values::Object *clauses);

	Environment *ExtendEnvironment(values::Object *params,
			values::Object *args, Environment *base);

	// Operating for local
	void Push(values::Object *o);

	void Pop(size_t n);

	values::Object *Last(size_t i) const;

	// Error output:
	void RaiseError(const char *err);

	void RaiseErrorf(const char *fmt, ...);

	//
	// Primitive Procedures:
	//
	values::Object *Add(values::Object *args);
	values::Object *Dec(values::Object *args);
	values::Object *Mul(values::Object *args);
	values::Object *Div(values::Object *args);
	values::Object *NumberEqual(values::Object *args);
	values::Object *NumberGreat(values::Object *args);
	values::Object *NumberLess(values::Object *args);
	values::Object *Display(values::Object *args);
	values::Object *Cons(values::Object *args);
	values::Object *Car(values::Object *args);
	values::Object *Cdr(values::Object *args);
	values::Object *List(values::Object *args);
	values::Object *SetCar(values::Object *args);
	values::Object *SetCdr(values::Object *args);
	values::Object *Load(values::Object *args);
	values::Object *IsBoolean(values::Object *args);
	values::Object *IsSymbol(values::Object *args);
	values::Object *IsChar(values::Object *args);
	values::Object *IsVector(values::Object *args);
	values::Object *IsPort(values::Object *args);
	values::Object *IsNull(values::Object *args);
	values::Object *IsPair(values::Object *args);
	values::Object *IsInteger(values::Object *args);
	values::Object *IsFloat(values::Object *args);
	values::Object *IsString(values::Object *args);
	values::Object *IsByteVector(values::Object *args);
	values::Object *IsProcedure(values::Object *args);
	values::Object *Error(values::Object *args);

	// Extension Primitive Procedures:
	values::Object *AjimuGcAllocated(values::Object *args);
	values::Object *AjimuGcState(values::Object *args);

	std::unique_ptr<values::ObjectManagement> obm_;
	std::unique_ptr<Local<values::Object>> local_val_;
	std::unique_ptr<Local<Environment>>    local_env_;
	std::unique_ptr<Lexer> lex_;
	std::stack<std::string> file_level_;
	std::vector<Observer> observer_;
	Environment *global_env_;
	int error_;
	int call_level_;
}; // class Mach

} // namespace vm
} // namespace ajimu

#endif //AJIMU_VM_MACH_H

