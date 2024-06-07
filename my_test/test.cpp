#include "daScript/daScript.h"
#include <iostream>
using namespace das;
void require_project_specific_modules();

void tutorial(char const* name) {
	TextPrinter tout;						  // output stream for all compiler messages (stdout. for stringstream use TextWriter)
	ModuleGroup dummyLibGroup;				  // module group for compiled program
	auto fAccess = make_smart<FsFileAccess>();// default file access
	CodeOfPolicies policies{};				  // default policies
// policies.persistent_heap = true;                // enable persistent heap for GC purposes
#if DAS_ENABLE_AOT
	policies.aot = true;
	policies.fail_on_no_aot = true;
#endif
	policies.fail_on_lack_of_aot_export = false;
	// compile program
	auto program = compileDaScript(name, fAccess, tout, dummyLibGroup, policies);
	if (program->failed()) {
		// if compilation failed, report errors
		tout << "failed to compile\n";
		for (auto& err : program->errors) {
			tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
		}
		return;
	}
	// create daScript context
	Context ctx(program->getContextStackSize());
	if (!program->simulate(ctx, tout)) {
		// if interpretation failed, report errors
		tout << "failed to simulate\n";
		for (auto& err : program->errors) {
			tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
		}
		return;
	}
	// find function 'main' in the context
	auto fnMain = ctx.findFunction("main");
	if (!fnMain) {
		tout << "function 'main' not found\n";
		return;
	}
	// verify if 'main' is a function, with the correct signature
	// note, this operation is slow, so don't do it every time for every call
	if (!verifyCall<void>(fnMain->debugInfo, dummyLibGroup)) {
		tout << "function 'main', call arguments do not match. expecting def main : void\n";
		return;
	}
	// evaluate 'main' function in the context
	// das is really fast!
	auto be = clock();
	for (size_t i = 0; i < 100000; ++i) {
		ctx.restartHeaps();
		ctx.evalWithCatch(fnMain, nullptr);
	}
	auto ed = clock();
	tout << ((double)(ed - be) / 1e3) << " second\n";
	if (auto ex = ctx.getException()) {// if function cased panic, report it
		tout << "exception: " << ex << "\n";
		return;
	}
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cout << "Bad arguments.";
		return 1;
	}
	NEED_ALL_DEFAULT_MODULES;
	require_project_specific_modules();
	// Initialize modules
	Module::Initialize();
	// run the tutorial
	tutorial(argv[1]);
	// shut-down daScript, free all memory
	Module::Shutdown();
	return 0;
}