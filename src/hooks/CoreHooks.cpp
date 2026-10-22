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
bool dispatchCommand(const std::string& rawInput);
bool hasCommand(const std::string& rawInput);
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
void* sOrigCommandRegistryRegisterCommand = nullptr;
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

extern "C" LL_API bool ll_command_dispatch_client(const char* fullCommand) {
    if (!fullCommand) return false;

    // 1. Check levilamroid::command dispatchCommand
    if (::levilamroid::command::dispatchCommand(fullCommand)) {
        return true;
    }

    std::string cmd = fullCommand;
    while (!cmd.empty() && (cmd.front() == '/' || cmd.front() == '!' || std::isspace(static_cast<unsigned char>(cmd.front())))) {
        cmd.erase(0, 1);
    }

    auto spacePos = cmd.find(' ');
    std::string baseCmd = (spacePos == std::string::npos) ? cmd : cmd.substr(0, spacePos);
    std::string args = (spacePos == std::string::npos) ? "" : cmd.substr(spacePos + 1);

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

static inline bool isPointerReadable(const void* ptr, size_t size = sizeof(void*)) {
    if (!ptr || size == 0) return false;
    uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
    if (v < 0x10000 || v > 0x7fffffffff) return false;
    if (v + size < v) return false;
    return true;
}

static std::vector<std::string> sPendingMainThreadCommands;
static std::mutex sPendingCmdMutex;

void queueMainThreadCommand(const std::string& cmd) {
    std::lock_guard<std::mutex> lock(sPendingCmdMutex);
    sPendingMainThreadCommands.push_back(cmd);
}

void executePendingMainThreadCommands(void* player) {
    std::vector<std::string> toExec;
    {
        std::lock_guard<std::mutex> lock(sPendingCmdMutex);
        if (sPendingMainThreadCommands.empty()) return;
        toExec.swap(sPendingMainThreadCommands);
    }
    for (const auto& cmd : toExec) {
        ll::Logger::getInstance().info("§6[MainThread] §aExecuting queued client command: §e", cmd);
        ll_command_dispatch_client(cmd.c_str());
    }
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
    executePendingMainThreadCommands(self);

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
static void* sOrigLoopbackPacketSenderSend = nullptr;
static void* sOrigLoopbackPacketSenderSendToServer = nullptr;
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
    if (!pos || !isPointerReadable(pos, 12)) return;

    const int* p = reinterpret_cast<const int*>(pos);
    int x = p[0];
    int y = p[1];
    int z = p[2];
    switch (face) {
        case 0: y--; break; // down
        case 1: y++; break; // up
        case 2: z--; break; // north
        case 3: z++; break; // south
        case 4: x--; break; // west
        case 5: x++; break; // east
        default: y++; break;
    }

    uintptr_t mcBase = getMcBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();
    bool placed = false;

    if (mcBase && off.blockItemUseOn && def.nativeItemPtr && itemStack && player) {
        try {
            using BlockUseOnFn = bool (*)(void*, void*, void*, const void*, uint8_t, const void*);
            auto blockUseOn = reinterpret_cast<BlockUseOnFn>(mcBase + off.blockItemUseOn);
            float hitVec[3] = {0.5f, 0.5f, 0.5f};
            placed = blockUseOn(def.nativeItemPtr, itemStack, player, pos, face, hitVec);
        } catch (...) {
            placed = false;
        }
    }

    if (!placed) {
        char cmd[128];
        std::snprintf(cmd, sizeof(cmd), "/setblock %d %d %d %s", x, y, z, def.identifier.c_str());
        ll_command_dispatch_client(cmd);
        ll_command_dispatch_client("/playsound dig.stone @a ~ ~ ~ 0.8 1.0");

        if (itemStack && isPointerReadable(itemStack, 0x24)) {
            uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
            if (stackBytes[0x22] > 0) stackBytes[0x22]--;
        }
    }

    ll_ui_send_toast(def.identifier.c_str(), "Block placed!", def.iconName.c_str(), 2000);
    ll::Logger::getInstance().info("§6[LeviLamroid] §aCustom block placed at (", x, ", ", y, ", ", z, "): ", def.identifier);
}

static bool hooked_gameModeUseItem(void* self, void* itemStack, uint32_t handSlot) {
    std::string ident;
    const ::levilamroid::item::CustomItemDefinition* def = nullptr;
    if (isCustomItem(itemStack, &ident, &def) && def) {
        ll::Logger::getInstance().info("Hooked GameMode::useItem intercepted custom item: ", ident);
        void* player = self ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 0x8) : nullptr;
        if (!player) player = sCurrentLocalPlayer;

        // Publish event for plugins to react
        ll::event::PlayerUseItemEvent ev(reinterpret_cast<Player*>(player), itemStack);
        ll::event::EventBus::getInstance().publish(ev);

        // Handle food consumption if this item has nutrition or is marked food
        if (def->isFood || def->nutrition > 0) {
            static auto sLastEatTime = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - sLastEatTime).count();
            if (elapsed > 400) {
                sLastEatTime = now;
                ll_command_dispatch_client("/playsound random.eat @a ~ ~ ~ 0.8 1.0");
                ll_command_dispatch_client("/playsound random.burp @a ~ ~ ~ 0.6 1.0");

                int dur = std::max(1, def->nutrition / 2);
                char effectCmd[128];
                std::snprintf(effectCmd, sizeof(effectCmd), "/effect @s saturation %d 1 true", dur);
                ll_command_dispatch_client(effectCmd);

                char msg[64];
                std::snprintf(msg, sizeof(msg), "+%d Hunger", def->nutrition);
                ll_ui_send_toast(def->identifier.c_str(), msg, def->iconName.c_str(), 1800);

                if (itemStack && isPointerReadable(itemStack, 0x24)) {
                    uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
                    if (stackBytes[0x22] > 0) stackBytes[0x22]--;
                }
            }
            return true;
        }
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
    if (isCustomItem(itemStack, &ident, &def) && def) {
        void* player = self ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 0x8) : nullptr;
        if (!player) player = sCurrentLocalPlayer;

        // Publish event for plugins to react
        ll::event::PlayerUseItemEvent ev(reinterpret_cast<Player*>(player), itemStack);
        ll::event::EventBus::getInstance().publish(ev);

        if (def->isBlock) {
            handleBlockPlacement(self, player, itemStack, *def, pos, face);
            return true;
        }
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
    bool enabled,
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
        reinterpret_cast<void(*)(void*, const std::string&, const char*, bool, int, int, int)>(
            sOrigCommandRegistryRegisterCommand)(self, name, desc, enabled, perm, flag1, flag2);
    }
}


static bool handleCommandPacket(void* packet) {
    if (!packet || !isPointerReadable(packet, 0x48)) return false;
    void** vtable = *reinterpret_cast<void***>(packet);
    if (!vtable || !isPointerReadable(vtable, 24)) return false;

    using GetIdFn = int (*)(void*);
    GetIdFn getId = reinterpret_cast<GetIdFn>(vtable[2]);
    if (!getId || !isPointerReadable(reinterpret_cast<void*>(getId), 4)) return false;

    int packetId = 0;
    try {
        packetId = getId(packet);
    } catch (...) {
        return false;
    }

    if (packetId == 0x4D) { // 77 = CommandRequestPacket
        const std::string* cmdStr = reinterpret_cast<const std::string*>(
            reinterpret_cast<const uint8_t*>(packet) + 0x30
        );
        if (isPointerReadable(cmdStr, sizeof(std::string)) && !cmdStr->empty() && cmdStr->size() < 1024) {
            std::string cmd = *cmdStr;
            std::string clean = cmd;
            while (!clean.empty() && std::isspace(static_cast<unsigned char>(clean.front()))) clean.erase(0, 1);
            if (!clean.empty() && clean.front() == '/') {
                std::string stripped = clean.substr(1);
                auto space = stripped.find(' ');
                std::string base = (space != std::string::npos) ? stripped.substr(0, space) : stripped;
                // Check if this command belongs to our mods
                if (::levilamroid::command::hasCommand(clean) ||
                    base == "fd" || base == "knife" || base == "pot" || base == "board" ||
                    base == "stove" || base == "food" || base == "fdkit" ||
                    base == "record" || base == "playback" ||
                    base == "adv" || base == "advancements" || base == "farmersdelight") {
                    ll::Logger::getInstance().info("§6[LoopbackPacketSender] §bIntercepted CommandRequestPacket; queueing to main thread: §e", clean);
                    queueMainThreadCommand(clean);
                    return true; // Dropped! Do not forward to server
                }
            }
        }
    }
    return false;
}

static void hooked_loopbackPacketSenderSend(void* self, void* packet) {
    if (handleCommandPacket(packet)) return;
    if (sOrigLoopbackPacketSenderSend) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigLoopbackPacketSenderSend)(self, packet);
    }
}

static void hooked_loopbackPacketSenderSendToServer(void* self, void* packet) {
    if (handleCommandPacket(packet)) return;
    if (sOrigLoopbackPacketSenderSendToServer) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigLoopbackPacketSenderSendToServer)(self, packet);
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

    if (off.loopbackPacketSenderSend) {
        if (ll::memory::hookRva(off.loopbackPacketSenderSend, (void*)&hooked_loopbackPacketSenderSend, &sOrigLoopbackPacketSenderSend)) {
            logger.info("Hooked LoopbackPacketSender::send [RVA: 0x", std::hex, off.loopbackPacketSenderSend, "]");
        }
    }

    if (off.loopbackPacketSenderSendToServer) {
        if (ll::memory::hookRva(off.loopbackPacketSenderSendToServer, (void*)&hooked_loopbackPacketSenderSendToServer, &sOrigLoopbackPacketSenderSendToServer)) {
            logger.info("Hooked LoopbackPacketSender::sendToServer [RVA: 0x", std::hex, off.loopbackPacketSenderSendToServer, "]");
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
        reinterpret_cast<void(*)(void*, const std::string&, const char*, bool, int, int, int)>(
            ::levilamroid::hooks::sOrigCommandRegistryRegisterCommand)(
            ::levilamroid::hooks::sEngineCommandRegistry, n, d, false, 0, 0, 0);
    }
}

LL_API void* ll_command_get_engine_registry() {
    return ::levilamroid::hooks::sEngineCommandRegistry;
}

LL_API void* ll_block_source_get_block(void* blockSource, int x, int y, int z) {
    if (!blockSource) return nullptr;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ::levilamroid::hooks::getMcBase();
    if (!mcBase || !off.blockSourceGetBlock) return nullptr;
    using GetBlockFn = void* (*)(void*, int, int, int);
    auto fn = reinterpret_cast<GetBlockFn>(mcBase + off.blockSourceGetBlock);
    return fn(blockSource, x, y, z);
}

LL_API bool ll_block_source_set_block(void* blockSource, int x, int y, int z, void* blockPermutation, int updateFlags) {
    if (!blockSource || !blockPermutation) return false;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ::levilamroid::hooks::getMcBase();
    if (!mcBase || !off.blockSourceSetBlock) return false;
    using SetBlockFn = bool (*)(void*, int, int, int, void*, int);
    auto fn = reinterpret_cast<SetBlockFn>(mcBase + off.blockSourceSetBlock);
    return fn(blockSource, x, y, z, blockPermutation, updateFlags);
}

LL_API int ll_block_source_get_brightness(void* blockSource, int x, int y, int z) {
    if (!blockSource) return 0;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ::levilamroid::hooks::getMcBase();
    if (!mcBase || !off.blockSourceGetBrightness) return 0;
    using GetBrightFn = int (*)(void*, int, int, int);
    auto fn = reinterpret_cast<GetBrightFn>(mcBase + off.blockSourceGetBrightness);
    return fn(blockSource, x, y, z);
}

LL_API int ll_item_stack_get_count(const void* itemStack) {
    if (!itemStack) return 0;
    return static_cast<int>(*reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(itemStack) + 0x22));
}

LL_API bool ll_item_stack_set_count(void* itemStack, uint8_t count) {
    if (!itemStack) return false;
    *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemStack) + 0x22) = count;
    return true;
}

LL_API short ll_item_stack_get_aux(const void* itemStack) {
    if (!itemStack) return 0;
    return *reinterpret_cast<const short*>(reinterpret_cast<const uint8_t*>(itemStack) + 0x20);
}

LL_API bool ll_item_stack_is_valid(const void* itemStack) {
    if (!itemStack) return false;
    return (*reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(itemStack) + 0x23) != 0);
}

LL_API void* ll_item_stack_get_item(const void* itemStack) {
    if (!itemStack) return nullptr;
    void* ctrl = *reinterpret_cast<void* const*>(reinterpret_cast<const uint8_t*>(itemStack) + 0x08);
    if (!ctrl) return nullptr;
    return *reinterpret_cast<void**>(ctrl);
}

} // extern "C"

namespace ll::hooks {

LL_API bool installAllHooks() {
    return ::levilamroid::hooks::installAllHooks();
}

} // namespace ll::hooks
