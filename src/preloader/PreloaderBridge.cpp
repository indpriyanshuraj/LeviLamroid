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

// Note: pl::modmenu is provided exclusively by libpreloader.so.
// LeviLamroid must NOT shadow or stub pl::modmenu, ensuring all client mods
// communicate directly with the launcher's native overlay and Mod Menu drawer.


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

PL_EXPORT bool dispatchTouchEvent(const TouchEvent &event) {
    std::lock_guard<std::mutex> lock(sInputMutex);
    bool handled = false;
    for (const auto& cb : sTouchCallbacks) {
        try {
            if (cb && cb(event)) handled = true;
        } catch (...) {}
    }
    return handled;
}

PL_EXPORT bool dispatchKeyEvent(const KeyEvent &event) {
    std::lock_guard<std::mutex> lock(sInputMutex);
    bool handled = false;
    for (const auto& cb : sKeyCallbacks) {
        try {
            if (cb && cb(event)) handled = true;
        } catch (...) {}
    }
    return handled;
}

PL_EXPORT bool dispatchMouseEvent(const MouseEvent &event) {
    std::lock_guard<std::mutex> lock(sInputMutex);
    bool handled = false;
    for (const auto& cb : sMouseCallbacks) {
        try {
            if (cb && cb(event)) handled = true;
        } catch (...) {}
    }
    return handled;
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
    return ll::memory::SignatureEngine::resolve(signature, 0, std::string(signature));
}

PL_EXPORT std::unordered_map<std::string, uintptr_t> resolveSignatures(std::span<const std::string> signatures, std::string_view moduleName) {
    (void)moduleName;
    std::unordered_map<std::string, uintptr_t> result;
    for (const auto& sig : signatures) {
        result[sig] = ll::memory::SignatureEngine::resolve(sig, 0, sig);
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
