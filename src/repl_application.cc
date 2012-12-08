#include "repl_application.h"
#include "mach.h"
#include "lexer.h"
#include "object_management.h"
#include "object.h"
#include "string.h"
#include <stdio.h>
#include <string>

namespace ajimu {
namespace app {

#define cRED    "\033[1;31m"
#define cGREEN  "\033[1;32m"
#define cYELLOW "\033[1;33m"
#define cBLUE   "\033[1;34m"
#define cPURPLE "\033[1;35m"
#define cAZURE  "\033[1;36m"

#define cDARK_RED    "\033[0;31m"
#define cDARK_GREEN  "\033[0;32m"
#define cDARK_YELLOW "\033[0;33m"
#define cDARK_BLUE   "\033[0;34m"
#define cDARK_PURPLE "\033[0;35m"
#define cDARK_AZURE  "\033[0;36m"

#define cEND    "\033[0m"

static const char *kTitle = \
"   _      _  _ _   _  _  _ \n"
"  / |    // // /| /| ||  ||\n"
" //_| __// // //|/|| ||  ||\n"
"//  |/++/ // //   || |_++_|\n"
"----------------------------\n";
static const char *kDesc = \
"Ajimu scheme shell.\n"
"Ctrl+D or Ctrl+C for exit.\n";

void *PrimitiveMethod2Pv(values::PrimitiveMethodPtr ptr) {
	union {
		values::PrimitiveMethodPtr pfn;
		void *pv;
	};
	pfn = ptr; return pv;
}

ReplApplication::ReplApplication()
	: color_mode_(AUTO)
	, mach_(new vm::Mach())
	, input_(stdin)
	, output_(stdout) {
}

ReplApplication::~ReplApplication() {
}

bool ReplApplication::Init() {
	using namespace std::placeholders;

	bool ok = mach_->Init();
	if (!ok)
		return false;
	mach_->AddObserver(std::bind(
				&ReplApplication::HandleError, this, _1, _2));
	fprintf(output_, "%s%s%s%s",
			Paint(cGREEN), kTitle, Paint(cEND), kDesc);
	return ok;
}

values::Object *ReplApplication::Load(const char *lib) {
	return mach_->EvalFile(lib);
}

int ReplApplication::Run() {
	std::string line;
	while (ReadLine(&line)) {
		mach_->Lex()->Feed(line.c_str(), line.size());

		values::Object *o, *rv;
		o = mach_->Lex()->Next();
		if (!o)
			continue;
		rv = mach_->Eval(o, mach_->GlobalEnvironment());
		if (!rv)
			continue;
		Print(rv);
		fprintf(output_, "\n");
	}
	return 0;
}

bool ReplApplication::ReadLine(std::string *line) {
	char partial[1024] = {0};
	int complete = 0;

	line->clear();
	do {
		Prompt(complete);
		if (!fgets(partial, sizeof(partial), input_))
			return false;
		for (auto c : partial) {
			if (c == '\0')
				break;
			if (c == '(')
				++complete;
			if (c == ')')
				--complete;
		}
		line->append(partial);
	} while (complete > 0);
	return true;
}

const char *ReplApplication::Paint(const char *esc) {
	switch (color_mode_) {
	case AUTO:
	case YES:
		return esc;
	case NO:
		break;
	}
	return "";
}

void ReplApplication::Print(values::Object *o) {
	bool rest = false;
	switch (o->OwnedType()) {
	case values::BOOLEAN:
		fprintf(output_, "%s%s%s",
				Paint(cDARK_RED),
				o->Boolean() ? "#t" : "#f",
				Paint(cEND));
		break;
	case values::SYMBOL:
		fprintf(output_, "%s%s%s",
				Paint(cBLUE),
				o->Symbol(),
				Paint(cEND));
		break;
	case values::FIXED:
		fprintf(output_, "%s%lld%s",
				Paint(cDARK_AZURE),
				o->Fixed(),
				Paint(cEND));
		break;
	case values::CHARACTER:
		fprintf(output_, "%s#%c%s",
				Paint(cDARK_RED),
				o->Character(),
				Paint(cEND));
		break;
	case values::STRING:
		fprintf(output_, "%s\"%s\"%s",
				Paint(cDARK_RED),
				o->String()->c_str(),
				Paint(cEND));
		break;
	case values::PAIR:
		if (mach_->Obm()->Null(o)) {
			fprintf(output_, "%snull%s",
					Paint(cDARK_AZURE), Paint(cEND));
			return;
		}
		fprintf(output_, "%s(%s",
				Paint(cPURPLE), Paint(cEND));
		while (!mach_->Obm()->Null(o)) {
			if (rest)
				fputc(' ', output_);
			Print(car(o));
			o = cdr(o);
			rest = true;
		}
		fprintf(output_, "%s)%s",
				Paint(cPURPLE), Paint(cEND));
		break;
	case values::CLOSURE:
		fprintf(output_, "%s<closure>%s",
				Paint(cDARK_YELLOW), Paint(cEND));
		fprintf(output_, "\n%sparams%s : ",
				Paint(cDARK_GREEN), Paint(cEND));
		Print(o->Params());
		fprintf(output_, "\n%sbody%s : ",
				Paint(cDARK_GREEN), Paint(cEND));
		Print(o->Body());
		break;
	case values::PRIMITIVE:
		fprintf(output_, "%s<primitive:%p>%s",
				Paint(cDARK_YELLOW),
				PrimitiveMethod2Pv(o->Primitive()),
				Paint(cEND));
		break;
	}
}

void ReplApplication::HandleError(const char *err, vm::Mach *sender) {
	const char *file = sender->File();
	if (!file[0]) {
		fprintf(output_, "[%sError%s(%s%d%s)] %s\n",
				Paint(cRED), Paint(cEND),
				Paint(cYELLOW), sender->Error(), Paint(cEND),
				err);
	} else {
		fprintf(output_, "[%sError%s(%s%d%s) %s%s%s:%s%d%s] %s\n",
				Paint(cRED), Paint(cEND),
				Paint(cYELLOW), sender->Error(), Paint(cEND),
				Paint(cBLUE), file, Paint(cEND),
				Paint(cYELLOW), sender->Line(), Paint(cEND),
				err);
	}
}

void ReplApplication::Prompt(int complete) {
	if (complete == 0) {
		fprintf(output_, "%s>%s ", Paint(cGREEN), Paint(cEND));
		return;
	}
	while (complete--)
		fprintf(output_, " %s..%s ", Paint(cYELLOW), Paint(cEND));

}

} // namespace app
} // namespace ajimu

