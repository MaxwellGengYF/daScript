set_xmakever('2.9.1')
add_rules('mode.release', 'mode.debug')
set_policy('build.ccache', not is_plat('windows'))

option("das_aot")
set_default(false)
set_showmenu(true)
option_end()

option("das_toolchain")
set_default(false)
set_showmenu(true)
option_end()

rule_end()
if (os.projectdir() == os.scriptdir()) then
    if is_mode("debug") then
        set_targetdir(path.absolute('bin/debug', os.projectdir()))
    else
        set_targetdir(path.absolute('bin/release', os.projectdir()))
    end
end

includes('xmake/xmake_func.lua')
includes('EASTL')
target('uriparser')
_config_project({
    project_kind = 'object'
})
add_files('3rdparty/uriparser/src/*.c')
add_defines('URI_NO_UNICODE', 'URI_STATIC_BUILD', {
    public = true
})
add_defines('URI_LIBRARY_BUILD')
add_includedirs('3rdparty/uriparser/include', {
    public = true
})
target_end()

target('daScript_lib')
_config_project({
    project_kind = 'static',
    enable_exception = true
    -- no_rtti = false
})
add_includedirs('include', 'src', 'xxHash', {
    public = true
})
add_cxflags("/bigobj", {
    tools = {"cl"}
})
add_deps('uriparser', 'eastl')
add_files('src/**.cpp', 'xxHash/xxhash.c')
set_pcxxheader('src/pch.h')
target_end()

target('daScript_modules')
_config_project({
    project_kind = 'static',
})
add_files('modules/d5_next/**.cpp')
add_includedirs('modules/d5_next', {
    public = true
})
add_deps('daScript_lib')
target_end()

target('daScript')
_config_project({
    project_kind = 'binary',
})
add_deps('daScript_modules')
add_files('utils/daScript/main.cpp')
target_end()

DAS_ENABLE_AOT = get_config('das_aot')
if DAS_ENABLE_AOT then
    target('my_test_codegen')
    set_kind("object")
    add_rules('codegen_das')
    set_policy("build.across_targets_in_parallel", false)
    add_files('my_test/*.das')
    add_deps('daScript', {public = false, inherit = false})
    target_end()
end

target('my_test_compile')
_config_project({
    project_kind = 'binary',
})
add_rules('compile_das')
add_deps('daScript_modules')
if DAS_ENABLE_AOT then
    add_deps('my_test_codegen', {public = false})
    add_defines("DAS_ENABLE_AOT")
    add_files('my_test/*.das')
end
set_policy("build.across_targets_in_parallel", false)
add_files('my_test/*.cpp')
set_pcxxheader('my_test/pch.h')
target_end()
