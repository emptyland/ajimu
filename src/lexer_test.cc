#include "lexer.h"
#include "object_management.h"
#include "string.h"
#include "gmock/gmock.h"
#include <stdio.h>
#include <vector>
#include <map>

namespace ajimu {
namespace vm {

using values::ObjectManagement;
using values::Object;
using values::String;

class LexerTest : public ::testing::Test {
protected:
	virtual void SetUp() override {
		obm_ = new ObjectManagement();
		obm_->Init();
		lexer_ = new Lexer(obm_);
		lexer_->AddObserver([this] (const char *err, Lexer *) {
			printf("error: %s\n", err);
		});
	}

	virtual void TearDown() override {
		delete lexer_;
		lexer_ = nullptr;
		delete obm_;
		obm_ = nullptr;
	}

	ObjectManagement *obm_;
	Lexer *lexer_;
};

TEST_F(LexerTest, Sanity) {
	std::string input = "#f";
	lexer_->Feed(input.c_str(), input.size());

	Object *ob = lexer_->Next();
	ASSERT_TRUE(ob != nullptr);
	ASSERT_EQ(values::BOOLEAN, ob->OwnedType());
	ASSERT_FALSE(ob->Boolean());

	input = "#t";
	lexer_->Feed(input.c_str(), input.size());
	ob = lexer_->Next();
	ASSERT_TRUE(ob != nullptr);
	ASSERT_EQ(values::BOOLEAN, ob->OwnedType());
	ASSERT_TRUE(ob->Boolean());

	input = "#x";
	lexer_->Feed(input.c_str(), input.size());
	ob = lexer_->Next();
	ASSERT_TRUE(ob == nullptr);
}

TEST_F(LexerTest, Character) {
	std::string input("#\\space");
	lexer_->Feed(input.c_str(), input.size());
	Object *ob = lexer_->Next();
	ASSERT_TRUE(ob != nullptr);
	ASSERT_EQ(values::CHARACTER, ob->OwnedType());
	ASSERT_EQ(' ', ob->Character());

	input = "#\\newline";
	lexer_->Feed(input.c_str(), input.size());
	ob = lexer_->Next();
	ASSERT_TRUE(ob != nullptr);
	ASSERT_EQ(values::CHARACTER, ob->OwnedType());
	ASSERT_EQ('\n', ob->Character());

	input = "#\\n #\\s #\\a #\\! #\\?";
	static const char expected[] = {'n', 's', 'a', '!', '?'};
	lexer_->Feed(input.c_str(), input.size());
	for (char c : expected) {
		ob = lexer_->Next();
		ASSERT_TRUE(ob != nullptr);
		ASSERT_EQ(c, ob->Character());
	}
}

TEST_F(LexerTest, Fixed) {
	std::string input("-1");
	lexer_->Feed(input.c_str(), input.size());
	Object *ob = lexer_->Next();
	ASSERT_TRUE(ob != nullptr);
	ASSERT_EQ(values::FIXED, ob->OwnedType());
	ASSERT_EQ(-1LL, ob->Fixed());

	input = "0 1 2 3 4 5 112 -112 65535 +1024 -1024";
	static const long long expected[] = {
		0, 1, 2, 3, 4, 5, 112, -112, 65535, 1024, -1024,
	};
	lexer_->Feed(input.c_str(), input.size());
	for (long long ll : expected) {
		ob = lexer_->Next();
		ASSERT_TRUE(ob != nullptr) << "Unexpected: " << ll;
		ASSERT_EQ(ll, ob->Fixed());
	}
}

TEST_F(LexerTest, Real) {
	std::string input("0.1");
	lexer_->Feed(input.c_str(), input.size());
	Object *ob = lexer_->Next();
	ASSERT_EQ(values::REAL, ob->OwnedType());
	ASSERT_DOUBLE_EQ(0.1, ob->Real());

	input = ".1 0.0002 1000.0001 +0.1 -0.1 -100000.00001";
	static const double expected[] = {
		0.1, 0.0002, 1000.0001, 0.1, -0.1, -100000.00001,
	};
	lexer_->Feed(input.c_str(), input.size());
	for (double val : expected) {
		ob = lexer_->Next();
		ASSERT_NE(nullptr, ob) << "Fail in: " << val
			<< " Current: " << lexer_->TEST_Current();
		ASSERT_DOUBLE_EQ(val, ob->Real());
	}
}

TEST_F(LexerTest, Symbol) {
	static const char *expected[] = {
		"a", "a+", "a-", "+", "-", "*", "/", "is-string?", "good!", "a*b?",
	};
	std::string input;
	for (const char *s : expected) {
		input.append(s);
		input.append(" ");
	}
	lexer_->Feed(input.c_str(), input.size());
	for (const char *s : expected) {
		Object *ob = lexer_->Next();
		ASSERT_TRUE(ob != nullptr) << "Unexpected: " << s;
		ASSERT_STREQ(s, ob->Symbol());
	}
}

TEST_F(LexerTest, Pair) {
	std::string input("()");
	lexer_->Feed(input.c_str(), input.size());
	Object *ob = lexer_->Next();
	ASSERT_TRUE(ob != nullptr);
	ASSERT_EQ(values::PAIR, ob->OwnedType());

	input = "(#f . #t)";
	lexer_->Feed(input.c_str(), input.size());
	ob = lexer_->Next();
	ASSERT_TRUE(ob != nullptr);
	ASSERT_EQ(values::PAIR, ob->OwnedType());
	ASSERT_FALSE(ob->Car()->Boolean());
	ASSERT_TRUE(ob->Cdr()->Boolean());

	input = "(#t #f ())";
	lexer_->Feed(input.c_str(), input.size());
	ob = lexer_->Next();
	ASSERT_TRUE(ob != nullptr);
}

TEST_F(LexerTest, List) {
	/*       +
	 *    /     \
	 * define    +
	 *        /     \
	 *       x       +
	 *             /
	 *            1
	 */
	std::string input("(define x 1)");
	lexer_->Feed(input.c_str(), input.size());
	Object *ob = lexer_->Next();
	ASSERT_NE(nullptr, ob);
	ASSERT_TRUE(ob->IsPair());
	ASSERT_TRUE(ob->Car()->IsSymbol()); // define
	ASSERT_TRUE(cadr(ob)->IsSymbol());  // x
	ASSERT_TRUE(caddr(ob)->IsFixed());   // 1
}

TEST_F(LexerTest, String) {
	static const char *k = "Hello, World!";
	std::string input;
	input.append("\"");
	input.append(k);
	input.append("\"");

	lexer_->Feed(input.c_str(), input.size());
	Object *ob = lexer_->Next();
	ASSERT_NE(nullptr, ob);
	String *str = ob->String();
	ASSERT_STREQ(k, str->c_str());
	ASSERT_EQ(k, str->str());
	ASSERT_EQ(strlen(k), str->Length());
}

TEST_F(LexerTest, StringESC) {
	static const char *k = "\\r\\n\\a\\b\\f\\t\\v\\\\\\\"";
	static const char *e = "\r\n\a\b\f\t\v\\\"";
	std::string input;
	input.append("\"");
	input.append(k);
	input.append("\"");

	lexer_->Feed(input.c_str(), input.size());
	Object *ob = lexer_->Next();
	ASSERT_NE(nullptr, ob);
	String *str = ob->String();
	ASSERT_STREQ(e, str->c_str());
}

TEST_F(LexerTest, StringX) {
	std::string e("\x00\x01\xff\xff\xaf", 5);
	std::string input;
	input.append("\"");
	input.append("\\x00\\x01\\xFF\\xff\\xaF");
	input.append("\"");

	lexer_->Feed(input.c_str(), input.size());
	Object *ob = lexer_->Next();
	ASSERT_NE(nullptr, ob);
	String *ref = ob->String();
	ASSERT_EQ(e, ref->str());
}

TEST_F(LexerTest, SyntaxDefinitionSymbol) {
	std::string input("_");
	lexer_->Feed(input.c_str(), input.size());
	Object *ob = lexer_->Next();
	ASSERT_NE(nullptr, ob);
	ASSERT_STREQ("_", ob->Symbol());
	ASSERT_EQ(obm_->Constant(values::kUnderLineSymbol), ob);

	input = "...";
	lexer_->Feed(input.c_str(), input.size());
	ob = lexer_->Next();
	ASSERT_NE(nullptr, ob);
	ASSERT_STREQ("...", ob->Symbol());
	ASSERT_EQ(obm_->Constant(values::kEllipsisSymbol), ob);
}

TEST_F(LexerTest, Comment) {
	std::string input(
	"; (display 'comment)\n"
	"Comment ; (display 'newline)\n"
	"; (final)"
	);
	lexer_->Feed(input.c_str(), input.size());
	Object *o = lexer_->Next();
	ASSERT_NE(nullptr, o);
	ASSERT_STREQ("Comment", o->Symbol());
}

} // namespace vm
} // namespace ajimu

