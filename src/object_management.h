#ifndef AJIMU_VALUES_OBJECT_MANAGEMENT_H
#define AJIMU_VALUES_OBJECT_MANAGEMENT_H

#include "object.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace ajimu {
namespace vm {
class Environment;
template<class T>
class Local;
} // namespace vm
namespace values {
class StringPool;

enum Constants {
	kFalse,
	kTrue,
	kEmptyList,
	kQuoteSymbol,
	kOkSymbol,
	kDefineSymbol,
	kLambdaSymbol,
	kBeginSymbol,
	kSetSymbol,
	kIfSymbol,
	kCondSymbol,
	kElseSymbol,
	kLetSymbol,
	kAndSymbol,
	kOrSymbol,

	kMax, // Make sure it to be last one
};

class ObjectManagement {
public:
	// All GC state (step)
	enum GcState {
		kPause,
		kPropagate,
		kSweepEnv,
		kSweepString,
		kSweep,
		kFinalize,
		kMaxState,
	};

	ObjectManagement();

	~ObjectManagement();

	void Init();

	//
	// For GC:
	//
	size_t AllocatedSize() const;

	int GcState() const {
		return gc_state_;
	}

	void GcTick(vm::Local<Object> *local,
			vm::Local<vm::Environment> *env);

	//
	// For Environment allocating:
	//
	vm::Environment *GlobalEnvironment() { return gc_root_; }

	vm::Environment *NewEnvironment(vm::Environment *top);

	vm::Environment *TEST_NewEnvironment(vm::Environment *top);

	Object *Constant(Constants i) const;

	bool Null(const Object *o) const {
		return DCHECK_NOTNULL(o) == Constant(kEmptyList);
	}

	//
	// New objects:
	//
	Object *NewFixed(long long value) {
		Object *o = AllocateObject(FIXED);
		o->fixed_ = value;
		return o;
	}

	Object *NewBoolean(bool value) {
		Object *o = AllocateObject(BOOLEAN);
		o->boolean_ = value;
		return o;
	}

	Object *NewCharacter(char value) {
		Object *o = AllocateObject(CHARACTER);
		o->character_ = value;
		return o;
	}

	Object *NewSymbol(const std::string &raw);

	Object *NewString(const char *raw, size_t len);

	Object *NewClosure(Object *params, Object *body,
			vm::Environment *env);

	Object *NewPrimitive(const std::string &name,
			PrimitiveMethodPtr method);

	Object *Cons(Object *car, Object *cdr) {
		Object *o = AllocateObject(PAIR);
		o->pair_.car = car;
		o->pair_.cdr = cdr;
		return o;
	}

	static Object *SetCar(Object *node, Object *car) {
		DCHECK(node->IsPair());
		node->pair_.car = DCHECK_NOTNULL(car);
		return node;
	}

	static Object *SetCdr(Object *node, Object *cdr) {
		DCHECK(node->IsPair());
		node->pair_.cdr = DCHECK_NOTNULL(cdr);
		return node;
	}

private:
	ObjectManagement(const ObjectManagement &) = delete;
	void operator = (const ObjectManagement &) = delete;

	Object *AllocateObject(Type type);

	void MarkObject(Object *o);

	void MarkEnvironment(vm::Environment *env);

	void SweepEnvironment();

	void SweepObject();

	void CollectObject(Object *o);

	bool ShouldMark(const Reachable *o) {
		return !o->IsBlack() && !o->TestWhite(white_flag_);
	}

	void Mark(Reachable *o) {
		if (ShouldMark(o)) o->ToBlack();
	}

	// All constants
	Object *constant_[kMax];

	// Symbol table
	std::unordered_map<std::string, Object*> symbol_;

	// String factory
	std::unique_ptr<StringPool> pool_;

	// For GC:
	vm::Environment *gc_root_;
	int gc_state_;
	bool gc_started_;
	unsigned white_flag_;
	size_t allocated_;

	// Object in gc
	Reachable *obj_list_;

	// Environment in gc
	Reachable *env_list_;

	std::unordered_set<void*> freed_;
}; // class ObjectManagement

} // namespace values
} // namespace ajimu

#endif //AJIMU_VALUES_OBJECT_MANAGEMENT_H

