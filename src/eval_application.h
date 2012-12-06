#ifndef AJIMU_APP_EVAL_APPLICATION_H
#define AJIMU_APP_EVAL_APPLICATION_H

#include <memory>

namespace ajimu {
namespace vm {
class Mach;
} // namespace vm
namespace app {

class EvalApplication {
public:
	explicit EvalApplication(const char *main_file);

	~EvalApplication();

	bool Init();

	int Run();

private:
	EvalApplication(const EvalApplication &) = delete;
	void operator = (const EvalApplication &) = delete;

	const char *main_file_;
	std::unique_ptr<vm::Mach> mach_;
}; // class EvalApplication

} // namespace app
} // namespace ajimu

#endif //AJIMU_APP_EVAL_APPLICATION_H

