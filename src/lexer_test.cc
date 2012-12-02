#include "lexer.cc"
#include "object_management.cc"
#include "object.cc"
#include "gmock/gmock.h"
#include <stdio.h>

namespace ajimu {
namespace vm {

class LexerTest : public ::testing::Test {
protected:
	virtual void SetUp() override {
		obm_ = new values::ObjectManagement();
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

	values::ObjectManagement *obm_;
	Lexer *lexer_;
};

TEST_F(LexerTest, Sanity) {
	std::string input = "#f";
	lexer_->Feed(input.c_str(), input.size());

	values::Object *ob = lexer_->Next();
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
	values::Object *ob = lexer_->Next();
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

TEST_F(LexerTest, Number) {
	std::string input("-1");
	lexer_->Feed(input.c_str(), input.size());
	values::Object *ob = lexer_->Next();
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
		values::Object *ob = lexer_->Next();
		ASSERT_TRUE(ob != nullptr) << "Unexpected: " << s;
		ASSERT_STREQ(s, ob->Symbol());
	}
}

TEST_F(LexerTest, Pair) {
	std::string input("()");
	lexer_->Feed(input.c_str(), input.size());
	values::Object *ob = lexer_->Next();
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
	values::Object *ob = lexer_->Next();
	ASSERT_NE(nullptr, ob);
	ASSERT_TRUE(ob->IsPair());
	ASSERT_TRUE(ob->Car()->IsSymbol()); // define
	ASSERT_TRUE(cadr(ob)->IsSymbol());  // x
	ASSERT_TRUE(caddr(ob)->IsFixed());   // 1
}

} // namespace vm
} // namespace ajimu

