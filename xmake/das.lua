function get_outdir()
    return "build/.das/"
end

function get_out_filepath(sourcefile)
    return path.join(get_outdir(), path.directory(sourcefile), path.basename(sourcefile) .. ".cpp")
end

function get_out_filepath_create(sourcefile, lib)
    local parent = path.join(get_outdir(), path.directory(sourcefile))
    lib.mkdirs(parent)
    return path.join(parent, path.basename(sourcefile) .. ".cpp")
end