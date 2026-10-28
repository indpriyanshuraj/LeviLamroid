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
    if (!className) return nullptr;

    uintptr_t mcBase = getMinecraftBase();
    if (mcBase) {
        const auto& off = ::levilamroid::mcpe::getOffsets();
        uintptr_t vtableSlot0 = 0;

        if (strstr(className, "MouseHandlerAndroid") != nullptr) {
            if (slot == 2 && off.mouseHandlerAndroidAbsoluteMouseInput) {
                return reinterpret_cast<void*>(mcBase + off.mouseHandlerAndroidAbsoluteMouseInput);
            }
            if (slot == 5 && off.mouseHandlerAndroidButtonInput) {
                return reinterpret_cast<void*>(mcBase + off.mouseHandlerAndroidButtonInput);
            }
            if (slot == 6 && off.mouseHandlerAndroidScrollInput) {
                return reinterpret_cast<void*>(mcBase + off.mouseHandlerAndroidScrollInput);
            }
            vtableSlot0 = off.mouseHandlerAndroidVtable;
        } else if (strstr(className, "ClientInstance") != nullptr) {
            vtableSlot0 = off.clientInstanceVtable;
        } else if (strstr(className, "MinecraftGame") != nullptr) {
            vtableSlot0 = off.minecraftGameVtable;
        } else if (strstr(className, "BlockSource") != nullptr) {
            vtableSlot0 = off.blockSourceVtable;
        }

        if (vtableSlot0 != 0) {
            void** vtArray = reinterpret_cast<void**>(mcBase + vtableSlot0);
            return vtArray[slot];
        }
    }

    // 1. Try PreLoader API only if host exports it AND it belongs to genuine libpreloader.so
    using ResolveVtableFn = void* (*)(std::string_view, size_t, std::string_view);
    static auto fnPreLoaderResolve = reinterpret_cast<ResolveVtableFn>(
        dlsym(RTLD_DEFAULT, "_ZN2pl6memory21resolveVtableFunctionENSt6__ndk117basic_string_viewIcNS1_11char_traitsIcEEEEmS5_")
    );
    if (fnPreLoaderResolve) {
        Dl_info dlinfo;
        if (dladdr(reinterpret_cast<void*>(fnPreLoaderResolve), &dlinfo) && dlinfo.dli_fname) {
            if (strstr(dlinfo.dli_fname, "libpreloader.so")) {
                void* res = fnPreLoaderResolve(className, slot, moduleName ? moduleName : "libminecraftpe.so");
                if (res) return res;
            }
        }
    }

    // 2. Direct Itanium VTable resolution: _ZTV + className / _ZTV + length + className
    std::string classStr(className);
    void* vt = nullptr;
    if (classStr.rfind("_ZTV", 0) == 0) {
        vt = findMinecraftSymbol(classStr.c_str());
    } else {
        std::string fallbackSym = "_ZTV" + classStr;
        vt = findMinecraftSymbol(fallbackSym.c_str());
        if (!vt) {
            std::string vtableSym = "_ZTV" + std::to_string(classStr.length()) + classStr;
            vt = findMinecraftSymbol(vtableSym.c_str());
        }
    }
    if (vt) {
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
    static PlMemoryHookFn fnPlMemHook = nullptr;
    static bool sPlResolved = false;
    if (!sPlResolved) {
        sPlResolved = true;
        void* h = dlopen("libpreloader.so", RTLD_NOLOAD);
        if (h) {
            fnPlMemHook = reinterpret_cast<PlMemoryHookFn>(dlsym(h, "pl_hook"));
            if (!fnPlMemHook) {
                fnPlMemHook = reinterpret_cast<PlMemoryHookFn>(dlsym(h, "_ZN2pl6memory4hookEPvS1_PS1_NS0_12HookPriorityE"));
            }
        }
        if (!fnPlMemHook) {
            fnPlMemHook = reinterpret_cast<PlMemoryHookFn>(dlsym(RTLD_DEFAULT, "pl_hook"));
        }
        if (!fnPlMemHook) {
            fnPlMemHook = reinterpret_cast<PlMemoryHookFn>(dlsym(RTLD_DEFAULT, "_ZN2pl6memory4hookEPvS1_PS1_NS0_12HookPriorityE"));
        }
    }
    if (fnPlMemHook) {
        // Pass HookPriority::Normal (200) to allow cooperative hook chaining with other mods (e.g. BedrockTools)
        int res = fnPlMemHook(target, detour, original, 200);
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
    if (rva == 0 || !detour) {
        if (original) *original = nullptr;
        return false;
    }
    uintptr_t base = getMinecraftBase();
    if (!base) return false;
    return hook(reinterpret_cast<void*>(base + rva), detour, original);
}

bool hookGotSlot(uintptr_t gotRva, void* detour, void** original) {
    if (gotRva == 0 || !detour) {
        if (original) *original = nullptr;
        return false;
    }
    uintptr_t base = getMinecraftBase();
    if (!base) return false;

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

bool hookSignature(const std::string& pattern, void* detour, void** original, uintptr_t fallbackRva) {
    uintptr_t base = getMinecraftBase();
    if (!base || pattern.empty() || !detour) return false;

    // 1. Fast-Path: Check if fallbackRva matches the signature
    if (fallbackRva != 0) {
        uintptr_t targetAddr = base + fallbackRva;
        if (verifyPattern(targetAddr, pattern)) {
            Logger::getInstance().info("Hooked via verified signature fast-path at RVA 0x", std::hex, fallbackRva);
            return hook(reinterpret_cast<void*>(targetAddr), detour, original);
        } else {
            Logger::getInstance().warn("Fallback RVA 0x", std::hex, fallbackRva, " did not match signature. Scanning libminecraftpe.so...");
        }
    }

    // 2. Localized scan near fallbackRva (+/- 64KB) if fallbackRva provided
    if (fallbackRva != 0) {
        uintptr_t winStart = (fallbackRva > 0x10000) ? (base + fallbackRva - 0x10000) : base;
        uintptr_t foundAddr = scanPattern(pattern, winStart, 0x20000);
        if (foundAddr != 0) {
            Logger::getInstance().info("Hooked via localized signature scan at 0x", std::hex, foundAddr,
                                       " (RVA 0x", std::hex, (foundAddr - base), ")");
            return hook(reinterpret_cast<void*>(foundAddr), detour, original);
        }
        Logger::getInstance().warn("Signature not found in localized window for RVA 0x", std::hex, fallbackRva);
        return false;
    }

    // 3. Fallback: scan .text section first, then full binary if needed
    uintptr_t foundAddr = scanMinecraftText(pattern);
    if (foundAddr == 0) {
        foundAddr = scanMinecraft(pattern);
    }
    if (foundAddr != 0) {
        Logger::getInstance().info("Hooked via signature scan at 0x", std::hex, foundAddr, 
                                   " (RVA 0x", std::hex, (foundAddr - base), ")");
        return hook(reinterpret_cast<void*>(foundAddr), detour, original);
    }

    Logger::getInstance().error("Failed to find signature in libminecraftpe.so: ", pattern);
    return false;
}

namespace internal {

bool registerHookHelper(const char* name, uintptr_t rva, void* detour, void** original);

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
    if (rva == 0) {
        Logger::getInstance().warn("Skipping hook [", name ? name : "unknown", "] with null RVA 0x0");
        if (original) *original = nullptr;
        return false;
    }
    Logger::getInstance().info("Registering hook [", name ? name : "unknown", "] at RVA 0x", std::hex, rva);
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

