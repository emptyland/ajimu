#define DECL_TOKEN(v) \
	v(LAMBDA, "lambda")     \
	v(IF, "if")             \
	v(BEGIN, "begin")       \
	v(ELLIPSIS, "...")      \

struct Token {
	enum Identifier {
#define DEFINE_ID(id, name) id,
	DECL_TOKEN(DEFINE_ID)
#undef DEFINE_ID
	};

	Identifier id;
	union {
		long long integer;

		double real;

		struct {
			long long first;
			long long second;
		} pair_number;

		struct {
			size_t len;
			const char *kbuf;
		} slice;

		struct {
			size_t size;
			char *buf;
		} allocated;
	};
};