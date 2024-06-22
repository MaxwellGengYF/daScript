#include "daScript/daScript.h"
using namespace das;
class Module_Serde : public Module {
public:
	static thread_local std::unordered_map<das::string, GcRootLambda> lambdas;
	static void setLambda(char const* name, Lambda lmb, Context* ctx);
	ModuleAotType aotRequire(TextWriter&) const override;
	Module_Serde();
};