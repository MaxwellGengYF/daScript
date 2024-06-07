function main(...)
    import("core.base.json")
    local lib = import("lib", {
        rootdir = "xmake"
    })
    local file_data, js
    try {function()
        file_data = io.readfile(".vscode/settings.json")
    end, catch {function(error)
        utils.error(".vscode/settings.json not found.")
        os.exit(1)
    end}}

    try {function()
        js = json.decode(file_data)
    end, catch {function(error)
        utils.error(".vscode/settings.json format error.")
        os.exit(1)
    end}}
    local config_args = js["xmake.additionalConfigArguments"]
    local run_args = js["xmake.runningTargetsArguments"]
    if run_args then
        if run_args["default"] then
            run_args = run_args["default"]
        end
    end
    local modes = {'debug', 'release'}
    local config = lib.StringBuilder()
    for _, mode in ipairs(modes) do
        config:clear()
        config:add("@echo off\n")
        config:add("xmake f -m "):add(mode):add(' ')
        if config_args then
            for _, i in ipairs(config_args) do
                config:add('"'):add(i):add('" ')
            end
            config:add("-P . "):add('-c')
        end
        config:write_to(path.join(os.scriptdir(), 'config_' .. mode .. '.cmd'))
    end
    local run_cmd = lib.StringBuilder()
    local buildrun_cmd = lib.StringBuilder()
    for _, target in ipairs({...}) do
        config:clear()
        run_cmd:clear()
        buildrun_cmd:clear()
        run_cmd:add('@echo off\n')
        buildrun_cmd:add(run_cmd)
        config:add("xmake run -P . "):add(target)
        if run_args then
            for _, a in ipairs(run_args) do
                config:add(' "'):add(a):add('"')
            end
        end
        buildrun_cmd:add("xmake -P .\nif %ERRORLEVEL% == 0  "):add(config)
        run_cmd:add(config)
        run_cmd:write_to(path.join(os.scriptdir(), "run_" .. target .. ".cmd"))
        buildrun_cmd:write_to(path.join(os.scriptdir(), "buildrun_" .. target .. ".cmd"))
    end
    io.writefile(path.join(os.scriptdir(), "update_intellisense.cmd"), "@echo off\nxmake lua xmake/update_intellisense.lua\n@echo done")
    config:dispose()
    buildrun_cmd:dispose()
    run_cmd:dispose()
    print("success.")
end
