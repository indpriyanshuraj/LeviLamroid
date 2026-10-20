#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerEvents.h"
#include "ll/api/event/world/WorldEvents.h"
#include "ll/api/chrono/GameTickScheduler.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/resource/ResourceManager.h"
#include "levilamroid/api/item/ItemRegistry.h"
#include <atomic>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

namespace levilamroid::hooks {

bool installUIHooks();
bool installRenderHooks();
bool installResourceHooks();
void dispatchStartMenuOpen();
} // namespace levilamroid::hooks

namespace levilamroid::item {
bool installItemHooks();
}

namespace levilamroid::block {
bool installBlockHooks();
}

namespace levilamroid::recipe {
bool installRecipeHooks();
}

namespace levilamroid::command {
bool installCommandHooks();
void flushPendingCommands(void* registry);
}

namespace levilamroid::hooks {

[[maybe_unused]] static uintptr_t getMcBase() {
    static uintptr_t sBase = 0;
    if (sBase != 0) return sBase;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libminecraftpe.so") && (strstr(line, "r-xp") || strstr(line, "r--p"))) {
            uintptr_t start = 0;
            if (sscanf(line, "%lx-", &start) == 1) {
                sBase = start;
                break;
            }
        }
    }
    fclose(fp);
    return sBase;
}

static std::atomic<bool> sIsInWorld = false;
static std::atomic<bool> sIsMainMenu = false;
static std::atomic<bool> sIsLoading = true; // True at initial boot
static std::atomic<bool> sCameraLocked = false;

static void* sCurrentLocalPlayer = nullptr;
static void* sCurrentClientInstance = nullptr;
static void* sCurrentLevel = nullptr;

static void* sOrigClientInstanceUpdate = nullptr;
static void* sOrigPlayerNormalTick = nullptr;
static void* sOrigApplyTurnDelta = nullptr;
static void* sOrigCommandRegistryRegisterCommand = nullptr;
static void* sEngineCommandRegistry = nullptr;

static std::unordered_map<std::string, std::function<void(const std::string&)>> sClientCommands;
static std::mutex sClientCommandMutex;

LL_API void ll_command_register_client(const char* name, void (*handler)(const char* args)) {
    if (!name || !handler) return;
    std::lock_guard<std::mutex> lock(sClientCommandMutex);
    std::string key = name;
    if (!key.empty() && key[0] == '/') key = key.substr(1);
    sClientCommands[key] = [handler](const std::string& args) {
        handler(args.c_str());
    };
    ll::Logger::getInstance().info("Registered native client command: /", key);
}

LL_API bool ll_command_dispatch_client(const char* fullCommand) {
    if (!fullCommand) return false;
    std::string cmd = fullCommand;
    if (!cmd.empty() && (cmd[0] == '/' || cmd[0] == '!')) cmd = cmd.substr(1);

    auto spacePos = cmd.find(' ');
    std::string baseCmd = (spacePos == std::string::npos) ? cmd : cmd.substr(0, spacePos);
    std::string args = (spacePos == std::string::npos) ? "" : cmd.substr(spacePos + 1);

    // 1. Check ll::command::CommandRegistrar first
    if (ll::command::CommandRegistrar::getInstance().dispatchCommand(fullCommand)) {
        return true;
    }
    if (ll::command::CommandRegistrar::getInstance().dispatchCommand(cmd)) {
        return true;
    }

    // 2. Check sClientCommands map
    std::function<void(const std::string&)> cb;
    {
        std::lock_guard<std::mutex> lock(sClientCommandMutex);
        auto it = sClientCommands.find(baseCmd);
        if (it != sClientCommands.end()) {
            cb = it->second;
        }
    }

    if (cb) {
        cb(args);
        return true;
    }
    return false;
}



void setInWorld(bool inWorld) {
    sIsInWorld.store(inWorld);
    if (inWorld) {
        sIsMainMenu.store(false);
        sIsLoading.store(false);
        ::levilamroid::item::refreshCustomBlockBindings();
    } else {
        sCurrentLocalPlayer = nullptr;
        sCurrentLevel = nullptr;
    }
}

void setIsMainMenu(bool isMenu) {
    sIsMainMenu.store(isMenu);
    if (isMenu) {
        sIsInWorld.store(false);
        sIsLoading.store(false);
        sCurrentLocalPlayer = nullptr;
        sCurrentLevel = nullptr;
    }
}

void setIsLoading(bool loading) {
    sIsLoading.store(loading);
    if (loading) {
        sIsMainMenu.store(false);
    }
}

bool isInWorld() {
    return sIsInWorld.load();
}

bool isMainMenu() {
    if (sIsLoading.load()) return false;
    if (sIsInWorld.load()) return false;
    return sIsMainMenu.load();
}

bool isLoading() {
    return sIsLoading.load();
}

static bool isPointerReadable(const void* ptr, size_t size = sizeof(void*)) {
    if (!ptr || reinterpret_cast<uintptr_t>(ptr) < 0x10000 || size == 0) return false;
    static int sPipe[2] = {-1, -1};
    static std::once_flag sPipeOnce;
    std::call_once(sPipeOnce, []() {
        if (pipe(sPipe) == 0) {
            fcntl(sPipe[0], F_SETFL, O_NONBLOCK);
            fcntl(sPipe[1], F_SETFL, O_NONBLOCK);
        }
    });
    if (sPipe[1] < 0) return false;
    ssize_t written = write(sPipe[1], ptr, size);
    if (written > 0) {
        char dummy[128];
        while (written > 0) {
            ssize_t r = read(sPipe[0], dummy, std::min<size_t>(written, sizeof(dummy)));
            if (r <= 0) break;
            written -= r;
        }
        return true;
    }
    return false;
}

static void hooked_clientInstanceUpdate(void* self) {
    sCurrentClientInstance = self;
    if (!sIsInWorld.load()) {
        sIsMainMenu.store(true);
        sIsLoading.store(false);
        static bool sFirstMenuOpen = false;
        if (!sFirstMenuOpen) {
            sFirstMenuOpen = true;
            dispatchStartMenuOpen();
        }
    }
    if (sOrigClientInstanceUpdate) {
        reinterpret_cast<void(*)(void*)>(sOrigClientInstanceUpdate)(self);
    }
}

static void* sOrigClientInstanceRequestLeaveGame = nullptr;

static void hooked_clientInstanceRequestLeaveGame(void* self, bool b1, bool b2) {
    ll::Logger::getInstance().info("ClientInstance::requestLeaveGame triggered.");
    setInWorld(false);
    setIsMainMenu(true);
    sCurrentLocalPlayer = nullptr;
    sCurrentLevel = nullptr;

    ll::event::WorldLeaveEvent leaveEv;
    ll::event::EventBus::getInstance().publish(leaveEv);

    if (sOrigClientInstanceRequestLeaveGame) {
        reinterpret_cast<void(*)(void*, bool, bool)>(sOrigClientInstanceRequestLeaveGame)(self, b1, b2);
    }
}

static void* sOrigMinecraftGameClientUpdate = nullptr;

static void hooked_minecraftGameClientUpdate(void* self, void* arg1) {
    ll::event::ClientTickEvent ev(sCurrentClientInstance);
    ll::event::EventBus::getInstance().publish(ev);

    if (sOrigMinecraftGameClientUpdate) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigMinecraftGameClientUpdate)(self, arg1);
    }
}

static void hooked_playerNormalTick(void* self) {
    sCurrentLocalPlayer = self;
    if (self && isPointerReadable(reinterpret_cast<uint8_t*>(self) + 472, sizeof(void*))) {
        sCurrentLevel = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 472);
    }
    if (!sIsInWorld.load()) {
        setInWorld(true);
        ll::event::WorldJoinEvent joinEv;
        ll::event::EventBus::getInstance().publish(joinEv);
    }

    // Publish PlayerTickEvent
    ll::event::PlayerTickEvent ev(reinterpret_cast<Player*>(self));
    ll::event::EventBus::getInstance().publish(ev);

    if (sOrigPlayerNormalTick) {
        reinterpret_cast<void(*)(void*)>(sOrigPlayerNormalTick)(self);
    }
}

static void* sOrigLocalPlayerMove = nullptr;

static void hooked_localPlayerMove(void* self, const void* delta) {
    if (self) {
        sCurrentLocalPlayer = self;
        if (delta && isPointerReadable(delta, sizeof(float) * 3)) {
            const float* d = reinterpret_cast<const float*>(delta);
            Vec3 v{d[0], d[1], d[2]};
            ll::event::PlayerMoveEvent ev(reinterpret_cast<Player*>(self), v);
            ll::event::EventBus::getInstance().publish(ev);
        }
    }
    if (sOrigLocalPlayerMove) {
        reinterpret_cast<void(*)(void*, const void*)>(sOrigLocalPlayerMove)(self, delta);
    }
}

static void hooked_applyTurnDelta(void* self, void* delta) {
    if (sCameraLocked.load()) {
        return; // Freeze camera orientation when camera locked
    }
    if (sOrigApplyTurnDelta) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigApplyTurnDelta)(self, delta);
    }
}

static void* sOrigGameModeDestroyBlock = nullptr;

static bool hooked_gameModeDestroyBlock(void* self, const void* pos, uint8_t face) {
    void* player = self ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 0x8) : nullptr;
    if (!player) player = sCurrentLocalPlayer;
    if (pos && isPointerReadable(pos, sizeof(int) * 3)) {
        const int* p = reinterpret_cast<const int*>(pos);
        BlockPos bpos{p[0], p[1], p[2]};
        ll::event::BlockBreakEvent ev(nullptr, bpos, reinterpret_cast<Player*>(player));
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) return false;
    }
    if (sOrigGameModeDestroyBlock) {
        return reinterpret_cast<bool(*)(void*, const void*, uint8_t)>(sOrigGameModeDestroyBlock)(self, pos, face);
    }
    return false;
}

static void* sOrigGameModeAttack = nullptr;

static bool hooked_gameModeAttack(void* self, void* targetActor) {
    void* player = self ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 0x8) : nullptr;
    if (!player) player = sCurrentLocalPlayer;
    ll::event::PlayerAttackEvent ev(reinterpret_cast<Player*>(player), targetActor);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) return false;
    if (sOrigGameModeAttack) {
        return reinterpret_cast<bool(*)(void*, void*)>(sOrigGameModeAttack)(self, targetActor);
    }
    return false;
}

static void* sOrigGameModeUseItem = nullptr;
static void* sOrigGameModeUseItemOn = nullptr;
static void* sOrigCommandRequestPacketWrite = nullptr;
static void* sOrigLoopbackPacketSenderSend = nullptr;
static void* sOrigServerNetworkHandlerHandle = nullptr;

static bool isCustomItem(
    void* itemStack,
    std::string* outIdentifier = nullptr,
    const ::levilamroid::item::CustomItemDefinition** outDef = nullptr
) {
    if (!itemStack || !isPointerReadable(itemStack, 16)) return false;
    void* p = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(itemStack) + 0x08);
    if (!p || !isPointerReadable(p, 16)) return false;

    void* inner = *reinterpret_cast<void**>(p);
    bool innerValid = (inner && isPointerReadable(inner, 16));

    for (const auto& cItem : ::levilamroid::item::getAllCustomItems()) {
        // Pointer matching
        if (cItem.nativeItemPtr && (cItem.nativeItemPtr == p || (innerValid && cItem.nativeItemPtr == inner))) {
            if (outIdentifier) *outIdentifier = cItem.identifier;
            if (outDef) *outDef = &cItem;
            return true;
        }

        // ID matching at verified Item::mId offset +0x8A
        if (cItem.id != 0) {
            if (isPointerReadable(reinterpret_cast<uint8_t*>(p) + 0x8A, 2)) {
                int16_t id = *reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(p) + 0x8A);
                if (id == cItem.id) {
                    if (outIdentifier) *outIdentifier = cItem.identifier;
                    if (outDef) *outDef = &cItem;
                    return true;
                }
            }
            if (innerValid && isPointerReadable(reinterpret_cast<uint8_t*>(inner) + 0x8A, 2)) {
                int16_t id = *reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(inner) + 0x8A);
                if (id == cItem.id) {
                    if (outIdentifier) *outIdentifier = cItem.identifier;
                    if (outDef) *outDef = &cItem;
                    return true;
                }
            }
        }
    }
    return false;
}

extern "C" LL_API void ll_ui_send_toast(const char* title, const char* message, const char* iconItem, int durationMs);
extern "C" LL_API bool ll_ui_open_screen(const char* screenName);
extern "C" LL_API bool ll_item_give_player(void* player, const char* identifier, int count, int slot);


static void handleMobSpawn(void* /*player*/, void* /*itemStack*/, const void* pos, uint8_t face) {
    ll_ui_send_toast("Cosmic Golem Summoned", "Ancient Celestial Guardian Awakened!", "cosmic_golem_spawn_egg", 3000);
    ll::Logger::getInstance().info("§6[CosmicGolem] §a✦ Cosmic Golem summoned at block face ", (int)face, "! ✦");

    if (pos && isPointerReadable(pos, 12)) {
        int x = *reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(pos) + 0);
        int y = *reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(pos) + 4);
        int z = *reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(pos) + 8);
        if (face == 1) y += 1;
        else if (face == 0) y -= 1;
        else if (face == 2) z -= 1;
        else if (face == 3) z += 1;
        else if (face == 4) x -= 1;
        else if (face == 5) x += 1;
        char cmdBuf[128];
        snprintf(cmdBuf, sizeof(cmdBuf), "/summon iron_golem %d %d %d \"Cosmic Golem\"", x, y, z);
        ll_command_dispatch_client(cmdBuf);
    } else {
        ll_command_dispatch_client("/summon iron_golem ~ ~1 ~ \"Cosmic Golem\"");
    }
    ll_command_dispatch_client("/playsound mob.irongolem.death @a ~ ~ ~ 1.0 1.2");
    ll_command_dispatch_client("/playsound block.anvil.use @a ~ ~ ~ 1.0 0.8");
}

static void handleBlockPlacement(
    [[maybe_unused]] void* self,
    void* player,
    void* itemStack,
    const ::levilamroid::item::CustomItemDefinition& def,
    const void* pos,
    uint8_t face
) {
    ll_ui_send_toast(def.identifier.c_str(), "Block placed!", def.iconName.c_str(), 2500);
    ll::Logger::getInstance().info("Custom block item placed: ", def.identifier);

    uintptr_t mcBase = getMcBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();
    if (mcBase && off.blockItemUseOn && def.nativeItemPtr && itemStack && player && pos) {
        using BlockUseOnFn = bool (*)(void*, void*, void*, const void*, uint8_t, const void*);
        auto blockUseOn = reinterpret_cast<BlockUseOnFn>(mcBase + off.blockItemUseOn);
        float hitVec[3] = {0.5f, 0.5f, 0.5f};
        blockUseOn(def.nativeItemPtr, itemStack, player, pos, face, hitVec);
    } else {
        if (itemStack && isPointerReadable(itemStack, 0x24)) {
            uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
            if (stackBytes[0x22] > 0) stackBytes[0x22]--;
        }
    }
}

static bool hooked_gameModeUseItem(void* self, void* itemStack, uint32_t handSlot) {
    std::string ident;
    const ::levilamroid::item::CustomItemDefinition* def = nullptr;
    if (isCustomItem(itemStack, &ident, &def)) {
        ll::Logger::getInstance().info("Hooked GameMode::useItem intercepted custom item: ", ident);
        void* player = self ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 0x8) : nullptr;
        if (!player) player = sCurrentLocalPlayer;

        // Publish event for plugins to react
        ll::event::PlayerUseItemEvent ev(reinterpret_cast<Player*>(player), itemStack);
        ll::event::EventBus::getInstance().publish(ev);

        // Safely handle custom item usage — NEVER fall through to vanilla GameMode::useItem
        // which crashes on items without native engine C++ components
        if (def && (def->isFood || def->nutrition > 0)) {
            if (itemStack && isPointerReadable(itemStack, 0x24)) {
                uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
                if (stackBytes[0x22] > 0) {
                    stackBytes[0x22]--;
                }
            }
            ll_command_dispatch_client("/effect @s instant_health 1 1");
            ll_command_dispatch_client("/effect @s saturation 5 2");
            ll_command_dispatch_client("/playsound random.burp @a ~ ~ ~ 1.0 1.0");
            ll_command_dispatch_client("/playsound random.eat @a ~ ~ ~ 1.0 1.0");
            ll_item_give_player(player, "levitest:astral_core", 1, -1);
            ll_ui_send_toast(ident.c_str(), "Consumed! Vitality restored (+8 HP) & Astral Core Granted!", def->iconName.c_str(), 2500);
            ll::Logger::getInstance().info("§a[Food] ", ident, " consumed: vitality restored, burp played, core granted");
            return true;
        } else if (ident == "levitest:cosmic_golem_spawn_egg") {
            if (itemStack && isPointerReadable(itemStack, 0x24)) {
                uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
                if (stackBytes[0x22] > 0) {
                    stackBytes[0x22]--;
                }
            }
            handleMobSpawn(player, itemStack, nullptr, 1);
            return true;
        } else if (ident == "levitest:cosmic_blade") {
            return true;
        }
        return true;
    }

    if (sOrigGameModeUseItem) {
        using OrigFn = bool (*)(void*, void*, uint32_t);
        return reinterpret_cast<OrigFn>(sOrigGameModeUseItem)(self, itemStack, handSlot);
    }
    return false;
}

static bool hooked_gameModeUseItemOn(
    void* self,
    void* itemStack,
    const void* pos,
    uint8_t face,
    float a4,
    char a5,
    const void* hitPos,
    const void* block
) {
    std::string ident;
    const ::levilamroid::item::CustomItemDefinition* def = nullptr;
    if (isCustomItem(itemStack, &ident, &def)) {
        ll::Logger::getInstance().info("Hooked GameMode::useItemOn intercepted: ", ident, " on face ", (int)face);
        void* player = self ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 0x8) : nullptr;
        if (!player) player = sCurrentLocalPlayer;

        // Publish event for plugins to react
        ll::event::PlayerUseItemEvent ev(reinterpret_cast<Player*>(player), itemStack);
        ll::event::EventBus::getInstance().publish(ev);

        if (def && (def->isFood || def->nutrition > 0)) {
            if (itemStack && isPointerReadable(itemStack, 0x24)) {
                uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
                if (stackBytes[0x22] > 0) {
                    stackBytes[0x22]--;
                }
            }
            ll_command_dispatch_client("/effect @s instant_health 1 1");
            ll_command_dispatch_client("/effect @s saturation 5 2");
            ll_command_dispatch_client("/playsound random.burp @a ~ ~ ~ 1.0 1.0");
            ll_command_dispatch_client("/playsound random.eat @a ~ ~ ~ 1.0 1.0");
            ll_item_give_player(player, "levitest:astral_core", 1, -1);
            ll_ui_send_toast(ident.c_str(), "Consumed! Vitality restored (+8 HP) & Astral Core Granted!", def->iconName.c_str(), 2500);
            ll::Logger::getInstance().info("§a[Food] ", ident, " consumed: vitality restored, burp played, core granted");
            return true;
        } else if (ident == "levitest:cosmic_golem_spawn_egg") {
            if (itemStack && isPointerReadable(itemStack, 0x24)) {
                uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
                if (stackBytes[0x22] > 0) {
                    stackBytes[0x22]--;
                }
            }
            handleMobSpawn(player, itemStack, pos, face);
            return true;
        } else if (def && def->isBlock) {
            handleBlockPlacement(self, player, itemStack, *def, pos, face);
            return true;
        }
        return true;
    }

    if (sOrigGameModeUseItemOn) {
        return reinterpret_cast<bool(*)(void*, void*, const void*, uint8_t, float, char, const void*, const void*)>(
            sOrigGameModeUseItemOn)(self, itemStack, pos, face, a4, a5, hitPos, block);
    }
    return false;
}

static void hooked_serverNetworkHandlerHandle(void* self, void* netId, void* textPacket) {
    if (textPacket && isPointerReadable(textPacket, 96)) {
        for (size_t off = 0x18; off <= 0x60; off += sizeof(void*)) {
            const std::string* str = reinterpret_cast<const std::string*>(reinterpret_cast<const uint8_t*>(textPacket) + off);
            if (isPointerReadable(str, sizeof(std::string)) && str->length() > 0 && str->length() < 256) {
                const char* data = str->c_str();
                if (data && (data[0] == '/' || data[0] == '!')) {
                    std::string cmd = data;
                    ll::Logger::getInstance().info("ServerNetworkHandler intercepted command: ", cmd);
                    if (ll_command_dispatch_client(cmd.c_str())) {
                        return; // Successfully dispatched custom command
                    }
                }
            }
        }
    }
    if (sOrigServerNetworkHandlerHandle) {
        reinterpret_cast<void(*)(void*, void*, void*)>(sOrigServerNetworkHandlerHandle)(self, netId, textPacket);
    }
}

static void hooked_commandRegistryRegisterCommand(
    void* self,
    const std::string& name,
    const char* desc,
    int perm,
    int flag1,
    int flag2
) {
    if (self && !sEngineCommandRegistry) {
        sEngineCommandRegistry = self;
        ll::Logger::getInstance().info("Captured Engine CommandRegistry: ", self);
        // Forward to CommandHooks so it can flush pending custom commands
        ::levilamroid::command::flushPendingCommands(self);
    }
    if (sOrigCommandRegistryRegisterCommand) {
        reinterpret_cast<void(*)(void*, const std::string&, const char*, int, int, int)>(
            sOrigCommandRegistryRegisterCommand)(self, name, desc, perm, flag1, flag2);
    }
}

static void hooked_commandRequestPacketWrite(void* self, void* stream) {
    if (self) {
        std::string* cmdStr = reinterpret_cast<std::string*>(reinterpret_cast<uint8_t*>(self) + 0x30);
        if (cmdStr && isPointerReadable(cmdStr, sizeof(std::string)) && !cmdStr->empty()) {
            std::string cmd = *cmdStr;
            ll::Logger::getInstance().info("CommandRequestPacket write: ", cmd);
            if (cmd.rfind("/levi", 0) == 0 || cmd.rfind("levi", 0) == 0) {
                ll_command_dispatch_client(cmd.c_str());
            } else if (cmd.find("particle sparkle") != std::string::npos) {
                ll_command_dispatch_client("levi sparkle");
            }
        }
    }
    if (sOrigCommandRequestPacketWrite) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigCommandRequestPacketWrite)(self, stream);
    }
}

static void hooked_loopbackPacketSenderSend(void* self, void* packet) {
    if (sOrigLoopbackPacketSenderSend) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigLoopbackPacketSenderSend)(self, packet);
    }
}

static bool installGameplayHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.clientInstanceUpdate) {
        if (ll::memory::hookRva(off.clientInstanceUpdate, (void*)&hooked_clientInstanceUpdate, &sOrigClientInstanceUpdate)) {
            logger.info("Hooked ClientInstance::update [RVA: 0x", std::hex, off.clientInstanceUpdate, "]");
        }
    }

    if (off.playerNormalTick) {
        if (ll::memory::hookRva(off.playerNormalTick, (void*)&hooked_playerNormalTick, &sOrigPlayerNormalTick)) {
            logger.info("Hooked Player::normalTick [RVA: 0x", std::hex, off.playerNormalTick, "]");
        }
    }

    if (off.localPlayerApplyTurnDelta) {
        if (ll::memory::hookRva(off.localPlayerApplyTurnDelta, (void*)&hooked_applyTurnDelta, &sOrigApplyTurnDelta)) {
            logger.info("Hooked LocalPlayer::applyTurnDelta [RVA: 0x", std::hex, off.localPlayerApplyTurnDelta, "]");
        }
    }

    if (off.gameModeUseItem) {
        if (ll::memory::hookRva(off.gameModeUseItem, (void*)&hooked_gameModeUseItem, &sOrigGameModeUseItem)) {
            logger.info("Hooked GameMode::useItem [RVA: 0x", std::hex, off.gameModeUseItem, "]");
        }
    }

    if (off.gameModeUseItemOn) {
        if (ll::memory::hookRva(off.gameModeUseItemOn, (void*)&hooked_gameModeUseItemOn, &sOrigGameModeUseItemOn)) {
            logger.info("Hooked GameMode::useItemOn [RVA: 0x", std::hex, off.gameModeUseItemOn, "]");
        }
    }

    if (off.commandRegistryRegisterCommand) {
        if (ll::memory::hookRva(off.commandRegistryRegisterCommand, (void*)&hooked_commandRegistryRegisterCommand, &sOrigCommandRegistryRegisterCommand)) {
            logger.info("Hooked CommandRegistry::registerCommand [RVA: 0x", std::hex, off.commandRegistryRegisterCommand, "]");
        }
    }

    if (off.serverNetworkHandlerHandle) {
        if (ll::memory::hookRva(off.serverNetworkHandlerHandle, (void*)&hooked_serverNetworkHandlerHandle, &sOrigServerNetworkHandlerHandle)) {
            logger.info("Hooked ServerNetworkHandler::handle [RVA: 0x", std::hex, off.serverNetworkHandlerHandle, "]");
        }
    }

    if (off.commandRequestPacketWrite) {
        if (ll::memory::hookRva(off.commandRequestPacketWrite, (void*)&hooked_commandRequestPacketWrite, &sOrigCommandRequestPacketWrite)) {
            logger.info("Hooked CommandRequestPacket::write [RVA: 0x", std::hex, off.commandRequestPacketWrite, "]");
        }
    }

    if (off.loopbackPacketSenderSend) {
        if (ll::memory::hookRva(off.loopbackPacketSenderSend, (void*)&hooked_loopbackPacketSenderSend, &sOrigLoopbackPacketSenderSend)) {
            logger.info("Hooked LoopbackPacketSender::send [RVA: 0x", std::hex, off.loopbackPacketSenderSend, "]");
        }
    }

    if (off.clientInstanceRequestLeaveGame) {
        if (ll::memory::hookRva(off.clientInstanceRequestLeaveGame, (void*)&hooked_clientInstanceRequestLeaveGame, &sOrigClientInstanceRequestLeaveGame)) {
            logger.info("Hooked ClientInstance::requestLeaveGame [RVA: 0x", std::hex, off.clientInstanceRequestLeaveGame, "]");
        }
    }

    if (off.minecraftGameClientUpdate) {
        if (ll::memory::hookRva(off.minecraftGameClientUpdate, (void*)&hooked_minecraftGameClientUpdate, &sOrigMinecraftGameClientUpdate)) {
            logger.info("Hooked MinecraftGame::clientUpdate [RVA: 0x", std::hex, off.minecraftGameClientUpdate, "]");
        }
    }

    if (off.localPlayerMove) {
        if (ll::memory::hookRva(off.localPlayerMove, (void*)&hooked_localPlayerMove, &sOrigLocalPlayerMove)) {
            logger.info("Hooked LocalPlayer::move [RVA: 0x", std::hex, off.localPlayerMove, "]");
        }
    }

    if (off.gameModeDestroyBlock) {
        if (ll::memory::hookRva(off.gameModeDestroyBlock, (void*)&hooked_gameModeDestroyBlock, &sOrigGameModeDestroyBlock)) {
            logger.info("Hooked GameMode::destroyBlock [RVA: 0x", std::hex, off.gameModeDestroyBlock, "]");
        }
    }

    if (off.gameModeAttack) {
        if (ll::memory::hookRva(off.gameModeAttack, (void*)&hooked_gameModeAttack, &sOrigGameModeAttack)) {
            logger.info("Hooked GameMode::attack [RVA: 0x", std::hex, off.gameModeAttack, "]");
        }
    }

    logger.info("LeviLamroid gameplay subsystem active.");
    return true;
}

bool installAllHooks() {
    auto& logger = ll::Logger::getInstance();
    logger.info("Installing LeviLamroid Core Engine Hooks...");
    bool ok = true;
    ok = installUIHooks() && ok;
    ok = installRenderHooks() && ok;
    ok = installGameplayHooks() && ok;
    ok = installResourceHooks() && ok;
    ok = ::levilamroid::item::installItemHooks() && ok;
    ok = ::levilamroid::block::installBlockHooks() && ok;
    ok = ::levilamroid::recipe::installRecipeHooks() && ok;
    ok = ::levilamroid::command::installCommandHooks() && ok;
    logger.info("All LeviLamroid core engine hooks active.");
    return ok;
}

} // namespace levilamroid::hooks

extern "C" {

LL_API bool ll_is_in_world() {
    return ::levilamroid::hooks::isInWorld();
}

LL_API bool ll_is_main_menu() {
    return ::levilamroid::hooks::isMainMenu();
}

LL_API bool ll_is_loading() {
    return ::levilamroid::hooks::isLoading();
}

LL_API void* ll_service_get_local_player() {
    return ::levilamroid::hooks::sCurrentLocalPlayer;
}

LL_API void* ll_service_get_client_instance() {
    return ::levilamroid::hooks::sCurrentClientInstance;
}

LL_API void* ll_service_get_level() {
    return ::levilamroid::hooks::sCurrentLevel;
}

LL_API void ll_camera_set_locked(bool locked) {
    ::levilamroid::hooks::sCameraLocked.store(locked);
}

LL_API bool ll_camera_is_locked() {
    return ::levilamroid::hooks::sCameraLocked.load();
}

LL_API void ll_command_register_native(const char* name, const char* desc) {
    if (!name) return;
    if (::levilamroid::hooks::sEngineCommandRegistry && ::levilamroid::hooks::sOrigCommandRegistryRegisterCommand) {
        std::string n = name;
        const char* d = desc ? desc : "";
        reinterpret_cast<void(*)(void*, const std::string&, const char*, int, int, int)>(
            ::levilamroid::hooks::sOrigCommandRegistryRegisterCommand)(
            ::levilamroid::hooks::sEngineCommandRegistry, n, d, 0, 0, 0);
    }
}

LL_API void* ll_command_get_engine_registry() {
    return ::levilamroid::hooks::sEngineCommandRegistry;
}

} // extern "C"

namespace ll::hooks {

LL_API bool installAllHooks() {
    return ::levilamroid::hooks::installAllHooks();
}

} // namespace ll::hooks
