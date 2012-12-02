#ifndef AJIMU_VALUES_OBJECT_H
#define AJIMU_VALUES_OBJECT_H

#include "glog/logging.h"
#include "string.h"

namespace ajimu {
namespace vm {
class Mach;
class Environment;
} // namespace vm
namespace values {
class Object;

enum Type {
	BOOLEAN,
	SYMBOL,
	FIXED,
	CHARACTER,
	STRING,
	PAIR,
	CLOSURE,
	PRIMITIVE,
};

typedef Object *(vm::Mach::*PrimitiveMethodPtr)(Object *);

class Object {
public:
	~Object();

	Type OwnedType() const {
		return owned_type_;
	}

	long long Fixed() const {
		DCHECK(IsFixed()); return fixed_;
	}

	bool Boolean() const {
		DCHECK(IsBoolean()); return boolean_;
	}

	char Character() const {
		DCHECK(IsCharacter()); return character_;
	}

	StringHandle String() const {
		DCHECK(IsString()); return StringHandle(string_);
	}

	const char *Symbol() const {
		DCHECK(IsSymbol()); return symbol_.name;
	}

	int SymbolIndex() const {
		DCHECK(IsSymbol()); return symbol_.index;
	}

	PrimitiveMethodPtr Primitive() const {
		DCHECK(IsPrimitive()); return primitive_;
	}

	Object *Params() const {
		DCHECK(IsClosure()); return closure_.params;
	}

	Object *Body() const {
		DCHECK(IsClosure()); return closure_.body;
	}

	vm::Environment *Environment() const {
		DCHECK(IsClosure()); return closure_.env;
	}

	Object *Car() const {
		DCHECK(IsPair()); return pair_.car;
	}

	Object *Cdr() const {
		DCHECK(IsPair()); return pair_.cdr;
	}

	bool IsFixed() const { return OwnedType() == FIXED; }
	bool IsBoolean() const { return OwnedType() == BOOLEAN; }
	bool IsCharacter() const { return OwnedType() == CHARACTER; }
	bool IsSymbol() const { return OwnedType() == SYMBOL; }
	bool IsString() const { return OwnedType() == STRING; }
	bool IsPair() const { return OwnedType() == PAIR; }
	bool IsClosure() const { return OwnedType() == CLOSURE; }
	bool IsPrimitive() const { return OwnedType() == PRIMITIVE; }

	friend class ObjectManagement;
private:
	Object(Type type)
		: owned_type_(type) {
	}

	Type owned_type_;
	union {
		// Boolean : #t or #f
		bool boolean_;

		// Fixed number
		long long fixed_;

		// 8bit Character
		char character_;

		// Pooled String
		struct String *string_;

		// Symbol : name > symbol literal
		//        : index > fast index
		struct {
			const char *name;
			int index;
		} symbol_;

		// Pair or List node
		struct {
			Object *car;
			Object *cdr;
		} pair_;

		// Closure:
		struct {
			Object *params;
			Object *body;
			vm::Environment *env;
		} closure_;

		// Primitive proc
		PrimitiveMethodPtr primitive_;
	};

	Object(const Object &) = delete;
	void operator = (const Object &) = delete;
}; // class Object

//
// List operators:
//
inline Object *car(Object *o) { return o->Car(); }
inline Object *cdr(Object *o) { return o->Cdr(); }

// 2 Parties operator:
inline Object *caar(Object *o) { return o->Car()->Car(); }
inline Object *cadr(Object *o) { return o->Cdr()->Car(); }
inline Object *cdar(Object *o) { return o->Car()->Cdr(); }
inline Object *cddr(Object *o) { return o->Cdr()->Cdr(); }

// 3 Parties operator:
inline Object *caaar(Object *o) { return o->Car()->Car()->Car(); }
inline Object *caadr(Object *o) { return o->Cdr()->Car()->Car(); }
inline Object *cadar(Object *o) { return o->Car()->Cdr()->Car(); }
inline Object *caddr(Object *o) { return o->Cdr()->Cdr()->Car(); }
inline Object *cdadr(Object *o) { return o->Cdr()->Car()->Cdr(); }
inline Object *cddar(Object *o) { return o->Car()->Cdr()->Cdr(); }
inline Object *cdaar(Object *o) { return o->Car()->Car()->Cdr(); }
inline Object *cdddr(Object *o) { return o->Cdr()->Cdr()->Cdr(); }

// 4 Parties operator:
inline Object *cadddr(Object *o) {
	return o->Cdr()->Cdr()->Cdr()->Car();
}

} // namespace values
} // namespace ajimu

#endif //AJIMU_VALUES_OBJECT_H

