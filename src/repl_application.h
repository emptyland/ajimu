#ifndef AJIMU_APP_REPL_APPLICATION_H
#define AJIMU_APP_REPL_APPLICATION_H

#include "glog/logging.h"
#include <stdio.h>
#include <memory>
#include <string>

namespace ajimu {
namespace vm {
class Mach;
} // namespace vm
namespace values {
class Object;
} // namespace values
namespace app {

// The Read-Eval-Print Loop Application
class ReplApplication {
public:
	enum ColorMode {
		AUTO, YES, NO,
	};

	ReplApplication();

	~ReplApplication();

	bool Init();

	void SetColorMode(ColorMode mode) {
		color_mode_ = mode;
	}

	void SetInputStream(FILE *fp) {
		input_ = DCHECK_NOTNULL(fp);
	}

	void SetOutputStream(FILE *fp) {
		output_ = DCHECK_NOTNULL(fp);
	}

	bool Load(const char *lib);

	int Run();

private:
	ReplApplication(const ReplApplication &) = delete;
	void operator = (const ReplApplication &) = delete;

	bool ReadLine(std::string *line);

	void Print(values::Object *rv);

	void HandleError(const char *err, vm::Mach *sender);

	void Prompt(int complete);

	const char *Paint(const char *esc);

	ColorMode color_mode_;
	std::unique_ptr<vm::Mach> mach_;
	FILE *input_;
	FILE *output_;
}; // class ReplApplication

} // namespace app
} // namespace ajimu

#endif //AJIMU_APP_REPL_APPLICATION_H

