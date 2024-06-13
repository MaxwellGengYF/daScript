function get_outdir()
    return "build/.das/"
end

function get_out_filepath(sourcefile)
    local dir = path.join(get_outdir(), path.directory(sourcefile))
    return {dir, path.basename(sourcefile)}
end