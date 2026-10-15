#include "levilamroid/api/plugin/PluginManager.h"
#include "levilamroid/api/Logger.h"
#include <dlfcn.h>
#include <filesystem>
#include <fstream>

namespace ll::plugin {

void PluginManager::loadPluginsFromDir(const std::string& dirPath) {
    auto& logger = Logger::getInstance();
    if (!std::filesystem::exists(dirPath)) {
        return;
    }

    logger.info("LeviLamroid PluginManager scanning for mods in: ", dirPath);
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_directory()) {
            // Check for .so inside mod directory
            for (const auto& file : std::filesystem::directory_iterator(entry.path())) {
                if (file.is_regular_file() && file.path().extension() == ".so") {
                    std::string soPath = file.path().string();
                    if (soPath.find("liblevilamroid.so") != std::string::npos ||
                        soPath.find("libLeviLamroid.so") != std::string::npos) {
                        continue;
                    }
                    loadPlugin(soPath);
                }
            }
        }
    }
}

bool PluginManager::loadPlugin(const std::string& soPath) {
    auto& logger = Logger::getInstance();
    if (mLoadedHandles.find(soPath) != mLoadedHandles.end()) {
        return true; // Already loaded
    }

    logger.info("Loading client mod: ", soPath);
    void* handle = dlopen(soPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        logger.error("Failed to dlopen mod: ", soPath, " Error: ", dlerror());
        return false;
    }

    mLoadedHandles[soPath] = handle;

    // Call standard LeviLamina / LeviLaunchroid entry points
    using PLGetModRegistrationFn = const void* (*)();
    using LLModLoadFn = bool (*)(void*);

    auto fnPL = reinterpret_cast<PLGetModRegistrationFn>(dlsym(handle, "PLGetModRegistration"));
    auto fnLL = reinterpret_cast<LLModLoadFn>(dlsym(handle, "ll_mod_load"));

    if (fnPL) {
        fnPL();
        logger.info("Successfully called PLGetModRegistration for ", soPath);
    } else if (fnLL) {
        fnLL(nullptr);
        logger.info("Successfully called ll_mod_load for ", soPath);
    } else {
        logger.info("Mod loaded without explicit lifecycle export (constructor initialized).");
    }

    PluginInfo info;
    info.name = std::filesystem::path(soPath).filename().string();
    info.path = soPath;
    info.handle = handle;
    info.enabled = true;
    mPlugins.push_back(info);
    return true;
}

bool PluginManager::unloadPlugin(const std::string& pluginName) {
    for (auto it = mPlugins.begin(); it != mPlugins.end(); ++it) {
        if (it->name == pluginName) {
            if (it->handle) {
                dlclose(it->handle);
            }
            mLoadedHandles.erase(it->path);
            mPlugins.erase(it);
            return true;
        }
    }
    return false;
}

} // namespace ll::plugin
