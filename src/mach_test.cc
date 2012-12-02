#include "mach.cc"
#include "object_management.cc"
#include "object.cc"
#include "lexer.cc"
#include "gmock/gmock.h"

namespace ajimu {
namespace vm {

class MachTest : public ::testing::Test {
protected:
	virtual void SetUp() override {
		mach_ = new Mach();
		mach_->Init();
	}

	virtual void TearDown() override {
		delete mach_;
		mach_ = nullptr;
	}

	Mach *mach_;
};

TEST_F(MachTest, Sanity) {
	Object *ok = mach_->Feed("#f");
	ASSERT_TRUE(ok != nullptr);
	ASSERT_EQ(false, ok->Boolean());

	ok = mach_->Feed("110");
	ASSERT_NE(nullptr, ok);
	ASSERT_EQ(110, ok->Fixed());

	ok = mach_->Feed("(define x 1)");
	ASSERT_NE(nullptr, ok);
	Object *var = mach_->GlobalEnvironment()->Lookup("x");
	ASSERT_NE(nullptr, var);
	ASSERT_TRUE(var->IsFixed());
	ASSERT_EQ(1, var->Fixed());

	ok = mach_->Feed("(define (add a b) (+ a b))");
	ASSERT_NE(nullptr, ok);
	var = mach_->GlobalEnvironment()->Lookup("add");
	ASSERT_NE(nullptr, var);
	ASSERT_TRUE(var->IsClosure());

	ok = mach_->Feed("(+ 1 1)");
	ASSERT_NE(nullptr, ok);
	ASSERT_EQ(2, ok->Fixed());
}

TEST_F(MachTest, Definition) {
	Object *ok = mach_->Feed(
		"(define x 1) "
		"(define y 2) "
		"(+ x y)"
	);
	ASSERT_EQ(3, ok->Fixed());

	ok = mach_->Feed(
		"(define x (+ 1 (* 2 4))) " // x == 9
		"(define y (* x 100)) "     // y == 900
		"(+ x y) "                  // x + y = 909
	);
	ASSERT_EQ(909, ok->Fixed());

	ok = mach_->Feed(
		"(define x (lambda (a) (+ 1 a))) "
		"(x 2)"
	);
	ASSERT_EQ(3, ok->Fixed());

	ok = mach_->Feed(
		"(define (foo a b c) (+ (/ a b) c))"
		"(foo 4 2 6)"
	);
	ASSERT_EQ(8, ok->Fixed());
}

TEST_F(MachTest, Begin) {
	Object *ok = mach_->Feed(
		"(begin (/ 10 2))"
	);
	ASSERT_EQ(5, ok->Fixed());

	ok = mach_->Feed(
		"(define x 0)"
		"(define y 0)"
		"(begin "
		"	(+ 1 2)"
		"	(- 0 2)"
		"	(* 10 10))"
	);
	ASSERT_EQ(100, ok->Fixed());
}

TEST_F(MachTest, Lambda) {
	Object *ok = mach_->Feed(
		"((lambda (a b c) (+ a (* b c))) 1 2 3)"
	);
	ASSERT_EQ(7, ok->Fixed());
}

} // namespace vm
} // namespace ajimu

