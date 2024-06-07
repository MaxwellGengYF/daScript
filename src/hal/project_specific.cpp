#include "daScript/misc/platform.h"
#include "daScript/daScriptModule.h"
#include "daScript/daScript.h"

// using namespace das;
// class Module_Tutorial02 : public Module {
// public:
// ModuleAotType aotRequire ( TextWriter & ) const override  { return ModuleAotType::cpp; }
// 	Module_Tutorial02() : Module("tutorial_02") {// module name, when used from das file
// 		ModuleLibrary lib(this);
// 		lib.addBuiltInModule();
// 		// adding constant to the module
// 		addConstant(*this, "SQRT2", sqrtf(2.0));
// 	}
// };
// REGISTER_MODULE(Module_Tutorial02);
void require_project_specific_modules() {
    // NEED_MODULE(Module_Tutorial02);
}
