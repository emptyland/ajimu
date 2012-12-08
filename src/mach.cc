#include "mach.h"
#include "object_management.h"
#include "object.h"
#include "environment.h"
#include "local.h"
#include "lexer.h"
#include "string.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace ajimu {
namespace vm {

using ::ajimu::values::Object;
using ::ajimu::values::ObjectManagement;

template<class T>
class StackPersisted {
public:
	StackPersisted(std::stack<T> *s, const T &val)
		: stack_(s) {
		stack_->push(val);
	}

	~StackPersisted() {
		DCHECK(!stack_->empty());
		stack_->pop();
	}

private:
	StackPersisted(const StackPersisted &) = delete;
	void operator = (const StackPersisted &) = delete;

	std::stack<T> *stack_;
};

inline values::PrimitiveMethodPtr UnsafeCast2Method(const char *k) {
	union {
		const char *input;
		values::PrimitiveMethodPtr output;
	};
	input = k;
	return output;
}

static auto kApply = UnsafeCast2Method(":apply:");
static auto kEval =  UnsafeCast2Method(":eval:");

#define Kof(i) obm_->Constant(::ajimu::values::k##i)
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
	, error_(0)
	, call_level_(0) {
}

Mach::~Mach() {
}

//
// The local variable operators
//
void Mach::Push(Object *o) {
	local_val_->Push(o);
}

void Mach::Pop(size_t n) {
	local_val_->Pop(n);
}

Object *Mach::Last(size_t i) const {
	return local_val_->Last(i);
}

int Mach::Line() const {
	return lex_->Line();
}

Object *Mach::Feed(const char *input) {
	return Feed(input, strlen(input));
}

bool Mach::Init() {
	struct PrimitiveEntry {
		const char *name;
		values::PrimitiveMethodPtr method;
	} kProcs[] = {
		// Type system procedures:
		{ "boolean?", &Mach::IsBoolean, },
		{ "symbol?",  &Mach::IsSymbol,  },
		{ "char?",    &Mach::IsChar,    },
		{ "vector?",  &Mach::IsVector,  },
		{ "port?",    &Mach::IsPort,    },
		{ "null?",    &Mach::IsNull,    },
		{ "pair?",    &Mach::IsPair,    },
		{ "number?",  &Mach::IsNumber,  },
		{ "string?",  &Mach::IsString,  },
		{ "bytevector?", &Mach::IsByteVector, },
		{ "procedure?",  &Mach::IsProcedure,  },

		// Arithmetic procedures:
		{ "+", &Mach::Add, },
		{ "-", &Mach::Dec, },
		{ "*", &Mach::Mul, },
		{ "/", &Mach::Div, },
		{ "=", &Mach::NumberEqual, },
		{ ">", &Mach::NumberGreat, },
		{ "<", &Mach::NumberLess,  },

		// Evaluting
		{ "apply", kApply, },
		{ "eval",  kEval,  },

		// List procedures:
		{ "cons", &Mach::Cons, },
		{ "car",  &Mach::Car,  },
		{ "cdr",  &Mach::Cdr,  },
		{ "list", &Mach::List, },
		{ "set-car!", &Mach::SetCar, },
		{ "set-cdr!", &Mach::SetCdr, },

		// Ouput:
		{ "display", &Mach::Display, },

		// File
		{ "load", &Mach::Load, },

		// Error Handling
		{ "error", &Mach::Error, },

		// Ajimu extensions:
		{ "ajimu.gc.allocated", &Mach::AjimuGcAllocated, },
		{ "ajimu.gc.state", &Mach::AjimuGcState, },
	};

	local_val_.reset(new Local<Object>());
	local_env_.reset(new Local<Environment>());

	obm_.reset(new ObjectManagement());
	obm_->Init();
	// Register primitive proc(s)
	for (const PrimitiveEntry &i : kProcs) {
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

Object *Mach::EvalFile(const char *name) {
	utils::Handle<FILE> fp(fopen(name, "r"));
	if (!fp.Valid()) {
		RaiseErrorf("load : Can not open file \"%s\".", name);
		return nullptr;
	}
	// Get file's size
	fseek(fp.Get(), 0, SEEK_END);
	size_t size = ftell(fp.Get());
	fseek(fp.Get(), 0, SEEK_SET);
	if (!size) // A empty file
		return Kof(EmptyList);
	std::unique_ptr<char[]> buf(new char[size]);
	// Read all
	if (fread(buf.get(), 1, size, fp.Get()) <= 0) {
		RaiseErrorf("load : Load file error \"%s\".", name);
		return nullptr;
	}
	// Eval it!
	StackPersisted<std::string> persisted(&file_level_, name);
	Lexer lex(obm_.get());
	lex.Feed(buf.get(), size);
	Object *o, *rv;
	while ((o = lex.Next()) != nullptr) {
		rv = Eval(o, GlobalEnvironment());
		if (!rv)
			return nullptr;
	}
	return rv;
}

Object *Mach::Eval(Object *expr, Environment *env) {
	utils::ScopedCounter<int>     counter(&call_level_);
	Local<Object>::Persisted      persisted_val(local_val_.get());
	Local<Environment>::Persisted persisted_env(local_env_.get());

newenv:
	local_env_->Push(env);
tailcall:
	local_val_->Push(expr);
	obm_->GcTick(local_val_.get(), local_env_.get());

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
		Pop(1);
		goto tailcall;
	}

	// Cond statement
	if (IsCond(expr, obm_.get())) {
		expr = ExpandClauses(cdr(expr)); // cdr: cond clauses
		Pop(1);
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
		Pop(1);
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
		Pop(1);
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
		Pop(1);
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
		Pop(1);
		goto tailcall;
	}

	// Exec block
	if (expr->IsPair()) { // Is application 
		Push(Eval(car(expr), env)); // proc | car: operator
		//if (!proc)
		if (!Last(0)) {
			Pop(1);
			return nullptr; // May be error.
		}
		Push(ListOfValues(cdr(expr), env)); // args | cdr: operands

		if (Last(1)->IsPrimitive() && Last(1)->Primitive() == kApply) {
			Object *args = Last(0);
			Pop(2); Push(args);
			Push(car(Last(0))); // proc
			Push(PrepareApplyOperands(cdr(Last(0)), obm_.get())); // args
		}
		if (Last(1)->IsPrimitive()) {
			auto fn = Last(1)->Primitive();
			if (fn == kEval) {
				expr = car(Last(0));
				// TODO env  = MakeEnvironment() cadr
				Pop(3);
				goto tailcall;
			}
			//Push(args);
			return (this->*fn)(Last(0));
		}
		if (Last(1)->IsClosure()) {
			env = ExtendEnvironment(Last(1)->Params(),
					Last(0),
					Last(1)->Environment());
			// Make begin block
			expr = obm_->Cons(Kof(BeginSymbol), Last(1)->Body());
			local_env_->Pop(1);
			Pop(3);
			goto newenv;
		}
		RaiseError("Unknown Last(0)edure type.");
		return nullptr;
	}
	RaiseError("Bad eval! no one can be evaluated.");
	return nullptr;
}

Object *Mach::LookupVariable(Object *expr, Environment *env) {
	Environment::Handle handle(expr->Symbol(), env);
	if (!handle.Valid())
		RaiseErrorf("Unbound variable, \"%s\".", expr->Symbol());
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
	//env->Define(var->Symbol(), val);
	handle.Set(var->Symbol(), val);
	return Kof(OkSymbol);
}

Object *Mach::ListOfValues(Object *operand, Environment *env) {
	Local<Object>::Persisted persisted(local_val_.get());

	Push(operand);
	if (Last(0) == Kof(EmptyList))
		return Kof(EmptyList);

	 // car: first operand
	//Object *first = Eval(car(operand), env);
	Push(Eval(car(Last(0)), env));
	if (!Last(0))
		return nullptr;
	// cdr: rest operands
	Push(ListOfValues(cdr(Last(1)), env));
	return obm_->Cons(Last(1), Last(0));
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
			env->Define(car(params)->Symbol(), Kof(EmptyList));
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

Object *Mach::Display(Object *args) {
	Object *o = car(args);
	printf("%s\n", o->ToString(obm_.get()).c_str());
	// TODO:
	return Kof(OkSymbol);
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


Object *Mach::Load(Object *args) {
	if (!car(args)->IsString()) {
		RaiseError("load : arg0 is not a string.");
		return nullptr;
	}
	const char *name = car(args)->String()->c_str();
	return EvalFile(name);
}

Object *Mach::IsBoolean(Object *args) {
	return car(args)->IsBoolean() ? Kof(True) : Kof(False);
}

Object *Mach::IsSymbol(Object *args) {
	return car(args)->IsSymbol() ? Kof(True) : Kof(False);
}

Object *Mach::IsChar(Object *args) {
	return car(args)->IsCharacter() ? Kof(True) : Kof(False);
}

Object *Mach::IsVector(Object *args) {
	// TODO: Implement vector
	(void)args;
	return Kof(False);
}

Object *Mach::IsPort(Object *args) {
	// TODO: Implement port
	(void)args;
	return Kof(False);
}

Object *Mach::IsNull(Object *args) {
	return car(args) == Kof(EmptyList) ? Kof(True) : Kof(False);
}

Object *Mach::IsPair(Object *args) {
	return car(args)->IsPair() ? Kof(True) : Kof(False);
}

Object *Mach::IsNumber(Object *args) {
	return car(args)->IsFixed() ? Kof(True) : Kof(False);
}

Object *Mach::IsString(Object *args) {
	return car(args)->IsString() ? Kof(True) : Kof(False);
}

Object *Mach::IsByteVector(Object *args) {
	// TODO: Implement bytevector
	(void)args;
	return Kof(False);
}

Object *Mach::IsProcedure(Object *args) {
	return car(args)->IsPrimitive() ||
		car(args)->IsClosure() ? Kof(True) : Kof(False);
}

//
// Error handling
//
Object *Mach::Error(Object *args) {
	Object *msg = (args != Kof(EmptyList)) ? car(args) : nullptr;
	RaiseErrorf("Error() : %s",
			msg ? msg->ToString(obm_.get()).c_str() : "Unspecified error.");
	return Kof(OkSymbol);
}

//
// Extension Primitive Procedures:
//
Object *Mach::AjimuGcAllocated(Object * /*args*/) {
	long long rv = obm_->AllocatedSize();
	return obm_->NewFixed(rv);
}

Object *Mach::AjimuGcState(Object * /*args*/) {
	static const char *kState[ObjectManagement::kMaxState] = {
		"pause",
		"propagate",
		"sweep-environment",
		"sweep-string",
		"sweep",
		"finalize",
	};
	int state = obm_->GcState();
	DCHECK_LT(state,
			static_cast<int>(sizeof(kState)/sizeof(kState[0])));
	return obm_->NewSymbol(kState[state]);
}

#undef Kof
} // namespace vm
} // namespace ajimu

