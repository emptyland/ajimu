#include "object_management.h"
#include "gmock/gmock.h"

namespace ajimu {
namespace values {

using vm::Environment;

class ObjectManagementTest : public ::testing::Test {
protected:
	virtual void SetUp() override {
		obm_ = new ObjectManagement();
		obm_->Init();
	}

	virtual void TearDown() override {
		delete obm_;
		obm_ = nullptr;
	}

	ObjectManagement *obm_;
};

TEST_F(ObjectManagementTest, Sanity) {
	Object *ob1 = obm_->NewFixed(0);
	ASSERT_EQ(FIXED, ob1->OwnedType());
	ASSERT_EQ(0LL, ob1->Fixed());

	Object *ob2 = obm_->NewFixed(65535);
	ASSERT_EQ(FIXED, ob2->OwnedType());
	ASSERT_EQ(65535LL, ob2->Fixed());

	Object *pair = obm_->Cons(ob1, ob2);
	ASSERT_EQ(PAIR, pair->OwnedType());
	ASSERT_EQ(ob1, pair->Car());
	ASSERT_EQ(ob2, pair->Cdr());

	Object *ob3 = obm_->NewBoolean(false);
	ASSERT_EQ(BOOLEAN, ob3->OwnedType());
	ASSERT_FALSE(ob3->Boolean());

	Object *ob4 = obm_->NewBoolean(true);
	ASSERT_EQ(BOOLEAN, ob4->OwnedType());
	ASSERT_TRUE(ob4->Boolean());
}

TEST_F(ObjectManagementTest, Symbol) {
	Object *ob1 = obm_->NewSymbol("a");
	ASSERT_STREQ("a", ob1->Symbol());

	Object *ob2 = obm_->NewSymbol("");
	ASSERT_STREQ("", ob2->Symbol());

	ASSERT_EQ(ob1, obm_->NewSymbol("a"));
	ASSERT_EQ(ob2, obm_->NewSymbol(""));
}

TEST_F(ObjectManagementTest, Environment) {
	Environment *env = obm_->GlobalEnvironment();
	ASSERT_NE(nullptr, env);
	ASSERT_EQ(env, obm_->GlobalEnvironment());

	ASSERT_NE(env, obm_->NewEnvironment(env));
}

TEST_F(ObjectManagementTest, CollectEnvironment) {
	Environment *env;
	int i = 100;
	while (i--) {
		env = obm_->TEST_NewEnvironment(nullptr);
		ASSERT_NE(nullptr, env);
	}
}

TEST_F(ObjectManagementTest, CollectObject) {
	int i = 10000;
	while (i--) {
		Object *o = obm_->NewFixed(10000);
		ASSERT_NE(nullptr, o);
	}
}

} // namespace values
} // namespace ajimu

