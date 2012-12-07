#include "repl_application.h"
#include "gmock/gmock.h"

namespace ajimu {
namespace app {

TEST(REPLTest, REPL) {
	ReplApplication app;
	ASSERT_TRUE(app.Init());
	app.Load("src/lib/stdlib.scm");
	fprintf(stdin, "(for-each (lambda (i) (display (list (ajimu.gc.state) (ajimu.gc.allocated)))) '(1 2 2 3 4 5 6 7 8 9 0))");
	app.Run();
}

} // namespace app
} // namespace ajimu

