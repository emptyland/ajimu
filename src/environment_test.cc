#include "environment.h"
#include "gmock/gmock.h"

namespace ajimu {
namespace vm {

using values::Object;

#define O(p) reinterpret_cast<Object*>(p)
static Object *kFoo = O(0x1L);
static Object *kBaz = O(0x2L);
static Object *kBar = O(0x3L);
#undef O

TEST(EnvironmentTest, Sanity) {
	Environment l1(nullptr), l2(&l1), l3(&l2);

	l1.Define("foo", kFoo);
	l2.Define("baz", kBaz);
	l3.Define("bar", kBar);

	ASSERT_EQ(kFoo, l1.Lookup("foo"));
	ASSERT_EQ(kBaz, l2.Lookup("baz"));
	ASSERT_EQ(kBar, l3.Lookup("bar"));

	ASSERT_EQ(nullptr, l1.Lookup("baz"));
	ASSERT_EQ(nullptr, l2.Lookup("bar"));
	ASSERT_EQ(nullptr, l3.Lookup("foo"));

	{
		Environment::Handle handle("foo", &l3);
		ASSERT_TRUE(handle.Valid());
		ASSERT_EQ(kFoo, handle.Get());
	}

	{
		Environment::Handle handle("baz", &l3);
		ASSERT_TRUE(handle.Valid());
		ASSERT_EQ(kBaz, handle.Get());
	}
}

} // namespace vm
} // namespace ajimu

