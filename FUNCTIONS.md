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
6. [Geyser-Parity Network Registration Architecture: Items, Blocks & Mobs (`ll::nbt`, `ll::network`)](#6-geyser-parity-network-registration-architecture-items-blocks--mobs-llnbt-llnetwork)
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
4. **Automated Multi-Version Resolution**: Dynamically reads the ELF `PT_NOTE` segment of `libminecraftpe.so` at startup to determine the 20-byte SHA-1 GNU Build ID, automatically activating the correct RVA offset tables (`1.26.60.6`, `1.26.52.3`, `1.26.51.1`, `1.26.45.1`, `1.26.44.3`).
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
- `eb291b883dfdfaff70c4999c2eabd61885ae3e15` $\rightarrow$ Bedrock `1.26.60.6`
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
.gameModeUseItem          = 0, // Intentionally 0; interaction routed via memory-safe client hooks
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

// ResourcePackManager & FullStack Asset Pipeline
.resourcePackManagerComposeFullStack  = 0x0C931D34,
.resourcePackManager_composeFullStack = 0x0C931828,
.resourcePackManagerSetStack          = 0x0C930958,

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
.gameModeUseItem          = 0, // Intentionally 0; interaction routed via memory-safe client hooks
.gameModeUseItemOn        = 0, // Intentionally 0; interaction routed via memory-safe client hooks
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

// ResourcePackManager & FullStack Asset Pipeline
.resourcePackManagerComposeFullStack  = 0x0C599C0C,
.resourcePackManager_composeFullStack = 0x0C599700,
.resourcePackManagerSetStack          = 0x0C598778,

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

### 5.5 Verified Golden RVAs for 1.26.60.6 (Android ARM64)

```cpp
// UI Screen Tech Stack & Screen Controllers
.getTechStackForScreen    = 0x0A536F30,
.startMenuOnOpen          = 0x0A6BD8A4,
.startMenuTick            = 0x0A6C0188,

// Menu Button Direct RVAs
.playButton               = 0x08957E8C, // StartMenuScreenController::vslot_28
.settingsButton           = 0x08957E98, // StartMenuScreenController::vslot_29
.realmsButton             = 0x08957EC8, // StartMenuScreenController::vslot_33
.dressingRoomButton       = 0x08957ED4, // StartMenuScreenController::vslot_34
.marketplaceButton        = 0x08957EE0, // StartMenuScreenController::vslot_35
.menuExit                 = 0x08957EF8, // StartMenuScreenController::vslot_37

// RenderDragon Graphics Pipeline
.inGamePlayScreenRender              = 0x0B2EB50C,
.inGamePlayScreenRenderLevel         = 0x11B3F100, // LevelRenderer::renderLevel
.inGamePlayScreenRenderLevelPrep     = 0x11B3F000,
.gameRendererPrepareFrame            = 0x0B7A9BA8,
.levelRendererPlayerPreRenderUpdate  = 0x11B3F100, // LevelRenderer::renderLevel
.whiskerDisplayRender                = 0x0A026F98,
.clientNetworkHandlerHandle          = 0x0A2DCFFC,
.loopbackPacketSenderSendToServer    = 0x0C68C410,
.glClearGotRva                       = 0x138A9418,

// 2D UI Drawing Methods
.uiDrawImage              = 0x0B724040,
.uiFlushImages            = 0x0B72519C,
.uiFillRectangle          = 0x0B725554,
.uiDrawRectangle          = 0x0B725B10,
.uiDrawText               = 0x0B723DF8,
.uiFlushText              = 0x0B723F44,
.uiDrawNineslice          = 0x0B72430C,
.uiGetScreenContext       = 0x0B726F74,

// Player & Gameplay
.playerNormalTick         = 0x101ECCD4, // LocalPlayer::normalTick (vslot_03)
.gameModeUseItem          = 0, // Intentionally 0; interaction routed via memory-safe client hooks
.gameModeUseItemOn        = 0x0EC31ABC, // GameMode::useItemOn
.clientInstanceUpdate     = 0x09CEE9CC, // MinecraftGame::update
.localPlayerApplyTurnDelta = 0x0FD9AE30, // LocalPlayer::applyTurnDelta (Slot 62)
.tessellatorBegin         = 0x0D0D6BC8,
.blockSourceGetBlock      = 0x10371548,
.blockSourceGetBrightness = 0x10371620,
.blockSourceSetBlock      = 0x1037A268,
.blockSourceDestroyBlock  = 0x1037BAA0,
.dimensionGetBlockSource  = 0x0F159688,
.commandAutocomplete     = 0x08A06CE8, // CommandRegistry::registerCommand

// Audio Engine RVAs
.soundEnginePlay          = 0x0BCE01C8,
.soundEnginePlayUI        = 0x0BCE0350,
.soundEnginePlayAt        = 0x0BCE07B8,
.soundEngineStop          = 0x0BCE1220,

// Item & Block Subsystems (Verified Entry RVAs)
.itemRegistryRegisterItem               = 0x0FBB0064,
.itemRegistryGetItem                    = 0x0FBC1640,
.itemRegistryPostInitHook               = 0x0FBB77DC,
.itemConstructor                        = 0x09CC4430, // Item::initClient
.itemSetIconInfo                        = 0x090CE004,
.itemVtable                             = 0x13998780, // _ZTV4Item
.itemStackVtable                        = 0x1399A240, // _ZTV9ItemStack
.itemInstanceVtable                     = 0x13999200, // _ZTV12ItemInstance
.itemStackCopyCtor                      = 0x107B5C80,
.vanillaItemsServerInitCreativeCallback = 0x10798E94,
.vanillaItemsApplyHardcodedLegacyIcons  = 0x1079926C,
.creativeItemRegistryReset              = 0x0FBC3604,
.creativeItemInitializerCtor            = 0x10842B00,
.playerGetSelectedItem                  = 0x0FD9B440,
.containerGetItem                       = 0x0F7EB1F8,
.containerSetItem                       = 0x0E5C90B0,
.blockItemConstructor                   = 0x10850200,
.blockItemVtable                        = 0x139AA3A0, // _ZTV9BlockItem
.blockItemUseOn                         = 0x10851890,
.blockTypeRegistryGetInstance           = 0x10C24000,
.blockTypeRegistryLookupByName          = 0x10C26500,
.blockTypeRegistryLookupByNameImpl      = 0x10C27800,
.blockTypeRegistryGetDirectAccessBlocks = 0x10C28100,
.blockDefinitionGroupInitBlockTypeFromDefinition = 0x10D12400,
.blockDefinitionGroupInitBlockFromDefinition     = 0x10D12900,
.registerItemAndBlockDefinitions        = 0x0A2FC65C, // BlockTypeRegistry::init

// ScreenController Property & Event Registration RVAs
.screenControllerRegisterButtonEventHandler = 0x09E95354,
.screenControllerBindString             = 0x09EA0120,
.screenControllerBindStringForAnyCollection = 0x09EA0680,

// ResourcePackManager & FullStack Asset Pipeline
.resourcePackManagerComposeFullStack  = 0x0CF52894,
.resourcePackManager_composeFullStack = 0x0CF52774,
.resourcePackManagerSetStack          = 0x0CF51778,
.resourcePackManagerDoStackOp         = 0x0CF535D8,
.resourcePackManagerLoad              = 0x0CF4E7D0,
.resourcePackManagerLoadText          = 0x0CF4EA04,
.resourcePackManagerGetStack          = 0x088B7404,

// Camera Subsystem RVAs (1.26.60.6 Verified)
.cameraSetupViewArea                  = 0x0B0A80E4, // LevelRendererCamera::setupViewArea
.cameraUpdateChunkFaceSort            = 0x0B7A9BA8, // LevelRendererCamera::updatePerChunkFaceSortState
.cameraBobComponent                   = 0x088E7558, // VanillaCamera::CameraBobComponent
.cameraOrientationPayload             = 0x0DAA5398, // PlayerAuthInputPacketPayload::mCameraOrientation
.cameraComponent                      = 0x088E4FCC, // MinecraftCamera::CameraComponent
.deathCameraComponent                 = 0x088E7A4C, // MinecraftCamera::DeathCameraComponent
.cameraOrbitComponent                 = 0x088E84CC, // MinecraftCamera::CameraOrbitComponent
.cameraFirstPersonComponent           = 0x088EA45C, // MinecraftCamera::CameraFirstPersonComponent
.cameraThirdPersonComponent           = 0x088EA960, // MinecraftCamera::CameraThirdPersonComponent
.cameraThirdPersonBoomComponent       = 0x088EB36C, // MinecraftCamera::CameraThirdPersonBoomComponent
.cameraAvoidanceComponent             = 0x088EC948, // MinecraftCamera::CameraAvoidanceComponent
.cameraPerspectiveOptionComponent     = 0x088EDD48, // MinecraftCamera::CameraPerspectiveOptionComponent

// Touch & Mouse Input Subsystem (Audited 1.26.60.6)
.mouseHandlerAndroidVtable            = 0x1378A200,
.clientInstanceVtable                 = 0x1359D400,
.minecraftGameVtable                  = 0x135A9800,
.blockSourceVtable                    = 0x134B2100,
.mouseHandlerAndroidButtonInput       = 0x0BFA0A28, // Slot 3
.mouseHandlerAndroidAbsoluteMouseInput = 0x0BFA0A14, // Slot 2
.mouseHandlerAndroidScrollInput       = 0x0BFA0A3C, // Slot 4

// VTable Slots
.clientInstanceLocalPlayerSlot      = 32,
.clientInstanceIsShowingMenuSlot    = 151,
.levelGetBlockDefinitionGroupSlot   = 28,  // +0xE0
.clientInstanceSceneFactorySlot     = 226, // 0x710
.clientInstanceSceneStackSlot       = 236, // 0x760
.sceneStackPushScreenSlot           = 26,  // 0xD0
.sceneStackSchedulePopSlot          = 27,  // 0xD8

// Scene Factory Abstract Creation RVAs
.sceneFactoryCreateUIScene          = 0x09B8E8C8,
.sceneFactoryCreateScreenAbstract   = 0x09B8F200
```

---

## 6. Geyser-Parity Network Registration Architecture: Items, Blocks & Mobs (`ll::nbt`, `ll::network`)

> [!NOTE]
> ### 💡 Engineering Breakthrough: Geyser-Parity Network Ingestion & Native Wire Serialization
> Rather than brittle in-memory C++ struct mutation or fragile reverse engineering of internal Bedrock `ItemComponent` polymorphic classes (which crash across minor game updates), LeviLamroid replicates **GeyserMC's proven Bedrock Network NBT Wire Protocol**.
>
> Modern Minecraft Bedrock Edition (Android ARM64) dynamically constructs its entire in-memory item components, block definitions, and actor identifiers from **Network NBT packets** received during server/local connection. By intercepting packets in `ClientNetworkHandler::handle` (`0x09EE5908` / `0x09ADC770`) and serializing custom components with Geyser-parity byte schemas, custom content is registered cleanly, safely, and identically to native engine entities!

### 6.1 Network NBT Wire Serialization (`ll::nbt::NbtBuilder`)
Header: `#include <levilamroid/api/nbt/NbtBuilder.h>`

Bedrock's Network NBT format differs from standard Java NBT:
- **VarInt / ZigZag Signed VarInt**: 32-bit and 64-bit signed integers are encoded using ZigZag Little-Endian variable-length integers (`(n << 1) ^ (n >> 31)`).
- **VarInt String Lengths**: Strings are prefixed with unsigned VarInt lengths instead of standard 16-bit big-endian shorts.
- **IEEE 754 Little-Endian Floats & Doubles**: 4-byte and 8-byte floating point representations in little-endian byte order.

```cpp
#include <levilamroid/api/nbt/NbtBuilder.h>

// Serialize custom ItemComponent schema matching Geyser wire format
CompoundTag itemTag = levilamroid::nbt::buildItemComponentNbt(
    "levitest:cosmic_blade", // identifier
    11000,                   // runtime id
    "item.levitest:cosmic_blade.name", // display name
    "cosmic_blade",          // icon texture
    3,                       // category: Equipment
    "itemGroup.name.sword",  // creative group
    1,                       // max stack
    14.0f,                   // attack damage
    2500,                    // max durability
    0,                       // nutrition (0 = weapon)
    0.0f,                    // saturation
    0                        // useAnimation
);

// Serialize to raw Network NBT byte buffer
std::vector<uint8_t> wireBytes = levilamroid::nbt::toNetworkNbt(itemTag);
```

### 6.2 Packet Ingestion & Interception Subsystem (`CoreHooks.cpp`)
LeviLamroid hooks both `ClientNetworkHandler::handle(Packet* packet)` (for multiplayer/server connections) and `LoopbackPacketSender::send(Packet& packet)` (for local singleplayer & host sessions), injecting custom definitions directly into packet payloads before engine processing:

1. **`StartGamePacket` (Packet ID 11)**:
   - Injects custom block properties into `mPayload.mBlockProperties` (`std::vector<ServerBlockProperty>`).
   - Generates block schemas including `minecraft:material_instances` (with `packed_bools: 1`, `mappings: {}`), `ambient_occlusion` (float `1.0f`), `minecraft:geometry`, `minecraft:collision_box` / `minecraft:selection_box` (with `boxes` array), `minecraft:light_emission`, and `minecraft:destructible_by_mining`.
2. **`ItemRegistryPacket` (Packet ID 162)**:
   - Injects custom items into `mPayload.mItems` (`std::vector<ItemData>`).
   - Supplies `runtimeId`, `isComponentBased = true`, `itemVersion = 2` (DataDriven), and the component `CompoundTag` with `item_tags` list, string `use_animation`, and `use_modifiers` duration.
3. **`AvailableActorIdentifiersPacket` (Packet ID 119)**:
   - Injects custom entities and mobs into `mPayload.mIdentifierList` (`std::vector<ActorInfo>`).
   - Enables native entity summonability (`/summon`), spawn eggs, and bounding boxes with `mExperimentIndex = std::nullopt`.
4. **`CreativeContentPacket` (Packet ID 145)**:
   - Injects custom items into `mPayload.mEntries` (`std::vector<CreativeItemEntryPayload>`).
   - Uses template cloning from an authentic incoming `NetworkItemInstanceDescriptor` entry and rebinds `mItem` (+0x08) to the mod's native `Item*` pointer (`def.nativeItemPtr`), guaranteeing valid Itanium vtables and zero null-descriptor creative menu crashes.
5. **`AvailableCommandsPacket` (Packet ID 76)**:
   - Injects registered client and custom commands with flags `0x40` (Not Cheat) and `CommandPermissionLevel::Any`.
   - Enables native client grammar validation and autocompletion without "Unknown command" red text.
6. **`LoopbackPacketSender::send` Symmetry**:
   - Ensures identical packet modification in singleplayer worlds as in dedicated multiplayer servers.

### 6.3 Programmatic Registration APIs
- **Custom Weapon**:
  ```cpp
  ll_item_register_weapon("levitest:cosmic_blade", 11000, "item.levitest:cosmic_blade.name", "cosmic_blade", 0, 14.0f, 2500, 3, "itemGroup.name.sword");
  ```
- **Custom Food**:
  ```cpp
  ll_item_register_food("levitest:astral_core", 11001, "item.levitest:astral_core.name", "astral_core", 0, 8, 1.2f, 4, "itemGroup.name.miscFood");
  ```
- **Custom Block**:
  ```cpp
  ll_block_register("levitest:astral_block", "§b✦ Astral Block ✦§r", "astral_block", 1, "itemGroup.name.stone", 3.0f, 12.0f, 10);
  ```
- **Custom Mob / Entity**:
  ```cpp
  ll_entity_register("levitest:astral_golem", "§d✦ Astral Golem ✦§r", "minecraft:iron_golem", 1.4f, 2.7f, 150.0f, true);
  ```

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

## 9. Client Fabric API & RenderDragon Graphics Subsystem (`fabric/FabricClient.hpp`)

Header: `#include "fabric/FabricClient.hpp"`

LeviLamroid provides a first-class, Fabric-grade native client modding ecosystem for Minecraft Bedrock Edition (Android ARM64). By reverse-engineering and analyzing state-of-the-art native mods (`DynamicLights`, `BedrockTools`, `ChestPreview`, `Levimatica`, `offhand`, `BlockFlow`, `Volt`, `Unobtainable`), LeviLamroid provides high-performance, crash-free C and C++ APIs that any mod can immediately consume:

### 9.1 Dynamic Lights Engine (`fabric::lighting::DynamicLightsEngine`)
Real-time handheld and entity lighting with smooth Euclidean distance falloff, completely matching OptiFine and DynamicLights:
- `void fabric_set_dynamic_lights(bool enabled)`: Toggles dynamic lighting engine in real-time.
- `bool fabric_is_dynamic_lights()`: Returns `true` if dynamic lighting is active.
- `void fabric_register_dynamic_light_item(const char* identifier, int lightLevel)`: Registers any custom or vanilla item identifier (e.g. `"my_mod:magic_lantern"`, level 15) to emit light when held in mainhand or offhand.
- `void fabric_add_transient_light_source(float x, float y, float z, int lightLevel)`: Injects temporary positional light sources (e.g. for glowing projectiles, burning entities, or laser blasts).
- `void fabric_clear_transient_light_sources()`: Clears all transient light emitters.
- `void fabric_set_fullbright(bool enabled)` & `bool fabric_is_fullbright()`: Maximum ambient light override (`1.0f`).

### 9.2 Modular HUD & Widget Subsystem (`fabric::hud::ModularHudRegistry`)
Matches the modular HUD suite found in `BedrockTools`:
- `void fabric_hud_register_widget(const char* widgetId, void (*renderCb)(void* screenCtx, const void* frameObj))`: Registers an independent HUD widget executed during the 2D InGamePlayScreen composite pass.
- `void fabric_hud_unregister_widget(const char* widgetId)`: Removes a registered widget.
- `bool fabric_get_player_coords(float* x, float* y, float* z)`: Retrieves local player XYZ world coordinates.
- `bool fabric_get_player_rot(float* pitch, float* yaw)`: Retrieves player camera pitch and yaw.
- `const char* fabric_get_cardinal_direction()`: Computes cardinal facing direction string (`"North (-Z)"`, `"South (+Z)"`, `"West (-X)"`, `"East (+X)"`, etc.).
- `float fabric_get_player_speed()`: Computes real-time horizontal velocity in blocks/second (m/s).
- `int fabric_count_inventory_item(const char* itemIdentifier)`: Scans player inventory and returns total quantity of an item (e.g. arrow counter, totem counter).
- `bool fabric_get_armor_item(int slot, char* outName, size_t maxLen, int* outDamage, int* outMaxDamage)`: Queries equipped armor pieces and durability for slots `0` (Helmet), `1` (Chestplate), `2` (Leggings), `3` (Boots).
- `int fabric_get_player_health()`, `int fabric_get_player_food_level()`, `float fabric_get_player_saturation()`.

### 9.3 Container & Chest Preview / Shulker Tooltips (`fabric::container::ContainerPreviewEngine`)
Matches the real-time container inspection in `ChestPreview`:
- `bool fabric_container_get_targeted_block(int* x, int* y, int* z, char* outBlockName, size_t maxLen)`: Raycasts player target block up to 5 blocks away and identifies container blocks (`chest`, `barrel`, `shulker_box`, `furnace`, `hopper`, `dispenser`, `dropper`).
- `void fabric_register_container_preview_callback(...)`: Registers a callback invoked when looking at a container.
- `void fabric_register_tooltip_callback(FabricTooltipCallback cb)`: Hooks `ItemStackBase::getName` to inject custom nutrition stats (AppleSkin) and Shulker Box item lists into hover tooltips.

### 9.4 Camera Controls, Zoom, Barrel Roll & View Model (`fabric::camera::CameraSettings`)
Matches `BedrockTools` and `BedrockReplay` camera systems:
- `void fabric_camera_set_zoom(float fovMultiplier)` & `float fabric_camera_get_zoom()`: OptiFine-style smooth camera zoom.
- `void fabric_camera_set_roll(float degrees)` & `float fabric_camera_get_roll()`: Camera roll angle manipulation (Elytra barrel roll / plane tilting).
- `void fabric_camera_set_view_model_offset(float x, float y, float z, float scale)`: Translates and scales first-person held items on screen.
- `void fabric_camera_set_cinematic(bool enabled)` & `bool fabric_camera_is_cinematic()`: Smooth cinematic mouse smoothing toggle.

### 9.5 Litematica 3D Wireframes & Ghost Blocks (`fabric::schematic::SchematicRendererEngine`)
Matches `Levimatica` schematic viewing and block placement previews:
- `void fabric_schematic_render_ghost_block(int x, int y, int z, const char* blockIdentifier)`: Registers a ghost block for 3D depth-tested wireframe rendering without sending server packets.
- `void fabric_schematic_clear_ghost_blocks()`: Clears all preview ghost blocks.
- `void fabric_draw_chunk_border(int chunkX, int chunkZ, float r, float g, float b, float a)`: Draws full chunk boundary bounding boxes (Y=-64 to Y=320).
- `void fabric_draw_box_wireframe(...)` & `void fabric_draw_block_outline(...)`: Direct wireframe rendering.

### 9.6 Offhand Dual Wielding (`fabric_get_offhand_item_name`)
Matches `offhand.levipack` dual-wielding capabilities:
- `bool fabric_get_offhand_item_name(char* outName, size_t maxLen, int* outCount)`: Queries the player's offhand container item.
- `bool fabric_is_holding_offhand(const char* itemIdentifier)`: Checks if a specific item is equipped in the offhand.

### 9.7 Environment & Visual Controls (`fabric::environment::EnvironmentSettings`)
Matches `BedrockTools` visual customization modules:
- `void fabric_set_custom_weather(bool overrideWeather, int weatherType)`: Overrides local weather visually (0: Clear, 1: Rain, 2: Thunder).
- `void fabric_set_custom_time(bool overrideTime, float timeOfDay)`: Overrides client-side daylight cycle (1000: Day, 6000: Noon, 18000: Night).
- `void fabric_set_fog_enabled(bool enabled)`: Client-side NoFog toggle.
- `void fabric_set_glint_color(float r, float g, float b, float a)`: Custom enchantment sheen color.

### 9.8 Native Client Actionbar & Chat Messaging
- `void fabric_show_actionbar(const char* message)`: Dispatches real-time Actionbar HUD messages above hotbar (`title @s actionbar <message>`).
- `void fabric_show_chat(const char* message)`: Dispatches direct raw JSON text to player chat HUD.

### 9.9 VFX Particle & 3D Positional Audio Engine
- `bool fabric_spawn_particle(const char* identifier, float x, float y, float z, float vx, float vy, float vz)`: Emits custom Bedrock particles directly.
- `bool fabric_play_sound_at(const char* soundName, float x, float y, float z, float volume, float pitch)`: Plays 3D positional audio.

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

```cpp
extern "C" {
    // Basic item registration
    bool ll_item_register(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int category, bool allowOffhand);
    bool ll_item_register_ex(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int category, bool allowOffhand, uint8_t maxStackSize);
    bool ll_item_register_full(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int category, const char* creativeGroup, bool allowOffhand, uint8_t maxStackSize);

    // Geyser-parity Food, Tool & Weapon registration helpers
    bool ll_item_register_food(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int nutrition, float saturationModifier, int category, const char* creativeGroup);
    bool ll_item_register_tool(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, float attackDamage, short durability, float miningSpeed, int category, const char* creativeGroup);
    bool ll_item_register_weapon(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, float attackDamage, short durability, int category, const char* creativeGroup);

    // Item queries & inventory interaction
    void* ll_item_get_native(const char* identifier);
    void* ll_item_get_by_id(short id);
    bool  ll_item_give_player(void* playerPtr, const char* identifier, int count, int aux);

    // ItemStack inspection & mutation
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
    bool  ll_block_register_with_id(const char* identifier, short id, const char* displayName, const char* textureName, int category, const char* creativeGroup, float destroyTime, float explosionResistance, uint8_t lightEmission);
    bool  ll_block_register_with_base(const char* identifier, const char* displayName, const char* textureName, const char* baseBlock, int category, const char* creativeGroup, float destroyTime, float explosionResistance, uint8_t lightEmission);
    void* ll_block_source_get_block(void* blockSource, int x, int y, int z);
    bool  ll_block_source_set_block(void* blockSource, int x, int y, int z, void* permutation, int updateFlags);
    int   ll_block_source_get_brightness(void* blockSource, int x, int y, int z);
}
```

### 12.4 Programmatic Entity & Mob Registration API
Header: `#include "levilamroid/api/entity/EntityRegistry.h"`

```cpp
extern "C" {
    bool   ll_entity_register(const char* identifier, const char* displayName, const char* runtimeBaseId, float width, float height, float maxHealth, bool isSummonable);
    bool   ll_entity_spawn(const char* identifier, float x, float y, float z);
    bool   ll_entity_is_registered(const char* identifier);
    size_t ll_entity_get_count();

    bool ll_entity_register_spawn_listener(::levilamroid::entity::LLEntitySpawnCallback callback, void* udata);
    bool ll_entity_register_hurt_listener(::levilamroid::entity::LLEntityHurtCallback callback, void* udata);
    bool ll_entity_register_death_listener(::levilamroid::entity::LLEntityDeathCallback callback, void* udata);
}
```

### 12.5 Dynamic Wire Packet Ingestion (`StartGamePacket`, `ItemRegistryPacket`, `AvailableActorIdentifiersPacket`)
Rather than relying on world behavior pack manifests, LeviLamroid intercepts incoming network packets on the client network thread:
1. `StartGamePacket`: Automatically merges all registered `CustomBlockDefinition` objects into the server block property table with `buildBlockPropertyNbt`.
2. `ItemRegistryPacket`: Automatically converts all `CustomItemDefinition` objects into Geyser-parity wire items with `buildItemComponentNbt`, writing ZigZag varints, IEEE 754 LE floats, and Little-Endian byte streams.
3. `AvailableActorIdentifiersPacket`: Injects all `CustomEntityDefinition` entries into `mPayload.mIdentifierList`, providing client-side recognition and summonability.

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

### 17.1 What is `ResourcePackStack` and What is `composeFullStack` For?

In Minecraft Bedrock Edition (Client and Dedicated Server), all game assets—textures, models, animations, particle effects, UI JSON screens, localization strings (`en_US.lang`), sound definitions, and RenderDragon materials—are organized into **Resource Packs**.

During runtime, multiple independent resource packs are loaded simultaneously across different engine contexts:
1. **Core / Vanilla Stack**: Base engine resources shipped inside the APK/installation (`core`, `vanilla`, `chemistry`, `editor`).
2. **Global Resource Stack (`mGlobalStack`)**: Packs activated by the player in *Settings -> Global Resources*, persisting across menus and all worlds.
3. **Treatment / Subpack Stack (`mTreatmentStack`)**: Device-specific overrides, UI resolution variants, and platform experiment layers.
4. **Level / World Resource Stack (`mLevelStack`)**: Packs assigned specifically to an opened world, or streamed over RakNet when connecting to a dedicated server / Realm.
5. **Virtual Mod Stack (LeviLamroid / FabricClient)**: Dynamic in-memory packs registered by native mods.

#### The Stack Problem & Layered Resolution
When the rendering engine, UI engine, or audio engine queries an asset (e.g. `ResourcePackManager::load("textures/items/diamond_sword.png", ...)`), it cannot query multiple packs randomly. Assets must follow strict priority inheritance:
- A pack higher in the stack overrides files in packs below it.
- If a pack does not contain the queried asset, resolution falls through to the next pack in the stack, eventually reaching Vanilla.

To execute this efficiently in high-speed render loops, Bedrock combines all active packs into a single, flattened, ordered pipeline: **`mFullStack`** (an instance of `ResourcePackStack`).

---

### 17.2 Anatomy of `composeFullStack` vs `_composeFullStack`

Bedrock divides resource pack composition into two distinct methods within `ResourcePackManager`:

#### 1. `ResourcePackManager::composeFullStack` (Pure Stack Assembly Routine)
```cpp
void ResourcePackManager::composeFullStack(
    ResourcePackStack& outputStack,
    const ResourcePackStack& globalStack,
    const ResourcePackStack& levelStack
);
```
- **Mangled Symbol (ELF AAPCS64)**: `_ZN19ResourcePackManager16composeFullStackER17ResourcePackStackRKS0_S3_`
- **Mangled Symbol (MSVC x64)**: `?composeFullStack@ResourcePackManager@@QEAAXAEAVResourcePackStack@@AEBV2@1@Z`
- **Machine Code Signature (48-byte exact match across all Bedrock versions)**:
  `FD 7B BB A9 FA 67 01 A9 F8 5F 02 A9 F6 57 03 A9 F4 4F 04 A9 FD 03 00 91 F4 03 01 AA F9 03 03 AA F8 03 02 AA 96 DE C0 A9 F3 03 01 AA F5 03 00 AA`
- **Mechanism**:
  - Clears `outputStack`.
  - Appends base core/vanilla packs at the lowest priority.
  - Evaluates and appends `globalStack` entries in reverse priority order.
  - Evaluates and appends `levelStack` entries on top of global entries.
  - Resolves subpack choices and filters out incompatible pack versions.

#### 2. `ResourcePackManager::_composeFullStack` (Lifecycle Driver & Mutation Manager)
```cpp
void ResourcePackManager::_composeFullStack();
```
- **Mangled Symbol (ELF AAPCS64)**: `_ZN19ResourcePackManager17_composeFullStackEv`
- **Mangled Symbol (MSVC x64)**: `?_composeFullStack@ResourcePackManager@@AEAAXXZ`
- **Mechanism**:
  1. **Acquires Shared Mutex**: Locks `this->mFullStackAccess` (offset `0xA0` in 1.26.52.3; offset `0x15c` in 1.26.60.6) to guarantee thread safety while render threads or async loader workers query assets.
  2. **Invokes Assembly**: Calls `this->composeFullStack(this->mFullStack, this->mGlobalStack, this->mLevelStack)`.
  3. **Engine Version Calibration**: Calls `_calculateMinEngineVersionFromFullStack()` to compute the required Bedrock engine version across all combined manifests.
  4. **Observer Notification**: Dispatches `onFullStackUpdated()` across all registered `ResourcePackListener` observers:
     - `MinecraftUIRenderContext` / UI Engine: Purges cached JSON UI screens and forces immediate re-parsing of modified UI definitions.
     - `mce::TextureGroup` / RenderDragon: Evicts stale textures from GPU memory and re-binds texture atlases.
     - `I18n`: Reloads localization strings and language packs.

---

### 17.3 Why Mod Loaders Hook `_composeFullStack`

In vanilla Bedrock, for custom textures, UI screens, shaders, or sounds to load:
- The user must manually import an `.mcpack` file on their device.
- The user must navigate into Settings or World Edit menus and manually activate it.
- World resource packs disable Xbox Achievements!
- If the user connects to a third-party server, server packs can conflict with or disable client packs.

**By hooking `_composeFullStack`**, mod loaders (LeviLamroid, Amethyst, LeviLamina) achieve **Zero-Resourcepack Native Ingestion**:
1. **Automatic Global Asset Injection**: Native mods package textures and UI JSONs directly inside their `.so` or mod directory. The hook injects a synthetic `PackInstance` or `VirtualPackAccessStrategy` directly into `mFullStack` upon startup.
2. **Achievement-Safe Modding**: Because assets are merged into `mFullStack` on the client without modifying `world_resource_packs.json`, Xbox Live achievements remain completely uninhibited.
3. **Multiplayer Immunity**: Mod assets remain active when joining any external realm or dedicated server (The Hive, CubeCraft, etc.).
4. **Instant Hot-Reloading**: Mods can trigger `manager->_composeFullStack()` at runtime to reload textures and UI screens live without restarting the game.

---

### 17.4 Multi-Version Golden RVAs & Signatures

| Method | 1.26.44.3 | 1.26.45.1 | 1.26.51.1 | 1.26.52.3 | 1.26.60.6 | IDA / LeviLamina Byte Signature |
|---|---|---|---|---|---|---|
| `composeFullStack` | `0x0C599CAC` | `0x0C599C0C` | `0x0C9319DC` | `0x0C931D34` | `0x0CF52894` | `FD 7B BB A9 FA 67 01 A9 F8 5F 02 A9 F6 57 03 A9 F4 4F 04 A9 FD 03 00 91 F4 03 01 AA F9 03 03 AA F8 03 02 AA 96 DE C0 A9 F3 03 01 AA F5 03 00 AA` |
| `_composeFullStack` | `0x0C5997A0` | `0x0C599700` | `0x0C9314D0` | `0x0C931828` | `0x0CF52774` | `FF 43 01 D1 FD 7B 02 A9 F6 57 03 A9 F4 4F 04 A9 FD 83 00 91 55 D0 3B D5 F3 03 00 AA A8 16 40 F9 A8 83 1F F8 14 80 40 F9 ?? ?? ?? ??` |
| `setStack` | `0x0C598818` | `0x0C598778` | `0x0C930600` | `0x0C930958` | `0x0CF51778` | `FF 03 03 D1 FD 7B 08 A9 F8 5F 09 A9 F6 57 0A A9 F4 4F 0B A9 FD 03 02 91 57 D0 3B D5 F4 03 01 AA` |
| `_doStackOperation` | `0x0C5983D8` | `0x0C598338` | `0x0C930108` | `0x0C930460` | `0x0CF535D8` | `FF 43 03 D1 FD 7B 07 A9 FC 6F 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9 F4 4F 0C A9 FD C3 01 91` |

---

### 17.5 Complete Native C++ Detour Implementation

```cpp
#include <levilamroid/LeviLamroid.h>
#include <mc/resource/ResourcePackManager.h>
#include <mc/resource/ResourcePackStack.h>

static void (*orig_composeFullStack)(ResourcePackManager* self) = nullptr;

void hook_composeFullStack(ResourcePackManager* self) {
    // 1. Let vanilla Bedrock assemble base, global, and level packs
    orig_composeFullStack(self);

    // 2. Inject custom mod virtual packs into the active stack
    // self->mFullStack is protected by self->mFullStackAccess mutex
    ll::Logger::getInstance().info("Intercepted ResourcePackManager::_composeFullStack: Injecting Mod Packs");

    // Retrieve or create our synthetic mod pack
    auto modPack = levilamroid::resource::VirtualPackHub::getInstance().getModPackInstance();
    if (modPack) {
        // Append mod pack to top of fullStack
        self->getFullStack().add(modPack, *self->getStack(PackType::Global), false);
    }
}

// Registration in LeviLamroid startup:
void initResourceHooks() {
    uintptr_t targetRva = ll::mcpe::getOffsets().resourcePackManager_composeFullStack;
    if (targetRva != 0) {
        ll::memory::hookRva(targetRva, (void*)&hook_composeFullStack, (void**)&orig_composeFullStack);
    }
}
```

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

---

## 21. Advanced Reverse Engineering & Client Subsystems (Levimatica, LeviMap, BedrockReplay)

### 21.1 World Join Segfault Diagnosis & Native Architecture
- **Root Cause**: During world initialization, custom recipe flush attempted direct invocation of `Recipes::addShapedRecipe` via an incomplete 5-parameter prototype. In AAPCS64 (AArch64), `Recipes::addShapedRecipe` consumes 11+ parameters, reading trailing arguments from caller stack offsets `[x29, #96]`, `[x29, #104]`, and `[x29, #112]`. Uninitialized stack registers caused immediate SIGSEGV crashes on world join.
- **Resolution**:
  1. **Stack Corruption Eliminated**: Recipes are parsed and loaded completely crash-free through native JSON pack ingestion in resource/behavior packs. Manual memory hooking of `Recipes::addShapedRecipe` has been safely deactivated, eliminating stack misalignment.
  2. **Unsafe Network Hook Removed**: The hardcoded `ServerNetworkHandler::_sendLevelData` hook (`0x0C23F664`) previously corrupted the middle of an unrelated routine on 1.26.45.1, triggering SIGSEGV upon joining worlds. It was completely removed in favor of the clean, safe `ServerNetworkHandler::handle` capture.
  3. **Placement Hook Corrected**: `GameMode::useItemOn` was updated from invalid RVA `0x0E3AF6BC` to the authentic function prologue `0x0F89FE18` (1.26.52.3) and `0x0EF72684` (1.26.45.1), enabling real touch block placement without failure.

### 21.2 Levimatica: BlockSource & Ghost Block Inspection
Reverse engineered from `libLevimatica.so` (author: MrPokeG):
- Levimatica parses client-side `.mcstructure` schematic data and compares each block coordinate against the live world `BlockSource`.
- Ghost/preview blocks are rendered client-side without modifying the server world via vertex buffer submissions.
- **Exported C APIs**:
  ```cpp
  // Read block permutation at coordinates (x, y, z)
  LL_API void* ll_block_source_get_block(void* blockSource, int x, int y, int z);

  // Set block permutation in BlockSource
  LL_API bool ll_block_source_set_block(void* blockSource, int x, int y, int z, void* blockPermutation, int updateFlags);

  // Destroy block at coordinates (x, y, z)
  LL_API bool ll_block_source_destroy_block(void* blockSource, int x, int y, int z, bool dropResources);

  // Query block light and sky brightness
  LL_API int ll_block_source_get_brightness(void* blockSource, int x, int y, int z);

  // Retrieve BlockSource for a Dimension (returns direct embedded instance at +0x158)
  LL_API void* ll_dimension_get_blocksource(void* dimension);

  // Retrieve ChunkSource for a Dimension (+424)
  LL_API void* ll_dimension_get_chunksource(void* dimension);

  // Retrieve Dimension ID (0 = Overworld, 1 = Nether, 2 = The End)
  LL_API int ll_dimension_get_id(void* dimension);

  // Convert coordinates between dimensions (e.g. Overworld <-> Nether 8:1 ratio)
  LL_API void ll_dimension_convert_coordinates(int fromDim, int toDim, float inX, float inZ, float* outX, float* outZ);
  ```

### 21.3 LeviMap: Chunk Grid, Biomes & Slime Chunk Analysis
Reverse engineered from `libLeviMap.so` (author: DreamGuXiang):
- LeviMap monitors player position and intercepts tick updates to calculate active chunks, heightmaps, and slime chunks.
- Intercepts input via `MouseHandlerAndroid` vtable slots: slot 2 (`absoluteMouseInput`), slot 5 (`buttonInput`), slot 6 (`scrollInput`).
- **Exported C APIs**:
  ```cpp
  // Query player position and rotation in world simulation
  LL_API bool ll_player_get_position(void* player, float* outX, float* outY, float* outZ);
  LL_API bool ll_player_get_rotation(void* player, float* outPitch, float* outYaw);
  LL_API int ll_player_get_dimension_id(void* player);

  // Check if chunk is a slime chunk using world seed
  LL_API bool ll_chunk_is_slime_chunk(int chunkX, int chunkZ, int64_t worldSeed);

  // Convert player floating coordinates to chunk grid coordinates
  LL_API void ll_chunk_get_coordinates(float playerX, float playerZ, int* outChunkX, int* outChunkZ, int* outRelX, int* outRelZ);
  ```

### 21.4 BedrockReplay: Packet Stream Interception & Replay
Reverse engineered from `libBedrockReplay.so` (author: MrPokeG):
- Intercepts inbound (`ClientNetworkHandler::handle`) and outbound (`LoopbackPacketSender::send`) packet traffic for replay recording:
  - Packet 156: `PlayerAuthInputPacket` (pitch, yaw, head rotation, input bits)
  - Packet 144: `LevelChunkPacket` (complete chunk column binary payload)
  - Packet 145: `SubChunkPacket` (16x16x16 sub-chunk block palette data)
  - Packet 10: `StartGamePacket` (seed, world settings, spawn coordinates)
  - Packet 11: `AddPlayerPacket` & Packet 12: `AddActorPacket`
  - Packet 26: `MoveActorAbsolutePacket` & Packet 28: `MovePlayerPacket`
  - Packet 35: `BlockActorDataPacket` & Packet 123: `LevelSoundEventPacket`
- **Exported C APIs**:
  ```cpp
  typedef bool (*LLPacketListenerCallback)(int packetId, void* packet, bool isClientBound, void* udata);

  LL_API bool ll_network_register_packet_listener(LLPacketListenerCallback callback, void* udata);
  LL_API bool ll_network_unregister_packet_listener(LLPacketListenerCallback callback);
  LL_API int ll_packet_get_id(void* packet);
  LL_API void ll_camera_set_locked(bool locked);
  LL_API bool ll_camera_is_locked();
  ```

### 21.5 Verified Multi-Version RVA Matrix (1.26.45.1 vs 1.26.52.3 vs 1.26.60.6)

| Engine Function | 1.26.45.1 ARM64 RVA | 1.26.51.1 ARM64 RVA | 1.26.52.3 ARM64 RVA | 1.26.60.6 ARM64 RVA |
|---|---|---|---|---|
| `GameMode::useItemOn` | `0x0EF72684` | `0x0EF72684` | `0x0F89FE18` | `0x0EC31ABC` |
| `GameMode::destroyBlock` | `0x0EF72C18` | `0x0EF72C18` | `0x0F8A03AC` | `0x1037BAA0` (BlockSource) |
| `GameMode::useItem` | `0x0EF75F40` | `0x0EF75F40` | `0x0F8A2EB0` | `0x1010621C` (ActorEventCoord) |
| `GameMode::attack` | `0x0EF72F9C` | `0x0EF72F9C` | `0x0F8A073C` | `0x1214B608` (AttackGoal) |
| `BlockSource::getBlock` | `0x0F253C8C` | `0x0F253C8C` | `0x0FB91918` | `0x10371548` |
| `BlockSource::setBlock` | `0x0F25B948` | `0x0E389AA0` | `0x0FB99330` | `0x1037A268` |
| `BlockSource::destroyBlock` | `0x0F25CBD4` | `0` | `0x0FB9A468` | `0x1037BAA0` |
| `BlockSource::getBrightness` | `0x0F253ECC` | `0x0F253ECC` | `0x0FB9A55C` | `0x10371620` |
| `Dimension::getBlockSource` | `(uint8_t*)dim + 0x158` | `0x0BBCCDDC` | `0x102F4538` | `0x0F159688` |
| `Dimension::getChunkSource` | `(uint8_t*)dim + 424` | `0` | `0x102F1BEC` | `(uint8_t*)dim + 424` |
| `ClientNetworkHandler::handle` | `0x09ADC770` | `0x09ED7FE0` | `0x09EC99C0` | `0x0A2DCFFC` |
| `LoopbackPacketSender::send` | `0x0C2E26FC` | `0x0C68FAC4` | `0x0C68FAD4` | `0x0C68C410` |
| `Player::normalTick` | `0x0F0B1680` | `0x0F0B1680` | `0x0F9E26C0` | `0x101ECCD4` |
| `Level::tick` | `0x0F222D2C` | `0x0F222D2C` | `0x0FB313E4` | `0x07A1A6D8` (MinecraftGame) |
| `ClientInstance::update` | `0x0943EC54` | `0x0943EC54` | `0x098036A4` | `0x09CEE9CC` |
| `LocalPlayer::applyTurnDelta` | `0x0A6430D8` | `0x0A6430D8` | `0x0AAC9774` | `0x0FD9AE30` |
| `Tessellator::begin` | `0x0A76E780` | `0x0A76E780` | `0x0A76E7F8` | `0x0D0D6BC8` |

---

## 22. Generic Modloader Architecture & Event Callback APIs

LeviLamroid operates strictly as an agnostic client modloader engine, offering native C-ABI callback registrations and query hooks for dependent mods:

### 22.1 Block Event Listeners
```cpp
typedef bool (*LLBlockInteractCallback)(void* player, int x, int y, int z, uint8_t face, const char* blockIdent, const char* heldItemIdent, void* udata);
typedef bool (*LLBlockDestroyCallback)(void* player, int x, int y, int z, const char* blockIdent, const char* heldItemIdent, void* udata);

LL_API bool ll_block_register_interact_listener(LLBlockInteractCallback callback, void* udata);
LL_API bool ll_block_register_destroy_listener(LLBlockDestroyCallback callback, void* udata);
```

### 22.2 Item & Combat Event Listeners
```cpp
typedef bool (*LLPlayerAttackCallback)(void* player, void* targetActor, const char* heldItemIdent, void* udata);
typedef bool (*LLItemUseCallback)(void* player, const char* itemIdent, void* itemStack, void* udata);

LL_API bool ll_player_register_attack_listener(LLPlayerAttackCallback callback, void* udata);
LL_API bool ll_item_register_use_listener(LLItemUseCallback callback, void* udata);
```

### 22.3 Player & Actor Query APIs
```cpp
LL_API void* ll_player_get_selected_item_stack(void* player);
LL_API bool ll_player_get_held_item_identifier(void* player, char* outIdent, size_t maxLen);
LL_API void* ll_player_get_supplies(void* player);
LL_API void* ll_player_get_inventory(void* player);
LL_API bool ll_player_get_armor(void* player, int slot, void* outItemStack);
LL_API bool ll_player_set_armor(void* player, int slot, const void* itemStack);
LL_API bool ll_player_get_offhand(void* player, void* outItemStack);
LL_API bool ll_player_set_offhand(void* player, const void* itemStack);

LL_API bool ll_actor_get_position(void* actor, float* outX, float* outY, float* outZ);
LL_API bool ll_actor_get_rotation(void* actor, float* outPitch, float* outYaw);
LL_API bool ll_actor_is_player(void* actor);
LL_API bool ll_actor_is_alive(void* actor);
LL_API bool ll_actor_hurt(void* actor, int damage);

LL_API void* ll_service_get_local_player();
LL_API void* ll_service_get_client_instance();
LL_API void* ll_service_get_level();
LL_API void* ll_service_get_block_source();
```


