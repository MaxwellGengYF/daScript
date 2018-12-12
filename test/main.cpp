#include "precomp.h"

#include "ast.h"
#include "interop.h"

#include <dirent.h>

using namespace std;
using namespace yzg;

bool g_reportCompilationFailErrors = false;

bool compilation_fail_test ( const string & fn ) {
    cout << fn << " ";
    string str;
    ifstream t(fn);
    if ( !t.is_open() ) {
        cout << "not found\n";
        return false;
    }
    t.seekg(0, ios::end);
    str.reserve(t.tellg());
    t.seekg(0, ios::beg);
    str.assign((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
    if ( auto program = parseDaScript(str.c_str(), [](const ProgramPtr & prog){}) ) {
        if ( program->failed() ) {
            bool failed = false;
            auto errors = program->expectErrors;
            for ( auto err : program->errors ) {
                auto it = find_if(errors.begin(), errors.end(), [&](CompilationError ce){
                    return err.cerr == ce;
                });
                if ( g_reportCompilationFailErrors || it==errors.end() ) {
                    cout << reportError(&str, err.at.line, err.at.column, err.what, err.cerr );
                }
                if ( it != errors.end() ) {
                    errors.erase(it);
                } else {
                    failed = true;
                }
            }
            if ( failed ) {
                cout << "failed, unexpected errors\n";
                return false;
            }
            if ( errors.size() ) {
                cout << "failed, expecting errors";
                for ( auto cerr : errors  ) {
                    cout << " " << int(cerr);
                }
                cout << "\n";
                return false;
            }
            cout << "ok\n";
            return true;
        } else {
            cout << "failed, compiled without errors\n";
            return false;
        }
    } else {
        cout << "failed, not expected to compile\n";
        return false;
    }
}

bool unit_test ( const string & fn ) {
    cout << fn << " ";
    string str;
    ifstream t(fn);
    if ( !t.is_open() ) {
        cout << "not found\n";
        return false;
    }
    t.seekg(0, ios::end);
    str.reserve(t.tellg());
    t.seekg(0, ios::beg);
    str.assign((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
    if ( auto program = parseDaScript(str.c_str(), [](const ProgramPtr & prog){}) ) {
        if ( program->failed() ) {
            cout << "failed to compile\n";
            for ( auto & err : program->errors ) {
                cout << reportError(&str, err.at.line, err.at.column, err.what, err.cerr );
            }
            return false;
        } else {
            // cout << *program << "\n";
            Context ctx(&str);
            program->simulate(ctx);
            int fnTest = ctx.findFunction("test");
            if ( fnTest != -1 ) {
                ctx.restart();
                bool result = cast<bool>::to(ctx.eval(fnTest, nullptr));
                if ( auto ex = ctx.getException() ) {
                    cout << "exception: " << ex << "\n";
                    return false;
                }
                if ( !result ) {
                    cout << "failed\n";
                    return false;
                }
                cout << "ok\n";
                return true;
            } else {
                cout << "function 'test' not found\n";
                return false;
            }
        }
    } else {
        return false;
    }
}

bool run_tests( const string & path, bool (*test_fn)(const string &) ) {
    bool ok = true;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if ( strstr(ent->d_name,".das") ) {
                ok = test_fn(path + "/" + ent->d_name) && ok;
            }
        }
        closedir (dir);
    }
    return ok;
}

bool run_unit_tests( const string & path ) {
    return run_tests(path, unit_test);
}

bool run_compilation_fail_tests( const string & path ) {
    return run_tests(path, compilation_fail_test);
}


int main(int argc, const char * argv[]) {
#if 0   // Debug this one test
    compilation_fail_test("../../test/compilation_fail_tests/const_ref.das");
    return 0;
#endif
    
    
    bool ok = true;
    ok = run_compilation_fail_tests("../../test/compilation_fail_tests") && ok;
    ok = run_unit_tests("../../test/unit_tests") && ok;
    cout << "TESTS " << (ok ? "PASSED" : "FAILED!!!") << "\n";
    return ok ? 0 : -1;
}
