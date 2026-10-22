#include "pl/ModMenu.hpp"
#include "pl/Input.hpp"
#include "pl/Mod.hpp"
#include "pl/memory/Hook.hpp"
#include "pl/memory/Patch.hpp"
#include "pl/memory/Vtable.hpp"
#include "pl/memory/Signature.hpp"

#include "levilamroid/api/Logger.h"
#include "levilamroid/api/memory/Hook.h"
#include "levilamroid/api/memory/PatternScan.h"
#include "levilamroid/api/mcpe/Offsets.h"

#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <span>
#include <cstring>
#include <cctype>

namespace pl::modmenu {

static std::mutex sModuleMutex;
static std::vector<ModuleInfo> sModules;
static std::mutex sButtonMutex;
static std::vector<ButtonInfo> sButtons;

PL_EXPORT bool registerModule(const ModuleInfo &info) {
    using RealFn = bool(*)(const ModuleInfo&);
    static auto realFn = reinterpret_cast<RealFn>(dlsym(RTLD_DEFAULT, "_ZN2pl7modmenu14registerModuleERKNS0_10ModuleInfoE"));
    Dl_info dlinfo;
    if (realFn && dladdr(reinterpret_cast<void*>(realFn), &dlinfo) && dlinfo.dli_fbase) {
        if (dlinfo.dli_fname && strstr(dlinfo.dli_fname, "libpreloader.so")) {
            return realFn(info);
        }
    }
    std::lock_guard<std::mutex> lock(sModuleMutex);
    sModules.push_back(info);
    levilamroid::Logger::getInstance().info("[PreLoader] Registered module: ", info.moduleId, " (", info.displayName, ")");
    if (info.defaultEnabled && info.onToggle) {
        try {
            info.onToggle(info.moduleId, true);
        } catch (...) {}
    }
    return true;
}

PL_EXPORT void unregisterModule(std::string_view moduleId) {
    std::lock_guard<std::mutex> lock(sModuleMutex);
    for (auto it = sModules.begin(); it != sModules.end();) {
        if (it->moduleId == moduleId) {
            it = sModules.erase(it);
        } else {
            ++it;
        }
    }
}

PL_EXPORT void setModuleEnabled(std::string_view moduleId, bool enabled) {
    std::lock_guard<std::mutex> lock(sModuleMutex);
    for (auto& m : sModules) {
        if (m.moduleId == moduleId) {
            m.defaultEnabled = enabled;
            if (m.onToggle) {
                try { m.onToggle(moduleId, enabled); } catch (...) {}
            }
            break;
        }
    }
}

PL_EXPORT void submitDrawCommands(std::string_view moduleId, std::span<const DrawCommand> commands) {
    (void)moduleId;
    (void)commands;
}

PL_EXPORT bool registerFont(std::string_view fontId, std::span<const unsigned char> ttfData) {
    (void)fontId;
    (void)ttfData;
    return true;
}

PL_EXPORT bool registerImage(std::string_view imageId, std::span<const unsigned char> imageData, int width, int height) {
    (void)imageId;
    (void)imageData;
    (void)width;
    (void)height;
    return true;
}

PL_EXPORT bool registerButton(const ButtonInfo &info) {
    using RealFn = bool(*)(const ButtonInfo&);
    static auto realFn = reinterpret_cast<RealFn>(dlsym(RTLD_DEFAULT, "_ZN2pl7modmenu14registerButtonERKNS0_10ButtonInfoE"));
    Dl_info dlinfo;
    if (realFn && dladdr(reinterpret_cast<void*>(realFn), &dlinfo) && dlinfo.dli_fbase) {
        if (dlinfo.dli_fname && strstr(dlinfo.dli_fname, "libpreloader.so")) {
            return realFn(info);
        }
    }
    std::lock_guard<std::mutex> lock(sButtonMutex);
    sButtons.push_back(info);
    levilamroid::Logger::getInstance().info("[PreLoader] Registered HUD button: ", info.buttonId, " (", info.label, ")");
    return true;
}

PL_EXPORT void setConfigSchemaJson(std::string_view moduleId, std::string_view schemaJson) {
    (void)moduleId;
    (void)schemaJson;
}

PL_EXPORT void unregisterButton(std::string_view buttonId) {
    std::lock_guard<std::mutex> lock(sButtonMutex);
    for (auto it = sButtons.begin(); it != sButtons.end();) {
        if (it->buttonId == buttonId) {
            it = sButtons.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace pl::modmenu

namespace pl::input {

static std::mutex sInputMutex;
static std::vector<TouchCallback> sTouchCallbacks;
static std::vector<KeyCallback> sKeyCallbacks;
static std::vector<MouseCallback> sMouseCallbacks;

PL_EXPORT void registerTouchCallback(TouchCallback callback) {
    std::lock_guard<std::mutex> lock(sInputMutex);
    if (callback) sTouchCallbacks.push_back(std::move(callback));
}

PL_EXPORT void registerKeyCallback(KeyCallback callback) {
    std::lock_guard<std::mutex> lock(sInputMutex);
    if (callback) sKeyCallbacks.push_back(std::move(callback));
}

PL_EXPORT void registerMouseCallback(MouseCallback callback) {
    std::lock_guard<std::mutex> lock(sInputMutex);
    if (callback) sMouseCallbacks.push_back(std::move(callback));
}

PL_EXPORT void showKeyboard() {}
PL_EXPORT void hideKeyboard() {}

} // namespace pl::input

namespace pl::mod {
PL_EXPORT NativeMod* NativeMod::current() noexcept {
    return nullptr;
}
} // namespace pl::mod

namespace pl::memory {

PL_EXPORT int hook(FuncPtr target, FuncPtr detour, FuncPtr *originalFunc, HookPriority priority) {
    (void)priority;
    if (!target || !detour) return -1;
    bool ok = ll::memory::hook(target, detour, originalFunc);
    return ok ? 0 : -1;
}

PL_EXPORT bool unhook(FuncPtr target, FuncPtr detour) {
    (void)detour;
    if (!target) return false;
    using DobbyDestroyFn = int (*)(void*);
    static auto fnDobbyDestroy = reinterpret_cast<DobbyDestroyFn>(dlsym(RTLD_DEFAULT, "DobbyDestroy"));
    if (fnDobbyDestroy) {
        return fnDobbyDestroy(target) == 0;
    }
    return true;
}

PL_EXPORT uintptr_t resolveVtableFunction(std::string_view typeInfoName, size_t slot, std::string_view moduleName) {
    std::string typeInfo(typeInfoName);
    std::string mod(moduleName.empty() ? "libminecraftpe.so" : moduleName);
    void* result = ll::memory::resolveVtableFunction(typeInfo.c_str(), slot, mod.c_str());
    return reinterpret_cast<uintptr_t>(result);
}

PL_EXPORT uintptr_t resolveSignature(std::string_view signature, std::string_view moduleName) {
    (void)moduleName;
    return ll::memory::scanMinecraft(std::string(signature));
}

PL_EXPORT std::unordered_map<std::string, uintptr_t> resolveSignatures(std::span<const std::string> signatures, std::string_view moduleName) {
    (void)moduleName;
    std::unordered_map<std::string, uintptr_t> result;
    for (const auto& sig : signatures) {
        result[sig] = ll::memory::scanMinecraft(sig);
    }
    return result;
}

struct PatchData {
    uintptr_t address;
    std::vector<uint8_t> originalBytes;
};
static std::mutex sPatchMutex;
static std::unordered_map<std::string, PatchData> sPatches;

static bool applyPatch(uintptr_t address, const uint8_t* newBytes, size_t len, std::string_view name) {
    if (!address || !newBytes || len == 0) return false;
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) pageSize = 4096;
    uintptr_t pageStart = address & ~(pageSize - 1);
    size_t totalLen = (address + len) - pageStart;
    if (mprotect(reinterpret_cast<void*>(pageStart), totalLen, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return false;
    }
    std::vector<uint8_t> orig(len);
    std::memcpy(orig.data(), reinterpret_cast<const void*>(address), len);
    std::memcpy(reinterpret_cast<void*>(address), newBytes, len);
    __builtin___clear_cache(reinterpret_cast<char*>(pageStart), reinterpret_cast<char*>(pageStart + totalLen));
    std::lock_guard<std::mutex> lock(sPatchMutex);
    sPatches[std::string(name)] = PatchData{address, std::move(orig)};
    return true;
}

PL_EXPORT bool writeBytes(uintptr_t address, std::span<const uint8_t> bytes, std::string_view name) {
    return applyPatch(address, bytes.data(), bytes.size(), name);
}

PL_EXPORT bool writeBytes(uintptr_t address, std::string_view hexBytes, std::string_view name) {
    std::vector<uint8_t> bytes;
    std::string hex;
    for (char c : hexBytes) {
        if (!isspace(c)) hex += c;
    }
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        uint8_t b = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
        bytes.push_back(b);
    }
    return writeBytes(address, std::span<const uint8_t>(bytes), name);
}

PL_EXPORT std::vector<uint8_t> readBytes(uintptr_t address, size_t length) {
    std::vector<uint8_t> result(length);
    if (address && length > 0) {
        std::memcpy(result.data(), reinterpret_cast<const void*>(address), length);
    }
    return result;
}

PL_EXPORT bool revertPatch(std::string_view name) {
    std::lock_guard<std::mutex> lock(sPatchMutex);
    auto it = sPatches.find(std::string(name));
    if (it == sPatches.end()) return false;
    const auto& p = it->second;
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) pageSize = 4096;
    uintptr_t pageStart = p.address & ~(pageSize - 1);
    size_t totalLen = (p.address + p.originalBytes.size()) - pageStart;
    mprotect(reinterpret_cast<void*>(pageStart), totalLen, PROT_READ | PROT_WRITE | PROT_EXEC);
    std::memcpy(reinterpret_cast<void*>(p.address), p.originalBytes.data(), p.originalBytes.size());
    __builtin___clear_cache(reinterpret_cast<char*>(pageStart), reinterpret_cast<char*>(pageStart + totalLen));
    sPatches.erase(it);
    return true;
}

PL_EXPORT void revertAllPatches() {
    std::lock_guard<std::mutex> lock(sPatchMutex);
    for (auto& [name, p] : sPatches) {
        long pageSize = sysconf(_SC_PAGESIZE);
        if (pageSize <= 0) pageSize = 4096;
        uintptr_t pageStart = p.address & ~(pageSize - 1);
        size_t totalLen = (p.address + p.originalBytes.size()) - pageStart;
        mprotect(reinterpret_cast<void*>(pageStart), totalLen, PROT_READ | PROT_WRITE | PROT_EXEC);
        std::memcpy(reinterpret_cast<void*>(p.address), p.originalBytes.data(), p.originalBytes.size());
        __builtin___clear_cache(reinterpret_cast<char*>(pageStart), reinterpret_cast<char*>(pageStart + totalLen));
    }
    sPatches.clear();
}

} // namespace pl::memory
