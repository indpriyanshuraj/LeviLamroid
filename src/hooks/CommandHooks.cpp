// CommandHooks.cpp — Phase 5: Native Command Registration (No BP)
// Commands are registered by calling CommandRegistry::registerCommand directly
// on the engine's CommandRegistry instance, captured from our existing hook.
// Client-side dispatch is handled via CommandRequestPacket interception.

#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/Logger.h"
#include "ll/api/memory/Hook.h"
#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <cstdio>
#include <cstring>

namespace levilamroid::command {

// ── Command Definition ─────────────────────────────────────────────────────

using CommandHandlerFn = std::function<void(const std::string& args)>;

struct CustomCommandDef {
    std::string name;                   // e.g. "levi"
    std::string description;            // shown in /help
    int permissionLevel{0};             // 0=any, 1=gamemaster, 2=admin, 3=host, 4=owner
    CommandHandlerFn handler;           // called on dispatch
    std::vector<std::string> aliases;   // e.g. {"ll", "leviitems"}
};

static std::unordered_map<std::string, CustomCommandDef> sCommands;
static std::mutex sCmdMutex;
static void* sEngineCommandRegistry = nullptr;  // set by CoreHooks capture
static bool  sCmdRegistryReady = false;

// ── Expose the engine registry pointer to CoreHooks ───────────────────────

// Forward declaration
void flushPendingCommands(void* registry);

void setEngineCommandRegistry(void* registry) {
    if (!registry || sEngineCommandRegistry == registry) return;
    sEngineCommandRegistry = registry;
    if (!sCmdRegistryReady) {
        sCmdRegistryReady = true;
        ll::Logger::getInstance().info("[CommandHooks] Engine CommandRegistry captured: ", registry);
        flushPendingCommands(registry);
    }
}

static uintptr_t getMcBase() {
    static uintptr_t sBase = 0;
    if (sBase) return sBase;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libminecraftpe.so") && (strstr(line, "r-xp") || strstr(line, "r--p"))) {
            uintptr_t start = 0;
            if (sscanf(line, "%lx-", &start) == 1) { sBase = start; break; }
        }
    }
    fclose(fp);
    return sBase;
}

// ── Native registration ────────────────────────────────────────────────────
//
// CommandRegistry::registerCommand(
//     const std::string& name,
//     const char* description,
//     CommandPermissionLevel perm,    // enum: 0=Any, 1=GameMasters, 2=Admin, 3=Host, 4=Owner
//     CommandFlag flag1,              // 0x00
//     CommandFlag flag2               // 0x00
// )
// RVA 1.26.52.3: verified in Offsets.cpp

void flushPendingCommands(void* registry) {
    if (!registry) return;
    auto& logger = ll::Logger::getInstance();
    uintptr_t mcBase = getMcBase();
    if (!mcBase) return;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    if (!off.commandRegistryRegisterCommand) {
        logger.warn("[CommandHooks] commandRegistryRegisterCommand RVA not set");
        return;
    }

    // Correct signature from mangled symbol:
    // _ZN15CommandRegistry15registerCommandERKNSt...EPKcb19CommandPermissionLevel11CommandFlagS8_
    // void CommandRegistry::registerCommand(
    //   const std::string& name, const char* desc,
    //   bool unknownBool,            <-- REQUIRED, was missing
    //   CommandPermissionLevel perm, // passes as int (enum underlying)
    //   CommandFlag flag1,           // passes as int
    //   CommandFlag flag2
    // )
    using RegCmdFn = void (*)(void*, const std::string&, const char*, bool, int, int, int);
    auto regCmd = reinterpret_cast<RegCmdFn>(mcBase + off.commandRegistryRegisterCommand);

    std::lock_guard<std::mutex> lock(sCmdMutex);
    for (auto& [name, def] : sCommands) {
        try {
            regCmd(registry, def.name, def.description.c_str(), false, def.permissionLevel, 0, 0);
            logger.info("[CommandHooks] Registered /", def.name, " (perm=", def.permissionLevel, ")");
            for (const auto& alias : def.aliases) {
                std::string aliasDesc = std::string("Alias for /") + def.name;
                regCmd(registry, alias, aliasDesc.c_str(), false, def.permissionLevel, 0, 0);
                logger.info("[CommandHooks] Registered alias /", alias, " -> /", def.name);
            }
        } catch (...) {
            logger.error("[CommandHooks] Exception registering /", def.name);
        }
    }
}

// ── Command dispatch (client-side, called from CoreHooks packet interceptor) ─

void dispatchCommand(const std::string& rawInput) {
    // Strip leading '/'
    std::string input = rawInput;
    if (!input.empty() && input[0] == '/') input = input.substr(1);

    // Split into command name + args
    size_t space = input.find(' ');
    std::string cmdName = (space != std::string::npos) ? input.substr(0, space) : input;
    std::string args    = (space != std::string::npos) ? input.substr(space + 1) : "";

    std::lock_guard<std::mutex> lock(sCmdMutex);
    // Direct match
    auto it = sCommands.find(cmdName);
    if (it != sCommands.end()) {
        it->second.handler(args);
        return;
    }
    // Alias match
    for (auto& [name, def] : sCommands) {
        for (const auto& alias : def.aliases) {
            if (alias == cmdName) {
                def.handler(args);
                return;
            }
        }
    }
    // Not our command — let the engine handle it (do nothing, packet already dispatched)
}

// ── Public registration API ───────────────────────────────────────────────

bool registerCommand(const std::string& name, const std::string& desc,
                     int permissionLevel, CommandHandlerFn handler,
                     const std::vector<std::string>& aliases)
{
    auto& logger = ll::Logger::getInstance();
    CustomCommandDef def;
    def.name = name;
    def.description = desc;
    def.permissionLevel = permissionLevel;
    def.handler = std::move(handler);
    def.aliases = aliases;

    {
        std::lock_guard<std::mutex> lock(sCmdMutex);
        sCommands[name] = std::move(def);
    }

    if (sCmdRegistryReady && sEngineCommandRegistry) {
        uintptr_t mcBase = getMcBase();
        const auto& off = ::levilamroid::mcpe::getOffsets();
        if (mcBase && off.commandRegistryRegisterCommand) {
            using RegCmdFn = void (*)(void*, const std::string&, const char*, bool, int, int, int);
            auto regCmd = reinterpret_cast<RegCmdFn>(mcBase + off.commandRegistryRegisterCommand);
            try {
                regCmd(sEngineCommandRegistry, name, desc.c_str(), false, permissionLevel, 0, 0);
                logger.info("[CommandHooks] Late-registered /", name);
            } catch (...) {
                logger.error("[CommandHooks] Exception late-registering /", name);
            }
        }
    }
    logger.info("[CommandHooks] Queued command: /", name);
    return true;
}

bool installCommandHooks() {
    ll::Logger::getInstance().info("[CommandHooks] Command subsystem ready (dispatch via packet intercept).");
    return true;
}

} // namespace levilamroid::command

extern "C" {

// Callback type for C callers: void handler(const char* args)
using CCommandHandler = void (*)(const char* args);

// Simple C-API command registration
LL_API bool ll_command_register(const char* name, const char* desc, int perm, CCommandHandler handler) {
    if (!name || !handler) return false;
    return ::levilamroid::command::registerCommand(
        name, desc ? desc : "", perm,
        [handler](const std::string& args) { handler(args.c_str()); },
        {});
}

// Register command with aliases
LL_API bool ll_command_register_with_aliases(
    const char* name, const char* desc, int perm, CCommandHandler handler,
    const char* alias1, const char* alias2, const char* alias3)
{
    if (!name || !handler) return false;
    std::vector<std::string> aliases;
    if (alias1) aliases.push_back(alias1);
    if (alias2) aliases.push_back(alias2);
    if (alias3) aliases.push_back(alias3);
    return ::levilamroid::command::registerCommand(
        name, desc ? desc : "", perm,
        [handler](const std::string& args) { handler(args.c_str()); },
        aliases);
}

// Dispatch a command string (called from packet interceptor in CoreHooks)
LL_API void ll_command_dispatch_client(const char* rawInput) {
    if (!rawInput) return;
    ::levilamroid::command::dispatchCommand(rawInput);
}

// Expose registry capture for CoreHooks
LL_API void ll_command_set_engine_registry(void* registry) {
    ::levilamroid::command::setEngineCommandRegistry(registry);
}

} // extern "C"
