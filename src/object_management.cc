#include "object_management.h"
#include "environment.h"
#include "local.h"
#include "string_pool.h"
#include "string.h"
#include "glog/logging.h"
#include <string.h>

namespace ajimu {
namespace values {

using vm::Environment;

ObjectManagement::ObjectManagement()
	: pool_(new StringPool)
	, gc_root_(nullptr)
	, gc_state_(kPause)
	, gc_started_(false)
	, white_flag_(Reachable::WHITE_BIT0)
	, allocated_(0)
	, obj_list_(nullptr)
	, env_list_(nullptr) {
	memset(constant_, 0, sizeof(constant_));
}

ObjectManagement::~ObjectManagement() {
	Reachable *i, *p;
	i = obj_list_;
	while (i) {
		p = i;
		i = i->next_;
		delete static_cast<Object*>(p);
	}
	i = env_list_;
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

size_t ObjectManagement::AllocatedSize() const {
	return allocated_ + pool_->Allocated();
}

Object *ObjectManagement::Constant(Constants e) const {
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
	Object *o = AllocateObject(STRING);
	o->string_ = pool_->NewString(raw, len, white_flag_);
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
	GlobalEnvironment()->Define(NewSymbol(name)->Symbol(), o);
	return o;
}

Environment *ObjectManagement::NewEnvironment(Environment *top) {
	if (gc_root_ && !top) {
		DLOG(ERROR) << "Local environment top not be `nullptr\'.";
		return nullptr;
	}
	allocated_ += sizeof(Environment);

	auto env = new Environment(top, env_list_, white_flag_);
	env_list_ = env; // Linked to environment list.
	return env;
}

Environment *ObjectManagement::TEST_NewEnvironment(vm::Environment *top) {
	allocated_ += sizeof(Environment);

	auto env = new Environment(top, env_list_, white_flag_);
	env_list_ = env; // Linked to environment list.
	return env;
}

Object *ObjectManagement::AllocateObject(Type type) {
	allocated_ += sizeof(Object);

	Object *o = new Object(type, obj_list_, white_flag_);
	freed_.erase(o);
	obj_list_= o; // Linked to object list.
	return o;
}

void ObjectManagement::GcTick(vm::Local<Object> *local,
		vm::Local<Environment> *env) {
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
		for (auto val : local->Values())
			MarkObject(val);
		for (auto val : env->Values())
			MarkEnvironment(val);
		++gc_state_; 
		DLOG(INFO) << "----------------------------------\n";
		DLOG(INFO) << "kPause - white:" << white_flag_
			<< " allocated:" << allocated_;
		break;
	case kPropagate: // Mark root environment
		// Mark constants
		for (auto k : constant_)
			MarkObject(k);
		// Mark one slot by one time!
		for (auto entry : gc_root_->Entries()) {
			DCHECK(symbol_.find(entry.first) != symbol_.end())
				<< "Symbol table has not: "
				<< entry.first
				<< " for mark!";

			MarkObject(symbol_[entry.first]);
			MarkObject(gc_root_->At(entry.second));
		}
		++gc_state_;
		DLOG(INFO) << "kPropagate -";
		break;
	case kSweepEnv: // Sweep environments
		SweepEnvironment();
		++gc_state_;
		DLOG(INFO) << "kSweepEnv - allocated:" << allocated_;
		break;
	case kSweepString:
		pool_->Sweep(white_flag_);
		++gc_state_;
		DLOG(INFO) << "kSweepString - allocated:" << allocated_;
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
tailcall:
	switch (o->OwnedType()) {
	case BOOLEAN:
		// Skip boolean type, beasure it's constant.
		break;
	case SYMBOL:
	case FIXED:
	case CHARACTER:
		Mark(o);
		break;
	case STRING:
		Mark(o);
		Mark(o->String());
		break;
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
			//MarkObject(cdr(o));
			o = cdr(o);
			goto tailcall;
		}
		break;
	}
}

void ObjectManagement::MarkEnvironment(Environment *env) {
tailcall:
	if (ShouldMark(env)) {
		env->ToBlack();
		for (auto entry : env->Entries()) {
			DCHECK(symbol_.find(entry.first) != symbol_.end())
					<< "Symbol table has not: "
					<< entry.first
					<< " for mark!";
			MarkObject(symbol_[entry.first]);
			MarkObject(env->At(entry.second));
		}
	}
	env = env->Next();
	if (env) goto tailcall;
}

void ObjectManagement::SweepEnvironment() {
	int sweeped = 0;
	Reachable *x = env_list_;
	Reachable dummy(x, white_flag_), *p = &dummy;
	while (x) {
		if (x->TestInvWhite(white_flag_)) {
			// Environment collection:
			p->next_ = x->next_;
			delete static_cast<Environment*>(x);
			allocated_ -= sizeof(Environment);
			++sweeped;
			x = p->next_;
		} else {
			x->ToWhite(white_flag_);
			p = x;
			x = x->next_;
		}
	}
	env_list_ = dummy.next_;
	DLOG(INFO) << "SweepEnvironment() xed:" << sweeped;
}

void ObjectManagement::SweepObject() {
	int sweeped = 0;
	Reachable *x = obj_list_;
	Reachable dummy(x, white_flag_), *p = &dummy;
	while (x) {
		Object *o = static_cast<Object*>(x);
		if (o->TestInvWhite(white_flag_) &&
				!o->IsBoolean() && !Null(o)) {
			p->next_ = x->next_;
			CollectObject(o);
			++sweeped;
			x = p->next_;
		} else {
			x->ToWhite(white_flag_);
			p = x;
			x = x->next_;
		}
	}
	obj_list_ = dummy.next_;
	DLOG(INFO) << "SweepObject() sweeped:" << sweeped;
}

void ObjectManagement::CollectObject(Object *o) {
	if (o->IsSymbol()) {
		DCHECK(symbol_.find(o->Symbol()) != symbol_.end());
		symbol_.erase(o->Symbol());
	}
	allocated_ -= sizeof(*o);
	delete o;
}

} // namespace values
} // namespace ajimu

