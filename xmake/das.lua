function get_outdir()
    local mode = get_config("mode")
    if not mode then
        mode = "default"
    end
    return format("build/.das/%s_%s_%s/", os.host(), os.arch(), mode)
end

function get_out_filepath(sourcefile)
    local dir = path.join(get_outdir(), path.directory(sourcefile))
    return {dir, path.basename(sourcefile)}
end