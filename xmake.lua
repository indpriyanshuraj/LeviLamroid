add_rules("mode.debug", "mode.release")

set_languages("cxx20")

target("levilamroid")
    set_kind("shared")
    set_filename("liblevilamroid.so")
    add_files(
        "src/LeviLamroid.cpp",
        "src/memory/Hook.cpp",
        "src/memory/PatternScan.cpp",
        "src/mcpe/Offsets.cpp",
        "src/mcpe/Symbols.cpp",
        "src/hooks/CoreHooks.cpp",
        "src/hooks/UIHooks.cpp",
        "src/hooks/RenderHooks.cpp",
        "src/hooks/ResourceHooks.cpp",
        "src/plugin/PluginManager.cpp",
        "src/resource/ResourceManager.cpp"
    )
    add_includedirs("include")
    add_linkdirs("/system/lib64", "/data/data/com.termux/files/usr/lib")
    add_syslinks("dl", "log", "GLESv3", "pthread")
    add_shflags("-static-libgcc", "-static-libstdc++", "-Wl,--version-script=levilamroid.version", {force = true})
