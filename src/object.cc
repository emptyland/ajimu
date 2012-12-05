#include "object.h"
#include "object_management.h"
#include "utils.h"

namespace ajimu {
namespace values {

std::string PrimitiveValue2String(Object *o);

#define Kof(i) obm->Constant(::ajimu::values::k##i)
Object::~Object() {
	switch (OwnedType()) {
	case STRING: 
		String().Free();
		break;
	case SYMBOL:
		delete[] symbol_.name;
		break;
	default:
		break;
	}
}

std::string Object::ToString(ObjectManagement *obm) {
	switch (OwnedType()) {
	case BOOLEAN:
		return utils::Formatf("%s", Boolean() ? "#t" : "#f");
	case SYMBOL:
		return utils::Formatf("%s", Symbol());
	case FIXED:
		return utils::Formatf("%lld", Fixed());
	case CHARACTER:
		return utils::Formatf("%c", Character());
	case STRING:
		return std::move(String().str());
	case PAIR: {
			if (this == Kof(EmptyList))
				return "null";
			int i = 0;
			std::string str("(");
			Object *o = this;
			while (o != Kof(EmptyList)) {
				if (i++ > 0)
					str.append(" ");
				str.append(car(o)->ToString(obm));
				o = cdr(o);
			}
			str.append(")");
			return std::move(str);
		}
		break;
	case CLOSURE:
		// TODO:
		break;
	case PRIMITIVE:
		// TODO:
		break;
	}
	return "";
}

#undef Kof
} // namespace values
} // namespace ajimu

