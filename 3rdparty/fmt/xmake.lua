target("das_fmt")
_config_project({
    project_kind = "object"
})
add_files("src/fmt.cc")
add_defines("FMT_LIB_EXPORT")
add_defines( "FMT_CONSTEVAL=constexpr", "FMT_USE_CONSTEXPR=1", "FMT_EXCEPTIONS=0", {public = true})
add_includedirs("include", {public = true})
target_end()