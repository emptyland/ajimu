#include "lexer.h"
#include "object_management.h"
#include "glog/logging.h"
#include <stdarg.h>
#include <stdio.h>

namespace ajimu {
namespace vm {

using values::ObjectManagement;
using values::Object;

Lexer::Lexer(ObjectManagement *obm)
	: obm_(obm)
	, cur_(nullptr)
	, end_(nullptr)
	, line_(0) {
}

void Lexer::Feed(const char *input, size_t len) {
	DCHECK(input);
	DCHECK_NE(0U, len);
	cur_ = input;
	end_ = cur_ + len;
	line_ = 0;
}

#define Kof(i) obm_->Constant(::ajimu::values::k##i)
Object *Lexer::Next() {
	DCHECK(cur_ != nullptr) << "Need call Feed() first!";

	EatWhiteSpace();
	while(!Eof()) {
		switch (*cur_) {
		case '#':
			++cur_;
			switch (*cur_++) {
			case 't':
				return Kof(True);
			case 'f':
				return Kof(False);
			case '\\':
				return ReadCharacter();
			default:
				RaiseError("Unknown # boolean or character.");
			}
			return nullptr;
		case '\"':
			return ReadString();
		case '(':
			++cur_;
			return ReadPair();
		case '-': case '+':
			if (IsDelimiter(cur_[1]))
				return ReadSymbol();
			return ReadNumber();
		case '.':
			if (isdigit(cur_[1]))
				return ReadReal(0LL, +1);
			return ReadSymbol();
		case '\'':
			++cur_;
			return obm_->Cons(Kof(QuoteSymbol),
					obm_->Cons(Next(), Kof(EmptyList)));
		case ';':
			while (!Eof() && *cur_++ != '\n')
				(void)0;
			break;
		default:
			if (isdigit(*cur_))
				return ReadNumber();
			else if (IsInitial(*cur_))
				return ReadSymbol();
			RaiseErrorf("Unknown token. char: \"%c\"", *cur_);
			goto final;
		}
	}
final:
	return nullptr;
}

Object *Lexer::ReadCharacter() {
	if (Eof()) {
		RaiseError("Incomplete character literal.");
		return nullptr;
	}

	char c = *cur_++;
	switch (c) {
	case 's':
		if (*cur_ == 'p') {
			if (!ExpectToken("pace") ||
				!ExpectDelimiter())
				return nullptr;
			return obm_->NewCharacter(' ');
		}
		break;
	case 'n':
		if (*cur_ == 'e') {
			if (!ExpectToken("ewline") ||
				!ExpectDelimiter())
				return nullptr;
			return obm_->NewCharacter('\n');
		}
		break;
	}

	return ExpectDelimiter() ? obm_->NewCharacter(c) : nullptr;
}

Object *Lexer::ReadPair() {
	if (!EatWhiteSpace())
		return nullptr;
	if (*cur_ == ')') { // Is empty list(pair) ?
		++cur_;
		return Kof(EmptyList);
	}
	Object *car = Next();
	if (!car)
		return nullptr;
	if (!EatWhiteSpace())
		return nullptr;
	if (cur_[0] != '.') {
		Object *cdr = ReadPair();
		return obm_->Cons(car, cdr);
	}
	if (cur_[1] == '.' && cur_[2] == '.') {
		Object *cdr = ReadPair();
		return obm_->Cons(car, cdr);
	}
	++cur_;
	if (!IsDelimiter(*cur_)) {
		RaiseError("Do not followed by delimiter.");
		return nullptr;
	}
	Object *cdr = Next();
	if (!cdr || !EatWhiteSpace())
		return nullptr;
	if (*cur_++ != ')') {
		RaiseError("Where was trailing right paren?");
		return nullptr;
	}
	return obm_->Cons(car, cdr);
}

Object *Lexer::ReadNumber() {
	long long sign = 1;
	if (*cur_ == '-') {
		++cur_; sign = -1;
	} else if (*cur_ == '+') {
		++cur_; sign = 1;
	}
	long long num = 0;
	while (!IsDelimiter(*cur_) && !Eof()) {
		if (isdigit(*cur_)) {
			num = (num * 10) + (*cur_++ - '0');
		} else if (*cur_ == '.') {
			return ReadReal(num, sign);
		} else {
			RaiseError("Unexpected digit character.");
			return nullptr;
		}
	}
	return ExpectDelimiter() ? obm_->NewFixed(num * sign) : nullptr;
}

Object *Lexer::ReadReal(long long initial, int sign) {
	++cur_; // Skip `.'
	char literal[1024] = {0};
	snprintf(literal, sizeof(literal),
			sign < 0 ? "-%lld." : "%lld.", initial);

	size_t p = strlen(literal);
	while (!IsDelimiter(*cur_) && !Eof()) {
		if (isdigit(*cur_)) {
			literal[p++] = *cur_++;
		} else {
			RaiseError("Unexpected digit character for float number.");
			return nullptr;
		}
	}
	literal[p] = '\0';
	return ExpectDelimiter() ?
		obm_->NewReal(atof(literal)) : nullptr;
}

Object *Lexer::ReadSymbol() {
	char buf[MAX_SYMBOL_LEN] = {0};
	size_t len = 0;

	while (IsInitial(*cur_)
			|| isdigit(*cur_)
			|| *cur_ == '+'
			|| *cur_ == '-') {
		if (len >= sizeof(buf)) {
			RaiseErrorf("Symbol too long, Maximum is : %d .",
				MAX_SYMBOL_LEN);
			return nullptr;
		}
		buf[len++] = *cur_++;
	}
	if (!IsDelimiter(*cur_)) {
		RaiseErrorf("Symbol not followed by delimiter. "
				"Unexpected character: %c.", *cur_);
		return nullptr;
	}
	buf[len] = '\0';
	//--cur_;
	return obm_->NewSymbol(buf);
}

#define APPEND(buf, len, c) \
	if (Eof()) { \
		RaiseError("ReadString : Non-terminated string literal."); \
		return nullptr; \
	} \
	if (len < sizeof(buf) - 1) \
		buf[len++] = c; \
	else { \
		RaiseErrorf("String too long, Maximum length is %d.", \
				MAX_SYMBOL_LEN); \
		return nullptr; \
	}(void)0

Object *Lexer::ReadString() {
	++cur_; // Skip first `"'
	char buf[MAX_SYMBOL_LEN] = {0};
	char c;
	size_t len = 0;
	while ((c = *cur_++) != '"') {
		if (c == '\\') {
			c = *cur_++;
			switch (c) {
			case 'a':
				c = '\a';
				break;
			case 'b':
				c = '\b';
				break;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'v':
				c = '\v';
				break;
			case '0':
				c = '\0';
				break;
			case '\\':
				c = '\\';
				break;
			case '"':
				c = '"';
				break;
			case 'x':
				if (!ReadByteX(&c)) return nullptr;
				break;
			}
		}
		APPEND(buf, len, c);
	}
	buf[len] = '\0';
	return obm_->NewString(buf, len);
}
#undef APPEND

bool Lexer::ReadByteX(char *byte) {
	char bit4[2];
	for (int i = 0; i < 2; ++i) {
		if (Eof()) {
			RaiseError("ReadByteX : Non-terminated \\xNN hex number.");
			return false;
		}
		char c = *cur_++;
		if (!isxdigit(c)) {
			RaiseErrorf("ReadByteX : Non-Hex number characer: %c ", c);
			return false;
		}
		if (isdigit(c)) {
			bit4[i] = c - '0';
		} else if (islower(c)) {
			bit4[i] = c - 'a' + 10;
		} else if (isupper(c)) {
			bit4[i] = c - 'A' + 10;
		} else {
			DLOG(FATAL) << "No reached!";
		}
	}
	*byte = (bit4[0] << 4) | bit4[1];
	return true;
}

bool Lexer::EatWhiteSpace() {
	while (!Eof() && isspace(*cur_)) {
		if (*cur_++ == '\n')
			++line_;
	}
	return !Eof();
}

/*static*/ bool Lexer::IsDelimiter(int c) {
	switch (c) {
	case '\0':
	case '(':
	case ')':
	case '\"':
	case ';':
		return true;
	default:
		return isspace(c);
	}
	return false;
}

/*static*/ bool Lexer::IsInitial(int c) {
	// All R7RS identifiers
	switch (c) {
	case '*':
	case '/':
	case '>':
	case '<':
	case '=':
	case '?':
	case '!':
	case '.':
	case ':':
	case '@':
	case '^':
	case '_':
	case '~':
		return true;
	default:
		return isalpha(c);
	}
	return false;
}

void Lexer::RaiseErrorf(const char *fmt, ...) {
	char buf[1024] = {0};
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	RaiseError(buf);
}

bool Lexer::ExpectToken(const char *s) {
	while (*s) {
		if (Eof() || *cur_ != *s++) {
			RaiseError("Incomplete token.");
			return false;
		}
		++cur_;
	}
	return true;
}

bool Lexer::ExpectDelimiter() {
	if (!IsDelimiter(*cur_)) {
		RaiseError("Unexpected delimiter.");
		return false;
	}
	return true;
}

#undef Kof
} // namespace vm
} // namespace ajimu

