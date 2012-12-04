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

TEST_F(MachTest, Assignment) {
	Object *ok = mach_->Feed(
		"(define x 0)"
		"(define y 0)"
		"(set! x 100)"
		"(set! y 2)"
		"(* x y)"
	);
	ASSERT_EQ(200, ok->Fixed());
}

TEST_F(MachTest, If) {
	Object *ok = mach_->Feed("(if #t 1)");
	ASSERT_EQ(1, ok->Fixed());
	ok = mach_->Feed("(if #f 1 0)");
	ASSERT_EQ(0, ok->Fixed());
	ok = mach_->Feed(
		"(define (foo a b c)"
		"	(and (= a b) (= b c)))"
		"(foo 1 1 1)"
	);
	ASSERT_TRUE(ok->Boolean());
	ok = mach_->Feed("(foo 1 1 2)");
	ASSERT_FALSE(ok->Boolean());
	ok = mach_->Feed(
		"(define (<= a b)"
		"	(or (= a b) (< a b)))"
		"(if (<= 2 2)"
		"	100"
		"	200)"
	);
	ASSERT_EQ(100, ok->Fixed());
}

TEST_F(MachTest, Let) {
	Object *ok = mach_->Feed(
		"(let ((a 1) (b 2))"
		"	(+ a b))"
	);
	ASSERT_EQ(3, ok->Fixed());
	ok = mach_->Feed(
		"(let ((x 1) (y 2))"
		"	(let ((x 2) (y 3))"
		"		(* x y)))"
	);
	ASSERT_EQ(6, ok->Fixed());
	ok = mach_->Feed(
		"(define x 100)"
		"(let ((a 1) (b 2))"
		"	(let ((c 3) (d 4))"
		"		(+ (+ (+ a b) (+ c d)) x)))"
	);
	ASSERT_EQ(110, ok->Fixed());
}

TEST_F(MachTest, Lambda) {
	Object *ok = mach_->Feed(
		"((lambda (a b c) (+ a (* b c))) 1 2 3)"
	);
	ASSERT_EQ(7, ok->Fixed());
}

TEST_F(MachTest, Eval) {
	Object *ok = mach_->Feed("(eval \'(+ 1 1))");
	ASSERT_EQ(2, ok->Fixed());
}

TEST_F(MachTest, List) {
	Object *ok = mach_->Feed("(cons 1 #t)");
	ASSERT_TRUE(ok->IsPair());
	ASSERT_EQ(1, car(ok)->Fixed());
	ASSERT_TRUE(cdr(ok)->Boolean());

	ok = mach_->Feed("(cons 1 (cons #t -1))");
	ASSERT_EQ(1, car(ok)->Fixed());
	ASSERT_TRUE(cadr(ok)->Boolean());
	ASSERT_EQ(-1, cddr(ok)->Fixed());

	ok = mach_->Feed(
		"(define foo '(1 #f 2 #t))"
		"(car foo)"
	);
	ASSERT_EQ(1, ok->Fixed());

	ok = mach_->Feed("(cdr foo)");
	ASSERT_FALSE(car(ok)->Boolean());

	ok = mach_->Feed(
		"(define baz (list 9 8 7 6))"
		"(set-car! baz -1)"
		"(car baz)"
	);
	ASSERT_EQ(-1, ok->Fixed());

	ok = mach_->Feed(
		"(set-cdr! baz -2)"
		"(cdr baz)"
	);
	ASSERT_EQ(-2, ok->Fixed());
}

TEST_F(MachTest, Cond) {
	Object *ok = mach_->Feed(
		"(define (foo x)"
		"	(cond"
		"		((= x 1) 10)"
		"		((= x 2) 20)"
		"		((= x 3) 30)"
		"		(else \"Suck my balls!\")"
		"	)"
		")"
		"(foo 2)"
	);
	ASSERT_EQ(20, ok->Fixed());

	ok = mach_->Feed("(foo 3)");
	ASSERT_EQ(30, ok->Fixed());

	ok = mach_->Feed("(foo 1)");
	ASSERT_EQ(10, ok->Fixed());

	ok = mach_->Feed("(foo 20)");
	ASSERT_EQ("Suck my balls!", ok->String().str());

	ok = mach_->Feed("(foo 1)");
	ASSERT_EQ(10, ok->Fixed());
}

TEST_F(MachTest, Load) {
	Object *ok = mach_->Feed(
		"(load \"src/lib/stdlib.scm\")"
		"(define seq '(0 1 2 3 4 5))"
		"(cadr seq)"
	);
	ASSERT_EQ(1, ok->Fixed());

	ok = mach_->Feed("(car (cddr seq))");
	ASSERT_EQ(2, ok->Fixed());
}

TEST_F(MachTest, Types) {
	Object *ok = mach_->Feed(
		"(define (typeof obj)"
		"	(cond "
		"		((null?      obj) \"null\")"
		"		((symbol?    obj) \"symbol\")"
		"		((number?    obj) \"number\")"
		"		((pair?      obj) \"pair\")"
		"		((procedure? obj) \"procedure\")"
		"		(else \"other\")"
		"	)"
		")"
		"(typeof 1)"
	);
	ASSERT_STREQ("number", ok->String().c_str());

	ok = mach_->Feed("(typeof typeof)");
	ASSERT_STREQ("procedure", ok->String().c_str());

	ok = mach_->Feed("(typeof 'typeof)");
	ASSERT_STREQ("symbol", ok->String().c_str());

	ok = mach_->Feed("(typeof '())");
	ASSERT_STREQ("null", ok->String().c_str());

	ok = mach_->Feed("(typeof '(1 2))");
	ASSERT_STREQ("pair", ok->String().c_str());
}

} // namespace vm
} // namespace ajimu

