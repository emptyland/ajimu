#include "mach.h"
#include "object_management.h"
#include "object.h"
#include "environment.h"
#include "lexer.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace ajimu {
namespace vm {

using ::ajimu::values::Object;
using ::ajimu::values::ObjectManagement;

inline values::PrimitiveMethodPtr UnsafeCast2Method(intptr_t i) {
	union {
		intptr_t n;
		values::PrimitiveMethodPtr fn;
	};
	n = i; return fn;
}

static auto kApply = UnsafeCast2Method(1);
static auto kEval =  UnsafeCast2Method(2);

#define Kof(i) obm_->Constant(::ajimu::values::ObjectManagement::k##i)
inline bool IsTaggedList(Object *expr, Object *tag) {
	Object  *car;
	if (expr->IsPair()) {
		car = expr->Car();
		return car->IsSymbol() && car == tag;
	}
	return false;
}

inline bool IsSelfEvaluating(Object *expr) {
	switch (expr->OwnedType()) {
	case values::BOOLEAN:
	case values::FIXED:
	case values::CHARACTER:
	case values::STRING:
		return true;
	default:
		break;
	}
	return false;
}

inline bool IsVariable(Object *expr) {
	return expr->IsSymbol();
}

inline bool IsDefinition(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(DefineSymbol));
}

inline bool IsLambda(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(LambdaSymbol));
}

inline bool IsBegin(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(BeginSymbol));
}

inline Object *MakeLambda(Object *params, Object *body,
		ObjectManagement *obm_) {
	return obm_->Cons(Kof(LambdaSymbol), obm_->Cons(params, body));
}

inline Object *PrepareApplyOperands(Object *args, ObjectManagement *obm_) {
	if (args->Cdr() == Kof(EmptyList))
		return args->Car();
	return obm_->Cons(args->Car(),
			PrepareApplyOperands(args->Cdr(), obm_));
}
//-----------------------------------------------------------------------------
// Class : Mach
//-----------------------------------------------------------------------------
Mach::Mach() 
	: global_env_(nullptr)
	, error_(0) {
}

Mach::~Mach() {
}

bool Mach::Init() {
	struct PrimitiveEntry {
		const char *name;
		values::PrimitiveMethodPtr method;
	} kProcs[] = {
		{ "+", &Mach::Add, },
		{ "-", &Mach::Dec, },
		{ "*", &Mach::Mul, },
		{ "/", &Mach::Div, },

		{ "apply", kApply, },
		{ "eval",  kEval,  },
	};

	obm_.reset(new ObjectManagement());
	obm_->Init();
	// Register primitive proc(s)
	for (PrimitiveEntry &i : kProcs) {
		obm_->NewPrimitive(i.name, i.method);
	}
	
	// Initialize Lexer
	lex_.reset(new Lexer(obm_.get()));
	lex_->AddObserver([this] (const char *err, Lexer *) {
		RaiseError(err);
	});
	global_env_ = obm_->GlobalEnvironment();
	return true;
}

Object *Mach::Feed(const char *input, size_t len) {
	lex_->Feed(input, len);
	values::Object *o, *rv = nullptr;
	while ((o = lex_->Next()) != nullptr) {
		rv = Eval(o, global_env_);
		if (!rv)
			return nullptr;
	}
	return rv;
}

Object *Mach::Feed(const char *input) {
	return Feed(input, strlen(input));
}

Object *Mach::Eval(Object *expr, Environment *env) {
tailcall:
	// Sample statement
	if (IsSelfEvaluating(expr))
		return expr;
	if (IsVariable(expr))
		return LookupVariable(expr, env);
	if (IsDefinition(expr, obm_.get()))
		return EvalDefinition(expr, env);
	if (IsLambda(expr, obm_.get()))
		return obm_->NewClosure(cadr(expr), cddr(expr), env);

	// Begin block
	if (IsBegin(expr, obm_.get())) {
		expr = expr->Cdr(); // cdr: begin action
		while (expr->Cdr() != Kof(EmptyList)) { // Is last action?
			Eval(expr->Car(), env); // car: first action
			expr = expr->Cdr();     // cdr: rest actions
		}
		expr = expr->Car();
		goto tailcall;
	}

	// Exec block
	if (expr->IsPair()) { // Is application ?
		Object *proc = Eval(expr->Car(), env);         // car: operator
		Object *args = ListOfValues(expr->Cdr(), env); // cdr: operands

		if (proc->IsPrimitive() && proc->Primitive() == kApply) {
			proc = args->Car();
			args = PrepareApplyOperands(args->Cdr(), obm_.get());
		}
		if (proc->IsPrimitive()) {
			auto fn = proc->Primitive();
			if (fn == kEval) {
				expr = args->Car();
				// TODO env  = MakeEnvironment() cadr
				goto tailcall;
			}
			return (this->*fn)(args);
		}
		if (proc->IsClosure()) {
			env = ExtendEnvironment(proc->Params(), args, proc->Environment());
			// Make begin block
			expr = obm_->Cons(Kof(BeginSymbol), proc->Body());
			goto tailcall;
		}
		RaiseError("Unknown procedure type.");
		return nullptr;
	}
	RaiseError("Bad eval! no one can be evaluated.");
	return nullptr;
}

Object *Mach::LookupVariable(Object *expr, Environment *env) {
	Environment::Handle handle(expr->Symbol(), env);
	if (!handle.Valid())
		RaiseErrorf("Unbound variable, %s.", expr->Symbol());
	return handle.Get();
}

Object *Mach::EvalDefinition(Object *expr, Environment *env) {
	Object *var, *val;
	if (cadr(expr)->IsSymbol()) {
		var = cadr(expr);
		val = caddr(expr);
	} else {
		var = caadr(expr);
		val = MakeLambda(cdadr(expr), cddr(expr), obm_.get());
	}
	val = Eval(val, env);
	env->Define(var->Symbol(), val);
	return Kof(OkSymbol);
}

Object *Mach::ListOfValues(Object *operand, Environment *env) {
	if (operand == Kof(EmptyList))
		return Kof(EmptyList);

	return obm_->Cons(Eval(operand->Car(), env), // car: first operand
			ListOfValues(operand->Cdr(), env));  // cdr: rest operands
}

Environment *Mach::ExtendEnvironment(Object *params, Object *args,
		Environment *top) {
	Environment *env = obm_->NewEnvironment(top);
	while (params != Kof(EmptyList)) {
		if (!params->Car()->IsSymbol()) {
			RaiseError("Parameters must be symbol.");
			return nullptr;
		}
		if (args != Kof(EmptyList)) {
			env->Define(params->Car()->Symbol(), args->Car());		
			args = args->Cdr();
		} else {
			// TODO: Use `nil' object
			env->Define(params->Car()->Symbol(), nullptr);
		}
		params = params->Cdr();
	}
	return env;
}

void Mach::RaiseError(const char *err) {
	++error_;
	if (observer_.empty())
		fprintf(stderr, "Unhandled error : %s\n", err);
	for (Observer fn : observer_)
		fn(err, this);
}

void Mach::RaiseErrorf(const char *fmt, ...) {
	va_list ap;
	char buf[1024] = {0};

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	RaiseError(buf);
}

//
// Primitive Proc(s)
//
Object *Mach::Add(Object *args) {
	long long rv = 0;
	while (args != Kof(EmptyList)) {
		rv += args->Car()->Fixed();
		args = args->Cdr();
	}
	return obm_->NewFixed(rv);
}

Object *Mach::Dec(Object *args) {
	long long rv = args->Car()->Fixed();
	while ((args = args->Cdr()) != Kof(EmptyList))
		rv -= args->Car()->Fixed();
	return obm_->NewFixed(rv);
}

Object *Mach::Mul(Object *args) {
	long long rv = 1;
	while (args != Kof(EmptyList)) {
		rv *= args->Car()->Fixed();
		args = args->Cdr();
	}
	return obm_->NewFixed(rv);
}

Object *Mach::Div(Object *args) {
	long long rv = args->Car()->Fixed();
	while ((args = args->Cdr()) != Kof(EmptyList)) {
		if (args->Car()->Fixed() == 0) {
			RaiseError("Div zeor!");
			return nullptr;
		}
		rv /= args->Car()->Fixed();
	}
	return obm_->NewFixed(rv);
}

} // namespace vm
} // namespace ajimu

