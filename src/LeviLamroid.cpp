#include "levilamroid/LeviLamroid.h"
#include <atomic>
#include <dlfcn.h>
#include <cstddef>

namespace ll {

namespace hooks {
    bool installAllHooks();
}

static std::atomic_bool sRuntimeInitialized = false;
static std::atomic_bool sEngineBound = false;
static std::mutex sInitMutex;

bool init() {
    std::lock_guard<std::mutex> lock(sInitMutex);
    auto& logger = ll::Logger::getInstance();

    if (!sRuntimeInitialized.exchange(true)) {
        logger.info("=================================================");
        logger.info("   LeviLamroid Android Native Mod Loader v", LEVILAMROID_VERSION_STRING);
        logger.info("   Core Engine Runtime & Reverse-Engineering Hook Hub");
        logger.info("=================================================");
    }

    if (!sEngineBound.load()) {
        uintptr_t base = ll::memory::getMinecraftBase();
        if (base != 0) {
            logger.info("Target process base verified at 0x", std::hex, base);
            ::levilamroid::mcpe::autoDetectVersion();
            hooks::installAllHooks();
            sEngineBound.store(true);
            logger.info("LeviLamroid engine hooks bound successfully.");
        } else {
            logger.info("Target process base not yet mapped; deferred until PreLoader load phase.");
        }
    }

    return true;
}

void shutdown() {
    std::lock_guard<std::mutex> lock(sInitMutex);
    if (!sRuntimeInitialized.exchange(false)) {
        return;
    }
    sEngineBound.store(false);
    ll::Logger::getInstance().info("Shutting down LeviLamroid...");
}

} // namespace ll

// ─── PreLoader Mod Registration Dispatch Table ─────────────────────────────
namespace {

struct PLModRegistration {
    void* instance;
    bool (*load)(void* instance, void* context);
    bool (*enable)(void* instance, void* context);
    bool (*disable)(void* instance, void* context);
    bool (*unload)(void* instance, void* context);
};

static bool pl_loader_load(void* /*instance*/, void* /*context*/) {
    return levilamroid::init();
}

static bool pl_loader_enable(void* /*instance*/, void* /*context*/) {
    return true;
}

static bool pl_loader_disable(void* /*instance*/, void* /*context*/) {
    return true;
}

static bool pl_loader_unload(void* /*instance*/, void* /*context*/) {
    levilamroid::shutdown();
    return true;
}

static PLModRegistration sLoaderRegistration = {
    reinterpret_cast<void*>(0x1),
    pl_loader_load,
    pl_loader_enable,
    pl_loader_disable,
    pl_loader_unload
};
} // namespace

// Lifecycle Exports & Automatic Constructor
__attribute__((constructor))
static void levilamroid_init_ctor() {
    // Promote liblevilamroid.so to RTLD_GLOBAL so subsequent dlopened mods can resolve its symbols
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&levilamroid_init_ctor), &info) && info.dli_fname) {
        dlopen(info.dli_fname, RTLD_NOW | RTLD_GLOBAL);
    }
    levilamroid::init();
}

__attribute__((destructor))
static void levilamroid_fini_dtor() {
    levilamroid::shutdown();
}

// LeviLaunchroid PreLoader C ABI Entrypoints
extern "C" {

__attribute__((visibility("default")))
const void* PLGetModRegistration() {
    levilamroid::init();
    return &sLoaderRegistration;
}

__attribute__((visibility("default")))
bool PLMod_Load(void* /*context*/) {
    return levilamroid::init();
}

__attribute__((visibility("default")))
bool PLMod_Enable(void* /*context*/) {
    return levilamroid::init();
}

__attribute__((visibility("default")))
bool PLMod_Disable(void* /*context*/) {
    return true;
}

__attribute__((visibility("default")))
bool PLMod_Unload(void* /*context*/) {
    levilamroid::shutdown();
    return true;
}

// LeviLamina Legacy C ABI Symbols
__attribute__((visibility("default")))
bool LeviMod_Load(void* /*context*/) {
    return levilamroid::init();
}

__attribute__((visibility("default")))
bool ll_mod_load(void* /*modContext*/) {
    return levilamroid::init();
}

__attribute__((visibility("default")))
bool ll_mod_enable(void* /*modContext*/) {
    return levilamroid::init();
}

__attribute__((visibility("default")))
bool ll_mod_disable(void* /*modContext*/) {
    return true;
}

__attribute__((visibility("default")))
bool ll_mod_unload(void* /*modContext*/) {
    levilamroid::shutdown();
    return true;
}

} // extern "C"
