#include "serde.h"
void Module_Serde::setLambda(char const* name, Lambda lmb, Context* ctx) {
	lambdas.try_emplace(das::string(name), lmb, ctx);
}
ModuleAotType Module_Serde::aotRequire(TextWriter&) const { return ModuleAotType::cpp; }
Module_Serde::Module_Serde() : Module("serde") {// module name, when used from das file
	ModuleLibrary lib(this);
	lib.addBuiltInModule();
	addExtern<DAS_BIND_FUN(setLambda)>(*this, lib, "set_lambda", SideEffects::modifyExternal, "set_lambda");
}
thread_local std::unordered_map<das::string, GcRootLambda> Module_Serde::lambdas;
REGISTER_MODULE(Module_Serde);

void require_project_specific_modules() {
	NEED_MODULE(Module_Serde);
}
