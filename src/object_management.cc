#include "object_management.h"
#include "environment.h"
#include "glog/logging.h"
#include <string.h>

namespace ajimu {
namespace values {

using vm::Environment;

ObjectManagement::ObjectManagement()
	: gc_root_(nullptr)
	, gc_state_(kPause)
	, gc_started_(false)
	, white_flag_(Reachable::WHITE_BIT0)
	, allocated_(0)
	, obj_list_(nullptr, white_flag_)
	, env_list_(nullptr, white_flag_)
	, env_mark_(0) {
	memset(constant_, 0, sizeof(constant_));
}

ObjectManagement::~ObjectManagement() {
	Reachable *i, *p;
	i = obj_list_.next_;
	while (i) {
		p = i;
		i = i->next_;
		delete static_cast<Object*>(p);
	}
	i = env_list_.next_;
	while (i) {
		p = i;
		i = i->next_;
		delete static_cast<Environment*>(p);
	}
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

	// GC startup:
	gc_started_ = true;
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

Object *ObjectManagement::NewPrimitive(const std::string &name,
		PrimitiveMethodPtr method) {
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
	allocated_ += sizeof(Environment);

	auto env = new Environment(top, env_list_.next_, white_flag_);
	env_list_.next_ = env; // Linked to environment list.
	return env;
}

Environment *ObjectManagement::TEST_NewEnvironment(vm::Environment *top) {
	allocated_ += sizeof(Environment);

	auto env = new Environment(top, env_list_.next_, white_flag_);
	env_list_.next_ = env; // Linked to environment list.
	return env;
}

Object *ObjectManagement::AllocateObject(Type type) {
	allocated_ += sizeof(Object);

	Object *o = new Object(type, obj_list_.next_, white_flag_);
	obj_list_.next_ = o; // Linked to object list.
	return o;
}

void ObjectManagement::GcTick(Object *rv, Object *expr) {
	if (!gc_started_) return;

//tail:
	switch (gc_state_) {
	case kPause: // GC Pause
		// TODO:
		// Switch the white flag!
		white_flag_ = InvWhite(white_flag_);
		// Mark root first.
		gc_root_->ToBlack();
		// Mark local expr
		if (expr) MarkObject(expr);
		if (rv)   MarkObject(rv);
		++gc_state_; 
		DLOG(INFO) << "----------------------------------\n";
		DLOG(INFO) << "kPause - white:" << white_flag_
			<< " allocated:" << allocated_;
		break;
	case kPropagate: // Mark root environment
		/*if (env_mark_ >= gc_root_->Count()) {
			env_mark_ = 0;
			++gc_state_;
			DLOG(INFO) << "kPropagate -";
			break;
		}*/
		// Mark one slot by one time!
		for (auto var : gc_root_->Variable())
			MarkObject(var);
		++gc_state_;
		DLOG(INFO) << "kPropagate -";
		break;
	case kSweepEnv: // Sweep environments
		SweepEnvironment();
		++gc_state_;
		DLOG(INFO) << "kSweepEnv - allocated:" << allocated_;
		break;
	case kSweep: // Sweep objects
		SweepObject();
		++gc_state_;
		DLOG(INFO) << "kSweep - allocated:" << allocated_;
		break;
	case kFinalize:
		gc_state_ = kPause;
		DLOG(INFO) << "kFinalize - allocated:" << allocated_;
		break;
	default:
		DLOG(FATAL) << "No reached!";
		break;
	}
}

void ObjectManagement::MarkObject(Object *o) {
	/*DCHECK(freed_.find(o) == freed_.end())
		<< "Dup freed obj! addr: " << o;*/

	switch (o->OwnedType()) {
	case SYMBOL:
	case BOOLEAN:
		// Skip boolean and symbol type
		break;
	case FIXED:
	case CHARACTER:
	case STRING:
	case PRIMITIVE:
		Mark(o);
		break;
	case CLOSURE:
		Mark(o);
		MarkObject(o->Params());
		MarkObject(o->Body());
		MarkEnvironment(o->Environment());
		break;
	case PAIR:
		if (o != Constant(kEmptyList)) {
			Mark(o);
			MarkObject(car(o));
			MarkObject(cdr(o));
		}
		break;
	}
}

void ObjectManagement::MarkEnvironment(Environment *env) {
tail:
	//if (!env->TestInvWhite(white_flag_)) {
	if (ShouldMark(env)) {
		/*DCHECK(freed_.find(env) == freed_.end())
			<< "Dup freed env! addr: " << env;*/
		env->ToBlack();
		for (auto elem : env->Variable())
			MarkObject(elem);
	}
	env = env->Next();
	if (env) goto tail;
}

void ObjectManagement::SweepEnvironment() {
	/*Reachable *sweep = env_sweep_;
	while (sweep->next_) {
		Reachable *next = sweep->next_;
		if (next->TestInvWhite(white_flag_)) {
			sweep->next_ = next->next_;
			delete static_cast<Environment*>(next);
			allocated_ -= sizeof(Environment);
			break;
		}
		next->ToWhite(white_flag_);
		sweep = next;
	}
	env_sweep_ = sweep->next_;
	*/
	int sweeped = 0;
	Reachable *sweep = &env_list_;
	while (sweep->next_) {
		Reachable *next = sweep->next_;
		if (next->TestInvWhite(white_flag_)) {
			// Environment Collected!
			sweep->next_ = next->next_;
			freed_.insert(next);
			delete static_cast<Environment*>(next);
			allocated_ -= sizeof(Environment);
			sweep = sweep->next_;
			++sweeped;
		} else {
			next->ToWhite(white_flag_);
			sweep = next;
		}
	}
	DLOG(INFO) << "SweepEnvironment() sweeped:" << sweeped;
}

void ObjectManagement::SweepObject() {
	Reachable *sweep = &obj_list_;
	int sweeped = 0;
	while (sweep->next_) {
		Object *next = static_cast<Object*>(sweep->next_);
		if (next->TestInvWhite(white_flag_) &&
				!next->IsSymbol() &&
				!next->IsBoolean() &&
				next != Constant(kEmptyList)) {
			// Object Collected!
			sweep->next_ = next->next_;
			freed_.insert(next);
			delete next;
			allocated_ -= sizeof(Object);
			sweep = sweep->next_;
			++sweeped;
		} else {
			if (next->IsBlack())
				next->ToWhite(white_flag_);
			sweep = next;
		}
	}
	DLOG(INFO) << "SweepObject() sweeped:" << sweeped;
}

} // namespace values
} // namespace ajimu

