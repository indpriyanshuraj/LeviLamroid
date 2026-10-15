#include "levilamroid/api/memory/Hook.h"
#include "levilamroid/api/memory/PatternScan.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/mcpe/Symbols.h"
#include "levilamroid/api/Logger.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <mutex>

namespace ll::memory {

static uintptr_t sMinecraftBase = 0;
static void* sMinecraftHandle = nullptr;
static std::mutex sBaseMutex;

uintptr_t getMinecraftBase() {
    std::lock_guard<std::mutex> lock(sBaseMutex);
    if (sMinecraftBase != 0) return sMinecraftBase;

    FILE* fp = fopen("/proc/self/maps", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "libminecraftpe.so") && (strstr(line, "r-xp") || strstr(line, "r--p"))) {
                uintptr_t start = 0;
                if (sscanf(line, "%lx-", &start) == 1) {
                    sMinecraftBase = start;
                    break;
                }
            }
        }
        fclose(fp);
    }

    if (sMinecraftBase == 0) {
        void* handle = dlopen("libminecraftpe.so", RTLD_NOLOAD);
        if (handle) {
            sMinecraftHandle = handle;
            Dl_info info;
            if (dladdr(handle, &info) && info.dli_fbase) {
                sMinecraftBase = reinterpret_cast<uintptr_t>(info.dli_fbase);
            }
        }
    }

    if (sMinecraftBase != 0) {
        Logger::getInstance().info("Discovered libminecraftpe.so base: 0x", std::hex, sMinecraftBase);
    }
    return sMinecraftBase;
}

void setMinecraftBase(uintptr_t base) {
    std::lock_guard<std::mutex> lock(sBaseMutex);
    sMinecraftBase = base;
}

void* findMinecraftSymbol(const char* symbolName) {
    if (!symbolName || symbolName[0] == '\0') return nullptr;

    if (!sMinecraftHandle) {
        sMinecraftHandle = dlopen("libminecraftpe.so", RTLD_NOLOAD);
    }
    if (sMinecraftHandle) {
        void* sym = dlsym(sMinecraftHandle, symbolName);
        if (sym) return sym;
    }

    void* defSym = dlsym(RTLD_DEFAULT, symbolName);
    if (defSym) return defSym;

    return nullptr;
}

void* resolveVtableFunction(const char* className, size_t slot, const char* moduleName) {
    // 1. Try PreLoader API if host exports it
    using ResolveVtableFn = void* (*)(std::string_view, size_t, std::string_view);
    static auto fnPreLoaderResolve = reinterpret_cast<ResolveVtableFn>(
        dlsym(RTLD_DEFAULT, "_ZN2pl6memory21resolveVtableFunctionENSt6__ndk117basic_string_viewIcNS1_11char_traitsIcEEEEmS5_")
    );
    if (fnPreLoaderResolve) {
        void* res = fnPreLoaderResolve(className, slot, moduleName);
        if (res) return res;
    }

    // 2. Direct Itanium VTable resolution: _ZTV + length + className (e.g., _ZTV20LoopbackPacketSender)
    std::string classStr(className);
    std::string vtableSym = "_ZTV" + std::to_string(classStr.length()) + classStr;
    void* vt = findMinecraftSymbol(vtableSym.c_str());
    if (!vt) {
        // Fallback to legacy un-prefixed symbol
        std::string fallbackSym = "_ZTV" + classStr;
        vt = findMinecraftSymbol(fallbackSym.c_str());
    }
    if (vt) {
        // In Itanium C++ ABI, vtable pointer starts after 2 header words (offset-to-top & RTTI)
        void** vtArray = reinterpret_cast<void**>(vt);
        return vtArray[2 + slot];
    }

    return nullptr;
}

// Low-level AArch64 inline trampoline detour
static bool inlineHookAArch64(void* target, void* detour, void** original) {
    if (!target || !detour) return false;

    uintptr_t targetAddr = reinterpret_cast<uintptr_t>(target);
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) pageSize = 4096;

    uintptr_t pageStart = targetAddr & ~(pageSize - 1);
    uintptr_t pageEnd = (targetAddr + 16 + pageSize - 1) & ~(pageSize - 1);
    size_t totalLen = pageEnd - pageStart;

    if (mprotect(reinterpret_cast<void*>(pageStart), totalLen, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return false;
    }

    static uint8_t sTrampolinePool[8192] __attribute__((aligned(4096)));
    static size_t sPoolOffset = 0;
    static std::mutex sPoolMutex;

    std::lock_guard<std::mutex> poolLock(sPoolMutex);
    if (sPoolOffset + 32 > sizeof(sTrampolinePool)) {
        return false;
    }

    uint8_t* trampoline = &sTrampolinePool[sPoolOffset];
    sPoolOffset += 32;

    mprotect(sTrampolinePool, sizeof(sTrampolinePool), PROT_READ | PROT_WRITE | PROT_EXEC);

    // Copy original 16 bytes
    std::memcpy(trampoline, target, 16);

    // Append jump back to target + 16:
    // LDR X16, #8; BR X16; <targetAddr + 16>
    uint32_t jumpBack[4] = {
        0x58000050, // ldr x16, #8
        0xd61f0200, // br x16
        static_cast<uint32_t>((targetAddr + 16) & 0xFFFFFFFF),
        static_cast<uint32_t>((targetAddr + 16) >> 32)
    };
    std::memcpy(trampoline + 16, jumpBack, 16);

    if (original) {
        *original = trampoline;
    }

    // Write detour jump to target: LDR X16, #8; BR X16; <detourAddr>
    uintptr_t detourAddr = reinterpret_cast<uintptr_t>(detour);
    uint32_t hookCode[4] = {
        0x58000050, // ldr x16, #8
        0xd61f0200, // br x16
        static_cast<uint32_t>(detourAddr & 0xFFFFFFFF),
        static_cast<uint32_t>(detourAddr >> 32)
    };
    std::memcpy(target, hookCode, 16);

    __builtin___clear_cache(reinterpret_cast<char*>(pageStart), reinterpret_cast<char*>(pageEnd));
    __builtin___clear_cache(reinterpret_cast<char*>(trampoline), reinterpret_cast<char*>(trampoline) + 32);

    return true;
}

bool hook(void* target, void* detour, void** original) {
    if (!target || !detour) return false;

    // 1. Attempt LeviLaunchroid PreLoader hook API if present in host
    using PlMemoryHookFn = int (*)(void*, void*, void**, int);
    static auto fnPlMemHook = reinterpret_cast<PlMemoryHookFn>(dlsym(RTLD_DEFAULT, "_ZN2pl6memory4hookEPvS1_PS1_NS0_12HookPriorityE"));
    if (fnPlMemHook) {
        int res = fnPlMemHook(target, detour, original, 0);
        if (res == 0) return true;
    }

    // 2. Dobby hook if linked
    using DobbyHookFn = int (*)(void*, void*, void**);
    static auto fnDobbyHook = reinterpret_cast<DobbyHookFn>(dlsym(RTLD_DEFAULT, "DobbyHook"));
    if (fnDobbyHook) {
        int res = fnDobbyHook(target, detour, original);
        if (res == 0) return true;
    }

    // 3. Fallback to native AArch64 inline trampoline
    return inlineHookAArch64(target, detour, original);
}

bool hookRva(uintptr_t rva, void* detour, void** original) {
    uintptr_t base = getMinecraftBase();
    if (!base) return false;
    return hook(reinterpret_cast<void*>(base + rva), detour, original);
}

bool hookGotSlot(uintptr_t gotRva, void* detour, void** original) {
    uintptr_t base = getMinecraftBase();
    if (!base || !detour) return false;

    uintptr_t gotAddr = base + gotRva;
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) pageSize = 4096;

    uintptr_t pageStart = gotAddr & ~(pageSize - 1);
    if (mprotect(reinterpret_cast<void*>(pageStart), pageSize, PROT_READ | PROT_WRITE) != 0) {
        return false;
    }

    void** slot = reinterpret_cast<void**>(gotAddr);
    if (original) {
        *original = *slot;
    }
    *slot = detour;

    __builtin___clear_cache(reinterpret_cast<char*>(pageStart), reinterpret_cast<char*>(pageStart + pageSize));
    return true;
}

namespace internal {

bool registerHookHelper(const char* name, const char* symbolOrPattern, void* detour, void** original) {
    if (!symbolOrPattern) return false;

    void* target = nullptr;

    // Check if it's a hex RVA address string: "0x..."
    if (symbolOrPattern[0] == '0' && (symbolOrPattern[1] == 'x' || symbolOrPattern[1] == 'X')) {
        uintptr_t rva = std::strtoull(symbolOrPattern, nullptr, 16);
        return registerHookHelper(name, rva, detour, original);
    }

    // Check if it's an IDA pattern
    if (strchr(symbolOrPattern, ' ') != nullptr || strchr(symbolOrPattern, '?') != nullptr) {
        uintptr_t addr = scanMinecraft(symbolOrPattern);
        if (addr != 0) target = reinterpret_cast<void*>(addr);
    } else {
        target = findMinecraftSymbol(symbolOrPattern);
    }

    if (target) {
        Logger::getInstance().info("Registering hook [", name, "] at ", target);
        return hook(target, detour, original);
    }

    // Try resolveSymbolAddress fallback with symbolOrPattern, name, or name without "Hook"
    target = levilamroid::mcpe::resolveSymbolAddress(symbolOrPattern);
    if (!target && name) {
        std::string nStr(name);
        target = levilamroid::mcpe::resolveSymbolAddress(nStr);
        if (!target && nStr.ends_with("Hook")) {
            std::string baseName = nStr.substr(0, nStr.length() - 4);
            target = levilamroid::mcpe::resolveSymbolAddress(baseName);
        }
    }

    if (target) {
        Logger::getInstance().info("Registering hook [", name, "] via symbol table at ", target);
        return hook(target, detour, original);
    }

    Logger::getInstance().warn("Failed to resolve target for hook [", name, "]: ", symbolOrPattern);
    return false;
}

LL_API bool registerHookHelper(const char* name, uintptr_t rva, void* detour, void** original) {
    Logger::getInstance().info("Registering hook [", name, "] at RVA 0x", std::hex, rva);
    return hookRva(rva, detour, original);
}

LL_API bool registerMemberHook(const char* name, const char* className, void* memberPtr, void* detour, void** original) {
    struct ItaniumMemberPtr {
        uintptr_t ptr;
        ptrdiff_t adj;
    };
    auto* imp = reinterpret_cast<ItaniumMemberPtr*>(&memberPtr);

#if defined(__aarch64__) || defined(__arm__)
    // In ARM C++ ABI, the virtual flag is bit 0 of adj, and ptr is the vtable byte offset
    bool isVirtual = (imp->adj & 1) != 0;
    size_t slot = imp->ptr / sizeof(void*);
#else
    // Standard Itanium ABI (x86_64): virtual flag is bit 0 of ptr
    bool isVirtual = (imp->ptr & 1) != 0;
    size_t slot = (imp->ptr - 1) / sizeof(void*);
#endif

    if (isVirtual) {
        // Virtual member function
        void* target = resolveVtableFunction(className, slot);

        if (target) {
            Logger::getInstance().info("Registering virtual hook [", name, "] on ", className, " slot ", slot, " at ", target);
            return hook(target, detour, original);
        }

        // Fallback: Check if we have an RVA in our symbol table for this hook or class
        if (name) {
            std::string nStr(name);
            void* symTarget = levilamroid::mcpe::resolveSymbolAddress(nStr);
            if (!symTarget && nStr.ends_with("Hook")) {
                std::string baseName = nStr.substr(0, nStr.length() - 4);
                symTarget = levilamroid::mcpe::resolveSymbolAddress(baseName);
            }
            if (symTarget) {
                Logger::getInstance().info("Registering virtual hook [", name, "] via symbol table fallback at ", symTarget);
                return hook(symTarget, detour, original);
            }
        }

        Logger::getInstance().warn("Could not resolve virtual method for [", name, "] on ", className, " slot ", slot, " (vtable offset 0x", std::hex, imp->ptr, ")");
        if (original) *original = nullptr;
        return false;
    } else if (imp->ptr > 0x10000) {
        // Non-virtual direct function pointer (valid userspace pointer)
        Logger::getInstance().info("Registering direct member hook [", name, "] at 0x", std::hex, imp->ptr);
        return hook(reinterpret_cast<void*>(imp->ptr), detour, original);
    }

    // Fallback: search symbol table by name
    return registerHookHelper(name, name, detour, original);
}

} // namespace internal

} // namespace ll::memory

