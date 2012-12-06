#include "eval_application.h"
#include "repl_application.h"
#include "glog/logging.h"
#include "gflags/gflags.h"

DEFINE_string(input, "", "Input script file, if not set, to REPL mode.");
DEFINE_string(color, "auto", "REPL printing color. yes|no|auto");

static const char *kUsage = \
"\n"
"\tajimu --input=path/to/file\n"
"\tajimu --color=(yes|no|auto)";

int main(int argc, char *argv[]) {
	google::SetUsageMessage(kUsage);
	google::InitGoogleLogging(argv[0]);
	google::ParseCommandLineFlags(&argc, &argv, true);

	int rv;
	if (FLAGS_input.empty()) {
		using ajimu::app::ReplApplication;
		
		ReplApplication app;
		if (FLAGS_color == "yes")
			app.SetColorMode(ReplApplication::YES);
		else if (FLAGS_color == "no")
			app.SetColorMode(ReplApplication::NO);
		else
			app.SetColorMode(ReplApplication::AUTO);
		if (app.Init())
			rv = app.Run();
	} else {
		using ajimu::app::EvalApplication;

		EvalApplication app(FLAGS_input.c_str());
		if (app.Init())
			rv = app.Run();
	}

	return rv;
}

