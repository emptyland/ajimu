#include "object.h"
#include "gmock/gmock.h"
#include "glog/logging.h"

namespace ajimu {
namespace values {

TEST(ObjectTest, Sanity) {
	printf("sizeof(Object): %zd\n", sizeof(Object));
	printf("sizeof(Object*): %zd\n", sizeof(Object*));
}

} // namespace values
} // namespace ajimu

