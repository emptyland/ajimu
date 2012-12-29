#ifndef AJIMU_VM_LEXER_H
#define AJIMU_VM_LEXER_H

#include <ctype.h>
#include <vector>
#include <functional>
#include <stddef.h>

namespace ajimu {
namespace values {
class ObjectManagement;
class Object;
} // namespace values
namespace vm {

class Lexer {
public:
	typedef std::function<void (const char *, Lexer *)> Observer;

	enum {
		MAX_SYMBOL_LEN = 160,
	};

	Lexer(values::ObjectManagement *obm);

	void AddObserver(const Observer &fn) {
		observer_.push_back(fn);
	}

	void Feed(const char *input, size_t len);

	values::Object *Next();

	values::Object *ReadPair();

	values::Object *ReadCharacter();

	values::Object *ReadNumber();

	values::Object *ReadReal(long long initial, int sign);

	values::Object *ReadSymbol();

	values::Object *ReadString();

	bool Eof() const { return cur_ >= end_; }

	int Line() const { return line_; }

	bool EatWhiteSpace();

	const char *TEST_Current() const {
		return cur_;
	}

	static bool IsDelimiter(int c);

	static bool IsInitial(int c);

private:
	values::ObjectManagement *obm_;
	const char *cur_;
	const char *end_;
	int line_;
	std::vector<Observer> observer_;

	void RaiseError(const char *err) {
		for (Observer fn : observer_) fn(err, this);
	}

	void RaiseErrorf(const char *fmt, ...);

	bool ExpectToken(const char *s);

	bool ExpectDelimiter();

	bool ReadByteX(char *byte);

	Lexer(const Lexer &) = delete;
	void operator = (const Lexer &) = delete;
}; // class Lexer

} // namespace vm
} // namespace ajimu

#endif //AJIMU_VM_LEXER_H

