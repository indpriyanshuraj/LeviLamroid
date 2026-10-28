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
#include "levilamroid/api/block/BlockTypeRegistry.h"
#include "levilamroid/api/entity/EntityRegistry.h"
#include "levilamroid/api/nbt/NbtBuilder.h"
#include "mc/network/packet/TextPacket.h"
#include "mc/network/packet/StartGamePacket.h"
#include "mc/network/packet/ItemRegistryPacket.h"
#include "mc/network/packet/AvailableActorIdentifiersPacket.h"
#include "mc/network/packet/CreativeContentPacket.h"
#include "mc/network/packet/CreativeItemEntryPayload.h"
#include "mc/network/packet/AvailableCommandsPacket.h"
#include "mc/network/packet/AvailableCommandsPacketPayload.h"
#include "mc/world/level/block/definition/ServerBlockProperty.h"
#include "pl/Input.hpp"
#include <atomic>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

typedef bool (*LLPacketListenerCallback)(int packetId, void* packet, bool isClientBound, void* udata);

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
std::vector<std::pair<std::string, std::string>> getAllCustomCommands();
}

extern "C" {
LL_API void* ll_dimension_get_blocksource(void* dimension);
LL_API bool ll_block_source_set_block(void* blockSource, int x, int y, int z, void* blockPermutation, int updateFlags);
}

namespace levilamroid::hooks {

[[maybe_unused]] static uintptr_t getMcBase() {
    return ::ll::memory::getMinecraftBase();
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

// ── Generic Mod Callback System ──────────────────────────────────────────
struct BlockInteractListener {
    LLBlockInteractCallback cb;
    void* udata;
};
static std::vector<BlockInteractListener> sBlockInteractListeners;
static std::mutex sBlockInteractMutex;

struct BlockDestroyListener {
    LLBlockDestroyCallback cb;
    void* udata;
};
static std::vector<BlockDestroyListener> sBlockDestroyListeners;
static std::mutex sBlockDestroyMutex;

struct PlayerAttackListener {
    LLPlayerAttackCallback cb;
    void* udata;
};
static std::vector<PlayerAttackListener> sPlayerAttackListeners;
static std::mutex sPlayerAttackMutex;

struct ItemUseListener {
    LLItemUseCallback cb;
    void* udata;
};
static std::vector<ItemUseListener> sItemUseListeners;
static std::mutex sItemUseMutex;

extern "C" {
LL_API bool ll_block_register_interact_listener(LLBlockInteractCallback callback, void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(sBlockInteractMutex);
    sBlockInteractListeners.push_back({callback, udata});
    return true;
}

LL_API bool ll_block_register_destroy_listener(LLBlockDestroyCallback callback, void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(sBlockDestroyMutex);
    sBlockDestroyListeners.push_back({callback, udata});
    return true;
}

LL_API bool ll_player_register_attack_listener(LLPlayerAttackCallback callback, void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(sPlayerAttackMutex);
    sPlayerAttackListeners.push_back({callback, udata});
    return true;
}

LL_API bool ll_item_register_use_listener(LLItemUseCallback callback, void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(sItemUseMutex);
    sItemUseListeners.push_back({callback, udata});
    return true;
}
}

static bool dispatchBlockInteractListeners(void* player, int x, int y, int z, uint8_t face, const char* blockIdent, const char* heldItemIdent) {
    std::vector<BlockInteractListener> copies;
    {
        std::lock_guard<std::mutex> lock(sBlockInteractMutex);
        copies = sBlockInteractListeners;
    }
    for (const auto& l : copies) {
        if (l.cb && l.cb(player, x, y, z, face, blockIdent, heldItemIdent, l.udata)) return true;
    }
    return false;
}

static bool dispatchBlockDestroyListeners(void* player, int x, int y, int z, const char* blockIdent, const char* heldItemIdent) {
    std::vector<BlockDestroyListener> copies;
    {
        std::lock_guard<std::mutex> lock(sBlockDestroyMutex);
        copies = sBlockDestroyListeners;
    }
    for (const auto& l : copies) {
        if (l.cb && l.cb(player, x, y, z, blockIdent, heldItemIdent, l.udata)) return true;
    }
    return false;
}

static void dispatchPlayerAttackListeners(void* player, void* targetActor, const char* heldItemIdent) {
    std::vector<PlayerAttackListener> copies;
    {
        std::lock_guard<std::mutex> lock(sPlayerAttackMutex);
        copies = sPlayerAttackListeners;
    }
    for (const auto& l : copies) {
        if (l.cb) l.cb(player, targetActor, heldItemIdent, l.udata);
    }
}

static bool dispatchItemUseListeners(void* player, const char* itemIdent, void* itemStack) {
    std::vector<ItemUseListener> copies;
    {
        std::lock_guard<std::mutex> lock(sItemUseMutex);
        copies = sItemUseListeners;
    }
    for (const auto& l : copies) {
        if (l.cb && l.cb(player, itemIdent, itemStack, l.udata)) return true;
    }
    return false;
}

static void* sServerNetworkHandler = nullptr;
static uint8_t sServerNetIdMem[256] = {0};
static std::atomic<bool> sHasServerNetId = false;
static void* sOrigServerNetworkHandlerHandle = nullptr;
static thread_local bool sInServerNetworkHandlerHandle = false;
static void* sLoopbackPacketSender = nullptr;
static void* sOrigLoopbackPacketSenderSend = nullptr;
static void* sOrigLoopbackPacketSenderSendToServer = nullptr;

bool executeServerCommand(const std::string& rawCmd) {
    if (sInServerNetworkHandlerHandle) return false;
    std::string fullCmd = rawCmd;
    while (!fullCmd.empty() && std::isspace(static_cast<unsigned char>(fullCmd.front()))) {
        fullCmd.erase(0, 1);
    }
    if (fullCmd.empty()) return false;
    if (fullCmd[0] != '/') {
        fullCmd = "/" + fullCmd;
    }

    if (sLoopbackPacketSender) {
        try {
            TextPacket pkt = TextPacket::createChatPacket("", fullCmd);
            if (sOrigLoopbackPacketSenderSendToServer) {
                reinterpret_cast<void(*)(void*, void*)>(sOrigLoopbackPacketSenderSendToServer)(sLoopbackPacketSender, &pkt);
                if (fullCmd.rfind("/particle", 0) != 0) {
                    ll::Logger::getInstance().info("§6[LoopbackPacketSenderSendToServer] §aDispatched server command: §e", fullCmd);
                }
                return true;
            } else if (sOrigLoopbackPacketSenderSend) {
                reinterpret_cast<void(*)(void*, void*)>(sOrigLoopbackPacketSenderSend)(sLoopbackPacketSender, &pkt);
                if (fullCmd.rfind("/particle", 0) != 0) {
                    ll::Logger::getInstance().info("§6[LoopbackPacketSender] §aDispatched server command: §e", fullCmd);
                }
                return true;
            }
        } catch (...) {}
    }

    if (sServerNetworkHandler && sHasServerNetId.load() && sOrigServerNetworkHandlerHandle) {
        try {
            auto sp = std::make_shared<TextPacket>(TextPacket::createChatPacket("", fullCmd));
            reinterpret_cast<void(*)(void*, const void*, const void*)>(sOrigServerNetworkHandlerHandle)(
                sServerNetworkHandler,
                sServerNetIdMem,
                &sp
            );
            ll::Logger::getInstance().info("§6[ServerNetworkHandler] §aExecuted server command: §e", fullCmd);
            return true;
        } catch (...) {
            ll::Logger::getInstance().warn("§6[ServerNetworkHandler] §cException executing server command: §e", fullCmd);
        }
    }

    return false;
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

    // 3. Fallback to server execution for engine commands (/setblock, /playsound, /give, /effect, etc.)
    return executeServerCommand(fullCommand);
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
        sServerNetworkHandler = nullptr;
        sHasServerNetId.store(false);
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
    uintptr_t untagged = v & 0x00ffffffffffffffULL;
    if (untagged < 0x10000 || untagged > 0x00007fffffffffffULL) return false;
    if (untagged + size < untagged) return false;
    return true;
}

static std::vector<std::string> sPendingMainThreadCommands;
static std::mutex sPendingCmdMutex;

static short sLastTouchX = 0;
static short sLastTouchY = 0;
static bool sIsTouchDown = false;
static std::chrono::steady_clock::time_point sTouchDownTime;
static bool sHasTriggeredHoldAction = false;

void queueMainThreadCommand(const std::string& cmd) {
    std::lock_guard<std::mutex> lock(sPendingCmdMutex);
    sPendingMainThreadCommands.push_back(cmd);
}

void executePendingMainThreadCommands([[maybe_unused]] void* player) {
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
    if (sOrigClientInstanceUpdate) {
        reinterpret_cast<void(*)(void*)>(sOrigClientInstanceUpdate)(self);
    }

    sCurrentClientInstance = self;
    static bool sFirstMenuOpen = false;
    if (!sFirstMenuOpen && !sIsInWorld.load()) {
        sFirstMenuOpen = true;
        sIsMainMenu.store(true);
        sIsLoading.store(false);
        dispatchStartMenuOpen();
    }

    if (sIsInWorld.load() && sCurrentLocalPlayer) {
        executePendingMainThreadCommands(sCurrentLocalPlayer);
    }
}

static void* sOrigClientInstanceRequestLeaveGame = nullptr;

static void hooked_clientInstanceRequestLeaveGame(void* self, bool b1, bool b2) {
    ll::Logger::getInstance().info("ClientInstance::requestLeaveGame triggered.");
    setInWorld(false);
    setIsMainMenu(true);
    sCurrentLocalPlayer = nullptr;
    sCurrentLevel = nullptr;
    sServerNetworkHandler = nullptr;
    sHasServerNetId.store(false);
    sIsTouchDown = false;
    sHasTriggeredHoldAction = false;

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

// Forward declarations for player tick processing
static bool isPlayerHoldingCustomItem(void* player, std::string* outIdent, const ::levilamroid::item::CustomItemDefinition** outDef, void** outStack);
static bool handleFoodConsumption(void* player, void* itemStack, const ::levilamroid::item::CustomItemDefinition* def);
static bool dispatchItemUseListeners(void* player, const char* itemIdent, void* itemStack);

extern "C" bool fabric_is_fullbright();

static void hooked_playerNormalTick(void* self) {
    sCurrentLocalPlayer = self;
    executePendingMainThreadCommands(self);

    if (self && isPointerReadable(reinterpret_cast<uint8_t*>(self) + 472, sizeof(void*))) {
        sCurrentLevel = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 472);
    }

    bool wasInWorld = sIsInWorld.exchange(true);
    if (!wasInWorld) {
        setInWorld(true);
        ll::Logger::getInstance().info("§6[LeviLamroid] §aLocalPlayer entered world simulation! (Player: ", self, ")");
        ll::event::WorldJoinEvent joinEv;
        ll::event::EventBus::getInstance().publish(joinEv);
        if (fabric_is_fullbright()) {
            ll_command_dispatch_client("effect @s night_vision 999999 0 true");
        }
    }

    // Authentic continuous item use & hold interaction (Food eating, Doctor Strange Mandala, custom usable items)
    if (sIsTouchDown && self) {
        std::string heldIdent;
        const ::levilamroid::item::CustomItemDefinition* heldDef = nullptr;
        void* heldStack = nullptr;
        if (isPlayerHoldingCustomItem(self, &heldIdent, &heldDef, &heldStack) && heldDef) {
            auto now = std::chrono::steady_clock::now();
            auto holdMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - sTouchDownTime).count();

            // 1. Food Consumption: authentic continuous chewing and hunger replenishment
            if (!heldDef->isBlock && (heldDef->isFood || heldDef->nutrition > 0)) {
                static auto sLastChewTime = now;
                auto chewElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - sLastChewTime).count();
                if (chewElapsed >= 320) {
                    sLastChewTime = now;
                    ll_command_dispatch_client("/playsound random.eat @a ~ ~ ~ 0.7 1.1");
                    ll_command_dispatch_client("/particle minecraft:crop_growth_area_emitter ~ ~1.3 ~");
                }
                if (holdMs >= 1500 && !sHasTriggeredHoldAction) {
                    sHasTriggeredHoldAction = true;
                    handleFoodConsumption(self, heldStack, heldDef);
                }
            }
            // 2. Custom weapons & tools (attackDamage > 0): Ability trigger on hold (>= 500ms), NEVER depletes count!
            else if (!heldDef->isBlock && heldDef->attackDamage > 0.0f) {
                if (!sHasTriggeredHoldAction && holdMs >= 500) {
                    sHasTriggeredHoldAction = true;
                    dispatchItemUseListeners(self, heldIdent.c_str(), heldStack);
                }
            }
            // 3. Custom usable / channeled items: Ability trigger on hold (>= 150ms)
            else if (!heldDef->isBlock) {
                if (!sHasTriggeredHoldAction && holdMs >= 150) {
                    sHasTriggeredHoldAction = true;
                    dispatchItemUseListeners(self, heldIdent.c_str(), heldStack);
                }
            }
        }
    }

    // Publish canonical PlayerTickEvent
    ll::event::PlayerTickEvent ev(reinterpret_cast<Player*>(self));
    ll::event::EventBus::getInstance().publish(ev);

    if (sOrigPlayerNormalTick) {
        reinterpret_cast<void(*)(void*)>(sOrigPlayerNormalTick)(self);
    }
}

static void* sOrigLocalPlayerMove = nullptr;

static void hooked_localPlayerMove(void* self, const void* delta) {
    if (self && isPointerReadable(self, 0x580)) {
        sCurrentLocalPlayer = self;
        if (isPointerReadable(reinterpret_cast<uint8_t*>(self) + 472, sizeof(void*))) {
            sCurrentLevel = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 472);
        }
        bool wasInWorld = sIsInWorld.exchange(true);
        if (!wasInWorld) {
            setInWorld(true);
            ll::Logger::getInstance().info("§6[LeviLamroid] §aLocalPlayer entered world simulation! (Player: ", self, ")");
            ll::event::WorldJoinEvent joinEv;
            ll::event::EventBus::getInstance().publish(joinEv);
        }

        executePendingMainThreadCommands(self);

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

extern "C" LL_API void ll_ui_send_toast(const char* title, const char* message, const char* iconItem, int durationMs);

static void* sOrigGameModeUseItem = nullptr;
static void* sOrigGameModeUseItemOn = nullptr;

static bool isCustomItem(
    void* itemStack,
    std::string* outIdentifier = nullptr,
    const ::levilamroid::item::CustomItemDefinition** outDef = nullptr
) {
    if (!itemStack || !isPointerReadable(itemStack, 0x24)) return false;

    // Check count and validity of item stack (offset 0x22 = count, 0x23 = valid byte)
    uint8_t count = *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(itemStack) + 0x22);
    if (count == 0) return false;
    uint8_t valid = *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(itemStack) + 0x23);
    if (valid == 0) return false;

    void* p = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(itemStack) + 0x08);
    if (!p || !isPointerReadable(p, 16)) return false;

    void* inner = *reinterpret_cast<void**>(p);
    bool innerValid = (inner && isPointerReadable(inner, 16));

    for (const auto& cItem : ::levilamroid::item::getAllCustomItems()) {
        if (cItem.nativeItemPtr && (cItem.nativeItemPtr == p || (innerValid && cItem.nativeItemPtr == inner))) {
            if (outIdentifier) *outIdentifier = cItem.identifier;
            if (outDef) *outDef = &cItem;
            return true;
        }

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

static bool isPlayerHoldingCustomItem(void* player, std::string* outIdent = nullptr, const ::levilamroid::item::CustomItemDefinition** outDef = nullptr, void** outStack = nullptr) {
    if (!player) player = sCurrentLocalPlayer;
    if (!player || !isPointerReadable(player, 0x580)) return false;
    uintptr_t mcBase = getMcBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    // 1. Try Player::getSelectedItem RVA if available
    if (off.playerGetSelectedItem && mcBase) {
        using GetItemFn = void* (*)(void*);
        auto fn = reinterpret_cast<GetItemFn>(mcBase + off.playerGetSelectedItem);
        try {
            void* stack = fn(player);
            if (stack && isCustomItem(stack, outIdent, outDef)) {
                if (outStack) *outStack = stack;
                return true;
            }
        } catch (...) {}
    }

    // 2. Direct PlayerInventory (Supplies) lookup — 100% reliable across all Bedrock profiles
    uint8_t* playerBytes = reinterpret_cast<uint8_t*>(player);
    if (isPointerReadable(playerBytes + 0x570, sizeof(void*))) {
        void* supplies = *reinterpret_cast<void**>(playerBytes + 0x570);
        if (supplies && isPointerReadable(supplies, 0xc0)) {
            uint8_t* suppliesBytes = reinterpret_cast<uint8_t*>(supplies);
            int slot = *reinterpret_cast<int*>(suppliesBytes + 0x10);
            if (slot >= 0 && slot <= 8) {
                void* inventory = *reinterpret_cast<void**>(suppliesBytes + 0xb8);
                if (inventory && isPointerReadable(inventory, 0x150)) {
                    if (off.containerGetItem && mcBase) {
                        using GetItemFn = void* (*)(void*, int);
                        auto getItem = reinterpret_cast<GetItemFn>(mcBase + off.containerGetItem);
                        try {
                            void* stack = getItem(inventory, slot);
                            if (stack && isCustomItem(stack, outIdent, outDef)) {
                                if (outStack) *outStack = stack;
                                return true;
                            }
                        } catch (...) {}
                    }
                }
            }
        }
    }

    // 3. Fallback to exported or unstripped dynamic symbol
    using GetItemFn = void* (*)(void*);
    static GetItemFn sGetSelectedItemFn = nullptr;
    static bool sResolved = false;
    if (!sResolved) {
        sResolved = true;
        void* sym = ::ll::memory::findMinecraftSymbol("_ZNK6Player15getSelectedItemEv");
        if (sym) sGetSelectedItemFn = reinterpret_cast<GetItemFn>(sym);
    }
    if (sGetSelectedItemFn) {
        try {
            void* stack = sGetSelectedItemFn(player);
            if (stack && isCustomItem(stack, outIdent, outDef)) {
                if (outStack) *outStack = stack;
                return true;
            }
        } catch (...) {}
    }

    return false;
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

        int bx = p[0], by = p[1], bz = p[2];
        std::string heldItem;
        const ::levilamroid::item::CustomItemDefinition* heldDef = nullptr;
        void* heldStack = nullptr;
        bool hasHeldCustom = isPlayerHoldingCustomItem(player, &heldItem, &heldDef, &heldStack);

        std::string brokenIdent;
        if (::levilamroid::block::getTrackedCustomBlock(bx, by, bz, &brokenIdent)) {
            ::levilamroid::block::untrackPlacedCustomBlock(bx, by, bz, nullptr);
            // First check if any registered mod listener handles this block destroy
            bool handled = dispatchBlockDestroyListeners(player, bx, by, bz, brokenIdent.c_str(), hasHeldCustom ? heldItem.c_str() : "");
            if (!handled) {
                // Default generic drop: grant 1 of the block's item
                std::string giveCmd = "/give @s " + brokenIdent + " 1";
                ll_command_dispatch_client(giveCmd.c_str());
                ll_ui_send_toast(brokenIdent.c_str(), "Block Harvested", brokenIdent.c_str(), 1800);
            }
            ll_command_dispatch_client("/playsound dig.wood @a ~ ~ ~ 0.8 1.0");
        } else {
            // For standard/vanilla blocks broken with a custom tool
            dispatchBlockDestroyListeners(player, bx, by, bz, "", hasHeldCustom ? heldItem.c_str() : "");
        }

        // Generic tool durability damage on block break
        if (hasHeldCustom && heldDef && heldDef->maxDamage > 0 && heldStack && isPointerReadable(heldStack, 0x24)) {
            uint8_t* stackBytes = reinterpret_cast<uint8_t*>(heldStack);
            int16_t* aux = reinterpret_cast<int16_t*>(stackBytes + 0x20);
            (*aux)++;
            if (*aux >= heldDef->maxDamage) {
                stackBytes[0x22] = 0; // Broken!
                ll_command_dispatch_client("/playsound random.break @a ~ ~ ~ 0.9 1.0");
                ll_ui_send_toast(heldDef->identifier.c_str(), "Tool Broken!", heldDef->iconName.c_str(), 1500);
            }
        }
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

    // Generic tool durability damage and listener dispatch on attack
    std::string heldItem;
    const ::levilamroid::item::CustomItemDefinition* heldDef = nullptr;
    void* heldStack = nullptr;
    if (isPlayerHoldingCustomItem(player, &heldItem, &heldDef, &heldStack) && heldDef) {
        // Durability damage on hit
        if (heldDef->maxDamage > 0 && heldStack && isPointerReadable(heldStack, 0x24)) {
            uint8_t* stackBytes = reinterpret_cast<uint8_t*>(heldStack);
            int16_t* aux = reinterpret_cast<int16_t*>(stackBytes + 0x20);
            (*aux)++;
            if (*aux >= heldDef->maxDamage) {
                stackBytes[0x22] = 0; // Broken!
                ll_command_dispatch_client("/playsound random.break @a ~ ~ ~ 0.9 1.0");
                ll_ui_send_toast(heldDef->identifier.c_str(), "Tool Broken!", heldDef->iconName.c_str(), 1500);
            }
        }

        // Notify mod listeners (e.g. LeviItems for butchering and weapon audio)
        dispatchPlayerAttackListeners(player, targetActor, heldItem.c_str());
    }

    if (sOrigGameModeAttack) {
        return reinterpret_cast<bool(*)(void*, void*)>(sOrigGameModeAttack)(self, targetActor);
    }
    return false;
}

extern "C" LL_API void ll_ui_send_toast(const char* title, const char* message, const char* iconItem, int durationMs);
extern "C" LL_API bool ll_ui_open_screen(const char* screenName);
extern "C" LL_API bool ll_item_give_player(void* player, const char* identifier, int count, int slot);

static void handleMobSpawn(void* /*player*/, void* /*itemStack*/, const void* pos, uint8_t face, const std::string& ident) {
    ll::Logger::getInstance().info("§6[MobSpawn] §aSpawn egg used: ", ident, " at block face ", (int)face);

    std::string entity = ident;
    auto posEgg = entity.rfind("_spawn_egg");
    if (posEgg != std::string::npos) {
        entity = entity.substr(0, posEgg);
    } else {
        posEgg = entity.rfind("_egg");
        if (posEgg != std::string::npos) {
            entity = entity.substr(0, posEgg);
        }
    }
    if (entity.empty()) entity = "chicken";

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
        char cmdBuf[256];
        snprintf(cmdBuf, sizeof(cmdBuf), "/summon %s %d %d %d", entity.c_str(), x, y, z);
        ll_command_dispatch_client(cmdBuf);
    } else {
        char cmdBuf[256];
        snprintf(cmdBuf, sizeof(cmdBuf), "/summon %s ~ ~1 ~", entity.c_str());
        ll_command_dispatch_client(cmdBuf);
    }
    ll_command_dispatch_client("/playsound random.pop @a ~ ~ ~ 1.0 1.0");
}

static bool isPlaceableItem(const std::string& id) {
    if (::levilamroid::block::isCustomBlock(id)) return true;
    const auto* def = ::levilamroid::item::getCustomItem(id);
    if (def && def->isBlock) return true;
    return false;
}

static std::string getFunctionalBlockForCustomItem(const std::string& id) {
    return ::levilamroid::block::getFunctionalBlockForCustomItem(id);
}

static const char* getPlacementSoundForBlock(const std::string& id) {
    return ::levilamroid::block::getPlacementSoundForBlock(id);
}

static bool handleSeedPlanting(
    [[maybe_unused]] void* player,
    void* itemStack,
    const ::levilamroid::item::CustomItemDefinition& def,
    const void* pos,
    uint8_t face
) {
    if (!pos || !isPointerReadable(pos, 12)) return false;
    if (face != 1) return false;

    const int* p = reinterpret_cast<const int*>(pos);
    int x = p[0];
    int y = p[1] + 1;
    int z = p[2];

    std::string plantBlock = ::levilamroid::block::getFunctionalBlockForCustomItem(def.identifier);
    if (plantBlock == "minecraft:stone" && !def.baseBlock.empty()) {
        plantBlock = def.baseBlock;
    }
    if (plantBlock == "minecraft:stone") {
        plantBlock = "minecraft:wheat";
    }

    char cmd[128];
    std::snprintf(cmd, sizeof(cmd), "/setblock %d %d %d %s [] keep", x, y, z, plantBlock.c_str());
    ll_command_dispatch_client(cmd);
    ll_command_dispatch_client("/playsound dig.grass @a ~ ~ ~ 0.8 1.4");

    ::levilamroid::block::trackPlacedCustomBlock(x, y, z, def.identifier);

    if (itemStack && isPointerReadable(itemStack, 0x24)) {
        uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
        if (stackBytes[0x22] > 0) {
            stackBytes[0x22]--;
            if (stackBytes[0x22] == 0) stackBytes[0x23] = 0;
        }
    }

    ll_ui_send_toast(def.identifier.c_str(), "Crops Planted!", def.iconName.c_str(), 1800);
    return true;
}

struct EngineHitResult {
    float startX, startY, startZ;
    float rayX, rayY, rayZ;
    int32_t type;
    uint8_t facing;
    uint8_t pad[3];
    int32_t blockX;
    int32_t blockY;
    int32_t blockZ;
    float posX, posY, posZ;
};

static bool isValidHitResult(const EngineHitResult* hr) {
    if (!hr || !isPointerReadable(hr, sizeof(EngineHitResult))) return false;
    if (hr->type != 0) return false;
    if (hr->facing > 5) return false;
    if (hr->blockY < -64 || hr->blockY > 320) return false;
    if (std::abs(hr->blockX) > 30000000 || std::abs(hr->blockZ) > 30000000) return false;
    return true;
}

static bool findTargetedBlock(BlockPos& outPos, uint8_t& outFace) {
    if (!sCurrentClientInstance || !isPointerReadable(sCurrentClientInstance, 0x1000)) return false;
    uint8_t* base = reinterpret_cast<uint8_t*>(sCurrentClientInstance);
    for (size_t off = 0x8; off <= 0x800; off += sizeof(void*)) {
        void* ptr = *reinterpret_cast<void**>(base + off);
        if (ptr && isPointerReadable(ptr, sizeof(EngineHitResult))) {
            const EngineHitResult* hr = reinterpret_cast<const EngineHitResult*>(ptr);
            if (isValidHitResult(hr)) {
                outPos = BlockPos{hr->blockX, hr->blockY, hr->blockZ};
                outFace = hr->facing;
                return true;
            }
            for (size_t subOff = 0; subOff <= 0x200; subOff += 8) {
                const EngineHitResult* subHr = reinterpret_cast<const EngineHitResult*>(reinterpret_cast<uint8_t*>(ptr) + subOff);
                if (isValidHitResult(subHr)) {
                    outPos = BlockPos{subHr->blockX, subHr->blockY, subHr->blockZ};
                    outFace = subHr->facing;
                    return true;
                }
            }
        }
    }

    // Mathematical player view-vector raycast fallback:
    if (sCurrentLocalPlayer) {
        float px = 0, py = 0, pz = 0;
        float pitch = 0, yaw = 0;
        if (ll_player_get_position(sCurrentLocalPlayer, &px, &py, &pz) &&
            ll_player_get_rotation(sCurrentLocalPlayer, &pitch, &yaw)) {
            float radYaw = (yaw + 90.0f) * (3.14159265f / 180.0f);
            float radPitch = -pitch * (3.14159265f / 180.0f);
            float dx = std::cos(radPitch) * std::cos(radYaw);
            float dy = std::sin(radPitch);
            float dz = std::cos(radPitch) * std::sin(radYaw);

            for (float step = 1.0f; step <= 4.5f; step += 0.25f) {
                int cx = static_cast<int>(std::floor(px + dx * step));
                int cy = static_cast<int>(std::floor(py + 1.62f + dy * step));
                int cz = static_cast<int>(std::floor(pz + dz * step));
                void* bs = ll_dimension_get_blocksource(nullptr);
                if (bs) {
                    const auto& off = ::levilamroid::mcpe::getOffsets();
                    uintptr_t mcBase = getMcBase();
                    if (mcBase && off.blockSourceGetBlock) {
                        using GetBlockFn = void* (*)(void*, const int[3]);
                        auto getBlock = reinterpret_cast<GetBlockFn>(mcBase + off.blockSourceGetBlock);
                        int cpos[3] = {cx, cy, cz};
                        void* blk = getBlock(bs, cpos);
                        if (blk && isPointerReadable(blk, 40)) {
                            uint32_t rId = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(blk) + 0x20);
                            if (rId != 0) { // Non-air solid block!
                                outPos = BlockPos{cx, cy, cz};
                                if (dy < -0.4f) outFace = 1;      // hit top
                                else if (dy > 0.4f) outFace = 0; // hit bottom
                                else if (std::abs(dx) > std::abs(dz)) outFace = (dx > 0) ? 4 : 5;
                                else outFace = (dz > 0) ? 2 : 3;
                                return true;
                            }
                        }
                    }
                }
            }
            // Ray did not intersect any solid block in reach distance
            return false;
        }
    }

    return false;
}

static bool getEstimatedTargetBlock(int& outX, int& outY, int& outZ) {
    if (!sCurrentClientInstance || !isPointerReadable(sCurrentClientInstance, 0x1000)) return false;
    uint8_t* base = reinterpret_cast<uint8_t*>(sCurrentClientInstance);
    for (size_t off = 0x8; off <= 0x800; off += sizeof(void*)) {
        void* ptr = *reinterpret_cast<void**>(base + off);
        if (ptr && isPointerReadable(ptr, sizeof(EngineHitResult))) {
            const EngineHitResult* hr = reinterpret_cast<const EngineHitResult*>(ptr);
            if (hr && (hr->rayX != 0.0f || hr->rayY != 0.0f || hr->rayZ != 0.0f)) {
                outX = static_cast<int>(std::floor(hr->startX + hr->rayX * 2.5f));
                outY = static_cast<int>(std::floor(hr->startY + hr->rayY * 2.5f));
                outZ = static_cast<int>(std::floor(hr->startZ + hr->rayZ * 2.5f));
                return true;
            }
        }
    }
    return false;
}

static void handleBlockPlacementFallback(
    [[maybe_unused]] void* self,
    [[maybe_unused]] void* player,
    void* itemStack,
    const ::levilamroid::item::CustomItemDefinition& def
) {
    std::string placeBlock = getFunctionalBlockForCustomItem(def.identifier);
    const char* sound = getPlacementSoundForBlock(def.identifier);

    char cmd[256];
    std::snprintf(cmd, sizeof(cmd), "/execute at @s run setblock ^ ^ ^2 %s [] keep", placeBlock.c_str());
    ll_command_dispatch_client(cmd);

    char soundCmd[128];
    std::snprintf(soundCmd, sizeof(soundCmd), "/playsound %s @a ~ ~ ~ 0.8 1.0", sound);
    ll_command_dispatch_client(soundCmd);

    int tx = 0, ty = 0, tz = 0;
    if (getEstimatedTargetBlock(tx, ty, tz)) {
        ::levilamroid::block::trackPlacedCustomBlock(tx, ty, tz, def.identifier);
    }

    if (itemStack && isPointerReadable(itemStack, 0x24)) {
        uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
        if (stackBytes[0x22] > 0) {
            stackBytes[0x22]--;
            if (stackBytes[0x22] == 0) stackBytes[0x23] = 0;
        }
    }

    std::string toastMsg = "Placed " + def.identifier.substr(def.identifier.find(':') + 1) + "!";
    ll_ui_send_toast(def.identifier.c_str(), toastMsg.c_str(), def.iconName.c_str(), 1800);
    ll::Logger::getInstance().info("§6[LeviLamroid] §aCustom block placed (raycast fallback): ", def.identifier);
}

static bool handleSeedPlantingFallback(
    [[maybe_unused]] void* player,
    void* itemStack,
    const ::levilamroid::item::CustomItemDefinition& def
) {
    std::string plantBlock = !def.baseBlock.empty() ? def.baseBlock : "minecraft:wheat";

    char cmd[256];
    std::snprintf(cmd, sizeof(cmd), "/execute at @s run setblock ~ ~ ~ %s [] keep", plantBlock.c_str());
    ll_command_dispatch_client(cmd);
    ll_command_dispatch_client("/playsound dig.grass @a ~ ~ ~ 0.8 1.4");

    int tx = 0, ty = 0, tz = 0;
    if (getEstimatedTargetBlock(tx, ty, tz)) {
        ::levilamroid::block::trackPlacedCustomBlock(tx, ty, tz, def.identifier);
    }

    if (itemStack && isPointerReadable(itemStack, 0x24)) {
        uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
        if (stackBytes[0x22] > 0) {
            stackBytes[0x22]--;
            if (stackBytes[0x22] == 0) stackBytes[0x23] = 0;
        }
    }

    ll_ui_send_toast(def.identifier.c_str(), "Crops Planted!", def.iconName.c_str(), 1800);
    return true;
}

static void handleBlockPlacement(
    [[maybe_unused]] void* self,
    [[maybe_unused]] void* player,
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

    bool placed = false;
    void* perm = ::levilamroid::block::getCustomBlockPermutation(def.identifier);
    void* bs = ll_dimension_get_blocksource(nullptr);
    if (bs && perm) {
        placed = ll_block_source_set_block(bs, x, y, z, perm, 3);
        if (placed) {
            ll::Logger::getInstance().info("§6[LeviLamroid] §aCustom block placed directly via BlockSource: ", def.identifier);
        }
    }

    const char* sound = ::levilamroid::block::getPlacementSoundForBlock(def.identifier);
    if (!placed) {
        std::string placeBlock = ::levilamroid::block::getFunctionalBlockForCustomItem(def.identifier);
        char cmd[128];
        std::snprintf(cmd, sizeof(cmd), "/setblock %d %d %d %s [] replace", x, y, z, placeBlock.c_str());
        placed = ll_command_dispatch_client(cmd);
    }

    if (placed) {
        if (sound && sound[0]) {
            char soundCmd[128];
            std::snprintf(soundCmd, sizeof(soundCmd), "/playsound %s @a ~ ~ ~ 0.8 1.0", sound);
            ll_command_dispatch_client(soundCmd);
        }

        if (itemStack && isPointerReadable(itemStack, 0x24)) {
            uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
            if (stackBytes[0x22] > 0) {
                stackBytes[0x22]--;
                if (stackBytes[0x22] == 0) stackBytes[0x23] = 0;
            }
        }

        std::string toastMsg = "Placed " + def.identifier.substr(def.identifier.find(':') + 1) + "!";
        ll_ui_send_toast(def.identifier.c_str(), toastMsg.c_str(), def.iconName.c_str(), 1800);
        ::levilamroid::block::trackPlacedCustomBlock(x, y, z, def.identifier);
        ll::Logger::getInstance().info("§6[LeviLamroid] §aCustom block placed at (", x, ", ", y, ", ", z, "): ", def.identifier);
    }
}

static bool handleFoodConsumption([[maybe_unused]] void* player, void* itemStack, const ::levilamroid::item::CustomItemDefinition* def) {
    if (!def || def->isBlock) return false;
    static auto sLastEatTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - sLastEatTime).count();
    if (elapsed <= 400) return true;
    sLastEatTime = now;

    ll_command_dispatch_client("/playsound random.burp @a ~ ~ ~ 0.8 1.0");

    int dur = std::max(1, def->nutrition / 2);
    char effectCmd[128];
    std::snprintf(effectCmd, sizeof(effectCmd), "/effect @s saturation %d 1 true", dur);
    ll_command_dispatch_client(effectCmd);

    if (def->saturationModifier >= 1.0f) {
        ll_command_dispatch_client("/effect @s regeneration 5 1 true");
        ll_command_dispatch_client("/effect @s absorption 60 0 true");
    }

    ll_command_dispatch_client("/particle minecraft:heart_particle ~ ~1.2 ~");

    char msg[64];
    std::snprintf(msg, sizeof(msg), "+%d Hunger", def->nutrition);
    ll_ui_send_toast(def->identifier.c_str(), msg, def->iconName.c_str(), 1800);

    if (itemStack && isPointerReadable(itemStack, 0x24)) {
        uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
        if (stackBytes[0x22] > 0) {
            stackBytes[0x22]--;
            if (stackBytes[0x22] == 0) {
                stackBytes[0x23] = 0; // Invalidate empty stack
            }
        }
    }
    return true;
}

static bool hooked_gameModeUseItem(void* self, void* itemStack, uint32_t handSlot) {
    std::string ident;
    const ::levilamroid::item::CustomItemDefinition* def = nullptr;
    if (isCustomItem(itemStack, &ident, &def) && def) {
        void* player = self ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 0x8) : nullptr;
        if (!player) player = sCurrentLocalPlayer;

        // Publish event for plugins to react
        ll::event::PlayerUseItemEvent ev(reinterpret_cast<Player*>(player), itemStack);
        ll::event::EventBus::getInstance().publish(ev);

        // Custom mod item use listener dispatch
        if (dispatchItemUseListeners(player, ident.c_str(), itemStack)) {
            return true;
        }

        // Spawn egg summoning fallback
        if (ident.find("spawn_egg") != std::string::npos) {
            handleMobSpawn(player, itemStack, nullptr, 1, ident);
            if (itemStack && isPointerReadable(itemStack, 0x24)) {
                uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
                if (stackBytes[0x22] > 0) {
                    stackBytes[0x22]--;
                    if (stackBytes[0x22] == 0) stackBytes[0x23] = 0;
                }
            }
            return true;
        }

        // 1. Let native GameMode::useItem run first (for native eating animations, bow charging, etc.)
        if (sOrigGameModeUseItem) {
            using OrigFn = bool (*)(void*, void*, uint32_t);
            bool nativeUsed = reinterpret_cast<OrigFn>(sOrigGameModeUseItem)(self, itemStack, handSlot);
            if (nativeUsed) {
                return true;
            }
        }

        // 2. Check if this is a placeable block item
        if (def->isBlock || isPlaceableItem(ident)) {
            BlockPos targetPos{0, 0, 0};
            uint8_t targetFace = 1;
            if (findTargetedBlock(targetPos, targetFace)) {
                handleBlockPlacement(self, player, itemStack, *def, &targetPos, targetFace);
                return true;
            } else {
                handleBlockPlacementFallback(self, player, itemStack, *def);
                return true;
            }
        }

        // 3. Check if seeds/crop planting
        if (ident.find("seeds") != std::string::npos || !def->baseBlock.empty()) {
            BlockPos targetPos{0, 0, 0};
            uint8_t targetFace = 1;
            if (findTargetedBlock(targetPos, targetFace)) {
                if (handleSeedPlanting(player, itemStack, *def, &targetPos, targetFace)) {
                    return true;
                }
            } else {
                if (handleSeedPlantingFallback(player, itemStack, *def)) {
                    return true;
                }
            }
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
    void* player = self ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 0x8) : nullptr;
    if (!player) player = sCurrentLocalPlayer;

    // 1. Intercept interactions on placed custom blocks!
    if (pos && isPointerReadable(pos, 12)) {
        const int* p = reinterpret_cast<const int*>(pos);
        std::string placedBlockIdent;
        if (::levilamroid::block::getTrackedCustomBlock(p[0], p[1], p[2], &placedBlockIdent)) {
            std::string heldIdent;
            const ::levilamroid::item::CustomItemDefinition* heldDef = nullptr;
            void* heldStack = nullptr;
            isPlayerHoldingCustomItem(player, &heldIdent, &heldDef, &heldStack);

            if (dispatchBlockInteractListeners(player, p[0], p[1], p[2], face, placedBlockIdent.c_str(), heldIdent.c_str())) {
                return true;
            }
        }
    }

    std::string ident;
    const ::levilamroid::item::CustomItemDefinition* def = nullptr;
    if (isCustomItem(itemStack, &ident, &def) && def) {
        // Publish event for plugins to react
        ll::event::PlayerUseItemEvent ev(reinterpret_cast<Player*>(player), itemStack);
        ll::event::EventBus::getInstance().publish(ev);

        // Check block interact listener if tapping a block with a custom item
        if (pos && isPointerReadable(pos, 12)) {
            const int* p = reinterpret_cast<const int*>(pos);
            std::string placedBlockIdent;
            ::levilamroid::block::getTrackedCustomBlock(p[0], p[1], p[2], &placedBlockIdent);
            if (dispatchBlockInteractListeners(player, p[0], p[1], p[2], face, placedBlockIdent.c_str(), ident.c_str())) {
                return true;
            }
        }

        // Custom item use listener
        if (dispatchItemUseListeners(player, ident.c_str(), itemStack)) {
            return true;
        }

        // Spawn egg summoning fallback
        if (ident.find("spawn_egg") != std::string::npos) {
            handleMobSpawn(player, itemStack, pos, face, ident);
            if (itemStack && isPointerReadable(itemStack, 0x24)) {
                uint8_t* stackBytes = reinterpret_cast<uint8_t*>(itemStack);
                if (stackBytes[0x22] > 0) {
                    stackBytes[0x22]--;
                    if (stackBytes[0x22] == 0) stackBytes[0x23] = 0;
                }
            }
            return true;
        }

        // 1. Let native GameMode::useItemOn attempt placement / interaction first
        if (sOrigGameModeUseItemOn) {
            bool nativeHandled = reinterpret_cast<bool(*)(void*, void*, const void*, uint8_t, float, char, const void*, const void*)>(
                sOrigGameModeUseItemOn)(self, itemStack, pos, face, a4, a5, hitPos, block);
            if (nativeHandled) {
                return true;
            }
        }

        // 2. Fallback: custom seed or crop planting action
        if (ident.find("seeds") != std::string::npos || !def->baseBlock.empty()) {
            if (handleSeedPlanting(player, itemStack, *def, pos, face)) {
                return true;
            }
        }

        // 3. Fallback: placeable custom block item
        if (def->isBlock || isPlaceableItem(ident)) {
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
    if (self) {
        sServerNetworkHandler = self;
    }
    if (netId && isPointerReadable(netId, 64)) {
        std::memcpy(sServerNetIdMem, netId, sizeof(sServerNetIdMem));
        sHasServerNetId.store(true);
    }
    if (textPacket && isPointerReadable(textPacket, 96)) {
        for (size_t off = 0x18; off <= 0x60; off += sizeof(void*)) {
            const std::string* str = reinterpret_cast<const std::string*>(reinterpret_cast<const uint8_t*>(textPacket) + off);
            if (isPointerReadable(str, sizeof(std::string)) && str->length() > 0 && str->length() < 256) {
                const char* data = str->c_str();
                if (data && (data[0] == '/' || data[0] == '!')) {
                    std::string cmd = data;
                    ll::Logger::getInstance().info("ServerNetworkHandler intercepted command: ", cmd);
                    sInServerNetworkHandlerHandle = true;
                    bool handled = ll_command_dispatch_client(cmd.c_str());
                    sInServerNetworkHandlerHandle = false;
                    if (handled) {
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
                // Check if this command belongs to custom registered commands
                bool isCustom = false;
                {
                    std::lock_guard<std::mutex> lk(sClientCommandMutex);
                    isCustom = sClientCommands.count(base) || sClientCommands.count(stripped);
                }
                if (isCustom || ::levilamroid::command::hasCommand(clean) || ::levilamroid::command::hasCommand(base)) {
                    ll::Logger::getInstance().info("§6[LoopbackPacketSender] §bIntercepted CommandRequestPacket; queueing to main thread: §e", clean);
                    queueMainThreadCommand(clean);
                    return true; // Dropped! Do not forward to server
                }
            }
        }
    }
    return false;
}

struct PacketListenerEntry {
    LLPacketListenerCallback callback;
    void* udata;
};
static std::vector<PacketListenerEntry> sPacketListeners;
static std::mutex sPacketListenerMutex;

static void* sOrigClientNetworkHandlerHandle = nullptr;

// ── Geyser-Parity Packet Ingestion Helpers ─────────────────────────────────

static void injectCustomBlockProperties(StartGamePacket* packet) {
    if (!packet) return;
    try {
        // Geyser Parity: Enable data-driven items and upcoming creator experiments
        try {
            auto& expStorage = packet->mPayload.mSettings.get().mExperiments.get();
            expStorage.mExperimentsEverToggled = true;
            auto& expData = expStorage.mExperimentData.get();
            if (expData.size() < 24) expData.resize(24, true);
            for (size_t i = 0; i < expData.size(); ++i) {
                expData[i] = true;
            }
            packet->mPayload.mSettings.get().mOverrideForceExperimentalGameplayFlag = true;
        } catch (...) {}

        auto customBlocks = levilamroid::block::getRegisteredCustomBlocks();
        if (customBlocks.empty()) return;

        auto& blockProps = packet->mPayload.mBlockProperties.get();
        size_t added = 0;
        for (const auto& def : customBlocks) {
            if (def.identifier.empty()) continue;
            bool exists = false;
            for (const auto& existing : blockProps) {
                if (existing.mName.get() == def.identifier) {
                    exists = true;
                    break;
                }
            }
            if (exists) continue;

            auto nbt = levilamroid::nbt::NbtBuilder::buildBlockPropertyNbt(def);
            if (nbt) {
                ServerBlockProperty prop;
                prop.mName.get() = def.identifier;
                prop.mTag.get() = std::move(*nbt);
                blockProps.push_back(std::move(prop));
                added++;
            }
        }
        if (added > 0) {
            ll::Logger::getInstance().info("§a[PacketInjector] §eInjected §b", added, " §ecustom block properties into StartGamePacket (Geyser parity)");
        }
    } catch (const std::exception& e) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Exception in injectCustomBlockProperties: ", e.what());
    } catch (...) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Unknown exception in injectCustomBlockProperties");
    }
}

static void injectCustomItemComponents(ItemRegistryPacket* packet) {
    if (!packet) return;
    try {
        auto& customItems = levilamroid::item::getAllCustomItems();
        if (customItems.empty()) return;

        auto& items = packet->mPayload.mItems.get();
        size_t added = 0;
        for (const auto& def : customItems) {
            if (def.identifier.empty()) continue;
            bool exists = false;
            for (const auto& existing : items) {
                if (existing.mName.get().getString() == def.identifier) {
                    exists = true;
                    break;
                }
            }
            if (exists) continue;

            auto nbt = levilamroid::nbt::NbtBuilder::buildItemComponentNbt(def);
            if (nbt) {
                ItemData itemData;
                itemData.mName.get() = ::HashedString(def.identifier);
                itemData.mId = def.id;
                itemData.mIsComponentBased = true;
                itemData.mItemVersion = ItemVersion::DataDriven;
                itemData.mComponentData.get() = std::move(*nbt);
                items.push_back(std::move(itemData));
                added++;
            }
        }
        if (added > 0) {
            ll::Logger::getInstance().info("§a[PacketInjector] §eInjected §b", added, " §ecustom item components into ItemRegistryPacket (Geyser parity)");
        }
    } catch (const std::exception& e) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Exception in injectCustomItemComponents: ", e.what());
    } catch (...) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Unknown exception in injectCustomItemComponents");
    }
}

static void injectCustomActorIdentifiers(AvailableActorIdentifiersPacket* packet) {
    if (!packet) return;
    try {
        auto customEntities = levilamroid::entity::getAllEntities();
        if (customEntities.empty()) return;

        auto& idList = packet->mPayload.mIdentifierList.get();
        size_t added = 0;
        static uint32_t sActorRuntimeIdAllocator = 15000;
        for (const auto& def : customEntities) {
            if (def.identifier.empty()) continue;
            bool exists = false;
            for (const auto& existing : idList) {
                if (existing.mIdentifier.get().mFullName.get() == def.identifier) {
                    exists = true;
                    break;
                }
            }
            if (exists) continue;

            ActorInfo info;
            info.mRuntimeId = sActorRuntimeIdAllocator++;
            info.mHasSpawnEgg = !def.spawnEggItem.empty();
            info.mIsSummonable = def.isSummonable;
            info.mExperimentIndex.get() = std::nullopt;

            std::string ns = "minecraft";
            std::string name = def.identifier;
            size_t colon = def.identifier.find(':');
            if (colon != std::string::npos) {
                ns = def.identifier.substr(0, colon);
                name = def.identifier.substr(colon + 1);
            }

            info.mIdentifier.get().mNamespace.get() = ns;
            info.mIdentifier.get().mIdentifier.get() = name;
            info.mIdentifier.get().mFullName.get() = def.identifier;
            info.mIdentifier.get().mCanonicalName.get() = ::HashedString(def.identifier);

            std::string baseId = !def.runtimeId.empty() ? def.runtimeId : "minecraft:zombie";
            info.mBaseIdentifier.get().mFullName.get() = baseId;
            info.mBaseIdentifier.get().mCanonicalName.get() = ::HashedString(baseId);

            idList.push_back(std::move(info));
            added++;
        }
        if (added > 0) {
            ll::Logger::getInstance().info("§a[PacketInjector] §eInjected §b", added, " §ecustom entities into AvailableActorIdentifiersPacket (Geyser parity)");
        }
    } catch (const std::exception& e) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Exception in injectCustomActorIdentifiers: ", e.what());
    } catch (...) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Unknown exception in injectCustomActorIdentifiers");
    }
}

static void injectCustomCreativeContent(CreativeContentPacket* packet) {
    if (!packet) return;
    try {
        auto& customItems = levilamroid::item::getAllCustomItems();
        if (customItems.empty()) return;

        auto& entries = packet->mPayload.mEntries.get();
        if (entries.empty()) return;

        const CreativeItemEntryPayload* templateEntry = nullptr;
        for (const auto& e : entries) {
            if (e.mItemDescriptor.get().mImpl.get() != nullptr) {
                templateEntry = &e;
                break;
            }
        }
        if (!templateEntry) return;

        static uint32_t sCreativeNetIdCounter = 12000;
        size_t added = 0;

        for (const auto& def : customItems) {
            if (def.identifier.empty() || !def.nativeItemPtr) continue;

            CreativeItemEntryPayload entry = *templateEntry;
            entry.mCreativeItemNetId = sCreativeNetIdCounter++;
            entry.mGroupIndex = (def.category > 0) ? static_cast<uint32_t>(def.category) : 4;
            entry.mItemDescriptor.get().mStackSize = (def.maxStackSize > 0) ? def.maxStackSize : 1;
            entry.mItemDescriptor.get().mBlockRuntimeId = 0;

            auto* implPtr = entry.mItemDescriptor.get().mImpl.get();
            if (implPtr && isPointerReadable(implPtr, 16)) {
                *reinterpret_cast<const void**>(reinterpret_cast<uint8_t*>(implPtr) + 8) = def.nativeItemPtr;
                *reinterpret_cast<short*>(reinterpret_cast<uint8_t*>(implPtr) + 16) = 0;
            }

            entries.push_back(std::move(entry));
            added++;
        }
        if (added > 0) {
            ll::Logger::getInstance().info("§a[PacketInjector] §eInjected §b", added, " §ecustom items into CreativeContentPacket (Geyser parity)");
        }
    } catch (const std::exception& e) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Exception in injectCustomCreativeContent: ", e.what());
    } catch (...) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Unknown exception in injectCustomCreativeContent");
    }
}

static void injectCustomCommands(AvailableCommandsPacket* packet) {
    if (!packet) return;
    try {
        auto& commands = packet->mPayload.mCommands.get();
        auto customCmds = ::levilamroid::command::getAllCustomCommands();
        if (customCmds.empty()) return;

        size_t added = 0;
        for (const auto& [name, desc] : customCmds) {
            if (name.empty()) continue;
            bool exists = false;
            for (const auto& existing : commands) {
                if (existing.name.get() == name) {
                    exists = true;
                    break;
                }
            }
            if (exists) continue;

            std::string cleanDesc = desc;
            size_t nl = cleanDesc.find('\n');
            if (nl != std::string::npos) {
                cleanDesc = cleanDesc.substr(0, nl);
            }
            if (cleanDesc.length() > 947) {
                cleanDesc = cleanDesc.substr(0, 944) + "...";
            }
            if (cleanDesc.empty()) {
                cleanDesc = "/" + name;
            }

            AvailableCommandsPacketPayload::CommandData cmdData;
            cmdData.name.get() = name;
            cmdData.description.get() = cleanDesc;
            cmdData.flags = 0x40; // Geyser: CommandData.Flag.NOT_CHEAT
            cmdData.permission = CommandPermissionLevel::Any;
            cmdData.aliasEnum = -1;

            commands.push_back(std::move(cmdData));
            added++;
        }
        if (added > 0) {
            ll::Logger::getInstance().info("§a[PacketInjector] §eInjected §b", added, " §ecustom commands into AvailableCommandsPacket (Geyser parity)");
        }
    } catch (const std::exception& e) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Exception in injectCustomCommands: ", e.what());
    } catch (...) {
        ll::Logger::getInstance().warn("§c[PacketInjector] Unknown exception in injectCustomCommands");
    }
}

static void hooked_clientNetworkHandlerHandle(void* self, void* netId, void* packetSharedPtr) {
    if (packetSharedPtr && isPointerReadable(packetSharedPtr, sizeof(void*))) {
        void* rawPacket = *reinterpret_cast<void**>(packetSharedPtr);
        if (rawPacket && isPointerReadable(rawPacket, sizeof(void*) * 3)) {
            int packetId = 0;
            void** vtable = *reinterpret_cast<void***>(rawPacket);
            if (vtable && isPointerReadable(vtable, sizeof(void*) * 3)) {
                using GetIdFn = int (*)(void*);
                // In Bedrock ARM64 ABI: vtable[0]=deleting dtor, vtable[1]=complete dtor, vtable[2]=getId()
                GetIdFn getId = reinterpret_cast<GetIdFn>(vtable[2]);
                if (getId && isPointerReadable(reinterpret_cast<void*>(getId), 4)) {
                    try { packetId = getId(rawPacket); } catch (...) {}
                }
            }
            if (packetId > 0) {
                // ── Geyser-Parity Packet Ingestion ─────────────────────────
                if (packetId == static_cast<int>(MinecraftPacketIds::StartGame)) {
                    injectCustomBlockProperties(reinterpret_cast<StartGamePacket*>(rawPacket));
                } else if (packetId == static_cast<int>(MinecraftPacketIds::ItemComponent)) {
                    injectCustomItemComponents(reinterpret_cast<ItemRegistryPacket*>(rawPacket));
                } else if (packetId == static_cast<int>(MinecraftPacketIds::AvailableActorIdentifiers)) {
                    injectCustomActorIdentifiers(reinterpret_cast<AvailableActorIdentifiersPacket*>(rawPacket));
                } else if (packetId == static_cast<int>(MinecraftPacketIds::CreativeContent)) {
                    injectCustomCreativeContent(reinterpret_cast<CreativeContentPacket*>(rawPacket));
                } else if (packetId == static_cast<int>(MinecraftPacketIds::AvailableCommands)) {
                    injectCustomCommands(reinterpret_cast<AvailableCommandsPacket*>(rawPacket));
                }

                std::lock_guard<std::mutex> lock(sPacketListenerMutex);
                for (const auto& entry : sPacketListeners) {
                    try {
                        if (entry.callback(packetId, rawPacket, true, entry.udata)) return;
                    } catch (...) {}
                }
            }
        }
    }
    if (sOrigClientNetworkHandlerHandle) {
        reinterpret_cast<void(*)(void*, void*, void*)>(sOrigClientNetworkHandlerHandle)(self, netId, packetSharedPtr);
    }
}

static void hooked_loopbackPacketSenderSend(void* self, void* packet) {
    if (self) sLoopbackPacketSender = self;
    if (handleCommandPacket(packet)) return;
    if (packet && isPointerReadable(packet, 16)) {
        int packetId = 0;
        void** vtable = *reinterpret_cast<void***>(packet);
        if (vtable && isPointerReadable(vtable, sizeof(void*) * 3)) {
            using GetIdFn = int (*)(void*);
            // In Bedrock ARM64 ABI: vtable[0]=deleting dtor, vtable[1]=complete dtor, vtable[2]=getId()
            GetIdFn getId = reinterpret_cast<GetIdFn>(vtable[2]);
            if (getId && isPointerReadable(reinterpret_cast<void*>(getId), 4)) {
                try { packetId = getId(packet); } catch (...) {}
            }
        }
        if (packetId > 0) {
            // Geyser-Parity Packet Ingestion for local/singleplayer sessions
            if (packetId == static_cast<int>(MinecraftPacketIds::StartGame)) {
                injectCustomBlockProperties(reinterpret_cast<StartGamePacket*>(packet));
            } else if (packetId == static_cast<int>(MinecraftPacketIds::ItemComponent)) {
                injectCustomItemComponents(reinterpret_cast<ItemRegistryPacket*>(packet));
            } else if (packetId == static_cast<int>(MinecraftPacketIds::AvailableActorIdentifiers)) {
                injectCustomActorIdentifiers(reinterpret_cast<AvailableActorIdentifiersPacket*>(packet));
            } else if (packetId == static_cast<int>(MinecraftPacketIds::CreativeContent)) {
                injectCustomCreativeContent(reinterpret_cast<CreativeContentPacket*>(packet));
            } else if (packetId == static_cast<int>(MinecraftPacketIds::AvailableCommands)) {
                injectCustomCommands(reinterpret_cast<AvailableCommandsPacket*>(packet));
            }

            std::lock_guard<std::mutex> lock(sPacketListenerMutex);
            for (const auto& entry : sPacketListeners) {
                try {
                    if (entry.callback(packetId, packet, false, entry.udata)) return;
                } catch (...) {}
            }
        }
    }
    if (sOrigLoopbackPacketSenderSend) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigLoopbackPacketSenderSend)(self, packet);
    }
}

static void hooked_loopbackPacketSenderSendToServer(void* self, void* packet) {
    if (self) sLoopbackPacketSender = self;
    if (handleCommandPacket(packet)) return;
    if (sOrigLoopbackPacketSenderSendToServer) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigLoopbackPacketSenderSendToServer)(self, packet);
    }
}

// ── MouseHandlerAndroid Input Subsystem (Audited from LeviMap) ───────────────

typedef bool (*LLTouchCallback)(int button, bool isDown, short x, short y, void* udata);
struct TouchCallbackEntry {
    LLTouchCallback callback;
    void* udata;
};
static std::vector<TouchCallbackEntry> sTouchCallbacks;
static std::mutex sTouchCallbackMutex;
static void* sOrigMouseHandlerButtonInput = nullptr;
static void* sOrigMouseHandlerAbsoluteMouseInput = nullptr;
static void* sOrigMouseHandlerScrollInput = nullptr;

static void hooked_mouseHandlerButtonInput(void* self, int button, bool isDown, const short* coords) {
    if (coords && isPointerReadable(coords, 4)) {
        sLastTouchX = coords[0];
        sLastTouchY = coords[1];
    }
    sIsTouchDown = isDown;
    if (isDown) {
        sTouchDownTime = std::chrono::steady_clock::now();
        sHasTriggeredHoldAction = false;
    } else {
        sHasTriggeredHoldAction = false;
    }

    bool swallowed = false;

    // 1. Dispatch to PreLoader pl::input
    pl::input::TouchEvent te;
    te.action = isDown ? 0 : 1; // 0=DOWN, 1=UP
    te.pointerId = button;
    te.x = static_cast<float>(sLastTouchX);
    te.y = static_cast<float>(sLastTouchY);
    if (pl::input::dispatchTouchEvent(te)) {
        swallowed = true;
    }

    pl::input::MouseEvent me;
    me.button = button;
    me.isDown = isDown;
    if (pl::input::dispatchMouseEvent(me)) {
        swallowed = true;
    }

    // 2. Dispatch to LeviLamroid native touch callbacks
    {
        std::lock_guard<std::mutex> lock(sTouchCallbackMutex);
        for (const auto& entry : sTouchCallbacks) {
            try {
                if (entry.callback(button, isDown, sLastTouchX, sLastTouchY, entry.udata)) {
                    swallowed = true;
                }
            } catch (...) {}
        }
    }

    // 3. Complete right-click routing for ALL custom items (Food, Blocks, Weapons, Usable Artifacts)
    // Swallowing (swallowed = true) prevents unhooked Bedrock engine from invoking missing ComponentItem
    // methods on raw Item instances, guaranteeing 100% crash-free interaction across all profiles.
    if (button == 2 && isDown && sCurrentLocalPlayer && sIsInWorld.load()) {
        std::string heldIdent;
        const ::levilamroid::item::CustomItemDefinition* heldDef = nullptr;
        void* heldStack = nullptr;
        if (isPlayerHoldingCustomItem(sCurrentLocalPlayer, &heldIdent, &heldDef, &heldStack) && heldDef) {
            // A. Custom Food consumption on right-click
            if (!heldDef->isBlock && (heldDef->isFood || heldDef->nutrition > 0)) {
                handleFoodConsumption(sCurrentLocalPlayer, heldStack, heldDef);
                swallowed = true;
            }
            // B. Custom Block placement on right-click
            else if (heldDef->isBlock) {
                BlockPos targetPos{0, 0, 0};
                uint8_t targetFace = 1;
                if (findTargetedBlock(targetPos, targetFace)) {
                    handleBlockPlacement(nullptr, sCurrentLocalPlayer, heldStack, *heldDef, &targetPos, targetFace);
                } else {
                    handleBlockPlacementFallback(nullptr, sCurrentLocalPlayer, heldStack, *heldDef);
                }
                swallowed = true;
            }
            // C. Usable Items, Weapons, Artifacts (Doctor Strange Mandala, Cosmic Blade vortex, etc.)
            else {
                dispatchItemUseListeners(sCurrentLocalPlayer, heldIdent.c_str(), heldStack);
                sHasTriggeredHoldAction = true;
                swallowed = true;
            }
        }
    }

    if (swallowed) return;

    if (sOrigMouseHandlerButtonInput) {
        reinterpret_cast<void(*)(void*, int, bool, const short*)>(sOrigMouseHandlerButtonInput)(
            self, button, isDown, coords
        );
    }
}

static void hooked_mouseHandlerAbsoluteMouseInput(void* self, const short* coords) {
    if (coords && isPointerReadable(coords, 4)) {
        sLastTouchX = coords[0];
        sLastTouchY = coords[1];
    }

    bool swallowed = false;
    if (sIsTouchDown) {
        pl::input::TouchEvent te;
        te.action = 2; // MOVE
        te.pointerId = 0;
        te.x = static_cast<float>(sLastTouchX);
        te.y = static_cast<float>(sLastTouchY);
        if (pl::input::dispatchTouchEvent(te)) {
            swallowed = true;
        }
    }

    if (swallowed) return;

    if (sOrigMouseHandlerAbsoluteMouseInput) {
        reinterpret_cast<void(*)(void*, const short*)>(sOrigMouseHandlerAbsoluteMouseInput)(self, coords);
    }
}

static void hooked_mouseHandlerScrollInput(void* self, int scrollDelta, const short* coords) {
    if (coords && isPointerReadable(coords, 4)) {
        sLastTouchX = coords[0];
        sLastTouchY = coords[1];
    }
    if (sOrigMouseHandlerScrollInput) {
        reinterpret_cast<void(*)(void*, int, const short*)>(sOrigMouseHandlerScrollInput)(self, scrollDelta, coords);
    }
}

// ── MainActivity Keyboard Input Subsystem ──────────────────────────────────
static void* sOrigNativeKeyHandler = nullptr;

static uint32_t hooked_nativeKeyHandler(void* env, void* thiz, int keyCode, int action, int flags) {
    bool isDown = (action == 0); // 0 = ACTION_DOWN, 1 = ACTION_UP
    pl::input::KeyEvent ev;
    ev.keyCode = keyCode;
    ev.isKeyDown = isDown;
    ev.unicodeChar = 0;

    if (pl::input::dispatchKeyEvent(ev)) {
        return 1; // Swallowed by mod key callback
    }

    if (sOrigNativeKeyHandler) {
        using Fn = uint32_t (*)(void*, void*, int, int, int);
        return reinterpret_cast<Fn>(sOrigNativeKeyHandler)(env, thiz, keyCode, action, flags);
    }
    return 0;
}

static bool installGameplayHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.clientInstanceUpdate) {
        if (ll::memory::hookRva(off.clientInstanceUpdate, (void*)&hooked_clientInstanceUpdate, &sOrigClientInstanceUpdate)) {
            logger.info("Hooked ClientInstance::update [RVA: 0x", std::hex, off.clientInstanceUpdate, "]");
        }
    }

    if (off.mouseHandlerAndroidButtonInput) {
        if (ll::memory::hookRva(off.mouseHandlerAndroidButtonInput, (void*)&hooked_mouseHandlerButtonInput, &sOrigMouseHandlerButtonInput)) {
            logger.info("Hooked MouseHandlerAndroid::buttonInput [RVA: 0x", std::hex, off.mouseHandlerAndroidButtonInput, "]");
        }
    }

    if (off.mouseHandlerAndroidAbsoluteMouseInput) {
        if (ll::memory::hookRva(off.mouseHandlerAndroidAbsoluteMouseInput, (void*)&hooked_mouseHandlerAbsoluteMouseInput, &sOrigMouseHandlerAbsoluteMouseInput)) {
            logger.info("Hooked MouseHandlerAndroid::absoluteMouseInput [RVA: 0x", std::hex, off.mouseHandlerAndroidAbsoluteMouseInput, "]");
        }
    }

    if (off.mouseHandlerAndroidScrollInput) {
        if (ll::memory::hookRva(off.mouseHandlerAndroidScrollInput, (void*)&hooked_mouseHandlerScrollInput, &sOrigMouseHandlerScrollInput)) {
            logger.info("Hooked MouseHandlerAndroid::scrollInput [RVA: 0x", std::hex, off.mouseHandlerAndroidScrollInput, "]");
        }
    }

    if (off.mainActivityNativeKeyHandler) {
        if (ll::memory::hookRva(off.mainActivityNativeKeyHandler, (void*)&hooked_nativeKeyHandler, &sOrigNativeKeyHandler)) {
            logger.info("Hooked MainActivity::nativeKeyHandler [RVA: 0x", std::hex, off.mainActivityNativeKeyHandler, "]");
        }
    } else {
        void* sym = dlsym(RTLD_DEFAULT, "Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler");
        if (!sym) {
            void* h = dlopen("libminecraftpe.so", RTLD_NOLOAD);
            if (h) sym = dlsym(h, "Java_com_mojang_minecraftpe_MainActivity_nativeKeyHandler");
        }
        if (sym && ll::memory::hook(sym, (void*)&hooked_nativeKeyHandler, &sOrigNativeKeyHandler)) {
            logger.info("Hooked MainActivity::nativeKeyHandler via dlsym symbol");
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

    if (off.clientNetworkHandlerHandle) {
        if (ll::memory::hookRva(off.clientNetworkHandlerHandle, (void*)&hooked_clientNetworkHandlerHandle, &sOrigClientNetworkHandlerHandle)) {
            logger.info("Hooked ClientNetworkHandler::handle [RVA: 0x", std::hex, off.clientNetworkHandlerHandle, "]");
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
    using GetBlockFn = void* (*)(void*, const int[3]);
    auto fn = reinterpret_cast<GetBlockFn>(mcBase + off.blockSourceGetBlock);
    int pos[3] = {x, y, z};
    try {
        return fn(blockSource, pos);
    } catch (...) {
        return nullptr;
    }
}

LL_API bool ll_block_source_set_block(void* blockSource, int x, int y, int z, void* blockPermutation, int updateFlags) {
    if (!blockSource || !blockPermutation) return false;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ::levilamroid::hooks::getMcBase();
    if (!mcBase || !off.blockSourceSetBlock) return false;
    using SetBlockFn = bool (*)(void*, const int[3], void*, int, uint8_t);
    auto fn = reinterpret_cast<SetBlockFn>(mcBase + off.blockSourceSetBlock);
    int pos[3] = {x, y, z};
    try {
        return fn(blockSource, pos, blockPermutation, updateFlags, 0);
    } catch (...) {
        return false;
    }
}

extern "C" LL_API bool ll_handle_food_consumption(void* player, void* itemStack, const ::levilamroid::item::CustomItemDefinition* def) {
    return ::levilamroid::hooks::handleFoodConsumption(player, itemStack, def);
}

extern "C" LL_API bool ll_player_consume_held_item(void* player, int count) {
    if (!player) player = ::levilamroid::hooks::sCurrentLocalPlayer;
    if (!player) return false;
    std::string heldIdent;
    const ::levilamroid::item::CustomItemDefinition* def = nullptr;
    void* heldStack = nullptr;
    if (::levilamroid::hooks::isPlayerHoldingCustomItem(player, &heldIdent, &def, &heldStack) && heldStack) {
        if (::levilamroid::hooks::isPointerReadable(heldStack, 0x24)) {
            uint8_t* stackBytes = reinterpret_cast<uint8_t*>(heldStack);
            if (stackBytes[0x22] >= count) {
                stackBytes[0x22] -= count;
            } else {
                stackBytes[0x22] = 0;
            }
            return true;
        }
    }
    return false;
}

LL_API bool ll_block_source_set_ghost_block(void* blockSource, int x, int y, int z, void* blockPermutation) {
    if (!blockSource) blockSource = ll_dimension_get_blocksource(nullptr);
    if (!blockSource || !blockPermutation) return false;
    // updateFlags = 4: Client-side local rendering only, no packet transmission
    return ll_block_source_set_block(blockSource, x, y, z, blockPermutation, 4);
}

LL_API const char* ll_block_get_name(void* block) {
    if (!block || !::levilamroid::hooks::isPointerReadable(block, 64)) return "";
    void* legacy = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(block) + 0x08);
    if (legacy && ::levilamroid::hooks::isPointerReadable(legacy, 128)) {
        for (size_t off = 0x10; off <= 0x80; off += 8) {
            const std::string* s = reinterpret_cast<const std::string*>(reinterpret_cast<uint8_t*>(legacy) + off);
            if (::levilamroid::hooks::isPointerReadable(s, sizeof(std::string)) && s->length() > 0 && s->length() < 128) {
                if (s->find(':') != std::string::npos || s->find("tile.") != std::string::npos) {
                    return s->c_str();
                }
            }
        }
    }
    return "";
}

LL_API uint32_t ll_block_get_runtime_id(void* block) {
    if (!block || !::levilamroid::hooks::isPointerReadable(block, 40)) return 0;
    return *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(block) + 0x20);
}

LL_API int ll_block_source_get_brightness(void* blockSource, int x, int y, int z) {
    if (!blockSource) return 0;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ::levilamroid::hooks::getMcBase();
    if (!mcBase || !off.blockSourceGetBrightness) return 0;
    using GetBrightFn = int (*)(void*, const int[3]);
    auto fn = reinterpret_cast<GetBrightFn>(mcBase + off.blockSourceGetBrightness);
    int pos[3] = {x, y, z};
    try {
        return fn(blockSource, pos);
    } catch (...) {
        return 0;
    }
}

LL_API bool ll_block_source_destroy_block(void* blockSource, int x, int y, int z, bool dropResources) {
    if (!blockSource) return false;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ::levilamroid::hooks::getMcBase();
    if (!mcBase || !off.blockSourceDestroyBlock) return false;
    using DestroyFn = bool (*)(void*, const int[3], bool);
    auto fn = reinterpret_cast<DestroyFn>(mcBase + off.blockSourceDestroyBlock);
    int pos[3] = {x, y, z};
    try {
        return fn(blockSource, pos, dropResources);
    } catch (...) {
        return false;
    }
}

LL_API void* ll_dimension_get_blocksource(void* dimension) {
    if (!dimension) {
        void* player = ::levilamroid::hooks::sCurrentLocalPlayer;
        if (player && ::levilamroid::hooks::isPointerReadable(player, 0x580)) {
            dimension = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + 0x390);
            if (!dimension) dimension = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + 0x398);
        }
    }
    if (!dimension || !::levilamroid::hooks::isPointerReadable(dimension, 360)) return nullptr;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ::levilamroid::hooks::getMcBase();
    if (mcBase && off.dimensionGetBlockSource) {
        using GetBsFn = void* (*)(void*);
        auto fn = reinterpret_cast<GetBsFn>(mcBase + off.dimensionGetBlockSource);
        try {
            void* bs = fn(dimension);
            if (bs && ::levilamroid::hooks::isPointerReadable(bs, 64)) return bs;
        } catch (...) {}
    }
    // Direct memory layout: BlockSource is embedded at Dimension +0x158 (344 bytes)
    void* directBs = reinterpret_cast<uint8_t*>(dimension) + 0x158;
    if (::levilamroid::hooks::isPointerReadable(directBs, 64)) {
        return directBs;
    }
    return nullptr;
}

LL_API int ll_dimension_get_id(void* dimension) {
    if (!dimension) return 0;
    if (!::levilamroid::hooks::isPointerReadable(dimension, 0x180)) return 0;
    int id = *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(dimension) + 368);
    if (id >= 0 && id <= 2) return id;
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(dimension) + 0x08);
}

LL_API void* ll_dimension_get_chunksource(void* dimension) {
    if (!dimension || !::levilamroid::hooks::isPointerReadable(dimension, 432)) return nullptr;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ::levilamroid::hooks::getMcBase();
    if (mcBase && off.dimensionGetChunkSource) {
        using GetCsFn = void* (*)(void*);
        auto fn = reinterpret_cast<GetCsFn>(mcBase + off.dimensionGetChunkSource);
        try { return fn(dimension); } catch (...) {}
    }
    void* cs = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(dimension) + 424);
    return (cs && ::levilamroid::hooks::isPointerReadable(cs, 16)) ? cs : nullptr;
}

LL_API void ll_dimension_convert_coordinates(int fromDim, int toDim, float inX, float inZ, float* outX, float* outZ) {
    float x = inX;
    float z = inZ;
    if (fromDim == 0 && toDim == 1) { // Overworld -> Nether
        x = inX / 8.0f;
        z = inZ / 8.0f;
    } else if (fromDim == 1 && toDim == 0) { // Nether -> Overworld
        x = inX * 8.0f;
        z = inZ * 8.0f;
    }
    if (outX) *outX = x;
    if (outZ) *outZ = z;
}

LL_API bool ll_player_get_position(void* player, float* outX, float* outY, float* outZ) {
    if (!player) player = ::levilamroid::hooks::sCurrentLocalPlayer;
    if (!player || !::levilamroid::hooks::isPointerReadable(player, 0x580)) return false;
    void* posPtr = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + 520);
    if (posPtr && ::levilamroid::hooks::isPointerReadable(posPtr, sizeof(float) * 3)) {
        float* coords = reinterpret_cast<float*>(posPtr);
        if (outX) *outX = coords[0];
        if (outY) *outY = coords[1];
        if (outZ) *outZ = coords[2];
        return true;
    }
    // Fallback candidate
    float* posCandidate = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(player) + 0x264);
    if (::levilamroid::hooks::isPointerReadable(posCandidate, sizeof(float) * 3)) {
        if (outX) *outX = posCandidate[0];
        if (outY) *outY = posCandidate[1];
        if (outZ) *outZ = posCandidate[2];
        return true;
    }
    return false;
}

LL_API bool ll_player_get_rotation(void* player, float* outPitch, float* outYaw) {
    if (!player) player = ::levilamroid::hooks::sCurrentLocalPlayer;
    if (!player || !::levilamroid::hooks::isPointerReadable(player, 0x580)) return false;
    void* rotPtr = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + 536);
    if (rotPtr && ::levilamroid::hooks::isPointerReadable(rotPtr, sizeof(float) * 2)) {
        float* rots = reinterpret_cast<float*>(rotPtr);
        if (outPitch) *outPitch = rots[0];
        if (outYaw) *outYaw = rots[1];
        return true;
    }
    // Fallback candidate
    float* rotCandidate = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(player) + 0x270);
    if (::levilamroid::hooks::isPointerReadable(rotCandidate, sizeof(float) * 2)) {
        if (outPitch) *outPitch = rotCandidate[0];
        if (outYaw) *outYaw = rotCandidate[1];
        return true;
    }
    return false;
}

LL_API int ll_player_get_dimension_id(void* player) {
    if (!player) player = ::levilamroid::hooks::sCurrentLocalPlayer;
    if (!player || !::levilamroid::hooks::isPointerReadable(player, 0x580)) return 0;
    void* dim = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + 0x390);
    if (!dim || !::levilamroid::hooks::isPointerReadable(dim, 16)) {
        dim = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + 0x398);
    }
    if (dim && ::levilamroid::hooks::isPointerReadable(dim, 0x10)) {
        return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(dim) + 0x08);
    }
    return 0;
}

LL_API bool ll_chunk_is_slime_chunk(int chunkX, int chunkZ, int64_t worldSeed) {
    uint64_t seed = (static_cast<uint64_t>(worldSeed)
        + static_cast<uint64_t>(chunkX * chunkX * 0x4c1906)
        + static_cast<uint64_t>(chunkX * 0x5ac0db)
        + static_cast<uint64_t>(chunkZ * chunkZ) * 0x4307a7ULL
        + static_cast<uint64_t>(chunkZ * 0x5f24f)
        ^ 0x3ad8025fULL);
    seed = (seed * 0x5deece66dULL + 0xbULL) & 0x0000ffffffffffffULL;
    int next = static_cast<int>(seed >> 17) % 10;
    return next == 0;
}

LL_API void ll_chunk_get_coordinates(float playerX, float playerZ, int* outChunkX, int* outChunkZ, int* outRelX, int* outRelZ) {
    int cx = static_cast<int>(std::floor(playerX / 16.0f));
    int cz = static_cast<int>(std::floor(playerZ / 16.0f));
    int rx = static_cast<int>(std::floor(playerX)) & 15;
    int rz = static_cast<int>(std::floor(playerZ)) & 15;
    if (outChunkX) *outChunkX = cx;
    if (outChunkZ) *outChunkZ = cz;
    if (outRelX) *outRelX = rx;
    if (outRelZ) *outRelZ = rz;
}

LL_API bool ll_network_register_packet_listener(LLPacketListenerCallback callback, void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(::levilamroid::hooks::sPacketListenerMutex);
    ::levilamroid::hooks::sPacketListeners.push_back({callback, udata});
    return true;
}

LL_API bool ll_network_unregister_packet_listener(LLPacketListenerCallback callback) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(::levilamroid::hooks::sPacketListenerMutex);
    auto it = std::remove_if(::levilamroid::hooks::sPacketListeners.begin(), ::levilamroid::hooks::sPacketListeners.end(),
        [callback](const ::levilamroid::hooks::PacketListenerEntry& e) { return e.callback == callback; });
    ::levilamroid::hooks::sPacketListeners.erase(it, ::levilamroid::hooks::sPacketListeners.end());
    return true;
}

LL_API int ll_packet_get_id(void* packet) {
    if (!packet || !::levilamroid::hooks::isPointerReadable(packet, 16)) return 0;
    void** vtable = *reinterpret_cast<void***>(packet);
    if (!vtable || !::levilamroid::hooks::isPointerReadable(vtable, sizeof(void*) * 3)) return 0;
    using GetIdFn = int (*)(void*);
    // In Bedrock ARM64 ABI: vtable[0]=deleting dtor, vtable[1]=complete dtor, vtable[2]=getId()
    GetIdFn fn = reinterpret_cast<GetIdFn>(vtable[2]);
    if (fn && ::levilamroid::hooks::isPointerReadable(reinterpret_cast<void*>(fn), 4)) {
        try {
            int id = fn(packet);
            if (id > 0 && id < 500) return id;
        } catch (...) {}
    }
    return 0;
}

LL_API bool ll_input_register_touch_callback(bool (*callback)(int, bool, short, short, void*), void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(::levilamroid::hooks::sTouchCallbackMutex);
    ::levilamroid::hooks::sTouchCallbacks.push_back({callback, udata});
    return true;
}

LL_API bool ll_input_unregister_touch_callback(bool (*callback)(int, bool, short, short, void*)) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(::levilamroid::hooks::sTouchCallbackMutex);
    auto it = std::remove_if(::levilamroid::hooks::sTouchCallbacks.begin(), ::levilamroid::hooks::sTouchCallbacks.end(),
        [callback](const ::levilamroid::hooks::TouchCallbackEntry& e) { return e.callback == callback; });
    ::levilamroid::hooks::sTouchCallbacks.erase(it, ::levilamroid::hooks::sTouchCallbacks.end());
    return true;
}

LL_API void ll_input_get_last_touch_pos(short* outX, short* outY) {
    if (outX) *outX = ::levilamroid::hooks::sLastTouchX;
    if (outY) *outY = ::levilamroid::hooks::sLastTouchY;
}

LL_API bool ll_input_is_touch_down() {
    return ::levilamroid::hooks::sIsTouchDown;
}

LL_API int ll_item_stack_get_count(const void* itemStack) {
    if (!itemStack) return 0;
    return static_cast<int>(*reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(itemStack) + 0x22));
}

LL_API bool ll_item_stack_set_count(void* itemStack, uint8_t count) {
    if (!itemStack || !::levilamroid::hooks::isPointerReadable(itemStack, 0x24)) return false;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(itemStack);
    bytes[0x22] = count;
    bytes[0x23] = (count > 0) ? 1 : 0;
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

LL_API void* ll_service_get_block_source() {
    return ll_dimension_get_blocksource(nullptr);
}

static bool extractItemIdentifierFromStack(const void* stack, char* outIdent, size_t maxLen) {
    if (!stack || !outIdent || maxLen == 0) return false;
    if (!::levilamroid::hooks::isPointerReadable(stack, 0x24)) return false;

    // 1. Custom Items check
    std::string customIdent;
    if (::levilamroid::hooks::isCustomItem(const_cast<void*>(stack), &customIdent, nullptr) && !customIdent.empty()) {
        std::strncpy(outIdent, customIdent.c_str(), maxLen - 1);
        outIdent[maxLen - 1] = '\0';
        return true;
    }

    // 2. Vanilla ItemStackBase: count at 0x22, valid at 0x23
    const uint8_t* stackBytes = reinterpret_cast<const uint8_t*>(stack);
    uint8_t count = stackBytes[0x22];
    uint8_t valid = stackBytes[0x23];
    if (count == 0 && valid == 0) return false;

    // 3. Item pointer at 0x08 (WeakPtr<Item>)
    void* p = *reinterpret_cast<void* const*>(stackBytes + 0x08);
    if (!p || !::levilamroid::hooks::isPointerReadable(p, 16)) return false;

    void* inner = *reinterpret_cast<void**>(p);
    void* itemObj = (inner && ::levilamroid::hooks::isPointerReadable(inner, 0x100)) ? inner : p;
    void** vtable = *reinterpret_cast<void***>(itemObj);

    if (vtable && ::levilamroid::hooks::isPointerReadable(vtable, 0x30)) {
        // Virtual slot 4: Item::getDescriptionId() const -> const std::string&
        try {
            using GetDescIdFn = const std::string& (*)(const void*);
            auto getDesc = reinterpret_cast<GetDescIdFn>(vtable[4]);
            if (getDesc && ::levilamroid::hooks::isPointerReadable(reinterpret_cast<void*>(getDesc), 4)) {
                const std::string& desc = getDesc(itemObj);
                if (!desc.empty() && desc.size() < maxLen) {
                    std::memcpy(outIdent, desc.data(), desc.size());
                    outIdent[desc.size()] = '\0';
                    return true;
                }
            }
        } catch (...) {}
    }

    // 4. Memory probe: check for embedded std::string or HashedString in Item object
    const uint8_t* itemBytes = reinterpret_cast<const uint8_t*>(itemObj);
    for (size_t probeOff = 0x60; probeOff <= 0x180; probeOff += 8) {
        if (!::levilamroid::hooks::isPointerReadable(itemBytes + probeOff, sizeof(void*))) continue;
        const char* strCandidate = *reinterpret_cast<const char* const*>(itemBytes + probeOff);
        if (strCandidate && ::levilamroid::hooks::isPointerReadable(strCandidate, 16)) {
            if (std::strncmp(strCandidate, "minecraft:", 10) == 0 || std::strncmp(strCandidate, "item.", 5) == 0) {
                size_t sLen = std::strlen(strCandidate);
                if (sLen > 0 && sLen < maxLen) {
                    std::memcpy(outIdent, strCandidate, sLen);
                    outIdent[sLen] = '\0';
                    return true;
                }
            }
        }
    }

    return false;
}

static struct PlayerContainerScanner {
    size_t invOffset = 0;
    size_t armorOffset = 0;
    size_t handOffset = 0;
    std::atomic<bool> initialized{false};
} sPlayerContainers;

static void findContainersInPlayer(void* player) {
    if (sPlayerContainers.initialized.load()) return;
    if (!player || !::levilamroid::hooks::isPointerReadable(player, 0x800)) return;

    uint8_t* p = reinterpret_cast<uint8_t*>(player);
    for (size_t off = 0x480; off <= 0x780; off += 8) {
        if (!::levilamroid::hooks::isPointerReadable(p + off, sizeof(void*))) continue;
        void* candidate = *reinterpret_cast<void**>(p + off);
        if (!candidate || !::levilamroid::hooks::isPointerReadable(candidate, 0x160)) continue;

        // Check for direct vector at candidate + 0x140
        uint8_t* b = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(candidate) + 0x140);
        uint8_t* e = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(candidate) + 0x148);
        if (b && e && e > b && ::levilamroid::hooks::isPointerReadable(b, 152)) {
            size_t diff = e - b;
            if ((diff == 36 * 152 || diff == 40 * 152) && sPlayerContainers.invOffset == 0) {
                sPlayerContainers.invOffset = off;
            } else if (diff == 4 * 152 && sPlayerContainers.armorOffset == 0) {
                sPlayerContainers.armorOffset = off;
            } else if (diff == 2 * 152 && sPlayerContainers.handOffset == 0) {
                sPlayerContainers.handOffset = off;
            }
        }

        // Check for subpointer inside PlayerInventory (e.g. candidate + 0xB8 -> Inventory)
        for (size_t subOff = 0x18; subOff <= 0xC0; subOff += 8) {
            if (!::levilamroid::hooks::isPointerReadable(reinterpret_cast<uint8_t*>(candidate) + subOff, sizeof(void*))) continue;
            void* sub = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(candidate) + subOff);
            if (!sub || !::levilamroid::hooks::isPointerReadable(sub, 0x160)) continue;

            uint8_t* sb = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(sub) + 0x140);
            uint8_t* se = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(sub) + 0x148);
            if (sb && se && se > sb && ::levilamroid::hooks::isPointerReadable(sb, 152)) {
                size_t diff = se - sb;
                if ((diff == 36 * 152 || diff == 40 * 152) && sPlayerContainers.invOffset == 0) {
                    sPlayerContainers.invOffset = off;
                } else if (diff == 4 * 152 && sPlayerContainers.armorOffset == 0) {
                    sPlayerContainers.armorOffset = off;
                }
            }
        }
    }

    if (sPlayerContainers.invOffset != 0 || sPlayerContainers.armorOffset != 0) {
        sPlayerContainers.initialized.store(true);
    }
}

LL_API void* ll_player_get_selected_item_stack(void* player) {
    if (!player) player = ::levilamroid::hooks::sCurrentLocalPlayer;
    if (!player || !::levilamroid::hooks::isPointerReadable(player, 0x800)) return nullptr;

    findContainersInPlayer(player);
    uint8_t* p = reinterpret_cast<uint8_t*>(player);

    // 1. Try hand container first if discovered
    if (sPlayerContainers.handOffset != 0) {
        void* handCont = *reinterpret_cast<void**>(p + sPlayerContainers.handOffset);
        if (handCont && ::levilamroid::hooks::isPointerReadable(handCont, 0x160)) {
            uint8_t* b = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(handCont) + 0x140);
            if (b && ::levilamroid::hooks::isPointerReadable(b, 152)) {
                return b; // Mainhand is index 0
            }
        }
    }

    // 2. Try inventory container
    size_t invOff = sPlayerContainers.invOffset ? sPlayerContainers.invOffset : 0x570;
    if (::levilamroid::hooks::isPointerReadable(p + invOff, sizeof(void*))) {
        void* supplies = *reinterpret_cast<void**>(p + invOff);
        if (supplies && ::levilamroid::hooks::isPointerReadable(supplies, 0x160)) {
            uint8_t* suppliesBytes = reinterpret_cast<uint8_t*>(supplies);
            int slot = *reinterpret_cast<int*>(suppliesBytes + 0x10);
            if (slot < 0 || slot > 8) slot = 0;

            // Direct items vector or subpointer
            uint8_t* itemsBegin = *reinterpret_cast<uint8_t**>(suppliesBytes + 0x140);
            uint8_t* itemsEnd   = *reinterpret_cast<uint8_t**>(suppliesBytes + 0x148);
            if (itemsBegin && itemsEnd && itemsEnd >= itemsBegin + (slot + 1) * 152) {
                return itemsBegin + (slot * 152);
            }

            // Subpointer at 0xB8
            void* inv = *reinterpret_cast<void**>(suppliesBytes + 0xb8);
            if (inv && ::levilamroid::hooks::isPointerReadable(inv, 0x160)) {
                uint8_t* ib = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(inv) + 0x140);
                uint8_t* ie = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(inv) + 0x148);
                if (ib && ie && ie >= ib + (slot + 1) * 152) {
                    return ib + (slot * 152);
                }
            }
        }
    }

    return nullptr;
}

LL_API bool ll_player_get_held_item_identifier(void* player, char* outIdent, size_t maxLen) {
    if (!outIdent || maxLen == 0) return false;
    void* stack = ll_player_get_selected_item_stack(player);
    return extractItemIdentifierFromStack(stack, outIdent, maxLen);
}

LL_API bool ll_player_get_armor_stack(void* player, int slot, void** outStack, char* outName, size_t maxLen, int* outDamage, int* outMaxDamage) {
    if (slot < 0 || slot > 3) return false;
    if (!player) player = ::levilamroid::hooks::sCurrentLocalPlayer;
    if (!player || !::levilamroid::hooks::isPointerReadable(player, 0x800)) return false;

    findContainersInPlayer(player);
    uint8_t* p = reinterpret_cast<uint8_t*>(player);

    void* stack = nullptr;
    if (sPlayerContainers.armorOffset != 0) {
        void* armorCont = *reinterpret_cast<void**>(p + sPlayerContainers.armorOffset);
        if (armorCont && ::levilamroid::hooks::isPointerReadable(armorCont, 0x160)) {
            uint8_t* b = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(armorCont) + 0x140);
            uint8_t* e = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(armorCont) + 0x148);
            if (b && e && e >= b + (slot + 1) * 152) {
                stack = b + (slot * 152);
            }
        }
    }

    if (!stack) return false;
    if (outStack) *outStack = stack;

    const uint8_t* sb = reinterpret_cast<const uint8_t*>(stack);
    uint8_t count = sb[0x22];
    uint8_t valid = sb[0x23];
    if (count == 0 && valid == 0) return false;

    char identBuf[128] = {0};
    bool hasName = extractItemIdentifierFromStack(stack, identBuf, sizeof(identBuf));
    if (outName && maxLen > 0) {
        if (hasName) {
            std::strncpy(outName, identBuf, maxLen - 1);
            outName[maxLen - 1] = '\0';
        } else {
            std::snprintf(outName, maxLen, "armor_%d", slot);
        }
    }

    short dmg = *reinterpret_cast<const short*>(sb + 0x20);
    if (outDamage) *outDamage = (dmg >= 0) ? dmg : 0;

    if (outMaxDamage) {
        std::string_view sv(identBuf);
        int maxDur = 100;
        if (sv.find("netherite") != std::string_view::npos) {
            if (slot == 0) maxDur = 407;
            else if (slot == 1) maxDur = 592;
            else if (slot == 2) maxDur = 555;
            else maxDur = 481;
        } else if (sv.find("diamond") != std::string_view::npos) {
            if (slot == 0) maxDur = 363;
            else if (slot == 1) maxDur = 528;
            else if (slot == 2) maxDur = 495;
            else maxDur = 429;
        } else if (sv.find("iron") != std::string_view::npos) {
            if (slot == 0) maxDur = 165;
            else if (slot == 1) maxDur = 240;
            else if (slot == 2) maxDur = 225;
            else maxDur = 195;
        } else if (sv.find("gold") != std::string_view::npos) {
            if (slot == 0) maxDur = 77;
            else if (slot == 1) maxDur = 112;
            else if (slot == 2) maxDur = 105;
            else maxDur = 91;
        } else if (sv.find("chain") != std::string_view::npos) {
            if (slot == 0) maxDur = 165;
            else if (slot == 1) maxDur = 240;
            else if (slot == 2) maxDur = 225;
            else maxDur = 195;
        } else if (sv.find("leather") != std::string_view::npos) {
            if (slot == 0) maxDur = 55;
            else if (slot == 1) maxDur = 80;
            else if (slot == 2) maxDur = 75;
            else maxDur = 65;
        } else if (sv.find("elytra") != std::string_view::npos) {
            maxDur = 432;
        } else if (sv.find("turtle") != std::string_view::npos) {
            maxDur = 275;
        }
        *outMaxDamage = maxDur;
    }

    return true;
}

LL_API void* ll_player_get_supplies(void* player) {
    if (!player) player = ::levilamroid::hooks::sCurrentLocalPlayer;
    if (!player || !::levilamroid::hooks::isPointerReadable(player, 0x800)) return nullptr;
    findContainersInPlayer(player);
    uint8_t* p = reinterpret_cast<uint8_t*>(player);
    size_t off = sPlayerContainers.invOffset ? sPlayerContainers.invOffset : 0x570;
    void* supplies = *reinterpret_cast<void**>(p + off);
    return (supplies && ::levilamroid::hooks::isPointerReadable(supplies, 0xc0)) ? supplies : nullptr;
}

LL_API bool ll_actor_get_position(void* actor, float* outX, float* outY, float* outZ) {
    return ll_player_get_position(actor, outX, outY, outZ);
}

LL_API bool ll_actor_get_rotation(void* actor, float* outPitch, float* outYaw) {
    return ll_player_get_rotation(actor, outPitch, outYaw);
}

LL_API bool ll_actor_is_player(void* actor) {
    if (!actor || !::levilamroid::hooks::isPointerReadable(actor, 0x580)) return false;
    if (actor == ::levilamroid::hooks::sCurrentLocalPlayer) return true;
    return false;
}

LL_API bool ll_actor_is_alive(void* actor) {
    return (actor && ::levilamroid::hooks::isPointerReadable(actor, 0x100));
}

LL_API bool ll_actor_hurt(void* actor, int damage) {
    if (!actor) return false;
    char cmd[128];
    std::snprintf(cmd, sizeof(cmd), "/damage @e[c=1,r=3] %d", damage > 0 ? damage : 1);
    return ::levilamroid::hooks::ll_command_dispatch_client(cmd);
}

} // extern "C"

namespace ll::hooks {

LL_API bool installAllHooks() {
    return ::levilamroid::hooks::installAllHooks();
}

} // namespace ll::hooks
