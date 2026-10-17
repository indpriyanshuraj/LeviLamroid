-- LeviLamroid - xmake build script (Minecraft Bedrock, Android ARM64)
-- The build system for this project. There is no second one.
--
--   xmake f -p android -a arm64-v8a -m release --ndk=$ANDROID_NDK_HOME -y
--   xmake -y
--
-- Output: build/android/arm64-v8a/release/liblevilamroid.so
--         build/android/arm64-v8a/release/levilamroid.levipack

set_project("LeviLamroid")
set_version("1.0.1")
set_license("LGPL-3.0")

add_rules("mode.debug", "mode.release")

-- Freeze dependency resolution (the npm-package.lock / cargo.lock equivalent).
set_policy("package.requires_lock", true)

-- LeviLaunchroid targets ARM64 only, so the build is ARM64 only.
set_allowedplats("android")
set_allowedarchs("arm64-v8a")

-- ---------------------------------------------------------------------------
-- Build cache (ccache-style, built into xmake)
-- ---------------------------------------------------------------------------
-- xmake's built-in object cache (build.ccache) is ON by default locally and
-- lives in build/.build_cache. The CI setup action turns it on via
-- build-cache: true. Tuning knobs, all optional:
--
--   set_policy("build.ccache", false)                 -- turn the cache off
--   set_policy("build.ccache.global_storage", true)   -- share one cache across
--                                                      checkouts: ~/.xmake/.build_cache
--   xmake f --ccachedir=/path/to/cache                 -- explicit cache dir
--
-- Keep include dirs RELATIVE (add_includedirs("include")) - an absolute path
-- changes the cache key and drops the hit rate to 0.
--
-- set_policy("build.ccache.global_storage", true)
set_policy("package.build.ccache", true)   -- also cache xrepo package builds (xmake >= 3.0.5)

-- ---------------------------------------------------------------------------
-- IDE / tooling files (generated on demand, not committed)
-- ---------------------------------------------------------------------------
-- xmake emits the files editors and analysers want, straight from this file, so
-- they can never drift from the real build:
--
--   xmake project -k compile_commands .vscode   # clangd / ccls / VS Code
--   xmake project -k ninja / -k makefile        # alternate backends
--   xmake project -k vsxmake -m "debug,release" # Visual Studio
--
-- Uncomment to regenerate compile_commands.json on every build:
-- add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode", lsp = "clangd"})

-- ---------------------------------------------------------------------------
-- OPTIONAL SLIM-DOWN: source the third-party libs from xrepo instead of
-- vendoring them. All three are in the official xmake-repo and are available
-- for android/arm64-v8a. Uncomment the add_requires() lines AND delete
-- include/glm, include/imgui, include/nlohmann (~500 files), then uncomment the
-- add_packages() line inside the target. This is what makes the CI package
-- cache worth having.
--
-- Note: GLES/EGL/KHR headers are NOT packages - they come from the NDK sysroot,
-- so those stay as-is either way.
-- ---------------------------------------------------------------------------
-- add_requires("glm 1.0.3")
-- add_requires("nlohmann_json v3.12.0")
-- add_requires("imgui v1.92.7", {configs = {android = true, opengl3 = true}})

option("ll_verbose_log")
    set_default(false)
    set_showmenu(true)
    set_description("Enable verbose engine discovery / hook logging")
option_end()

target("levilamroid")
    set_kind("shared")
    set_basename("levilamroid")              -- -> liblevilamroid.so
    set_languages("c++20")
    set_runtimes("c++_shared")

    add_files("src/**.cpp")
    add_includedirs("include")
    -- add_packages("glm", "nlohmann_json", "imgui")   -- with the add_requires above

    if has_config("ll_verbose_log") then
        add_defines("LEVILAMROID_VERBOSE_LOG")
    end

    set_warnings("all", "extra")

    if is_mode("release") then
        set_optimize("fastest")
        set_strip("all")
        set_policy("build.optimization.lto", true)
    end

    -- Visibility: only the loader ABI listed in levilamroid.version leaves the .so.
    add_cxflags("-fPIC",
                "-fvisibility=hidden",
                "-fvisibility-inlines-hidden",
                "-ffunction-sections",
                "-fdata-sections")

    add_shflags("-Wl,--gc-sections",
                "-Wl,-z,max-page-size=16384",   -- required for Android 16 KB pages
                "-Wl,--version-script=" .. path.join(os.projectdir(), "levilamroid.version"),
                {force = true})

    add_links("log", "GLESv3", "dl", "m", "android")

    -- Package liblevilamroid.so + manifest.json + data/ into a .levipack.
    after_build(function (target)
        if not target:is_plat("android") then return end
        import("utils.archive")

        local outdir  = target:targetdir()
        local packdir = path.join(outdir, "pack")
        os.tryrm(packdir)
        os.mkdir(packdir)
        os.cp(target:targetfile(), path.join(packdir, "liblevilamroid.so"))
        os.cp(path.join(os.projectdir(), "manifest.json"), packdir)
        if os.isdir(path.join(os.projectdir(), "data")) then
            os.cp(path.join(os.projectdir(), "data"), path.join(packdir, "data"))
        end

        local levipack     = path.join(outdir, "levilamroid.levipack")
        local levipack_zip = levipack .. ".zip"
        os.tryrm(levipack)
        os.tryrm(levipack_zip)
        archive.archive(levipack_zip, "*", {curdir = packdir, recurse = true})
        os.mv(levipack_zip, levipack)
        cprint("${bright green}[LeviLamroid]${reset} packaged " .. levipack)
    end)

-- ---------------------------------------------------------------------------
-- Local deploy: `xmake deploy`
-- Replaces the deployment half of the old package.sh: copies the built
-- .levipack into every LeviLauncher profile on a connected device. Run after
-- a successful `xmake` build.
-- ---------------------------------------------------------------------------
task("deploy")
    set_category("plugin")
    set_description("Copy the built .levipack into LeviLauncher profiles")
    on_run(function ()
        local outdir   = path.join(os.projectdir(), "build", "android", "arm64-v8a", "release")
        local levipack = path.join(outdir, "levilamroid.levipack")
        assert(os.isfile(levipack), "build it first: xmake f -p android -a arm64-v8a -m release --ndk=$ANDROID_NDK_HOME -y && xmake -y")

        local root = "/sdcard/Android/media/org.levimc.launcher/minecraft"
        local count = 0
        if os.isdir(root) then
            for _, profile in ipairs(os.dirs(path.join(root, "*"))) do
                local moddir = path.join(profile, "mods", "LeviLamroid")
                os.mkdir(moddir)
                os.cp(levipack, path.join(moddir, "levilamroid.levipack"))
                cprint("${bright green}[deploy]${reset} " .. moddir)
                count = count + 1
            end
        end
        if count == 0 then
            cprint("${yellow}[deploy]${reset} no LeviLauncher profiles found; package is at " .. levipack)
        else
            cprint("${bright green}[deploy]${reset} installed to " .. count .. " profile(s)")
        end
    end)
