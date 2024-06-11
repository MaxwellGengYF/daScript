#include "daScript/daScript.h"
using namespace das;
class Module_Serde : public Module {
public:
	ModuleAotType aotRequire(TextWriter&) const override { return ModuleAotType::cpp; }
	Module_Serde() : Module("serde") {// module name, when used from das file
		ModuleLibrary lib(this);
		lib.addBuiltInModule();
	}
};
REGISTER_MODULE(Module_Serde);

void require_project_specific_modules() {
    NEED_MODULE(Module_Serde);
}
