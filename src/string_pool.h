#ifndef AJIMU_VALUES_STRING_POOL_H
#define AJIMU_VALUES_STRING_POOL_H

#include "string.h"

namespace ajimu {
namespace values {

class StringPool {
public:

private:
	StringPool(const StringPool &) = delete;
	void operator = (const StringPool &) = delete;
}; // class StringPool

} // namespace values
} // namespace ajimu

#endif //AJIMU_VALUES_STRING_POOL_H

