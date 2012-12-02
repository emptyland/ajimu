#include "lexer.h"
#include "object_management.h"
#include "glog/logging.h"
#include <stdarg.h>
#include <stdio.h>

namespace ajimu {
namespace vm {

Lexer::Lexer(values::ObjectManagement *obm)
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

#define Kof(i) obm_->Constant(::ajimu::values::ObjectManagement::k##i)
values::Object *Lexer::Next() {
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
			break;
		case '(':
			++cur_;
			return ReadPair();
		case '-': case '+':
			if (IsDelimiter(cur_[1]))
				return ReadSymbol();
			return ReadFixed();
		case '\'':
			++cur_;
			return obm_->Cons(Kof(QuoteSymbol),
					obm_->Cons(Next(), Kof(EmptyList)));
		default:
			if (isdigit(*cur_))
				return ReadFixed();
			else if (IsInitial(*cur_))
				return ReadSymbol();
			RaiseError("Unknown token.");
			goto final;
		}
	}
final:
	return nullptr;
}

values::Object *Lexer::ReadCharacter() {
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

values::Object *Lexer::ReadPair() {
	if (!EatWhiteSpace())
		return nullptr;
	if (*cur_ == ')') { // Is empty list(pair) ?
		++cur_;
		return Kof(EmptyList);
	}
	values::Object *car = Next();
	if (!car)
		return nullptr;
	if (!EatWhiteSpace())
		return nullptr;
	if (*cur_ != '.') {
		values::Object *cdr = ReadPair();
		return obm_->Cons(car, cdr);
	}
	++cur_;
	if (!IsDelimiter(*cur_)) {
		RaiseError("Do not followed by delimiter.");
		return nullptr;
	}
	values::Object *cdr = Next();
	if (!cdr || !EatWhiteSpace())
		return nullptr;
	if (*cur_++ != ')') {
		RaiseError("Where was trailing right paren?");
		return nullptr;
	}
	return obm_->Cons(car, cdr);
}

values::Object *Lexer::ReadFixed() {
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
		} else {
			RaiseError("Unexpected digit character.");
			return nullptr;
		}
	}
	return ExpectDelimiter() ? obm_->NewFixed(num * sign) : nullptr;
}

values::Object *Lexer::ReadSymbol() {
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
	switch (c) {
	case '*':
	case '/':
	case '>':
	case '<':
	case '=':
	case '?':
	case '!':
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

} // namespace vm
} // namespace ajimu

