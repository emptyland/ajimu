#include "repl_application.cc"
#include "mach.cc"
#include "lexer.cc"
#include "object_management.cc"
#include "object.cc"
#include "gmock/gmock.h"

namespace ajimu {
namespace app {

TEST(REPLTest, REPL) {
	ReplApplication app;
	ASSERT_TRUE(app.Init());
	app.Run();
}

} // namespace app
} // namespace ajimu

