#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace ll::plugin {

struct PluginInfo {
    std::string name;
    std::string version;
    std::string path;
    void* handle = nullptr;
    bool enabled = false;
};

class PluginManager {
private:
    std::vector<PluginInfo> mPlugins;
    std::map<std::string, void*> mLoadedHandles;

    PluginManager() = default;

public:
    static PluginManager& getInstance() {
        static PluginManager instance;
        return instance;
    }

    /// Discover and load all mods from a given directory
    void loadPluginsFromDir(const std::string& dirPath);

    /// Load a specific .so mod
    bool loadPlugin(const std::string& soPath);

    /// Unload a plugin
    bool unloadPlugin(const std::string& pluginName);

    const std::vector<PluginInfo>& getLoadedPlugins() const { return mPlugins; }
};

} // namespace ll::plugin
