#include "object.h"
#include "object_management.h"
#include "string.h"
#include "utils.h"

namespace ajimu {
namespace values {

std::string PrimitiveValue2String(Object *o);

Object::~Object() {
	switch (OwnedType()) {
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
		return std::move(String()->str());
	case PAIR: {
			if (obm->Null(this))
				return "null";
			int i = 0;
			std::string str("(");
			Object *o = this;
			while (!obm->Null(o)) {
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

} // namespace values
} // namespace ajimu

