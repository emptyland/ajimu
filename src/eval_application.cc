#include "eval_application.h"
#include "mach.h"
#include <stdio.h>

namespace ajimu {
namespace app {

EvalApplication::EvalApplication(const char *main_file)
	: main_file_(main_file)
	, mach_(new vm::Mach()){

}

EvalApplication::~EvalApplication() {
}


bool EvalApplication::Init() {
	bool ok = mach_->Init();
	if (!ok)
		return false;
	mach_->AddObserver([this] (const char *err, vm::Mach *sender) {
		printf("[Error:%s:%d] %s\n",
			sender->File(),
			sender->Line(),
			err);
	});
	return ok;
}

int EvalApplication::Run() {
	values::Object *rv = mach_->EvalFile(main_file_);
	return rv ? 0 : 1;
}


} // namespace app
} // namespace ajimu

