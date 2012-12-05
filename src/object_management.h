#ifndef AJIMU_VALUES_OBJECT_MANAGEMENT_H
#define AJIMU_VALUES_OBJECT_MANAGEMENT_H

#include "object.h"
#include <unordered_map>

namespace ajimu {
namespace vm {
class Environment;
} // namespace vm
namespace values {

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
	ObjectManagement();

	~ObjectManagement();

	void Init();

	//
	// For Environment allocating:
	//
	vm::Environment *GlobalEnvironment() { return gc_root_; }

	vm::Environment *NewEnvironment(vm::Environment *top);

	Object *Constant(Constants i);

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

	Object *NewPrimitive(const std::string &name, PrimitiveMethodPtr method);

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

	vm::Environment *gc_root_;
	Object *constant_[kMax];
	std::unordered_map<std::string, Object*> symbol_;
}; // class ObjectManagement

#define OBM(method) \
	::ajimu::values::ObjectManagement::##method

} // namespace values
} // namespace ajimu

#endif //AJIMU_VALUES_OBJECT_MANAGEMENT_H

