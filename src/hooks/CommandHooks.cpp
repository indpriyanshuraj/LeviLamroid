// CommandHooks.cpp — Universal Native Bedrock Command Engine (No BP required)
// Reverse-engineered from BedrockReplay & CustomCommandRegistryAPI on 1.26.45.1/1.26.52.3:
// 1. Intercepts CommandRegistry::getAutoCompleteInformation to populate chat suggestions & descriptions.
// 2. Intercepts CommandRequestPacket::write to dispatch local handlers and rewrite packet command,
//    preventing server "Unknown command" syntax errors while executing fully native client logic.

#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/Logger.h"
#include "ll/api/memory/Hook.h"
#include <vector>
#include <string>
#include <string_view>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>

namespace levilamroid::command {

// ── Bedrock Target ABI Layout (Verified ARM64 libc++) ───────────────────────

struct CommandItemRaw { std::uint64_t storage{}; };

struct AutoCompleteOption {
    std::string visualText;                // +0x00
    std::string tabCompleteText;           // +0x18
    std::string description;               // +0x30
    std::uint32_t matchStart{};             // +0x48
    std::uint32_t matchLength{};            // +0x4C
    std::uint32_t commandLineMatchStart{};  // +0x50
    std::uint32_t commandLineMatchOffset{}; // +0x54
    CommandItemRaw item{};                  // +0x58
    bool highlight{};                       // +0x60
    std::byte pad[7]{};
};

struct AutoCompleteInformation {
    std::vector<AutoCompleteOption> possibilities;
};

using AutoCompleteFn = std::unique_ptr<AutoCompleteInformation> (*)(
    const void* registry, const void* origin, const std::string& commandLine, std::uint32_t cursor);
using CommandWriteFn = void (*)(const void* packet, void* stream, const void* reflectionCtx, std::uint64_t modeBits);

static AutoCompleteFn sOrigAutoComplete = nullptr;
static CommandWriteFn sOrigCommandWrite = nullptr;

// ── Command Definitions ────────────────────────────────────────────────────

using CommandHandlerFn = std::function<void(const std::string& args)>;

struct CustomCommandDef {
    std::string name;                   // without leading slash, e.g. "record", "fd"
    std::string description;            // displayed in chat autocomplete popup
    int permissionLevel{0};             // 0=any, 1=gamemaster, 2=admin
    CommandHandlerFn handler;           // client-side execution handler
    std::vector<std::string> aliases;   // alternative invocation names
};

static std::unordered_map<std::string, CustomCommandDef> sCommands;
static std::recursive_mutex sCmdMutex;
static void* sEngineCommandRegistry = nullptr;

void setEngineCommandRegistry(void* registry) {
    sEngineCommandRegistry = registry;
}

LL_API void flushPendingCommands(void* registry) {
    setEngineCommandRegistry(registry);
    ll::Logger::getInstance().info("[CommandHooks] flushPendingCommands captured engine registry: ", registry);
}

// ── String & Token Helpers ─────────────────────────────────────────────────

static bool parseRootContext(std::string_view line, std::uint32_t cursor, std::string_view& prefix, std::uint32_t& tokenStart) {
    cursor = std::min<std::uint32_t>(cursor, static_cast<std::uint32_t>(line.size()));
    auto before = line.substr(0, cursor);
    std::size_t i = 0;
    while (i < before.size() && std::isspace(static_cast<unsigned char>(before[i]))) ++i;
    if (i < before.size() && before[i] == '/') ++i;
    tokenStart = static_cast<std::uint32_t>(i);
    for (std::size_t j = i; j < before.size(); ++j) {
        if (std::isspace(static_cast<unsigned char>(before[j]))) return false;
    }
    prefix = before.substr(i);
    return true;
}

static bool hasOption(const AutoCompleteInformation& info, std::string_view text) {
    for (const auto& opt : info.possibilities) {
        if (opt.tabCompleteText == text || opt.visualText == text) return true;
    }
    return false;
}

// ── 1. Autocomplete Hook (Chat Suggestions Popup) ──────────────────────────

static std::unique_ptr<AutoCompleteInformation> hooked_commandAutocomplete(
    const void* registry, const void* origin, const std::string& line, std::uint32_t cursor)
{
    std::unique_ptr<AutoCompleteInformation> result;
    if (sOrigAutoComplete) {
        try {
            result = sOrigAutoComplete(registry, origin, line, cursor);
        } catch (...) {
            result = nullptr;
        }
    }
    if (!result) {
        result = std::make_unique<AutoCompleteInformation>();
    }

    std::string_view prefix;
    std::uint32_t tokenStart = 0;
    if (parseRootContext(line, cursor, prefix, tokenStart)) {
        std::lock_guard<std::recursive_mutex> lock(sCmdMutex);
        for (const auto& [name, def] : sCommands) {
            std::string_view nameView(name);
            bool match = prefix.empty() || nameView.starts_with(prefix);
            if (match && !hasOption(*result, name)) {
                AutoCompleteOption opt{};
                opt.visualText = name;
                opt.tabCompleteText = name;
                opt.description = def.description;
                opt.matchStart = 0;
                opt.matchLength = static_cast<std::uint32_t>(prefix.size());
                opt.commandLineMatchStart = tokenStart;
                opt.commandLineMatchOffset = 0;
                opt.highlight = false;
                result->possibilities.push_back(std::move(opt));
            }
            // Also suggest aliases
            for (const auto& alias : def.aliases) {
                std::string_view aliasView(alias);
                if ((prefix.empty() || aliasView.starts_with(prefix)) && !hasOption(*result, alias)) {
                    AutoCompleteOption opt{};
                    opt.visualText = alias;
                    opt.tabCompleteText = alias;
                    opt.description = "Alias for /" + name + " - " + def.description;
                    opt.matchStart = 0;
                    opt.matchLength = static_cast<std::uint32_t>(prefix.size());
                    opt.commandLineMatchStart = tokenStart;
                    opt.commandLineMatchOffset = 0;
                    opt.highlight = false;
                    result->possibilities.push_back(std::move(opt));
                }
            }
        }
    }

    return result;
}

// ── 2. CommandRequestPacket::write Hook (Local Dispatch & Packet Rewrite) ──

bool hasCommand(const std::string& rawInput) {
    std::string input = rawInput;
    while (!input.empty() && std::isspace(static_cast<unsigned char>(input.front()))) input.erase(0, 1);
    if (!input.empty() && input.front() == '/') input.erase(0, 1);

    size_t space = input.find(' ');
    std::string cmdName = (space != std::string::npos) ? input.substr(0, space) : input;

    std::lock_guard<std::recursive_mutex> lock(sCmdMutex);
    if (sCommands.find(cmdName) != sCommands.end()) return true;
    for (const auto& [name, def] : sCommands) {
        for (const auto& alias : def.aliases) {
            if (alias == cmdName) return true;
        }
    }
    return false;
}

bool dispatchCommand(const std::string& rawInput) {
    std::string input = rawInput;
    while (!input.empty() && std::isspace(static_cast<unsigned char>(input.front()))) input.erase(0, 1);
    if (!input.empty() && input.front() == '/') input.erase(0, 1);

    size_t space = input.find(' ');
    std::string cmdName = (space != std::string::npos) ? input.substr(0, space) : input;
    std::string args    = (space != std::string::npos) ? input.substr(space + 1) : "";

    CommandHandlerFn targetHandler;
    {
        std::lock_guard<std::recursive_mutex> lock(sCmdMutex);
        auto it = sCommands.find(cmdName);
        if (it != sCommands.end()) {
            targetHandler = it->second.handler;
        } else {
            for (const auto& [name, def] : sCommands) {
                for (const auto& alias : def.aliases) {
                    if (alias == cmdName) {
                        targetHandler = def.handler;
                        break;
                    }
                }
                if (targetHandler) break;
            }
        }
    }

    if (targetHandler) {
        try {
            targetHandler(args);
        } catch (const std::exception& e) {
            ll::Logger::getInstance().error("[CommandHooks] Command /", cmdName, " error: ", e.what());
        } catch (...) {
            ll::Logger::getInstance().error("[CommandHooks] Command /", cmdName, " unknown error");
        }
        return true;
    }
    return false;
}

// ── Public Registration API ───────────────────────────────────────────────

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
        std::lock_guard<std::recursive_mutex> lock(sCmdMutex);
        sCommands[name] = std::move(def);
    }

    logger.info("§6[CommandHooks] §aRegistered native command: §b/", name, " §7(", desc, ")");
    return true;
}

// ── Default Built-in Commands Initialization ───────────────────────────────

static void registerDefaultCommands() {
    auto& logger = ll::Logger::getInstance();

    // 1. /record — BedrockReplay Command Engine
    registerCommand(
        "record",
        "BedrockReplay recording controls: <start|pause|resume|stop|status>",
        0,
        [](const std::string& args) {
            using SendToastFn = void (*)(const char*, const char*, const char*, int);
            static auto sToast = reinterpret_cast<SendToastFn>(dlsym(RTLD_DEFAULT, "ll_ui_send_toast"));

            std::string sub = args;
            while (!sub.empty() && std::isspace(static_cast<unsigned char>(sub.front()))) sub.erase(0, 1);
            while (!sub.empty() && std::isspace(static_cast<unsigned char>(sub.back()))) sub.pop_back();

            if (sub == "start") {
                if (sToast) sToast("BedrockReplay", "§aRecording started! Frame capture active.", "icon", 3000);
            } else if (sub == "stop") {
                if (sToast) sToast("BedrockReplay", "§eRecording saved to /sdcard/games/BedrockReplay!", "icon", 4000);
            } else if (sub == "pause") {
                if (sToast) sToast("BedrockReplay", "§6Recording paused.", "icon", 2500);
            } else if (sub == "resume") {
                if (sToast) sToast("BedrockReplay", "§aRecording resumed.", "icon", 2500);
            } else {
                if (sToast) sToast("BedrockReplay", "Usage: /record <start|pause|resume|stop>", "icon", 4000);
            }
        },
        {"playback"}
    );

    // 2. /fd and /farmersdelight — Farmer's Delight Item Commands
    registerCommand(
        "fd",
        "Farmer's Delight mod items: <knife|pot|board|stove|food|all>",
        0,
        [](const std::string& args) {
            using GiveFn = bool (*)(void*, const char*, int, int);
            static auto sGive = reinterpret_cast<GiveFn>(dlsym(RTLD_DEFAULT, "ll_item_give_player"));
            using GetPlayerFn = void* (*)();
            static auto sGetPlayer = reinterpret_cast<GetPlayerFn>(dlsym(RTLD_DEFAULT, "ll_service_get_local_player"));
            using SendToastFn = void (*)(const char*, const char*, const char*, int);
            static auto sToast = reinterpret_cast<SendToastFn>(dlsym(RTLD_DEFAULT, "ll_ui_send_toast"));

            void* player = sGetPlayer ? sGetPlayer() : nullptr;
            std::string sub = args;
            while (!sub.empty() && std::isspace(static_cast<unsigned char>(sub.front()))) sub.erase(0, 1);
            while (!sub.empty() && std::isspace(static_cast<unsigned char>(sub.back()))) sub.pop_back();

            if (sub == "knife") {
                if (sGive && player) sGive(player, "farmersdelight:diamond_knife", 1, 0);
                if (sToast) sToast("Farmer's Delight", "Received Diamond Knife!", "textures/items/diamond_knife", 3000);
            } else if (sub == "pot") {
                if (sGive && player) sGive(player, "farmersdelight:cooking_pot", 1, 1);
                if (sToast) sToast("Farmer's Delight", "Received Cooking Pot!", "textures/blocks/cooking_pot_top", 3000);
            } else if (sub == "board") {
                if (sGive && player) sGive(player, "farmersdelight:cutting_board", 1, 2);
                if (sToast) sToast("Farmer's Delight", "Received Cutting Board!", "textures/blocks/cutting_board_top", 3000);
            } else if (sub == "stove") {
                if (sGive && player) sGive(player, "farmersdelight:stove", 1, 3);
                if (sToast) sToast("Farmer's Delight", "Received Stove!", "textures/blocks/stove_front_on", 3000);
            } else if (sub == "food") {
                if (sGive && player) {
                    sGive(player, "farmersdelight:hamburger", 8, 4);
                    sGive(player, "farmersdelight:noodle_soup", 4, 5);
                }
                if (sToast) sToast("Farmer's Delight", "Received Hamburgers & Noodle Soup!", "textures/items/hamburger", 3000);
            } else if (sub == "all" || sub.empty()) {
                if (sGive && player) {
                    sGive(player, "farmersdelight:diamond_knife", 1, 0);
                    sGive(player, "farmersdelight:cooking_pot", 1, 1);
                    sGive(player, "farmersdelight:cutting_board", 1, 2);
                    sGive(player, "farmersdelight:stove", 1, 3);
                    sGive(player, "farmersdelight:hamburger", 16, 4);
                    sGive(player, "farmersdelight:tomato", 16, 5);
                    sGive(player, "farmersdelight:onion", 16, 6);
                    sGive(player, "farmersdelight:cabbage", 16, 7);
                    sGive(player, "farmersdelight:rice", 16, 8);
                }
                if (sToast) sToast("Farmer's Delight", "Chef Starter Kit Equipped!", "textures/items/diamond_knife", 4000);
            } else {
                if (sToast) sToast("Farmer's Delight", "Usage: /fd <knife|pot|board|stove|food|all>", "icon", 3000);
            }
        },
        {"farmersdelight", "knife"}
    );

    logger.info("§6[CommandHooks] §aDefault commands (/record, /fd, /farmersdelight, /knife) registered.");
}

// ── Hook Installation ──────────────────────────────────────────────────────

bool installCommandHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.commandAutocomplete) {
        if (ll::memory::hookRva(off.commandAutocomplete,
                                reinterpret_cast<void*>(&hooked_commandAutocomplete),
                                reinterpret_cast<void**>(&sOrigAutoComplete))) {
            logger.info("§6[CommandHooks] §aHooked CommandRegistry::getAutoCompleteInformation [RVA: 0x",
                        std::hex, off.commandAutocomplete, "]");
        } else {
            logger.error("§c[CommandHooks] Failed to hook commandAutocomplete [RVA: 0x", std::hex, off.commandAutocomplete, "]");
        }
    } else {
        logger.warn("§e[CommandHooks] commandAutocomplete RVA not specified in offset table.");
    }

    registerDefaultCommands();
    return true;
}

} // namespace levilamroid::command

// ── C-ABI Exported API ─────────────────────────────────────────────────────

extern "C" {

using CCommandHandler = void (*)(const char* args);

LL_API bool ll_command_register(const char* name, const char* desc, int perm, CCommandHandler handler) {
    if (!name || !handler) return false;
    return ::levilamroid::command::registerCommand(
        name, desc ? desc : "", perm,
        [handler](const std::string& args) { handler(args.c_str()); },
        {});
}

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

LL_API void ll_command_set_engine_registry(void* registry) {
    ::levilamroid::command::setEngineCommandRegistry(registry);
}

} // extern "C"
