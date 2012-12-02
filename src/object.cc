#include "object.h"

namespace ajimu {
namespace values {

Object::~Object() {
	switch (OwnedType()) {
	case STRING: 
		String().Drop();
		break;
	case SYMBOL:
		delete[] symbol_.name;
		break;
	default:
		break;
	}
}

} // namespace values
} // namespace ajimu

