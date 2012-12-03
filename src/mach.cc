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
	Object  *first;
	if (expr->IsPair()) {
		first = car(expr);
		return first->IsSymbol() && first == tag;
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

inline bool IsQuoted(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(QuoteSymbol));
}

inline bool IsAssignment(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(SetSymbol));
}

inline bool IsIf(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(IfSymbol));
}

inline bool IsLet(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(LetSymbol));
}

inline bool IsAnd(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(AndSymbol));
}

inline bool IsOr(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(OrSymbol));
}

inline bool IsCond(Object *expr, ObjectManagement *obm_) {
	return IsTaggedList(expr, Kof(CondSymbol));
}

inline Object *MakeLambda(Object *params, Object *body,
		ObjectManagement *obm_) {
	return obm_->Cons(Kof(LambdaSymbol), obm_->Cons(params, body));
}

inline Object *MakeIf(Object *pred, Object *cons, Object *alter,
		ObjectManagement *obm_) {
	return obm_->Cons(Kof(IfSymbol),
				obm_->Cons(pred,
					obm_->Cons(cons,
						obm_->Cons(alter, Kof(EmptyList)))));
}

inline Object *PrepareApplyOperands(Object *args, ObjectManagement *obm_) {
	if (cdr(args) == Kof(EmptyList))
		return car(args);
	return obm_->Cons(car(args),
			PrepareApplyOperands(cdr(args), obm_));
}

inline Object *BindingsParams(Object *bindings, ObjectManagement *obm_) {
	return bindings == Kof(EmptyList) ?
		Kof(EmptyList) :
		obm_->Cons(caar(bindings), // caar: binding parameter
				BindingsParams(cdr(bindings), obm_));
}

inline Object *BindingsArgs(Object *bindings, ObjectManagement *obm_) {
	return bindings == Kof(EmptyList) ?
		Kof(EmptyList) :
		obm_->Cons(cadar(bindings), // cadar: binding argument
				BindingsArgs(cdr(bindings), obm_));
}

inline Object *SequenceToExpr(Object *seq, ObjectManagement *obm_) {
	if (seq == Kof(EmptyList))
		return Kof(EmptyList);
	if (cdr(seq) == Kof(EmptyList)) // cdr: last expr
		return car(seq); // car: first expr
	return obm_->Cons(Kof(BeginSymbol), seq); // make begin
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
		// Arithmetic procedures:
		{ "+", &Mach::Add, },
		{ "-", &Mach::Dec, },
		{ "*", &Mach::Mul, },
		{ "/", &Mach::Div, },
		{ "=", &Mach::NumberEqual, },
		{ ">", &Mach::NumberGreat, },
		{ "<", &Mach::NumberLess,  },

		{ "apply", kApply, },
		{ "eval",  kEval,  },

		// List procedures:
		{ "cons", &Mach::Cons, },
		{ "car",  &Mach::Car,  },
		{ "cdr",  &Mach::Cdr,  },
		{ "list", &Mach::List, },
		{ "set-car!", &Mach::SetCar, },
		{ "set-cdr!", &Mach::SetCdr, },
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

int Mach::Line() const {
	return lex_->Line();
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
	if (IsQuoted(expr, obm_.get()))
		return cadr(expr); // Text of quotation
	if (IsAssignment(expr, obm_.get()))
		return EvalAssignment(expr, env);

	// If statement
	if (IsIf(expr, obm_.get())) {
		expr = Eval(cadr(expr), env) != Kof(False) ? // cadr: if predicate
			caddr(expr) : // caddr: consequent
			// if Alternative
			cdddr(expr) == Kof(EmptyList) ? Kof(False) : cadddr(expr);
		goto tailcall;
	}

	// Cond statement
	if (IsCond(expr, obm_.get())) {
		expr = ExpandClauses(cdr(expr)); // cdr: cond clauses
		goto tailcall;
	}

	// Let statement
	if (IsLet(expr, obm_.get())) {
		Object *operators = MakeLambda(
				BindingsParams(cadr(expr), obm_.get()),
				cddr(expr), // cddr: let body
				obm_.get());
		// cadr: let bings
		Object *operands = BindingsArgs(cadr(expr), obm_.get());
		expr = obm_->Cons(operators, operands);
		goto tailcall;
	}

	// And Or statement
	if (IsAnd(expr, obm_.get())) {
		expr = cdr(expr); // cdr: and tests
		if (expr == Kof(EmptyList))
			return Kof(True);
		while (cdr(expr) != Kof(EmptyList)) {  // Is not last expr?
			Object *rv = Eval(car(expr), env); // car: first expr
			if (!rv)
				return nullptr;
			if (rv == Kof(False))
				return Kof(False);
			expr = cdr(expr); // cdr: rest expr
		}
		expr = car(expr);
		goto tailcall;
	}

	if (IsOr(expr, obm_.get())) {
		expr = cdr(expr); // cdr: or tests
		if (expr == Kof(EmptyList))
			return Kof(False);
		while (cdr(expr) != Kof(EmptyList)) { // Is not last expr?
			Object *rv = Eval(car(expr), env); // car: first expr
			if (!rv)
				return nullptr;
			if (rv == Kof(True))
				return Kof(True);
			expr = cdr(expr); // cdr: rest expr
		}
		expr = car(expr);
		goto tailcall;
	}

	// Begin block
	if (IsBegin(expr, obm_.get())) {
		expr = cdr(expr); // cdr: begin action
		while (cdr(expr) != Kof(EmptyList)) { // Is last action?
			if (!Eval(car(expr), env)) // car: first action
				return nullptr;
			expr = cdr(expr);     // cdr: rest actions
		}
		expr = car(expr);
		goto tailcall;
	}

	// Exec block
	if (expr->IsPair()) { // Is application ?
		Object *proc = Eval(car(expr), env);         // car: operator
		if (!proc)
			return nullptr; // May be error.
		Object *args = ListOfValues(cdr(expr), env); // cdr: operands

		if (proc->IsPrimitive() && proc->Primitive() == kApply) {
			proc = car(args);
			args = PrepareApplyOperands(cdr(args), obm_.get());
		}
		if (proc->IsPrimitive()) {
			auto fn = proc->Primitive();
			if (fn == kEval) {
				expr = car(args);
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
	if (!val)
		return nullptr;
	env->Define(var->Symbol(), val);
	return Kof(OkSymbol);
}

Object *Mach::EvalAssignment(Object *expr, Environment *env) {
	Object *var = cadr(expr); // cadr: assignment variable
	if (!var->IsSymbol()) {
		RaiseError("set! : Unexpected symbol.");
		return nullptr;
	}
	Object *val = Eval(caddr(expr), env); // caddr: assignment values
	if (!val)
		return nullptr;
	Environment::Handle handle(var->Symbol(), env);
	if (!handle.Valid()) {
		RaiseErrorf("Unbound variable, %s.", var->Symbol());
		return nullptr;
	}
	env->Define(var->Symbol(), val);
	return Kof(OkSymbol);
}

Object *Mach::ListOfValues(Object *operand, Environment *env) {
	if (operand == Kof(EmptyList))
		return Kof(EmptyList);

	 // car: first operand
	Object *first = Eval(car(operand), env);
	if (!first)
		return nullptr;
	// cdr: rest operands
	return obm_->Cons(first, ListOfValues(cdr(operand), env));
}

Object *Mach::ExpandClauses(Object *clauses) {
	if (clauses == Kof(EmptyList))
		return Kof(False);

	Object *first = car(clauses);
	Object *rest  = cdr(clauses);
	if (car(first) == Kof(ElseSymbol)) { // is cond else clause?
		if (rest != Kof(EmptyList)) {
			RaiseError("Else clause is not last cond->if.");
			return nullptr;
		}
		// rest == EmptyList
		// cdr: cond actions
		return SequenceToExpr(cdr(first), obm_.get());
	}
	// car: cond predicate
	return MakeIf(car(first),
			// cdr: cond action
			SequenceToExpr(cdr(first), obm_.get()),
			ExpandClauses(rest),
			obm_.get());
}


Environment *Mach::ExtendEnvironment(Object *params, Object *args,
		Environment *top) {
	Environment *env = obm_->NewEnvironment(top);
	while (params != Kof(EmptyList)) {
		if (!car(params)->IsSymbol()) {
			RaiseError("Parameters must be symbol.");
			return nullptr;
		}
		if (args != Kof(EmptyList)) {
			env->Define(car(params)->Symbol(), car(args));
			args = cdr(args);
		} else {
			// TODO: Use `nil' object
			env->Define(car(params)->Symbol(), nullptr);
		}
		params = cdr(params);
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
	int i = 0;
	while (args != Kof(EmptyList)) {
		if (!car(args)->IsFixed()) {
			RaiseErrorf("+ : Unexpected type: arg%d, expected fixednum.", i);
			return nullptr;
		}
		rv += car(args)->Fixed();
		args = cdr(args);
		++i;
	}
	return obm_->NewFixed(rv);
}

Object *Mach::Dec(Object *args) {
	long long rv = car(args)->Fixed();
	int i = 0;
	while ((args = cdr(args)) != Kof(EmptyList)) {
		if (!car(args)->IsFixed()) {
			RaiseErrorf("- : Unexpected type: arg%d, expected fixednum.", i);
			return nullptr;
		}
		rv -= car(args)->Fixed();
		++i;
	}
	return obm_->NewFixed(rv);
}

Object *Mach::Mul(Object *args) {
	long long rv = 1;
	int i = 0;
	while (args != Kof(EmptyList)) {
		if (!car(args)->IsFixed()) {
			RaiseErrorf("* : Unexpected type: arg%d, expected fixednum.", i);
			return nullptr;
		}
		rv *= car(args)->Fixed();
		args = cdr(args);
		++i;
	}
	return obm_->NewFixed(rv);
}

Object *Mach::Div(Object *args) {
	long long rv = car(args)->Fixed();
	int i = 0;
	while ((args = cdr(args)) != Kof(EmptyList)) {
		if (!car(args)->IsFixed()) {
			RaiseErrorf("/ : Unexpected type: arg%d, expected fixednum.", i);
			return nullptr;
		}
		if (car(args)->Fixed() == 0) {
			RaiseError("/ : Div zeor!");
			return nullptr;
		}
		rv /= car(args)->Fixed();
		++i;
	}
	return obm_->NewFixed(rv);
}

Object *Mach::NumberEqual(Object *args) {
	if (!car(args)->IsFixed()) {
		RaiseError("= : Unexpected type: arg0, expected fixednum.");
		return nullptr;
	}
	long long arg0 = car(args)->Fixed();
	int i = 1;
	while ((args = cdr(args)) != Kof(EmptyList)) {
		if (!car(args)->IsFixed()) {
			RaiseErrorf("= : Unexpected type: arg%d, expected fixednum.", i);
			return nullptr;
		}
		if (arg0 != car(args)->Fixed()) {
			return Kof(False);
		}
		++i;
	}
	return Kof(True);
}

Object *Mach::NumberGreat(Object *args) {
	if (!car(args)->IsFixed()) {
		RaiseError("> : Unexpected type: arg0, expected fixednum.");
		return nullptr;
	}
	long long prev = car(args)->Fixed();
	int i = 1;
	while ((args = cdr(args)) != Kof(EmptyList)) {
		if (!car(args)->IsFixed()) {
			RaiseErrorf("> : Unexpected type: arg%d, expected fixednum.", i);
			return nullptr;
		}
		long long next = car(args)->Fixed();
		if (prev > next)
			prev = next;
		else
			return Kof(False);
		++i;
	}
	return Kof(True);
}

Object *Mach::NumberLess(Object *args) {
	if (!car(args)->IsFixed()) {
		RaiseError("< : Unexpected type: arg0, expected fixednum.");
		return nullptr;
	}
	long long prev = car(args)->Fixed();
	int i = 1;
	while ((args = cdr(args)) != Kof(EmptyList)) {
		if (!car(args)->IsFixed()) {
			RaiseErrorf("< : Unexpected type: arg%d, expected fixednum.", i);
			return nullptr;
		}
		long long next = car(args)->Fixed();
		if (prev < next)
			prev = next;
		else
			return Kof(False);
		++i;
	}
	return Kof(True);
}

Object *Mach::Cons(Object *args) {
	return obm_->Cons(car(args), cadr(args));
}

Object *Mach::Car(Object *args) {
	if (!car(args)->IsPair()) {
		RaiseError("car : arg0 is not a pair or list.");
		return nullptr;
	}
	return caar(args);
}

Object *Mach::Cdr(Object *args) {
	return cdar(args);
}

Object *Mach::List(Object *args) {
	return args;
}

Object *Mach::SetCar(Object *args) {
	if (!car(args)->IsPair()) {
		RaiseError("set-car! : arg0 is not a pair or list.");
		return nullptr;
	}
	ObjectManagement::SetCar(car(args), cadr(args));
	return Kof(OkSymbol);
}

Object *Mach::SetCdr(Object *args) {
	if (!car(args)->IsPair()) {
		RaiseError("set-cdr! : arg0 is not a pair or list.");
		return nullptr;
	}
	ObjectManagement::SetCdr(car(args), cadr(args));
	return Kof(OkSymbol);
}

} // namespace vm
} // namespace ajimu

