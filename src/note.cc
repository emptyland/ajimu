

namespace ajimu {

class Mach {

	void Run(Chunk);

	Chunk *Feed(const char *script, size_t len);

	ObjectManagement obm_;
};

class Lex {
public:
	Lex(ObjectManagement *obm)
		: obm_(obm)
		, cur_(nullptr)
		, end_(nullptr)
		, line_(0) {
	}

	Chunk *Feed(const char *script, size_t len) {
		DCHECK(script);
		DCHECK_NE(len, 0);
		cur_ = script;
		end_ = cur_ + len;
		std::vector<Object *> list;
		while (Eof()) {
			Object *ob = Next();
			if (!ob)
				return nullptr; // Error!
			list.push_back(ob);
		}
			//list.push_back(Next());
		return obm_->NewChunk(&list[0], list.size());
	}

	#define Kof(i) obm_->Constant(ObjectConstant::##i)
	Object *Next() {
		for (;;) {
			switch (*cur_) {
			case '#':
				++cur_;
				switch (*cur_) {
				case 't':
				case 'T':
					return Kof(True);
				case 'f':
				case 'F':
					return Kof(False);
				case '\\':
					return ReadCharacter();
				default:
					TraceError("Need t/f or a \'\\\' character.");
				}
				return nullptr;
			case '\"':
				break;
			case '(':
				return ReadPair();
			}
		}
	}

	bool Eof() { return cur_ >= end_; }

	Object *ReadPair() {
		if (!EatWhiteSpace())
			return nullptr;
		if (*cur_++ == ')')
			return Kof(EmptyList);
		Object *car = Next();
		if (!car)
			return nullptr;
		if (!EatWhiteSpace())
			return nullptr;
		if (cur_[1] != '.') {
			Object *cdr = ReadPair();
			return obm_->Cons(car, cdr);
		}
		++cur_;
		if (!IsDelimiter(cur_[1])) {
			TraceError("Do not followed by delimiter.");
			return nullptr;
		}
		Object *cdr = Next();
		if (!cdr || !EatWhiteSpace())
			return nullptr;
		if (*cur_++ != ')') {
			TraceError("Where was trailing right paren?");
			return nullptr;
		}
		return obm_->Cons(car, cdr);
	}

	bool EatWhiteSpace() {
		do {
			++cur_;
		} while (isspace(*cur_) && !Eof())
		return !Eof();
	}

	static bool IsDelimiter(int c) {
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

private:
	ObjectManagement *obm_;
	const char *cur_;
	const char *end_;
	int line_;
	std::string last_error_;
};

class ObjectManagement {

	Object *Cons(Object *car, Object *cdr) {
		Object *ob = New(Object::PAIR);
		ob->car = car;
		ob->cdr = cdr;
		return ob;
	}

	Object *kTrue;
	Object *kFalse;
};




class Object {

private:
	friend class ObjectManagement;

	Type type_;
	union {
		// boolean: #t/#f/#true/#false
		bool boolean_;

		// symbol: foo, bar
		String *symbol_;

		// number:
		long long fixed_;

		char character_;

		String *string_;

		struct {
			Object *car;
			Object *cdr;
		} pair_;

		// TODO:
	};
};

} // namespace ajimu