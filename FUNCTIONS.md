# LeviLamroid Functions & API Master Reference

> **LeviLamroid Version**: `1.0.5-beta` (Beta Testing Release)  
> **Target Platform**: Minecraft Bedrock Edition (Android ARM64) on LeviLauncher  
> **Maintainer**: **SpyGamingOG** (`spygamingog`)  
> **Architecture**: Native C++ Mod Loader Runtime (Windows LeviLamina Source Parity + Android PreLoader Extensions)  
> **Scope**: Complete master reference cataloging every class, struct, function, signature, parameter, return value, event, and offset table.

> [!WARNING]
> ### ⚠️ Critical Reverse Engineering Notice: Items, Blocks & Components are NOT Completely Discovered
> **Important Technical Disclosure**:
> Minecraft Bedrock Edition (Android ARM64) engine internals—specifically custom item components (`FoodItemComponent`, `SpawnEggItem`), custom block definitions (`BlockDefinitionGroup`, block state permutation tables), and creative inventory registration—are **under active reverse engineering and are NOT completely discovered**.
>
> - **What is Verified & Stable**: Engine entry points (`Item::Item`, `BlockItem::BlockItem`, `BlockItem::_useOn`, `GameMode::useItem`), UI screen controllers (`PauseScreenController`, `StartMenuScreenController`), RenderDragon rendering passes (`RenderLevelEvent`, `UIRenderEvent`), audio hooks, input interception, and memory detours.
> - **What is Partially Discovered / Experimental**: Deep native C++ component graph construction (`FoodItemComponent` struct layout, dynamic `SpawnEggItem::getColor` downcasting, internal permutation indices in `BlockTypeRegistry`).
> - **Current Mitigation & Safety**: To prevent fatal crashes (`SIGSEGV` dereferencing unmapped component pointers), LeviLamroid intercepts `GameMode::useItem` and `GameMode::useItemOn` to handle consumption and placement safely via client command bridges and stack decrementing, while component schemas and vtables continue to be reverse engineered. See [Section 6](#6-reverse-engineering-discovery-status-items-blocks--components) for full details.

---

## Table of Contents

1. [Overview & Core Architecture](#1-overview--core-architecture)
2. [Master Header Directory & Include Matrix](#2-master-header-directory--include-matrix)
3. [Mod Lifecycle & Registration Subsystems (`ll::mod` & `pl::mod`)](#3-mod-lifecycle--registration-subsystems-llmod--plmod)
4. [Memory Detours, Pattern Scanning & Dynamic Signatures (`ll::memory`)](#4-memory-detours-pattern-scanning--dynamic-signatures-llmemory)
5. [Multi-Version Engine & Golden RVA Tables (`ll::mcpe`)](#5-multi-version-engine--golden-rva-tables-llmcpe)
6. [Reverse Engineering Discovery Status: Items, Blocks & Components](#6-reverse-engineering-discovery-status-items-blocks--components)
7. [Thread-Safe Event Bus System (`ll::event::EventBus`)](#7-thread-safe-event-bus-system-lleventeventbus)
8. [Complete Built-in Event Catalog](#8-complete-built-in-event-catalog)
9. [Client Fabric API & RenderDragon Graphics Subsystem (`ll::event`)](#9-client-fabric-api--renderdragon-graphics-subsystem-llevent)
10. [Universal UI, Pause Menu & Screen Navigation Subsystem (`ll::ui`)](#10-universal-ui-pause-menu--screen-navigation-subsystem-llui)
11. [Android Mobile Touch, HUD Buttons & Input (`pl::modmenu` & `pl::input`)](#11-android-mobile-touch-hud-buttons--input-plmodmenu--plinput)
12. [Custom Items, Custom Blocks & Addon Ingestion (`ll::item`, `ll::block`, `ll::addon`)](#12-custom-items-custom-blocks--addon-ingestion-llitem-llblock-lladdon)
13. [Minecraft Bedrock Engine Class API Reference (`mc/` & `mcpe/`)](#13-minecraft-bedrock-engine-class-api-reference-mc--mcpe)
14. [Inventory, Container & NBT Manipulation Subsystems (`ll::inventory`, `mc::CompoundTag`)](#14-inventory-container--nbt-manipulation-subsystems-llinventory-mccompoundtag)
15. [Client Command System & Scheduling (`ll::command`, `ll::chrono`)](#15-client-command-system--scheduling-llcommand-llchrono)
16. [Audio Systems (`levilamroid::sound::FmodAudioEngine` & `SoundEngine`)](#16-audio-systems-levilamroidsoundfmodaudioengine--soundengine)
17. [Virtual In-Memory Resource Packs & Asset Strategies (`levilamroid::resource`)](#17-virtual-in-memory-resource-packs--asset-strategies-levilamroidresource)
18. [Core Engine Services, Threading & Logging (`ll::service`, `ll::thread`, `ll::Logger`)](#18-core-engine-services-threading--logging-llservice-llthread-lllogger)
19. [LeviLamina Migration Guide & Android Parity Matrix](#19-levilamina-migration-guide--android-parity-matrix)
20. [Starter Template & Distribution Packaging (`v1.0.0`)](#20-starter-template--distribution-packaging-v100)

---

## 1. Overview & Core Architecture

**LeviLamroid** is the primary, production-grade native C++ mod loader runtime designed specifically for **Minecraft Bedrock Edition (Android ARM64)** executing within **LeviLauncher**.

It establishes five core architectural guarantees:

1. **Canonical Modding Namespace (`ll`)**: LeviLamroid adheres directly to **`namespace ll`** as its official coding namespace, matching LeviLamina on Windows. Modders write against `ll::Logger`, `ll::event::EventBus`, `ll::mcpe::getOffsets()`, and `ll::mod::NativeMod`. For compatibility, `namespace levilamroid` and `namespace llntol` are provided as seamless aliases.
2. **100% LeviLamina Windows Source Parity**: Code written against the official LeviLamina Windows C++ API compiles and runs identically on Android without rewriting detour boilerplate or class structures.
3. **LeviLauncher PreLoader Native Extensions (`pl`)**: Native Android mobile capabilities—including touch pointer routing, physical keyboard mapping, floating HUD touch buttons, launcher drawer registration, and storage access framework compatibility—are exposed out-of-the-box (`pl::*`).
4. **Automated Multi-Version Resolution**: Dynamically reads the ELF `PT_NOTE` segment of `libminecraftpe.so` at startup to determine the 20-byte SHA-1 GNU Build ID, automatically activating the correct RVA offset tables (`1.26.52.3`, `1.26.51.1`, `1.26.45.1`, `1.26.44.3`).
5. **Strict Initial Versioning Policy**: All new mods, template starter projects, and LeviLamroid distribution bundles are initialized strictly at version **`v1.0.0`** (or `1.0.0`) unless explicitly instructed by the user to bump versions.

---

## 2. Master Header Directory & Include Matrix

To build mods for LeviLamroid, add `${LEVILAMROID_ROOT}/include` to your include search paths:

| Header Include | Namespace / Scope | Purpose & Description |
|---|---|---|
| `<levilamroid/LeviLamroid.h>` | `ll` / `levilamroid` | Canonical Master Header; imports all subsystems, version macros, and types |
| `<ll/api/mod/NativeMod.h>` | `ll::mod` | Canonical LeviLamina mod base class with profile-isolated storage |
| `<ll/api/mod/RegisterHelper.h>` | Global macros | `LL_REGISTER_MOD` macro exporting all required loader ABIs |
| `<ll/api/service/Bedrock.h>` | `ll::service` | Global engine accessors: `getClientInstance()`, `getLevel()`, `getLocalPlayer()` |
| `<ll/api/thread/ThreadPoolExecutor.h>` | `ll::thread` | Thread pool worker executor for non-blocking asynchronous mod tasks |
| `<ll/api/plugin/PluginManager.h>` | `ll::plugin` | Dynamic `.so` plugin loading, tracking, and symbol discovery |
| `<ll/api/memory/Hook.h>` | `ll::memory` | Function detour hooking, RVA hooking, GOT slots, RTTI VTable resolution, and `LL_TYPE_INSTANCE_HOOK` |
| `<ll/api/event/EventBus.h>` | `ll::event` | Thread-safe type-based publish/subscribe event bus |
| `<ll/api/event/ui/UIEvents.h>` | `ll::event` | UI events: Start menu, Pause menu open/close, buttons, and tech stack |
| `<ll/api/event/render/RenderEvents.h>` | `ll::event` | RenderDragon events: 3D world render level, 2D UI HUD render, frame prepare |
| `<ll/api/ui/UI.h>` | `ll::ui` | High-level UI manipulation: JSON UI routing, pause menu queries, screen push/pop, toasts |
| `<ll/api/inventory/ItemStack.h>` | `ll::inventory` | Container and inventory manipulation C ABI and classes |
| `<ll/api/command/CommandRegistrar.h>` | `ll::command` | Native client-side command registration and local dispatch |
| `<ll/api/Logger.h>` | `ll` | Multi-level formatted logger outputting to Android Logcat and file |
| `<levilamroid/api/mcpe/Offsets.h>` | `levilamroid::mcpe` | Engine version detection and golden RVA tables |
| `<levilamroid/api/memory/PatternScan.h>` | `levilamroid::memory` | Fast multi-threaded IDA-style wildcard byte pattern scanner |
| `<levilamroid/api/sound/FmodSound.h>` | `levilamroid::sound` | Method A: Direct in-memory FMOD audio playback (WAV, OGG, MP3) without resource packs |
| `<levilamroid/api/sound/SoundEngine.h>` | `levilamroid::mcpe` | Method B: In-game audio engine hook for 3D spatial sound and acoustic physics |
| `<pl/Mod.hpp>` | `pl::mod` | PreLoader mod metadata, mod context, and lifecycle macros (`PL_EXPORT_MOD`) |
| `<pl/Input.hpp>` | `pl::input` | Touch, physical keyboard, and mouse button event interception |
| `<pl/ModMenu.hpp>` | `pl::modmenu` | On-screen floating HUD touch buttons & launcher drawer module registration |
| `<pl/memory/Hook.hpp>` | `pl::memory` | PreLoader function hooking bridge |
| `<pl/memory/Patch.hpp>` | `pl::memory` | Machine code byte patching with named rollback and RAII `PatchHandle` |
| `<mc/client/ClientInstance.h>` | Engine | Root client interface containing player, level renderer, and network sender |
| `<mc/world/actor/player/Player.h>` | Engine | Player actor entity base class and inventory |
| `<mc/world/actor/player/LocalPlayer.h>` | Engine | Client-controlled player subclass with movement, camera, and interaction logic |
| `<mc/world/level/BlockSource.h>` | Engine | World chunk region interface for block inspection and placement |
| `<mc/world/item/ItemStack.h>` | Engine | Native Minecraft Bedrock item stack representation |

---

## 3. Mod Lifecycle & Registration Subsystems (`ll::mod` & `pl::mod`)

LeviLamroid supports both the canonical **LeviLamina** OOP mod lifecycle and the **LeviLauncher PreLoader** C ABI.

### 3.1 Class `ll::mod::NativeMod`

Located in `#include <ll/api/mod/NativeMod.h>`.  
Base class for all LeviLamina-style mods. Inherit from this class and implement lifecycle hooks:

```cpp
#include <ll/api/mod/NativeMod.h>

class MyMod : public ll::mod::NativeMod {
public:
    MyMod() : ll::mod::NativeMod("MyMod") {}

    static MyMod& getInstance() {
        static MyMod instance;
        return instance;
    }

    virtual bool load() override {
        getLogger().info("MyMod loaded!");
        return true;
    }

    virtual bool enable() override {
        getLogger().info("MyMod enabled!");
        return true;
    }

    virtual bool disable() override {
        getLogger().info("MyMod disabled!");
        return true;
    }

    virtual bool unload() override {
        getLogger().info("MyMod unloaded!");
        return true;
    }
};
```

#### Member Functions
- `const std::string& getName() const noexcept`: Returns the mod identifier string.
- `ll::Logger& getLogger() noexcept`: Returns the mod's logger instance tagged with the mod's name.
- `const std::filesystem::path& getDataDir() const noexcept`: Returns the dedicated data storage folder (`/sdcard/Android/media/org.levimc.launcher/minecraft/<profile>/mods/<ModName>/`).

### 3.2 Registration Macro `LL_REGISTER_MOD`

Located in `#include <ll/api/mod/RegisterHelper.h>`.  
Generates and exports all required C ABI entry points for LeviLauncher and LeviLamina:

```cpp
#include <ll/api/mod/RegisterHelper.h>

LL_REGISTER_MOD(MyMod, MyMod::getInstance());
```

This single macro automatically generates:
1. `extern "C" bool ll_mod_load(void*)`: Windows LeviLamina dynamic loader entry point.
2. `extern "C" bool ll_mod_enable(void*)`: Called when the mod is enabled.
3. `extern "C" bool ll_mod_disable(void*)`: Called when the mod is disabled.
4. `extern "C" bool ll_mod_unload(void*)`: Called when the mod is unloaded.
5. `extern "C" void* PLGetModRegistration()`: LeviLauncher PreLoader entry returning a populated, 64-bit aligned registration struct.

---

## 4. Memory Detours, Pattern Scanning & Dynamic Signatures (`ll::memory`)

Located in `#include <ll/api/memory/Hook.h>` and `#include <levilamroid/api/memory/PatternScan.h>`.

### 4.1 Memory Detour Primitives
- `bool ll::memory::hook(void* target, void* detour, void** original, int priority = 200)`:  
  Installs an inline AArch64 detour trampoline. Automatically invalidates CPU instruction caches (`__builtin___clear_cache`). Uses cooperative default priority `200` to prevent conflict with other native mods (such as BedrockTools).
- `bool ll::memory::hookRva(uintptr_t rva, void* detour, void** original)`:  
  Hooks a target at `getMinecraftBase() + rva`.
- `bool ll::memory::hookGotSlot(uintptr_t gotRva, void* detour, void** original)`:  
  Intercepts dynamic linker GOT slots (e.g., `glClear`).
- `uintptr_t ll::memory::getMinecraftBase()`:  
  Returns the base memory address of `libminecraftpe.so` in `/proc/self/maps`.

### 4.2 Dynamic Boot Signature Scanner Engine
- `uintptr_t ll::memory::scanMinecraftText(const std::string& pattern)`:  
  Scans exclusively within the `.text` executable code section (~205.8 MB) using `memchr`-accelerated vector scanning.
- `ll::memory::SignatureEngine::resolve(std::string_view name, uintptr_t fallbackRva, const std::string& pattern)`:  
  Executes a 3-tier dynamic resolution strategy:
  1. **Fast-Path**: Verifies memory at `base + fallbackRva` against the pattern in $O(1)$ time.
  2. **Localized Search**: Scans a $\pm 64\text{ KB}$ window around `fallbackRva` for rapid offset drift relocation.
  3. **Full `.text` Scan**: Scans the complete 205.8 MB `.text` segment.
  4. Caches resolved RVAs into a thread-safe map for zero-overhead repeat lookups.
- `bool ll::memory::hookSignature(const std::string& pattern, void* detour, void** original, uintptr_t fallbackRva)`:  
  Resolves the target using the signature engine and installs the hook.

### 4.3 RAII Byte Patching (`pl::memory::PatchHandle`)
```cpp
#include <pl/memory/Patch.hpp>

{
    // Apply ARM64 NOP (0x1F2003D5)
    pl::memory::PatchHandle patch(targetAddress, "1F 20 03 D5", "BypassCheck");
    // Code runs with patch applied...
} // Automatically reverts to original opcode upon exit
```

---

## 5. Multi-Version Engine & Golden RVA Tables (`ll::mcpe`)

Located in `#include <levilamroid/api/mcpe/Offsets.h>`.

### 5.1 Automated Version Identification
LeviLamroid probes the loaded ELF `PT_NOTE` segment via `dl_iterate_phdr` to match the 20-byte SHA-1 GNU Build ID:
- `56de9eed077631e03a31f4f58eb2f0e00071d335` $\rightarrow$ Bedrock `1.26.52.3`
- `19d675681604a80ce6a8ff35dfb66f287e0766e0` $\rightarrow$ Bedrock `1.26.51.1`
- `868e275cb295e9a275bb29d2258edc2f7dc48761` $\rightarrow$ Bedrock `1.26.45.1`
- `7ff153b6cba8d148e64c2ec582f3484f9ea4457e` $\rightarrow$ Bedrock `1.26.44.3`

### 5.2 Version Accessors
- `const EngineOffsets& ll::mcpe::getOffsets()`: Returns the active version's golden offset table.
- `std::string ll::mcpe::getDetectedVersion()`: Returns active version string (e.g. `"1.26.52.3"`).
- `extern "C" const char* ll_mcpe_get_detected_version()`: Returns active version string pointer for C and dlsym consumers.

### 5.3 Verified Golden RVAs for 1.26.52.3 (Android ARM64)

```cpp
// UI Screen Tech Stack & Screen Controllers
.getTechStackForScreen    = 0x0974E844,
.startMenuOnOpen          = 0x0A2DF314,
.startMenuTick            = 0x0A2E1FF4,

// Menu Button Direct RVAs
.playButton               = 0x0A31344C,
.settingsButton           = 0x0A3171AC,
.realmsButton             = 0x0A3174DC,
.dressingRoomButton       = 0x0A31815C,
.marketplaceButton        = 0x0A3187A4,
.menuExit                 = 0x0A567540,

// RenderDragon Graphics Pipeline
.inGamePlayScreenRender              = 0x09ACC828,
.inGamePlayScreenRenderLevel         = 0x09ACD4C8,
.inGamePlayScreenRenderLevelPrep     = 0x09ACC534,
.gameRendererPrepareFrame            = 0x0B2EB50C,
.levelRendererPlayerPreRenderUpdate  = 0x0B29B620,
.whiskerDisplayRender                = 0x0B33126C,
.clientNetworkHandlerHandle          = 0x09EE5908,
.loopbackPacketSenderSendToServer    = 0x0C68C410,
.glClearGotRva                       = 0x132893d8,

// 2D UI Drawing Methods
.uiDrawImage              = 0x0B25099C,
.uiFlushImages            = 0x0B251D70,
.uiFillRectangle          = 0x0B25232C,
.uiDrawRectangle          = 0x0B2536FC,
.uiDrawText               = 0x0B2507DC,
.uiFlushText              = 0x0B2508DC,
.uiDrawNineslice          = 0x0B25198C,
.uiGetScreenContext       = 0x0B2541E8,

// Player & Gameplay
.playerNormalTick         = 0x0F9E26C0,
.gameModeUseItem          = 0x0F8A2EB0,
.clientInstanceUpdate     = 0x098036A4,
.localPlayerApplyTurnDelta = 0x0AAC9774,
.tessellatorBegin         = 0x0A76E7F8,
.blockSourceGetBlock      = 0x0FB91918,
.blockSourceGetBrightness = 0x0FB92F9C,
.commandAutocomplete     = 0x0EBEAE44,

// Audio Engine RVAs
.soundEnginePlay          = 0x0B79F7F4,
.soundEnginePlayUI        = 0x0B79F978,
.soundEnginePlayAt        = 0x0B79FDE0,
.soundEngineStop          = 0x0B79FF1C,

// Item & Block Subsystems (Verified Entry RVAs)
.itemRegistryRegisterItem               = 0x0FBB0064,
.itemRegistryGetItem                    = 0x0FBC1640,
.itemRegistryPostInitHook               = 0x0FBB77DC,
.itemConstructor                        = 0x0FF7D35C,
.itemSetIconInfo                        = 0x0FF83DDC,
.itemVtable                             = 0x1308A650,
.itemStackVtable                        = 0x1308BE60,
.itemInstanceVtable                     = 0x1308AC00,
.itemStackCopyCtor                      = 0x0FF9DB08,
.vanillaItemsServerInitCreativeCallback = 0x0FF5A2D4,
.vanillaItemsApplyHardcodedLegacyIcons  = 0x0FF43810,
.creativeItemRegistryReset              = 0x0FBC3604,
.creativeItemInitializerCtor            = 0x0FFFC988,
.playerGetSelectedItem                  = 0x0F9F7BE4,
.containerGetItem                       = 0x0F6EFE7C,
.containerSetItem                       = 0x0F6EFEB8,
.blockItemConstructor                   = 0x10009DC4,
.blockItemVtable                        = 0x13099938, // _ZTV9BlockItem
.blockItemUseOn                         = 0x1000B300, // BlockItem::_useOn
.blockSourceSetBlock                    = 0x0FB99330,
.blockTypeRegistryGetInstance           = 0x103E33A8,
.blockTypeRegistryLookupByName          = 0x103E5C9C,
.blockTypeRegistryLookupByNameImpl      = 0x103E7220,
.blockTypeRegistryGetDirectAccessBlocks = 0x103E7800,
.blockDefinitionGroupInitBlockTypeFromDefinition = 0x104826B0,
.blockDefinitionGroupInitBlockFromDefinition     = 0x10482AE0,
.registerItemAndBlockDefinitions        = 0x0FBC3A2C,

// ScreenController Property & Event Registration RVAs
.screenControllerRegisterButtonEventHandler = 0x09B18244,
.screenControllerBindString             = 0x09B1CBA0,
.screenControllerBindStringForAnyCollection = 0x09B1D084,

// VTable Slots
.clientInstanceLocalPlayerSlot      = 32,
.clientInstanceIsShowingMenuSlot    = 151,
.levelGetBlockDefinitionGroupSlot   = 28,  // +0xE0
.clientInstanceSceneFactorySlot     = 226, // 0x710
.clientInstanceSceneStackSlot       = 236, // 0x760
.sceneStackPushScreenSlot           = 26,  // 0xD0
.sceneStackSchedulePopSlot          = 27,  // 0xD8

// Scene Factory Abstract Creation RVAs
.sceneFactoryCreateUIScene          = 0x09AE87E8,
.sceneFactoryCreateScreenAbstract   = 0x09AEB298
```

### 5.4 Verified Golden RVAs for 1.26.45.1 (Android ARM64)

```cpp
// UI Screen Tech Stack & Screen Controllers
.getTechStackForScreen    = 0x0938B714,
.startMenuOnOpen          = 0x09EBD95C,
.startMenuTick            = 0x09EC05D8,

// Menu Button Direct RVAs
.playButton               = 0x09EF3D64,
.settingsButton           = 0x09EF72C0,
.realmsButton             = 0x09EF75F0,
.dressingRoomButton       = 0x09EF8270,
.marketplaceButton        = 0x09EF88A4,
.menuExit                 = 0x0A15248C,

// RenderDragon Graphics Pipeline
.inGamePlayScreenRender              = 0x0974066C,
.inGamePlayScreenRenderLevel         = 0x0974097C,
.inGamePlayScreenRenderLevelPrep     = 0x0973E864,
.gameRendererPrepareFrame            = 0x0ADD98DC,
.levelRendererPlayerPreRenderUpdate  = 0x0AE27C64,
.whiskerDisplayRender                = 0x0AE70680,
.clientNetworkHandlerHandle          = 0x09ADC770,
.loopbackPacketSenderSend            = 0x0C2E26EC,
.loopbackPacketSenderSendToServer    = 0x0C2E26FC,
.glClearGotRva                       = 0x124C9688,

// 2D UI Drawing Methods
.uiDrawImage              = 0x0AD845F0,
.uiFlushImages            = 0x0AD859C4,
.uiFillRectangle          = 0x0AD85F80,
.uiDrawRectangle          = 0x0AD87350,
.uiDrawText               = 0x0AD84430,
.uiFlushText              = 0x0AD84530,
.uiDrawNineslice          = 0x0AD855E0,
.uiGetScreenContext       = 0x0AD87E3C,

// Player & Gameplay
.playerNormalTick         = 0x0F0B1680,
.gameModeUseItem          = 0x0EF75F40,
.gameModeUseItemOn        = 0x0EF72684,
.clientInstanceUpdate     = 0x0943EC54,
.localPlayerApplyTurnDelta = 0x0A6430D8,
.tessellatorBegin         = 0x0A76E780,
.blockSourceGetBlock      = 0x0F253C8C,
.blockSourceGetBrightness = 0x0F253ECC,
.commandAutocomplete     = 0x0E536964,

// Audio Engine RVAs
.soundEnginePlay          = 0x0B2848E0,
.soundEnginePlayUI        = 0x0B284A5C,
.soundEnginePlayAt        = 0x0B284EC4,
.soundEngineStop          = 0x0B285000,

// Item & Block Subsystems (Verified Entry RVAs)
.itemRegistryRegisterItem               = 0x0F2AC5DC,
.itemRegistryGetItem                    = 0x0F27CE28,
.itemRegistryPostInitHook               = 0x0F2B41D8,
.itemConstructor                        = 0x0F65A2D4,
.itemSetIconInfo                        = 0x0F65D824,
.itemVtable                             = 0x122E1F18,
.itemStackVtable                        = 0x122DE850,
.itemInstanceVtable                     = 0x122DB0F8,
.itemStackCopyCtor                      = 0x0F63AB48,
.vanillaItemsServerInitCreativeCallback = 0x0F6370A4,
.vanillaItemsApplyHardcodedLegacyIcons  = 0x0F62065C,
.creativeItemRegistryReset              = 0x0F2BF7BC,
.creativeItemInitializerCtor            = 0x0F6D93AC,
.creativeItemInitializerAddCreativeServer = 0x0F6D9374,
.playerGetSelectedItem                  = 0x0F0C67A8,
.containerGetItem                       = 0x0EF7AA90,
.containerSetItem                       = 0x0EF7A3F4,
.blockItemConstructor                   = 0x0F6E4D0C,
.blockItemVtable                        = 0x122F10F0, // _ZTV9BlockItem
.blockItemUseOn                         = 0x0F6E645C, // BlockItem::_useOn
.blockSourceSetBlock                    = 0x0F25B948,
.blockTypeRegistryGetInstance           = 0x0FB149D8,
.blockTypeRegistryLookupByName          = 0x0FB33684,
.blockTypeRegistryLookupByNameImpl      = 0x0FB337B0,
.blockTypeRegistryGetDirectAccessBlocks = 0x0FB33BDC,
.blockDefinitionGroupInitBlockTypeFromDefinition = 0x0FB13E64,
.blockDefinitionGroupInitBlockFromDefinition     = 0x0FB1429C,
.registerItemAndBlockDefinitions        = 0x0F2BF9F0,

// ScreenController Property & Event Registration RVAs
.screenControllerRegisterButtonEventHandler = 0x096D9D38,
.screenControllerBindString             = 0x096DED60,
.screenControllerBindStringForAnyCollection = 0x096DEF90,

// VTable Slots
.clientInstanceLocalPlayerSlot      = 32,
.clientInstanceIsShowingMenuSlot    = 150,
.levelGetBlockDefinitionGroupSlot   = 28,  // +0xE0
.clientInstanceSceneFactorySlot     = 224,
.clientInstanceSceneStackSlot       = 235,
.sceneStackPushScreenSlot           = 26,  // +0xD0
.sceneStackSchedulePopSlot          = 27,  // +0xD8

// Scene Factory Abstract Creation RVAs
.sceneFactoryCreateUIScene          = 0x0975E3B0,
.sceneFactoryCreateScreenAbstract   = 0x097609CC
```

---

## 6. Reverse Engineering Discovery Status: Items, Blocks & Components

> [!IMPORTANT]
> ### 🔍 Transparency on What is Known vs. What Remains Undiscovered
> While core engine symbols and hook trampolines have been established, the internal architecture of **Custom Items, Food Components, Spawn Eggs, and Custom Blocks is NOT completely discovered**. Modders must understand the boundary between verified facts and active research.

### 6.1 What is Confirmed & Verified
1. **`Item::Item` Base Constructor (`0x0FF7D35C`)**:  
   AAPCS64 registers `(x0: this, x1: const std::string& name, x2: short id)`. Successfully registers an entry in `ItemRegistry`.
2. **`BlockItem::BlockItem` Constructor (`0x10009DC4`) & VTable (`0x13099938`)**:  
   Overwrites `*this` vtable to `_ZTV9BlockItem` (`0x13099938`) and stores the associated `BlockLegacy*` pointer at offset `this + 0x138` (decimal 312).
3. **`BlockItem::_useOn` (`0x1000B300`)**:  
   Direct engine entry point for placing a block item at target coordinates.
4. **`GameMode::useItem` Interception (`0x0F8A2EB0`)**:  
   Hooking this function cleanly intercepts right-click / tap item interactions for custom items without crashing.
5. **Creative Inventory Visibility Rules**:  
   Bedrock 1.21+ rejects and discards custom item JSONs containing `"minecraft:block_placer"` if the referenced block definition has not been registered in the internal schema beforehand. Removing `"minecraft:block_placer"` and categorizing under `itemGroup.name.stone` allows custom block items to display cleanly in creative inventory.

### 6.2 What is NOT Completely Discovered (Active Reverse Engineering)
1. **`FoodItemComponent` Struct Layout & Attachment**:  
   In Bedrock 1.26.x ARM64, items with `useAnimation == 1` (EAT) query `Item::getFoodComponent()`. The exact C++ struct layout of `FoodItemComponent` and how components attach to raw `Item` instances at runtime is not fully mapped. Direct offset poking into `Item` memory (e.g. `+0x1C4`, `+0x1CC`) smashes internal STL pointers and causes instant `SIGSEGV` crashes.
2. **`SpawnEggItem` Dynamic Downcast & `getColor()`**:  
   Assigning custom items to creative group `"itemGroup.name.mobEgg"` causes Bedrock's item renderer to dynamic-cast `Item*` to `SpawnEggItem*` and call `getColor()`. Because custom items lack the `SpawnEggItem` vtable, this results in an unmapped function jump and immediate crash.
3. **`BlockDefinitionGroup` JSON Schema & State Permutation Graphs**:  
   Modern Bedrock does not permit ad-hoc `new BlockType()` allocations. Permutations and states are loaded through `BlockDefinitionGroup::initBlockTypeFromDefinition` (`0x104826B0`) and `initBlockFromDefinition` (`0x10482AE0`). The full internal parameter descriptors and how custom state permutations map to network runtime IDs are actively being investigated.

### 6.3 Current Workarounds & Safe Modding Guidelines
- **Food Consumption**: Intercept in `hooked_gameModeUseItem` (or listen to `ll::event::PlayerUseItemEvent`). Decrement stack count (`itemStack[0x22]--`), apply potion effects (`/effect @s instant_health`), play sounds (`/playsound random.burp`), and return `true`. **Never fall through to vanilla `GameMode::useItem`**.
- **Spawn Eggs**: Map creative group to `"itemGroup.name.misc"` instead of `"mobEgg"`. Intercept in `GameMode::useItem` and summon the entity using `/summon` and command bridges.
- **Block Items**: Ingest `blocks/` before `items/` in [AddonIngestion.cpp](file:///home/devroom/LeviLamroid/src/resource/AddonIngestion.cpp). Remove `"minecraft:block_placer"` from the item JSON. Set `vtable = 0x13099938` and bind `BlockLegacy*` at `+0x138`.

---

## 7. Thread-Safe Event Bus System (`ll::event::EventBus`)

Located in `#include <ll/api/event/EventBus.h>`.

### 7.1 Subscribing to Events
```cpp
#include <ll/api/event/EventBus.h>
#include <ll/api/event/ui/UIEvents.h>

auto listener = ll::event::EventBus::getInstance().subscribe<ll::event::StartMenuOpenEvent>([](auto& ev) {
    ll::Logger::getInstance().info("Start Menu opened!");
});
```

### 7.2 Publishing Custom Events
```cpp
struct CustomModEvent : public ll::event::Event {
    int data;
    CustomModEvent(int d) : data(d) {}
};

ll::event::EventBus::getInstance().publish(CustomModEvent(42));
```

---

## 8. Complete Built-in Event Catalog

### 8.1 UI Events (`#include <ll/api/event/ui/UIEvents.h>`)
- `ll::event::StartMenuOpenEvent`: Fired when title / start screen opens.
- `ll::event::PauseMenuOpenEvent`: Fired when in-game pause screen opens (`pause_screen`).
- `ll::event::PauseMenuCloseEvent`: Fired when user resumes or exits the pause screen.
- `ll::event::PlayButtonClickedEvent`: Fired when user clicks "Play".
- `ll::event::SettingsButtonClickedEvent`: Fired when user clicks "Settings".
- `ll::event::RealmsButtonClickedEvent`: Fired when user clicks "Realms".
- `ll::event::MarketplaceButtonClickedEvent`: Fired when user clicks "Marketplace".
- `ll::event::DressingRoomButtonClickedEvent`: Fired when user clicks "Dressing Room".
- `ll::event::MenuExitEvent`: Fired when returning to the main menu.
- `ll::event::ScreenTechStackQueryEvent`: Queries UI technology stack (JSON UI vs OreUI/Cohtml).

### 8.2 Render Events (`#include <ll/api/event/render/RenderEvents.h>`)
- `ll::event::RenderLevelEvent`: Fired inside `InGamePlayScreen::_renderLevel` (`0x09ACD4C8`) for 3D world geometry, entity ESP, waypoints, and block wireframes.
- `ll::event::UIRenderEvent`: Fired inside `InGamePlayScreen::render` (`0x09ACC828`) and `MinecraftUIRenderContext::flushImages` (`0x0B251D70`) for 2D in-game HUDs and overlays.
- `ll::event::RenderFramePrepareEvent`: Fired inside `GameRenderer::_prepareFrame` (`0x0B2EB50C`) for camera transforms and projection matrices.
- `ll::event::GlClearEvent`: Fired inside `glClear` before frame rendering.
- `ll::event::CubemapRenderEvent`: Fired before panorama cubemap passes. Can cancel vanilla cubemap drawing.
- `ll::event::ProgressBarRenderEvent`: Fired during world loading progress bar passes.

### 8.3 Sound Events (`#include <ll/api/event/sound/SoundEvents.h>`)
- `ll::event::SoundPlayEvent`: Fired when any 3D/2D sound plays. Exposes `soundName`, `position`, `volume`, and `pitch`.
- `ll::event::LevelSoundEventOccurred`: High-level Minecraft gameplay sound events.

### 8.4 Player Events (`#include <ll/api/event/player/PlayerEvents.h>`)
- `ll::event::PlayerMoveEvent`: Fired during local player movement (`LocalPlayer::move`). Exposes player pointer and 3D delta vector `Vec3`. Cancellable.
- `ll::event::PlayerTickEvent`: Fired every player tick (`Player::normalTick`). Cancellable.
- `ll::event::PlayerUseItemEvent`: Fired when a player uses or consumes an item stack (`GameMode::useItem` / `useItemOn`). Cancellable.
- `ll::event::PlayerAttackEvent`: Fired when the player swings or attacks an entity.
- `ll::event::PlayerRespawnEvent`: Fired when a player respawns after death.

### 8.5 World & Entity Events (`#include <ll/api/event/world/WorldEvents.h>`, `<ll/api/event/entity/ActorEvents.h>`)
- `ll::event::LevelTickEvent`: Fired every engine level tick (`Level::tick`). Cancellable.
- `ll::event::BlockBreakEvent`: Fired when a block is broken (`GameMode::destroyBlock`). Cancellable.
- `ll::event::BlockPlaceEvent`: Fired when a block is placed (`BlockSource::setBlock`). Cancellable.
- `ll::event::ActorHurtEvent`: Fired when an entity takes damage (`Actor::hurt`). Cancellable.
- `ll::event::ActorDieEvent`: Fired when an entity dies (`Actor::die`).

---

## 9. Client Fabric API & RenderDragon Graphics Subsystem (`ll::event`)

Headers: `#include "fabric/ClientSignatures.hpp"`, `#include "ll/api/event/render/RenderEvents.h"`

### 9.1 In-World 3D Rendering Pass (`RenderLevelEvent`)
```cpp
ll::event::EventBus::getInstance().subscribe<ll::event::RenderLevelEvent>([](auto& ev) {
    void* screenContext = ev.getScreenContext();
    // Render 3D tracers, waypoints, bounding boxes, entity ESP
});
```

### 9.2 In-Game 2D HUD Pass (`UIRenderEvent`)
```cpp
ll::event::EventBus::getInstance().subscribe<ll::event::UIRenderEvent>([](auto& ev) {
    void* screenContext = ev.getScreenContext();
    // Draw 2D custom overlays, armor status, coordinates, crosshairs
});
```

### 9.3 Frame Preparation & Camera Interpolation
- **`GameRenderer::_prepareFrame`** (`0x0B2EB50C`): Dispatches `RenderFramePrepareEvent` for camera frustum updates.
- **`LevelRendererPlayer::preRenderUpdate`** (`0x0B29B620`): Hooked for smooth camera interpolation, view bobbing, and free-cam.

---

## 10. Universal UI, Pause Menu & Screen Navigation Subsystem (`ll::ui`)

Header: `#include "ll/api/ui/UI.h"`

### 10.1 Pause Menu APIs (Issue #1 Resolution)
- `void ll_ui_on_pause_menu_open(void (*cb)(void* pauseScreenController))`: Register a callback fired when the in-game pause screen opens (`pause_screen`).
- `void ll_ui_on_pause_menu_close(void (*cb)())`: Register a callback fired when the user resumes or exits the pause screen.
- `bool ll_ui_is_in_pause_menu()`: Returns `true` if the client is currently viewing the pause screen.

### 10.2 JSON UI Routing & Buttons
- `void ll_ui_force_json_ui(const char* name)`: Forces Bedrock's screen tech stack router (`getTechStackForScreen`) to return `0` (JSON UI) for the specified screen.
- `void ll_ui_register_button(const char* buttonName, void (*handler)(void* controller, void* propertyBag))`: Registers a custom callback for any button clicked in JSON UI across any menu. Button names are indexed by 32-bit FNV-1a hash.
- `bool ll_ui_open_screen(const char* screenName)`: Pushes a screen onto the active scene stack.
- `bool ll_ui_close_screen()`: Closes the current active screen.
- `const char* ll_ui_get_current_screen()`: Retrieves the identifier of the active screen.
- `void ll_ui_send_toast(const char* title, const char* message, const char* iconItem, int durationMs)`: Displays an animated notification toast on the HUD.

### 10.3 Engine State Queries
Header: `#include "levilamroid/LeviLamroid.h"`
- `bool ll::isInWorld()`: Returns `true` when playing in an active world. Continuously affirmed by golden tick hooks (`Player::normalTick`, `Level::tick`).
- `bool ll::isMainMenu()`: Returns `true` strictly while idling on the start menu.
- `bool ll::isLoading()`: Returns `true` while the engine is loading assets or world data.

---

## 11. Android Mobile Touch, HUD Buttons & Input (`pl::modmenu` & `pl::input`)

### 11.1 On-Screen Floating HUD Buttons (`pl::modmenu`)
Header: `#include <pl/ModMenu.hpp>`

```cpp
#include <pl/ModMenu.hpp>

pl::modmenu::ButtonInfo btn;
btn.buttonId = "my_custom_button";
btn.moduleId = "MyMod";
btn.displayName = "Activate Feature";
btn.label = "Act";
btn.behavior = pl::modmenu::ButtonBehavior::Click;
btn.defaultVisible = true;
btn.stylePreset = pl::modmenu::ButtonStylePreset::Keycap;
btn.normalBgColor = 0xDD1E1B4B;
btn.activeBgColor = 0xFF7C3AED;
btn.onEvent = [](std::string_view id, pl::modmenu::ButtonEvent event, float) {
    if (event == pl::modmenu::ButtonEvent::Click) {
        // Handle touch button click
    }
};
pl::modmenu::registerButton(btn);
```

### 11.2 Touch & Hardware Input Interception (`pl::input`)
Header: `#include <pl/Input.hpp>`
- `pl::input::registerTouchCallback(bool (*cb)(const TouchEvent&))`: Intercepts multi-touch screen coordinates. Returning `true` consumes the event, preventing Minecraft camera rotation.
- `pl::input::registerKeyCallback(bool (*cb)(const KeyEvent&))`: Intercepts physical keyboard inputs (e.g. Bluetooth keyboards or key shortcuts).

---

## 12. Custom Items, Custom Blocks & Addon Ingestion (`ll::item`, `ll::block`, `ll::addon`)

### 12.1 Unified BP-in-RP Addon Ingestion
LeviLamroid natively parses Behavior Pack JSON files (`items/*.json`, `blocks/*.json`, `recipes/*.json`) placed directly inside the mod's single Resource Pack directory:
- **Scan Order**: Scans `blocks/` before `items/` to guarantee that block references exist when items are created.
- **Component Sanitization**: Automatically strips `"minecraft:block_placer"` and remaps creative groups (`"itemGroup.name.mobEgg"` $\rightarrow$ `"misc"`, `"craftingTable"` $\rightarrow$ `"stone"`) to prevent engine crashes.
- **No Experimental Prompts**: Bypasses the need for world-level Behavior Pack toggles, keeping Xbox achievements active.

### 12.2 Programmatic Item Registration API
Header: `#include "levilamroid/api/item/ItemRegistry.h"`

extern "C" {
    bool ll_item_register_full(const char* id, const char* name, const char* icon, int cat, const char* grp, int stack, bool offhand, bool handEq, short maxDmg);
    bool ll_item_give_player(void* playerPtr, const char* identifier, int count, int aux);
    void* ll_item_get_native(const char* identifier);
    int   ll_item_stack_get_count(const void* itemStack);
    bool  ll_item_stack_set_count(void* itemStack, uint8_t count);
    short ll_item_stack_get_aux(const void* itemStack);
    bool  ll_item_stack_is_valid(const void* itemStack);
    void* ll_item_stack_get_item(const void* itemStack);
}
```

### 12.3 Programmatic Block Registration & Inspection API
Header: `#include "levilamroid/api/block/BlockTypeRegistry.h"`

```cpp
extern "C" {
    bool  ll_block_register(const char* identifier, const char* displayName, const char* textureName, int category, const char* creativeGroup, float destroyTime, float explosionResistance, uint8_t lightEmission);
    bool  ll_block_register_with_base(const char* identifier, const char* displayName, const char* textureName, int category, const char* creativeGroup, float destroyTime, float explosionResistance, uint8_t lightEmission, const char* baseBlock);
    void* ll_block_source_get_block(void* blockSource, int x, int y, int z);
    bool  ll_block_source_set_block(void* blockSource, int x, int y, int z, void* permutation, int updateFlags);
    int   ll_block_source_get_brightness(void* blockSource, int x, int y, int z);
}
```

---

## 13. Minecraft Bedrock Engine Class API Reference (`mc/` & `mcpe/`)

### 13.1 `ClientInstance`
Header: `#include <mc/client/ClientInstance.h>`
- `virtual LocalPlayer* getLocalPlayer() const`: Local client player pointer (Slot 32).
- `virtual bool isShowingMenu() const`: True if any UI screen or pause menu is displayed (Slot 151).
- `virtual LevelRenderer* getLevelRenderer() const`: World renderer pointer.
- `virtual PacketSender& getPacketSender() const`: Network packet sender.
- `virtual Level* getLevel() const`: World level pointer.

### 13.2 `Player` & `LocalPlayer`
Headers: `#include <mc/world/actor/player/Player.h>`, `<mc/world/actor/player/LocalPlayer.h>`
- `virtual void normalTick()`: Actor update tick (`0x0F9E26C0`).
- `virtual void move(const Vec3& delta)`: Applies movement displacement to local player.
- `const ItemStack& getSelectedItem() const`: Held item in active hotbar slot (`0x0F9F7BE4`).
- `ItemStack& getOffhandSlot()`: Offhand item reference.
- `Vec3 getEyePos() const noexcept`: 3D floating-point eye coordinates.
- `Vec3 getViewVector(float a = 1.0f) const noexcept`: Forward-facing look unit vector.

### 13.3 `BlockSource`
Header: `#include <mc/world/level/BlockSource.h>`
- `Block const& getBlock(BlockPos const& pos) const`: Returns discrete block permutation (`0x0FB91918`).
- `bool setBlock(BlockPos const& pos, Block const& block, int updateFlags = 3, void* actor = nullptr)`: Places block into world (`0x0FB99330`).

---

## 14. Inventory, Container & NBT Manipulation Subsystems (`ll::inventory`, `mc::CompoundTag`)

Headers: `#include "ll/api/inventory/ItemStack.h"`, `#include <mc/deps/nbt/CompoundTag.h>`

### 14.1 Inventory C ABI
```cpp
extern "C" {
    void* ll_player_get_inventory(void* player);
    bool ll_player_get_armor(void* player, int slot, void* outItemStack);
    bool ll_player_set_armor(void* player, int slot, const void* itemStack);
    bool ll_player_get_offhand(void* player, void* outItemStack);
    bool ll_player_set_offhand(void* player, const void* itemStack);
    int ll_player_get_selected_slot(void* player);
    bool ll_player_set_selected_slot(void* player, int slot);
    int ll_container_get_size(void* container);
    bool ll_container_get_item(void* container, int slot, void* outItemStack);
    bool ll_container_set_item(void* container, int slot, const void* itemStack);
    bool ll_container_clear_slot(void* container, int slot);
}
```

### 14.2 Binary NBT Serialization (`CompoundTag`)
```cpp
// Parses raw .mcstructure or NBT bytes (handles Gzip/Zlib automatically)
CompoundTag::BinaryNbtResult result = CompoundTag::fromBinaryNbt(rawBytes);
if (result) {
    CompoundTag* root = result.get();
}
```

---

## 15. Client Command System & Scheduling (`ll::command`, `ll::chrono`)

### 15.1 Client Command Registration
Header: `#include "ll/api/command/CommandRegistrar.h"`

```cpp
extern "C" {
    // Registers client command intercepted locally before network transmission
    bool ll_command_register(const char* name, const char* desc, int perm, void (*handler)(const char* args));
    bool ll_command_dispatch_client(const char* fullCommand);
}
```

### 15.2 Camera Orientation Control
- `void ll_camera_set_locked(bool locked)`: Freezes or unlocks camera pitch/yaw deltas via `LocalPlayer::applyTurnDelta`.
- `bool ll_camera_is_locked()`: Returns whether camera rotation is currently frozen.

### 15.3 GameTick Task Scheduler
Header: `#include "ll/api/chrono/GameTickScheduler.h"`
```cpp
ll::chrono::GameTickScheduler::getInstance().addSchedule(20, []() {
    // Runs after 20 game ticks (1 second)
});
```

---

## 16. Audio Systems (`levilamroid::sound::FmodAudioEngine` & `SoundEngine`)

### 16.1 Method A: Direct In-Memory FMOD Audio (`FmodAudioEngine`)
Header: `#include <levilamroid/api/sound/FmodSound.h>`  
Plays raw WAV/OGG/MP3 byte arrays in memory without touching disk or creating resource packs:
```cpp
levilamroid::sound::FmodAudioEngine::getInstance().playMemory(rawWavBytes, wavSize);
```

### 16.2 Method B: Engine Core `SoundEngine` & Sound Physics
Header: `#include <levilamroid/api/sound/SoundEngine.h>`  
Hooks `SoundEngine::play` (`0x0B79F7F4`), `SoundEngine::playUI` (`0x0B79F978`), and `SoundEngine::playAt` (`0x0B79FDE0`) for 3D acoustic occlusion, reverb, and spatial attenuation.

---

## 17. Virtual In-Memory Resource Packs & Asset Strategies (`levilamroid::resource`)

Header: `#include <levilamroid/api/resource/VirtualPackHub.h>`

- **Texture Dynamic Patching**: Overrides vanilla block textures (`textures/blocks/stone.png`) or item textures dynamically in RAM without modifying disk files.
- **Dynamic Atlas Injection**: Intercepts `textures/item_texture.json` and `textures/terrain_texture.json`, appending custom atlas entries with `"resource_pack_name": "vanilla"` so custom textures resolve in both 2D UI and 3D RenderDragon passes.

---

## 18. Core Engine Services, Threading & Logging (`ll::service`, `ll::thread`, `ll::Logger`)

### 18.1 Global Services (`ll::service`)
Header: `#include <ll/api/service/Bedrock.h>`
- `ClientInstance* ll::service::getClientInstance()`
- `Level* ll::service::getLevel()`
- `LocalPlayer* ll::service::getLocalPlayer()`

### 18.2 Asynchronous Thread Pool (`ll::thread::ThreadPoolExecutor`)
Header: `#include <ll/api/thread/ThreadPoolExecutor.h>`
```cpp
ll::thread::ThreadPoolExecutor pool(4);
pool.execute([]() {
    // Background worker task
});
```

### 18.3 Multi-Target Logging (`ll::Logger`)
Header: `#include <ll/api/Logger.h>`
- `ll::Logger::getInstance().info(...)`
- `ll::Logger::getInstance().warn(...)`
- `ll::Logger::getInstance().error(...)`
- Outputs formatted text simultaneously to Android Logcat (`__android_log_print`) and file logs.

---

## 19. LeviLamina Migration Guide & Android Parity Matrix

| Windows LeviLamina Feature | LeviLamroid Android ARM64 Equivalent | Compatibility Status |
|---|---|---|
| `ll::mod::NativeMod` | `ll::mod::NativeMod` | 100% Drop-in Parity |
| `LL_REGISTER_MOD` | `LL_REGISTER_MOD` | 100% Drop-in Parity |
| `ll::event::EventBus` | `ll::event::EventBus` | 100% Drop-in Parity |
| `ll::service::*` | `ll::service::*` | 100% Drop-in Parity |
| `ll::memory::hook` | `ll::memory::hook` (AArch64) | 100% Drop-in Parity |
| Windows Memory Pointers | Android ARM64 RVAs / Build ID tables | Handled automatically |
| Windows DirectInput | `pl::input` (Touch / Android View) | Android-native extensions |
| ImGui DirectX11/12 | ImGui OpenGL ES 3.0 | Android-native extensions |

---

## 20. Starter Template & Distribution Packaging (`v1.0.0`)

A complete starter template is provided in [`template/`](template/):
- **C++20 Build Configuration** (`CMakeLists.txt`)
- **Mod Descriptor** (`manifest.json`)
- **Lifecycle Implementation** (`LL_REGISTER_MOD`)
- **Event Listeners & Floating HUD Buttons**
- **One-Click Packaging Script** (`package.sh`)

```bash
cd template
bash package.sh
```
This produces both `build/<mod_name>.so` and `dist/<mod_name>.levipack`, automatically deploying to active LeviLauncher profiles.
