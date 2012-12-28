#include "macro_analyzer.h"
#include "lexer.h"
#include "object_management.h"
#include "gmock/gmock.h"

namespace ajimu {
namespace vm {

using values::ObjectManagement;
using values::Object;

class MacroAnalyzerTest : public ::testing::Test {
protected:
	virtual void SetUp() override {
		obm_ = new ObjectManagement();
		obm_->Init();
		factory_ = new MacroAnalyzer(obm_);
		lexer_ = new Lexer(obm_);
		lexer_->AddObserver([this] (const char *err, Lexer *) {
			printf("error: %s\n", err);
		});
	}

	virtual void TearDown() override {
		delete lexer_;
		lexer_ = nullptr;
		delete factory_;
		factory_ = nullptr;
		delete obm_;
		obm_ = nullptr;
	}

	Object *AssertMakeSyntax(const char *script) {
		size_t len = strlen(script);
		lexer_->Feed(script, len);
		return lexer_->Next();
	}

	void AssertExtend(const char *expected, const char *script,
			Object *syntax) {
		size_t len = strlen(script);
		lexer_->Feed(script, len);
		Object *o = lexer_->Next();
		o = factory_->Extend(syntax, o);
		ASSERT_NE(nullptr, o);
		if (expected)
			ASSERT_EQ(expected, o->ToString(obm_));
		else
			printf("Extended: %s\n", o->ToString(obm_).c_str());
	}

	MacroAnalyzer *factory_;
	ObjectManagement *obm_;
	Lexer *lexer_;
};

TEST_F(MacroAnalyzerTest, Sanity) {
	Object *syntax, *o;
	std::string input;
	input =
	"(define-syntax let"
	"	(syntax-rules ()"
	"		((_ ((x v) ...) body ...)"
	"		((lambda (x ...) body ...) v ...))))";
	lexer_->Feed(input.c_str(), input.size());
	syntax = lexer_->Next();
	ASSERT_NE(nullptr, syntax);

	input = "(let ((x 1) (y 2)) (+ x y) (- x y))";
	lexer_->Feed(input.c_str(), input.size());
	o = lexer_->Next();
	ASSERT_NE(nullptr, o);

	o = factory_->Extend(syntax, o);
	ASSERT_NE(nullptr, o);
	ASSERT_EQ("((lambda (x y) (+ x y) (- x y)) 1 2)", o->ToString(obm_));

	input =
	"(define-syntax and"
	"	(syntax-rules ()"
	"		((_) #t)"
	"		((_ e) e)"
	"		((_ e1 e2)"
	"			(if e1 e2 #f))"
	"		((_ e1 e2 e3 ...)"
	"			(if e1 (and e2 e3 ...) #f))))";
	lexer_->Feed(input.c_str(), input.size());
	syntax = lexer_->Next();
	ASSERT_NE(nullptr, syntax);

	input = "(and 1 2 3 4 5)";
	lexer_->Feed(input.c_str(), input.size());
	o = lexer_->Next();
	ASSERT_NE(nullptr, o);

	o = factory_->Extend(syntax, o);
	ASSERT_NE(nullptr, o);
	ASSERT_EQ("(if 1 (and 2 3 4 5) #f)", o->ToString(obm_));
}

TEST_F(MacroAnalyzerTest, AndSyntax) {
	Object *s = AssertMakeSyntax(
	"(define-syntax and"
	"	(syntax-rules ()"
	"		((_) #t)"
	"		((_ e) e)"
	"		((_ e1 e2)"
	"			(if e1 e2 #f))"
	"		((_ e1 e2 e3 ...)"
	"			(if e1 (and e2 e3 ...) #f))))");
	AssertExtend("#t",
			"(and)", s);
	AssertExtend("1", 
			"(and 1)", s);
	AssertExtend("(if 1 2 #f)",
			"(and 1 2)", s);
	AssertExtend("(if 1 (and 2 3) #f)",
			"(and 1 2 3)", s);
	AssertExtend("(if 1 (and 2 3 4) #f)",
			"(and 1 2 3 4)", s);
}

TEST_F(MacroAnalyzerTest, OrSyntax) {
	Object *s = AssertMakeSyntax(
	"(define-syntax or"
	"	(syntax-rules ()"
	"		((_) #f)"
	"		((_ e) e)"
	"		((_ e1 e2)"
	"			(let ((t e1))"
	"				(if t t e2)))"
	"		((_ e1 e2 e3 ...)"
	"			(let ((t e1))"
	"				(if t t (or e2 e3 ...))))))"
	);
	AssertExtend("#f",
			"(or)", s);
	AssertExtend("1",
			"(or 1)", s);
	AssertExtend("(let ((t 1)) (if t t 2))",
			"(or 1 2)", s);
	AssertExtend("(let ((t 1)) (if t t (or 2 3)))",
			"(or 1 2 3)", s);
	AssertExtend("(let ((t 1)) (if t t (or 2 3 4)))",
			"(or 1 2 3 4)", s);
}

TEST_F(MacroAnalyzerTest, WhenSyntax) {
	Object *s = AssertMakeSyntax(
	"(define-syntax when"
	"	(syntax-rules ()"
	"		((_ test expr ...)"
	"			(if test (begin expr ...)))))"
	);
	AssertExtend("(if (= a b) (begin (display (quote equals))))",
			"(when (= a b) (display 'equals))", s);
	AssertExtend("(if (> a b) (begin (display (quote great)) "
			"(display (quote good)) a))",
			"(when (> a b)"
			"	(display 'great)"
			"	(display 'good)"
			"	a)", s);
}

TEST_F(MacroAnalyzerTest, UnlessSyntax) {
	Object *s = AssertMakeSyntax(
	"(define-syntax unless"
	"	(syntax-rules ()"
	"		((_ test expr ...)"
	"			(if (not test) (begin expr ...)))))"
	);
	AssertExtend("(if (not (= a b)) (begin (display (quote no-equals))))",
			"(unless (= a b) (display 'no-equals))", s);
}

} // namespace vm
} // namespace ajimu

