#include "local.h"
#include "object.h"
#include "string.h"
#include "gmock/gmock.h"

namespace ajimu {
namespace vm {

using values::Object;
using values::String;

class LocalTest : public ::testing::Test {
protected:
	virtual void SetUp() override {
		local_ = new Local<Object>();
	}

	virtual void TearDown() override {
		delete local_;
		local_ = nullptr;
	}

	Object *Int2Ptr(intptr_t val) {
		union {
			intptr_t input;
			Object * output;
		};
		input = val; return output;
	}

	Local<Object> *local_;
};

#define P(i) Int2Ptr(i)

TEST_F(LocalTest, Sanity) {
	local_->Push(P(1));
	local_->Push(P(2));

	ASSERT_EQ(2U, local_->Count());
	ASSERT_EQ(P(1), local_->Last(1));
	ASSERT_EQ(P(2), local_->Last(0));

	local_->Pop(1);
	ASSERT_EQ(1U, local_->Count());
	local_->Pop(1);
	ASSERT_EQ(0U, local_->Count());
}

} // namespace vm
} // namespace ajimu

