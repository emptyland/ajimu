#include "object_management.h"
#include "environment.h"
#include "glog/logging.h"
#include <string.h>

namespace ajimu {
namespace values {

using vm::Environment;

ObjectManagement::ObjectManagement()
	: gc_root_(nullptr) {
	memset(constant_, 0, sizeof(constant_));
}

ObjectManagement::~ObjectManagement() {
}

void ObjectManagement::Init() {
	// Constants initializing:
	constant_[kFalse] = NewBoolean(false);
	constant_[kTrue]  = NewBoolean(true);
	constant_[kEmptyList] = Cons(nullptr, nullptr);
	constant_[kQuoteSymbol] = NewSymbol("quote");
	constant_[kOkSymbol] = NewSymbol("ok");
	constant_[kDefineSymbol] = NewSymbol("define");
	constant_[kLambdaSymbol] = NewSymbol("lambda");
	constant_[kBeginSymbol] = NewSymbol("begin");
	constant_[kSetSymbol] = NewSymbol("set!");
	constant_[kIfSymbol] = NewSymbol("if");
	constant_[kCondSymbol] = NewSymbol("cond");
	constant_[kElseSymbol] = NewSymbol("else");
	constant_[kLetSymbol] = NewSymbol("let");
	constant_[kAndSymbol] = NewSymbol("and");
	constant_[kOrSymbol] = NewSymbol("or");

	// Environment initializing:
	gc_root_ = NewEnvironment(nullptr);
}

Object *ObjectManagement::Constant(Constants e) {
	int i = static_cast<int>(e);
	DCHECK(i >= 0 && i < kMax);
	return constant_[i];
}

Object *ObjectManagement::NewSymbol(const std::string &raw) {
	auto iter = symbol_.find(raw);
	if (iter != symbol_.end())
		return iter->second;

	char *dup = new char[raw.size() + 1];
	memcpy(dup, raw.c_str(), raw.size());
	dup[raw.size()] = '\0';

	Object *o = AllocateObject(SYMBOL);
	symbol_.insert(std::move(std::make_pair(raw, o)));
	o->symbol_.name = dup;
	o->symbol_.index = -1;
	return o;
}

Object *ObjectManagement::NewString(const char *raw, size_t len) {
	// TODO: gc
	union {
		String *to_str;
		char   *to_psz;
	};
	to_psz = new char[len + 1 + sizeof(String)];
	to_str->hash = 0;
	to_str->len  = len;
	memcpy(to_str->land, raw, len);
	to_str->land[len] = '\0';

	Object *o = AllocateObject(STRING);
	o->string_ = to_str;
	return o;
}

Object *ObjectManagement::NewClosure(Object *params, Object *body,
		Environment *env) {
	Object *o = AllocateObject(CLOSURE);
	o->closure_.params = params;
	o->closure_.body   = body;
	o->closure_.env    = env;
	return o;
}

Object *ObjectManagement::NewPrimitive(const std::string &name, PrimitiveMethodPtr method) {
	Object *o = AllocateObject(PRIMITIVE);
	o->primitive_ = method;

	// Primitive Proc must be in global environment!
	GlobalEnvironment()->Define(name, o);
	return o;
}

Environment *ObjectManagement::NewEnvironment(Environment *top) {
	if (gc_root_ && !top) {
		DLOG(ERROR) << "Local environment top not be `nullptr\'.";
		return nullptr;
	}
	// TODO: Implement gc.
	return new Environment(top);
}

Object *ObjectManagement::AllocateObject(Type type) {
	// TODO: Implement gc.
	return new Object(type);
}

} // namespace values
} // namespace ajimu

