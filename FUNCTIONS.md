# LeviLamroid Functions & API Master Reference

> **LeviLamroid Version**: `1.0.0-beta` (Initial Beta Testing Release)  
> **Initial Mod Project Version**: `v1.0.0-beta`  
> **Target Platform**: Minecraft Bedrock Edition (Android ARM64) on LeviLauncher  
> **Maintainer**: **SpyGamingOG** (`spygamingog`)  
> **Architecture**: Native C++ Mod Loader Runtime (Windows LeviLamina Source Parity + Android PreLoader Extensions)  
> **Scope**: 100% self-contained reference. Every single class, struct, function, signature, parameter, and return value is cataloged below.

> [!WARNING]
> ### ⚠️ Beta Testing Notice
> LeviLamroid is under active development. Functions and RVAs documented herein represent the current reverse-engineering state for Bedrock 1.26.x. Certain hooks or functions might be experimental or undergoing verification across specific device configurations and engine revisions.

---

## Table of Contents
1. [Overview & Core Philosophy](#1-overview--core-philosophy)
2. [Master Header Directory & Include Matrix](#2-master-header-directory--include-matrix)
3. [Mod Lifecycle & Registration Subsystems (`ll::mod` & `pl::mod`)](#3-mod-lifecycle--registration-subsystems-llmod--plmod)
4. [LeviLamina Parity Core Services (`ll::service`, `ll::thread`, `ll::plugin`)](#4-levilamina-parity-core-services-llservice-llthread-llplugin)
5. [Memory Detours, RVAs, GOT & Memory Probing (`ll::memory`)](#5-memory-detours-rvas-got--memory-probing-llmemory)
6. [Pattern Scanning & Wildcard Byte Matching (`levilamroid::memory` & `pl::memory`)](#6-pattern-scanning--wildcard-byte-matching-levilamroidmemory--plmemory)
7. [Byte Patching & RAII Patch Handles (`pl::memory`)](#7-byte-patching--raii-patch-handles-plmemory)
8. [Multi-Version Engine & Golden RVA Tables (`levilamroid::mcpe`)](#8-multi-version-engine--golden-rva-tables-levilamroidmcpe)
9. [Android Mobile Input Interception (`pl::input`)](#9-android-mobile-input-interception-plinput)
10. [On-Screen Floating HUD Buttons & Launcher Drawer (`pl::modmenu`)](#10-on-screen-floating-hud-buttons--launcher-drawer-plmodmenu)
11. [Thread-Safe Event Bus System (`ll::event::EventBus`)](#11-thread-safe-event-bus-system-lleventeventbus)
12. [Complete Built-in Event Catalog](#12-complete-built-in-event-catalog)
13. [Audio Systems (`levilamroid::sound::FmodAudioEngine` & `SoundEngine`)](#13-audio-systems-levilamroidsoundfmodaudioengine--soundengine)
14. [Virtual In-Memory Resource Packs (`levilamroid::resource`)](#14-virtual-in-memory-resource-packs-levilamroidresource)
15. [Dear ImGui v1.91.x & OpenGL ES 3.0 Rendering Backend](#15-dear-imgui-v191x--opengl-es-30-rendering-backend)
16. [Multi-Target Logging Subsystems (`ll::Logger` & `pl::log::Logger`)](#16-multi-target-logging-subsystems-lllogger--plloglogger)
17. [Minecraft Bedrock Engine Class API Reference (`mc/` & `mcpe/`)](#17-minecraft-bedrock-engine-class-api-reference-mc--mcpe)
    - 17.1 [ClientInstance & IClientInstance](#171-clientinstance--iclientinstance)
    - 17.2 [Player & LocalPlayer](#172-player--localplayer)
    - 17.3 [Inventory & Equipment Slots](#173-inventory--equipment-slots)
    - 17.4 [BlockSource & Chunk Management](#174-blocksource--chunk-management)
    - 17.5 [Block, BlockLegacy & BlockType](#175-block-blocklegacy--blocktype)
    - 17.6 [BlockTypeRegistry & Direct Access Blocks](#176-blocktyperegistry--direct-access-blocks)
    - 17.7 [ItemStack & ItemInstance](#177-itemstack--iteminstance)
    - 17.8 [CompoundTag, Tag & Binary NBT Serialization](#178-compoundtag-tag--binary-nbt-serialization)
    - 17.9 [GameMode](#179-gamemode)
    - 17.10 [StructureTemplate & StructureManager](#1710-structuretemplate--structuremanager)
    - 17.11 [ChestBlockActor & BlockActor](#1711-chestblockactor--blockactor)
    - 17.12 [CommandRegistry](#1712-commandregistry)
    - 17.13 [MinecraftUIRenderContext & ScreenContext](#1713-minecraftuirendercontext--screencontext)
    - 17.14 [PacketSender & TextPacket](#1714-packetsender--textpacket)
    - 17.15 [I18n & Localization](#1715-i18n--localization)
    - 17.16 [Math & Spatial Coordinate Types (`BlockPos`, `Vec3`, `HashedString`, `BoundingBox`)](#1716-math--spatial-coordinate-types-blockpos-vec3-hashedstring-boundingbox)
18. [Bundled Third-Party Utilities (`nlohmann::json`, `glm`)](#18-bundled-third-party-utilities-nlohmannjson-glm)
19. [Version Policy & Starter Template (`v1.0.0`)](#19-version-policy--starter-template-v100)
24. [High-Level Human Engine APIs (`ll::ui`, `ll::render`, `ll::isInWorld`)](#24-high-level-human-engine-apis-llui-llrender-llisinworld)

---

## 1. Overview & Core Philosophy

**LeviLamroid** is the primary, production-grade native C++ mod loader runtime designed specifically for **Minecraft Bedrock Edition (Android ARM64)** executing within **LeviLauncher**.

It establishes five non-negotiable architectural guarantees:
1. **Canonical Modding Namespace (`ll`)**: LeviLamroid adheres directly to **`namespace ll`** as its official coding namespace, matching LeviLamina on Windows. Modders write against `ll::Logger`, `ll::event::EventBus`, `ll::mcpe::getOffsets()`, and `ll::mod::NativeMod`. This guarantees 100% desktop-to-mobile source parity with zero code rewrites. For compatibility, `namespace levilamroid` and `namespace llntol` are provided as seamless aliases.
2. **100% LeviLamina Windows Source Parity**: Code written against the official LeviLamina Windows C++ API (`ll::*`) compiles and runs identically on Android without rewriting detour boilerplate or class structures.
3. **LeviLauncher PreLoader Native Extensions**: Native Android mobile capabilities—including touch pointer routing, physical keyboard mapping, floating HUD touch buttons, launcher drawer registration, and storage access framework compatibility—are exposed out-of-the-box (`pl::*`).
4. **Automated Multi-Version Resolution**: Dynamically reads the ELF `PT_NOTE` segment of `libminecraftpe.so` at startup to determine the 20-byte SHA-1 GNU Build ID, automatically activating the correct RVA offset tables (`1.26.52.3`, `1.26.45.1`).
5. **Strict Initial Versioning Policy**: All new mods, template starter projects, and LeviLamroid distribution bundles are initialized strictly at version **`v1.0.0`** (or `1.0.0`) unless explicitly instructed by the user to bump versions.

---

## 2. Master Header Directory & Include Matrix

To build mods for LeviLamroid, add `${LEVILAMROID_ROOT}/include` to your include search paths:

| Header Include | Namespace / Scope | Purpose & Description |
|---|---|---|
| `<levilamroid/LeviLamroid.h>` | `ll` / `levilamroid` | Canonical Master Header for all LeviLamroid modders; imports all subsystems, version macros, and types |
| `<ll/api/mod/NativeMod.h>` | `ll::mod` | Canonical LeviLamina mod base class with profile-isolated storage |
| `<ll/api/mod/RegisterHelper.h>` | Global macros | `LL_REGISTER_MOD` macro exporting all required loader ABIs |
| `<ll/api/service/Bedrock.h>` | `ll::service` | Global engine accessors: `getClientInstance()`, `getLevel()`, `getLocalPlayer()` |
| `<ll/api/thread/ThreadPoolExecutor.h>` | `ll::thread` | Thread pool worker executor for non-blocking asynchronous mod tasks |
| `<ll/api/plugin/PluginManager.h>` | `ll::plugin` | Dynamic `.so` plugin loading, tracking, and symbol discovery |
| `<ll/api/memory/Hook.h>` | `ll::memory` | Function detour hooking, RVA hooking, GOT slots, RTTI VTable resolution, and `LL_TYPE_INSTANCE_HOOK` |
| `<ll/api/event/EventBus.h>` | `ll::event` | Thread-safe type-based publish/subscribe event bus |
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
| `<pl/memory/Signature.hpp>` | `pl::memory` | Module signature scanning utilities |
| `<pl/memory/Vtable.hpp>` | `pl::memory` | Primary VTable slot function resolution via Itanium ABI RTTI |
| `<pl/Logger.hpp>` | `pl::log` | PreLoader formatted logger using `fmt` and Android `__android_log_print` |
| `<pl/Config.hpp>` | `pl::reflection` | Reflection-driven type-safe JSON configuration serialization |
| `<mc/client/ClientInstance.h>` | Engine | Root client interface containing player, level renderer, and network sender |
| `<mc/world/actor/player/Player.h>` | Engine | Player actor entity base class and inventory |
| `<mc/world/actor/player/LocalPlayer.h>` | Engine | Client-controlled player subclass with movement, camera, and interaction logic |
| `<mc/world/level/BlockSource.h>` | Engine | World chunk region interface for block inspection and placement |
| `<mc/world/level/block/Block.h>` | Engine | Permutated world block with runtime ID, network ID, and properties |
| `<mc/world/item/ItemStack.h>` | Engine | Item stacks with quantity, aux metadata, enchantments, and custom NBT |
| `<mc/deps/nbt/CompoundTag.h>` | Engine | NBT compound dictionary with automatic Gzip/Zlib binary deserializer |
| `<mc/locale/I18n.h>` | Engine | Localization lookup and translation |
| `<mcpe/BlockTypeRegistry.h>` | `mcpe` | High-frequency direct access block pointers (`Air`, `Stone`, `Dirt`) |
| `<mcpe/ItemRegistry.h>` | `mcpe` | Item registration and runtime identifier resolution |
| `<imgui.h>` / `<imgui/imgui.h>` | `ImGui` | Dear ImGui v1.91.x immediate mode GUI toolkit |
| `<imgui_impl_opengl3.h>` | `ImGui` | Android OpenGL ES 3.0 renderer backend with context-loss recovery |
| `<nlohmann/json.hpp>` | `nlohmann` | Industry-standard modern C++ JSON library |
| `<glm/glm.hpp>` | `glm` | OpenGL Mathematics library for vector and matrix calculus |

---

## 3. Mod Lifecycle & Registration Subsystems (`ll::mod` & `pl::mod`)

LeviLamroid supports both the canonical LeviLamina mod pattern (`ll::mod::NativeMod`) and the PreLoader mod pattern (`pl::mod::NativeMod`).

### 3.1 Class `ll::mod::NativeMod`
Located in `#include <ll/api/mod/NativeMod.h>`.  
Base class for your mod instance. Automatically isolates configuration files into profile directories:  
`/sdcard/Android/media/org.levimc.launcher/minecraft/<profile>/mods/<ModName>/`

```cpp
#include <ll/api/mod/NativeMod.h>
#include <ll/api/mod/RegisterHelper.h>

class MyMod : public ll::mod::NativeMod {
public:
    MyMod() : NativeMod("MyMod") {}

    static MyMod& getInstance() {
        static MyMod instance;
        return instance;
    }

    bool load() {
        getLogger().info("MyMod v1.0.0 loaded!");
        return true;
    }

    bool enable() {
        getLogger().info("MyMod v1.0.0 enabled!");
        return true;
    }

    bool disable() {
        getLogger().info("MyMod disabled!");
        return true;
    }
};

LL_REGISTER_MOD(MyMod, MyMod::getInstance());
```

#### Member Functions
- `const std::string& getName() const noexcept`  
  Returns the registered name string of the mod.
- `ll::Logger& getLogger() noexcept`  
  Returns the active `ll::Logger` instance configured with the mod's name as its tag.
- `std::filesystem::path getConfigDir() const`  
  Returns the configuration directory path. Creates the directory on disk if it does not exist.
- `std::filesystem::path getDataDir() const`  
  Returns the persistent data storage directory path for the mod.
- `static NativeMod* current()`  
  Returns a pointer to the currently executing `NativeMod` instance, or `nullptr`.

---

### 3.2 Registration Macro `LL_REGISTER_MOD`
Located in `#include <ll/api/mod/RegisterHelper.h>`.  
Generates a complete set of exported C ABIs to ensure universal binary loader compatibility:

```cpp
LL_REGISTER_MOD(MyMod, MyMod::getInstance());
```

**Exported ABIs Generated:**
- `extern "C" bool mod_load()`
- `extern "C" bool mod_enable()`
- `extern "C" bool mod_disable()`
- `extern "C" bool mod_unload()`
- `extern "C" bool ll_mod_load(void*)`
- `extern "C" bool ll_mod_enable(void*)`
- `extern "C" bool ll_mod_disable(void*)`
- `extern "C" bool ll_mod_unload(void*)`
- `extern "C" bool PLMod_Load(void*)`
- `extern "C" bool PLMod_Enable(void*)`
- `extern "C" bool PLMod_Disable(void*)`
- `extern "C" bool PLMod_Unload(void*)`
- `extern "C" bool LeviMod_Load(void*)`
- `extern "C" const void* PLGetModRegistration()`  
  PreLoader-compatible vtable structure pointer containing aligned lifecycle callbacks.

---

### 3.3 Class `pl::mod::NativeMod` & `ModContext`
Located in `#include <pl/Mod.hpp>`.  
Alternative lifecycle model passed directly by LeviLauncher's PreLoader loader.

#### `pl::mod::ModContext`
- `JavaVM* javaVm() const noexcept`: Android Java Virtual Machine pointer.
- `const ModInfo& info() const noexcept`: Manifest metadata (`id`, `displayName`, `author`, `version`).
- `pl::log::Logger& logger() const noexcept`: PreLoader logger reference.
- `const std::string& id() const noexcept`: Mod unique identifier.
- `const std::string& name() const noexcept`: Mod display name.
- `const std::filesystem::path& modRootPath() const noexcept`: Root directory of the mod.
- `std::filesystem::path dataDir() const`: Path to `/mods/<id>/data`.
- `std::filesystem::path configDir() const`: Path to `/mods/<id>/config`.
- `std::filesystem::path resourceDir() const`: Path to `/mods/<id>/resources`.

---

## 4. LeviLamina Parity Core Services (`ll::service`, `ll::thread`, `ll::plugin`)

### 4.1 Global Engine Services (`namespace ll::service`)
Located in `#include <ll/api/service/Bedrock.h>`.

- `ClientInstance* getClientInstance()`  
  Returns the active singleton `ClientInstance` object pointer. Returns `nullptr` if called before the engine initialization completes.
- `Level* getLevel()`  
  Returns the primary singleplayer / local `Level` world instance.
- `Level* getMultiPlayerLevel()`  
  Returns the active multiplayer world level instance.
- `LocalPlayer* getLocalPlayer()`  
  Convenience helper. Fetches `getClientInstance()` and returns `client->getLocalPlayer()`.

```cpp
#include <ll/api/service/Bedrock.h>

LocalPlayer* player = ll::service::getLocalPlayer();
if (player) {
    Vec3 pos = player->getEyePos();
}
```

---

### 4.2 Asynchronous Thread Pool Executor (`ll::thread::ThreadPoolExecutor`)
Located in `#include <ll/api/thread/ThreadPoolExecutor.h>`.  
A thread-safe worker pool for executing non-blocking background tasks (such as network calls, disk I/O, or structure generation) without lagging the Minecraft main thread.

```cpp
#include <ll/api/thread/ThreadPoolExecutor.h>

// Create a pool with 2 worker threads
ll::thread::ThreadPoolExecutor pool("MyTaskWorker", 2);

// Dispatch a background job
pool.execute([] {
    // Perform intensive disk parsing or HTTP requests here
});

// Explicitly terminate workers when disabling
pool.destroy();
```

#### Member Functions
- `ThreadPoolExecutor(std::string name, size_t threads = 1)`  
  Constructs a worker pool with `threads` worker threads.
- `~ThreadPoolExecutor()`  
  Invokes `destroy()` to gracefully join and shut down all worker threads.
- `void execute(std::function<void()> task)`  
  Queues a callable task. Immediately notifies an idle worker thread.
- `void destroy()`  
  Signals workers to stop, clears queues, and joins all active threads.

---

### 4.3 Plugin Manager (`ll::plugin::PluginManager`)
Located in `#include <ll/api/plugin/PluginManager.h>`.  
Manages runtime loading, dependency resolution, and symbol tracking of `.so` mod libraries.

#### Member Functions
- `static PluginManager& getInstance()`  
  Singleton accessor.
- `void loadPluginsFromDir(const std::string& dirPath)`  
  Iterates through `dirPath`, searching for `.so` files and loading valid mods.
- `bool loadPlugin(const std::string& soPath)`  
  Calls `dlopen` on `soPath`, resolves initialization symbols, and invokes lifecycle hooks.
- `bool unloadPlugin(const std::string& pluginName)`  
  Calls disable callbacks and unloads the shared library via `dlclose`.
- `const std::vector<PluginInfo>& getLoadedPlugins() const`  
  Returns an immutable vector of all loaded plugin records (`name`, `version`, `path`, `handle`, `enabled`).

---

## 5. Memory Detours, RVAs, GOT & Memory Probing (`ll::memory`)

Located in `#include <ll/api/memory/Hook.h>`.

### 5.1 Base Address & Dynamic Resolution
- `LL_API uintptr_t getMinecraftBase()`  
  Returns the base virtual address of `libminecraftpe.so` in process memory. Automatically discovered via `/proc/self/maps` traversal.
- `LL_API void setMinecraftBase(uintptr_t base)`  
  Manually sets or overrides the discovered base address.
- `LL_API void* findMinecraftSymbol(const char* symbolName)`  
  Resolves an exported symbol dynamically via `dlsym` and ELF string table traversal.
- `LL_API void* resolveVtableFunction(const char* className, size_t slot, const char* moduleName = "libminecraftpe.so")`  
  Resolves a virtual method function pointer directly from an Itanium C++ RTTI VTable using class name and slot index.

---

### 5.2 Hooking Primitives
- `LL_API bool hook(void* target, void* detour, void** original)`  
  Installs an inline AArch64 machine-code detour trampoline at `target` redirecting to `detour`. The trampoline calling original code is returned in `*original`.  
  *Features*: Atomic 16-byte instruction patch, executable trampoline buffer allocation, and instruction cache invalidation (`__builtin___clear_cache`).
- `LL_API bool hookRva(uintptr_t rva, void* detour, void** original)`  
  Convenience helper hooking `getMinecraftBase() + rva`.
- `LL_API bool hookGotSlot(uintptr_t gotRva, void* detour, void** original)`  
  Hooks an imported function pointer in the Global Offset Table (GOT) at `getMinecraftBase() + gotRva`. Used for intercepting EGL/GLES calls (`glClear`, `eglSwapBuffers`).

---

### 5.3 Memory Safety Probing (`SafeMemory`)
- `SafeMemory::isReadable(const void* ptr, size_t size)`  
  Tests whether the memory range `[ptr, ptr + size)` can be read without crashing. Probes kernel memory access using `/dev/null` system writes, completely avoiding `SIGSEGV` faults.

---

### 5.4 LeviLamina Windows Parity Hook Macros
Allows porting Windows LeviLamina instance hooks directly without rewriting boilerplate:

```cpp
#include <ll/api/memory/Hook.h>
#include <mc/world/actor/player/LocalPlayer.h>

LL_TYPE_INSTANCE_HOOK(
    LocalPlayerMoveHook,
    HookPriority::Normal,
    LocalPlayer,
    &LocalPlayer::move,
    void,
    const Vec3& delta
) {
    // Custom pre-move logic
    origin(delta); // Call original method
}
```

Macros provided:
- `LL_TYPE_INSTANCE_HOOK(HookName, Priority, TargetClass, TargetFunc, ReturnType, ...)`
- `LL_AUTO_TYPED_INSTANCE_HOOK(...)`
- `LL_AUTO_TYPE_INSTANCE_HOOK(...)`

---

## 6. Pattern Scanning & Wildcard Byte Matching (`levilamroid::memory` & `pl::memory`)

Located in `#include <levilamroid/api/memory/PatternScan.h>` and `#include <pl/memory/Signature.hpp>`.

### 6.1 `levilamroid::memory::PatternScan`
Provides multi-threaded IDA-style wildcard pattern matching over the executable ELF segments of `libminecraftpe.so`.

```cpp
#include <levilamroid/api/memory/PatternScan.h>

// Scan with '?' or '??' wildcards
uintptr_t addr = levilamroid::memory::PatternScan::scanMinecraft(
    "FF 83 01 D1 F6 57 02 A9 ?? ?? ?? ?? E0 03 13 AA"
);

if (addr) {
    uintptr_t rva = addr - ll::memory::getMinecraftBase();
    // Hook or invoke address...
}
```

---

### 6.2 PreLoader Signature Resolver (`pl::memory`)
- `uintptr_t resolveSignature(std::string_view signature, std::string_view moduleName)`  
  Resolves a single hex signature within `moduleName`.
- `std::unordered_map<std::string, uintptr_t> resolveSignatures(std::span<const std::string> signatures, std::string_view moduleName)`  
  Batch resolves multiple signatures simultaneously.

---

## 7. Byte Patching & RAII Patch Handles (`pl::memory`)

Located in `#include <pl/memory/Patch.hpp>`.  
Used to patch machine instructions (such as `NOP`-ing conditional branches or rewriting opcodes) with automated tracking and unpatching.

### 7.1 Free Functions
- `bool writeBytes(uintptr_t address, std::span<const uint8_t> bytes, std::string_view name)`  
  Writes raw bytes to `address`, backing up original bytes under `name`.
- `bool writeBytes(uintptr_t address, std::string_view hexBytes, std::string_view name)`  
  Writes hex bytes (e.g. `"1F 20 03 D5"` for ARM64 `NOP`).
- `std::vector<uint8_t> readBytes(uintptr_t address, size_t length)`  
  Safely reads `length` bytes from `address`.
- `bool revertPatch(std::string_view name)`  
  Restores original bytes previously overwritten under `name`.
- `void revertAllPatches()`  
  Restores all active patches across the process.

### 7.2 RAII Class `pl::memory::PatchHandle`
Applies a patch on creation and automatically reverts the patch when it goes out of scope or is destroyed.

```cpp
#include <pl/memory/Patch.hpp>

{
    // Apply ARM64 NOP (0x1F2003D5)
    pl::memory::PatchHandle patch(targetAddress, "1F 20 03 D5", "BypassCheck");
    // Code runs with patch applied...
} // Automatically reverts to original opcode upon exit
```

---

## 8. Multi-Version Engine & Golden RVA Tables (`ll::mcpe`)

Located in `#include <levilamroid/api/mcpe/Offsets.h>` or through `#include <levilamroid/LeviLamroid.h>`.

### 8.1 Automated Version Identification
LeviLamroid reads the ELF `PT_NOTE` segment of `libminecraftpe.so` at boot time to match the 20-byte SHA-1 GNU Build ID:
- `56de9eed077631e03a31f4f58eb2f0e00071d335` $\rightarrow$ Bedrock `1.26.52.3`
- `868e275cb295e9a275bb29d2258edc2f7dc48761` $\rightarrow$ Bedrock `1.26.45.1`

### 8.2 Version Functions
- `const EngineOffsets& ll::mcpe::getOffsets()`: Returns the active RVA offset table.
- `std::string ll::mcpe::getDetectedVersion()`: Returns active version string (`"1.26.52.3"` or `"1.26.45.1"`).
- `void ll::mcpe::setTargetVersion(const std::string& version)`: Manually forces a version profile.
- `void ll::mcpe::autoDetectVersion()`: Probes ELF GNU Build ID and `/proc/self/maps`.
- `uintptr_t ll::mcpe::findFunctionRva(std::string_view name)`: Looks up an RVA by symbol name.

### 8.3 Fields in `struct EngineOffsets`
- **UI Screen Tech Stack**: `getTechStackForScreen`
- **Start Menu**: `startMenuOnOpen`, `startMenuTick`
- **Buttons**: `playButton`, `settingsButton`, `realmsButton`, `dressingRoomButton`, `marketplaceButton`, `menuExit`
- **RenderDragon**: `progressBarRender`, `panoramaRender`, `cubemapBackgroundRender`, `cubemapFaceSubmit`, `cubemapRendererRender`, `mainMenuCubemapPass`, `cubemapDraw`
- **GPU Hooking**: `glClearGotRva`
- **UI Context Drawing**: `uiDrawImage`, `uiFlushImages`, `uiFillRectangle`, `uiDrawRectangle`, `uiDrawText`, `uiFlushText`, `uiDrawNineslice`, `uiGetScreenContext`
- **Gameplay**: `localPlayerMove`, `playerNormalTick`, `levelTick`, `gameModeDestroyBlock`, `gameModeUseItem`, `gameModeUseItemOn`, `gameModeAttack`
- **Audio Engine**: `soundEnginePlay`, `soundEnginePlayUI`, `soundEnginePlayAt`, `soundEngineStop`
- **Resource Management**: `directoryPackGetAsset`, `zipPackGetAsset`, `resourcePackManagerLoad`, `textureGroupLoadAsync`
- **OreUI (Cohtml Engine)**: `cohtmlLoadURL`, `cohtmlExecuteScript`, `cohtmlTriggerEvent`, `cohtmlMouseEvent`, `cohtmlTouchEvent`
- **JSON UI**: `screenControllerTick`, `screenControllerHandleEvent`, `screenControllerBind`
- **VTable Slots**: `clientInstanceLocalPlayerSlot` (32), `clientInstanceIsShowingMenuSlot` (151 for 1.26.5x / 150 for 1.26.4x)

### 8.4 Full RVA Parity: 1.26.52.3 vs 1.26.45.1
Both `1.26.52.3` and `1.26.45.1` now have **100% complete golden offset coverage** across all 79 fields in `EngineOffsets`:
- **Minecraft 1.26.52.3**:
  - `directoryPackGetAsset` (0x111EE8A8), `zipPackGetAsset` (0x11157F00)
  - `directoryPackHasAsset` (0x111F888C), `zipPackHasAsset` (0x11158654)
  - `packForEachInAssetSet` (0x1117FD58), `resourcePackGetResource` (0x06A3DEA0)
  - `textureGroupLoadAsync` (0x0A777500), `textureGroupEnableLRU` (0x0A778998)
  - `textureHotReloaderRegister` (0x0B1FC23C), `textureHotReloaderRegisterAtlas` (0x0B1FDBC4)
  - `blockGraphicsGetTextureItem` (0x0A66DC10), `cubemapLoadPanoramas` (0x09B79A28)
  - `resourcePackRepositoryGetResourcePackInPath` (0x0C9356E4), `repositoryLoadingFindVanillaPacks` (0x0C96C79C)
  - `glClearGotRva` (0x132893d8)
  - VTable Slots: `clientInstanceLocalPlayerSlot` = 32, `clientInstanceIsShowingMenuSlot` = 151
- **Minecraft 1.26.45.1**:
  - All 19 resource pack and texture access strategy RVAs fully discovered and matched:
    - `directoryPackGetAsset` (0x1042CB64), `zipPackGetAsset` (0x1042D28C)
    - `directoryPackHasAsset` (0x104417EC), `zipPackHasAsset` (0x10432C5C)
    - `packForEachInAssetSet` (0x1043791C), `resourcePackGetResource` (0x066951F4)
    - `resourcePackManagerLoad` (0x0C594B18), `resourcePackManagerGetStack` (0x0C5948C0), `resourcePackManagerDoStackOp` (0x0C59B2CC)
    - `textureGroupLoadAsync` (0x0A403690), `textureGroupEnableLRU` (0x0A404B28)
    - `textureHotReloaderRegister` (0x0ACF8718), `textureHotReloaderRegisterAtlas` (0x0ACFA0A0)
    - `blockGraphicsGetTextureItem` (0x0A224634), `cubemapLoadPanoramas` (0x097C6640)
    - `resourcePackRepositoryGetResourcePackInPath` (0x0C59FEF0), `repositoryLoadingFindVanillaPacks` (0x0C5D9184)
  - Full OreUI Cohtml engine (`cohtmlLoadURL` 0x069C1A7C, `cohtmlExecuteScript` 0x069D0094, `oreUiInit` 0x09230484)
  - Full JSON UI & ScreenControllers (`startMenuOnOpen` 0x09EBD95C, `screenControllerTick` 0x096D73DC, etc.)
  - Full Chat, Commands & Network Packets (`commandRegistryRegisterCommand` 0x0D6075CC, `loopbackPacketSenderSend` 0x0C2E26FC)
  - Full Recipes, Items, Blocks & Scripting (`recipesAddShapedRecipe` 0x0F2FC108, `itemRegistryTryRegisterAndInitItem` 0x0F2AFB68, `actorNormalTick` 0x0EA412D4, `actorHurt` 0x0EC847A8, `actorDie` 0x0EC88C50, `scriptingQuickJS` 0x10917BD4)
  - `glClearGotRva` (0x124C9688)
  - VTable Slots: `clientInstanceLocalPlayerSlot` = 32, `clientInstanceIsShowingMenuSlot` = 150

---

### 8.5 The 2,036 Symbol Universal Dictionary (`Symbols.cpp` & `Symbols.h`)
Instead of shipping redundant, conflicting header dumps in `include/mcpe/`, LeviLamroid embeds a unified, high-performance symbol resolver inside `liblevilamroid.so`:
- **Header**: `#include <levilamroid/api/mcpe/Symbols.h>`
- **Database**: 2,036 compiled symbols from `src/mcpe/Symbols_1_26_52_3.inl` and `src/mcpe/Symbols.cpp`.
- **Runtime Resolution**:
  ```cpp
  // Lookup symbol metadata by human name or mangled identifier
  const SymbolEntry* info = ll::mcpe::findSymbolInfo("Actor::die");
  
  // Resolve runtime memory address (dynamic dlsym -> version RVA -> pattern scan fallback)
  void* addr = ll::mcpe::resolveSymbolAddress("Actor::die");
  ```

#### The 9 Symbol Domains in the Universal Symbol Table
1. **Domain: PLAYER (102 symbols)**: `LocalPlayer::move`, `Player::normalTick`, `Player::getOffhandSlot`, `Player::getSelectedItem`, etc.
2. **Domain: ACTOR (186 symbols)**: `Actor::baseTick`, `Actor::die`, `Actor::getHealth`, `Actor::hurt`, `Actor::isSneaking`, `Actor::normalTick`, `Actor::setPos`, `Actor::setRot`, `Actor::teleportTo`, `Actor::setCustomName`, etc.
3. **Domain: BLOCK (321 symbols)**: `BlockTypeRegistry::init`, `BlockSource::getBlock`, `BlockSource::setBlock`, `BlockLegacy::init`, etc.
4. **Domain: ITEM (245 symbols)**: `ItemRegistry::tryRegisterAndInitItem`, `ItemRegistry::parseItemIdentifier`, `ItemStack::reinit`, etc.
5. **Domain: WORLD (198 symbols)**: `Dimension::getBlockSource`, `LevelChunk::getBlock`, `LevelChunk::setBlock`, etc.
6. **Domain: LEVEL (84 symbols)**: `Level::tick`, `Level::getPacketSender`, `Level::saveData`, `Level::getPlayers`, etc.
7. **Domain: NETWORK (312 symbols)**: `PacketSender::send`, `LoopbackPacketSender::send`, `ServerNetworkHandler::handle`, etc.
8. **Domain: RENDER (280 symbols)**: `ActorRenderer::render`, `MinecraftUIRenderContext::fillRectangle`, `MinecraftUIRenderContext::drawImage`, etc.
9. **Domain: UI (308 symbols)**: `ScreenController::tick`, `ScreenController::handleEvent`, `StartMenuScreenController::onOpen`, `Cohtml::loadURL`, etc.

---

### 8.6 Recommended Hooking Patterns

#### Pattern 1: Runtime Dynamic Golden Offsets (`ll::mcpe::getOffsets()`)
Automatically resolves to the active Minecraft version (`1.26.52.3` or `1.26.45.1`) detected from the ELF Build ID:
```cpp
#include <levilamroid/LeviLamroid.h>

void hookLocalPlayer() {
    uintptr_t rva = ll::mcpe::getOffsets().localPlayerMove;
    ll::memory::hook(
        reinterpret_cast<void*>(ll::memory::getMinecraftBase() + rva),
        (void*)&hooked_move,
        &sOrigMove
    );
}
```

#### Pattern 2: Universal Symbol Resolver (`ll::mcpe::resolveSymbolAddress`)
Resolves any of the 2,036 symbols dynamically across dynamic export tables, version RVAs, and pattern scanning:
```cpp
#include <levilamroid/LeviLamroid.h>
#include <levilamroid/api/mcpe/Symbols.h>

void hookActorTick() {
    void* target = ll::mcpe::resolveSymbolAddress("Actor::baseTick");
    if (target) {
        ll::memory::hook(target, (void*)&hooked_tick, &sOrigTick);
    }
}
```

## 9. Android Mobile Input Interception (`pl::input`)

Located in `#include <pl/Input.hpp>`.  
Directly intercepts Android View input events. Returning `true` from a callback consumes the event, preventing Minecraft from rotating the player camera or moving when typing or interacting with mod menus.

### 9.1 Touch Event Interception
```cpp
#include <pl/Input.hpp>

pl::input::registerTouchCallback([](const pl::input::TouchEvent& ev) -> bool {
    // ev.action: 0=DOWN, 1=UP, 2=MOVE, 3=CANCEL
    // ev.pointerId: Multi-touch finger ID
    // ev.x, ev.y: Screen pixel coordinates
    if (gModMenuVisible) {
        // Feed coordinates to ImGui
        return true; // Consume event!
    }
    return false;
});
```

### 9.2 Physical Keyboard & Game Controller Interception
```cpp
pl::input::registerKeyCallback([](const pl::input::KeyEvent& ev) -> bool {
    // ev.keyCode: Android KeyCode (41='M', 111=ESC, 4=BACK, 66=ENTER)
    // ev.unicodeChar: Character typed
    // ev.isKeyDown: True if pressed, False if released
    if (ev.isKeyDown && ev.keyCode == 41) { // 'M' key
        gModMenuVisible = !gModMenuVisible;
        return true; // Consume key
    }
    return gModMenuVisible;
});
```

### 9.3 Mouse Buttons & Soft Keyboard
- `void registerMouseCallback(MouseCallback callback)`: Intercepts mouse button down/up.
- `void showKeyboard()`: Opens the Android software keyboard (IME).
- `void hideKeyboard()`: Hides the software keyboard.

---

## 10. On-Screen Floating HUD Buttons & Launcher Drawer (`pl::modmenu`)

Located in `#include <pl/ModMenu.hpp>`.

### 10.1 Floating HUD Button Registration
Creates an on-screen touch button without writing custom OpenGL code:

```cpp
#include <pl/ModMenu.hpp>

pl::modmenu::ButtonInfo btn;
btn.buttonId = "mymod_btn";
btn.moduleId = "MyMod";
btn.displayName = "My Mod Action";
btn.label = "Menu";
btn.behavior = pl::modmenu::ButtonBehavior::Click; // Click, Hold, Toggle
btn.stylePreset = pl::modmenu::ButtonStylePreset::Keycap; // Keycap, Accent
btn.normalBgColor = 0xAA1E293B; // ARGB Dark Slate
btn.activeBgColor = 0xDD00E5FF; // ARGB Glow Cyan
btn.textColor = 0xFFFFFFFF;
btn.widthScale = 1.2f;
btn.heightScale = 1.2f;
btn.onEvent = [](std::string_view id, pl::modmenu::ButtonEvent ev, float val) {
    if (ev == pl::modmenu::ButtonEvent::Click) {
        gModMenuVisible = !gModMenuVisible;
    }
};

pl::modmenu::registerButton(btn);
```

### 10.2 Launcher Drawer Module Registration
Adds your mod to the top-bar drawer menu in LeviLauncher:

```cpp
pl::modmenu::ModuleInfo mod;
mod.moduleId = "MyMod";
mod.displayName = "My Mod";
mod.description = "Custom client mod for Bedrock Android";
mod.defaultEnabled = true;
mod.onToggle = [](std::string_view id, bool enabled) {
    // Handle toggle switch state change
};

pl::modmenu::registerModule(mod);
```

---

## 11. Thread-Safe Event Bus System (`ll::event::EventBus`)

Located in `#include <ll/api/event/EventBus.h>`.  
A type-safe, thread-safe publish-subscribe event bus.

### 11.1 Subscribing to Events
```cpp
#include <ll/api/event/EventBus.h>
#include <ll/api/event/ui/UIEvents.h>

auto listener = ll::event::EventBus::getInstance().subscribe<ll::event::StartMenuOpenEvent>([](auto& ev) {
    ll::Logger::getInstance().info("Start Menu opened!");
});
```

### 11.2 Publishing Custom Events
```cpp
struct CustomModEvent : public ll::event::Event {
    int data;
    CustomModEvent(int d) : data(d) {}
};

ll::event::EventBus::getInstance().publish(CustomModEvent(42));
```

---

## 12. Complete Built-in Event Catalog

### 12.1 UI Events (`#include <ll/api/event/ui/UIEvents.h>`)
- `ll::event::StartMenuOpenEvent`: Fired when title / start screen opens.
- `ll::event::PlayButtonClickedEvent`: Fired when user clicks "Play".
- `ll::event::SettingsButtonClickedEvent`: Fired when user clicks "Settings".
- `ll::event::RealmsButtonClickedEvent`: Fired when user clicks "Realms".
- `ll::event::MarketplaceButtonClickedEvent`: Fired when user clicks "Marketplace".
- `ll::event::DressingRoomButtonClickedEvent`: Fired when user clicks "Dressing Room".
- `ll::event::MenuExitEvent`: Fired when returning to the main menu.
- `ll::event::ScreenTechStackQueryEvent`: Queries UI technology stack (JSON UI vs OreUI/Cohtml).

### 12.2 Render Events (`#include <ll/api/event/render/RenderEvents.h>`)
- `ll::event::GlClearEvent`: Fired inside `glClear` before frame rendering. Provides surface width, height, and render thread ID.
- `ll::event::CubemapRenderEvent`: Fired before panorama cubemap passes. Can cancel vanilla cubemap drawing to render custom backgrounds.
- `ll::event::ProgressBarRenderEvent`: Fired during world loading progress bar passes.

### 12.3 Sound Events (`#include <ll/api/event/sound/SoundEvents.h>`)
- `ll::event::SoundPlayEvent`: Fired when any 3D/2D sound plays. Exposes `soundName`, `position`, `volume`, and `pitch`. Modifying volume or pitch alters the audio in real time (e.g. cavern reverb, muffling).
- `ll::event::LevelSoundEventOccurred`: High-level Minecraft gameplay sound events.

### 12.4 Player Events (`#include <ll/api/event/player/PlayerEvents.h>`)
- `ll::event::PlayerMoveEvent`: Fired during local player movement (`LocalPlayer::move`). Exposes the player pointer and 3D delta vector `ll::event::Vec3`. Cancellable to restrict movement.
- `ll::event::PlayerTickEvent`: Fired every player tick (`Player::normalTick`). Cancellable.
- `ll::event::PlayerRespawnEvent`: Fired when a player respawns after death.
- `ll::event::PlayerUseItemEvent`: Fired when a player uses or consumes an item stack (`GameMode::useItem` / `useItemOn`). Cancellable.

### 12.5 World Events (`#include <ll/api/event/world/WorldEvents.h>`)
- `ll::event::LevelTickEvent`: Fired every engine level tick (`Level::tick`). Cancellable.
- `ll::event::BlockBreakEvent`: Fired when a block is broken in game mode (`GameMode::destroyBlock`). Exposes the player pointer, `ll::event::BlockPos`, and face index. Cancellable to protect areas.
- `ll::event::BlockPlaceEvent`: Fired when a block is placed into a region (`BlockSource::setBlock`). Exposes `BlockPos` and block pointer. Cancellable.

### 12.6 Entity & Actor Events (`#include <ll/api/event/entity/ActorEvents.h>`)
- `ll::event::ActorHurtEvent`: Fired when any entity or player takes damage (`Actor::hurt`). Exposes `Actor*`, `ActorDamageSource*`, `float damage`, knockback, and ignite flags. Modify damage amount via `setDamage(newDamage)` or cancel the event via `setCancelled(true)`.
- `ll::event::ActorDieEvent`: Fired when an entity or player dies (`Actor::die`). Exposes `Actor*` and `ActorDamageSource*`.

---

## 13. Audio Systems (`levilamroid::sound::FmodAudioEngine` & `SoundEngine`)

Located in `#include <levilamroid/api/sound/FmodSound.h>` and `#include <levilamroid/api/sound/SoundEngine.h>`.

### Method A: Direct In-Memory FMOD Audio (`FmodAudioEngine`)
Plays raw embedded audio files (WAV, OGG, MP3) directly from RAM using Minecraft's loaded `libfmod.so` without touching disk or creating resource packs:

```cpp
#include <levilamroid/api/sound/FmodSound.h>

// Play audio bytes from memory
levilamroid::sound::FmodAudioEngine::getInstance().playMemory(
    rawBytes.data(),
    rawBytes.size(),
    /*volume=*/ 1.0f,
    /*pitch=*/ 1.0f,
    /*loop=*/ false
);
```

### Method B: Native Sound Engine Physics (`SoundEngine`)
Hooks the internal Bedrock audio engine to position spatialized 3D sounds in the world:

```cpp
#include <levilamroid/api/sound/SoundEngine.h>

// Play sound at 3D coordinates
levilamroid::mcpe::SoundEngine::play(
    "random.explode",
    levilamroid::mcpe::Vec3(100.0f, 64.0f, -200.0f),
    /*volume=*/ 1.0f,
    /*pitch=*/ 0.9f
);
```

---

## 14. Virtual In-Memory Resource Packs (`levilamroid::resource`)

Override textures, sounds, and UI definitions in RAM on-the-fly without modifying APK assets or copying resource packs to storage.

```cpp
#include <levilamroid/api/resource/VirtualResourcePack.h>

// Override vanilla diamond sword texture with custom PNG bytes in memory
levilamroid::resource::VirtualPackHub::getInstance().overrideItemTexture(
    "diamond_sword",
    customPngBytes
);
```

---

## 15. Dear ImGui v1.91.x & OpenGL ES 3.0 Rendering Backend

LeviLamroid bundles **Dear ImGui v1.91.x** with a dedicated OpenGL ES 3.0 backend.

### Context-Loss Resilience
On Android, switching apps, rotating screens, or opening the system file picker destroys the `EGLSurface` and invalidates OpenGL ES textures and shader programs. LeviLamroid includes specialized recovery functions:
- `ImGui_ImplOpenGL3_AreDeviceObjectsValid()`: Tests GL handles via `glIsProgram` and `glIsTexture`.
- `ImGui_ImplOpenGL3_InvalidateDeviceObjects()`: Safely clears handles without invoking invalid `glDelete*` calls on dead contexts.
- `ImGui_ImplOpenGL3_CreateDeviceObjects()`: Recompiles shaders and re-uploads font textures upon new surface creation.

---

## 16. Multi-Target Logging Subsystems (`ll::Logger` & `pl::log::Logger`)

### 16.1 `ll::Logger` (LeviLamina Parity)
Located in `#include <ll/api/Logger.h>`.  
Outputs formatted logs to Android Logcat (`__android_log_print`) and `/sdcard/Android/media/org.levimc.launcher/minecraft/levilamroid.log`.

```cpp
#include <ll/api/Logger.h>

ll::Logger logger("MyMod");
logger.debug("Debug msg: {}", val);
logger.info("Player {} logged in", name);
logger.warn("Warning occurred!");
logger.error("Fatal failure: {}", err);
```

### 16.2 `pl::log::Logger` (PreLoader Formatted Logger)
Located in `#include <pl/Logger.hpp>`.  
Uses `{fmt}` syntax for message formatting:

```cpp
#include <pl/Logger.hpp>

auto& logger = pl::log::Logger::getOrCreate("MyMod");
logger.info("Initialized successfully with code {}", 200);
```

---

## 17. Minecraft Bedrock Engine Class API Reference (`mc/` & `mcpe/`)

### 17.1 ClientInstance & IClientInstance
Header: `#include <mc/client/ClientInstance.h>`

Root engine interface representing the client session.

```cpp
class ClientInstance : public IClientInstance {
public:
    virtual ~ClientInstance() override;

    // Slot 32 on Android ARM64 Bedrock
    virtual LocalPlayer* getLocalPlayer() const override;

    // Slot 151 (1.26.5x+) / Slot 150 (1.26.4x)
    virtual bool isShowingMenu() const override;

    [[nodiscard]] virtual bool isInGameInputEnabled() const noexcept override;
    virtual LevelRenderer* getLevelRenderer() const override;
    virtual PacketSender& getPacketSender() const override;
    virtual Level* getLevel() const override;
};
```

#### Methods
- `LocalPlayer* getLocalPlayer() const`: Returns the local client player instance, or `nullptr` if not in a world.
- `bool isShowingMenu() const`: Returns `true` if any UI screen or pause menu is currently displayed over gameplay.
- `bool isInGameInputEnabled() const noexcept`: Returns `true` if player movement input is enabled.
- `LevelRenderer* getLevelRenderer() const`: Returns the world level renderer.
- `PacketSender& getPacketSender() const`: Returns the client network packet sender.
- `Level* getLevel() const`: Returns the client level world.

---

### 17.2 Player & LocalPlayer
Headers: `#include <mc/world/actor/player/Player.h>` and `#include <mc/world/actor/player/LocalPlayer.h>`

#### `Player` Methods
- `virtual void normalTick()`: Standard actor update tick.
- `virtual void setEquippedSlot(EquipmentSlot slot, const ItemStack& item)`: Equips an item into armor or offhand slot.
- `ItemStack& getOffhandSlot()`: Returns reference to the item currently held in the offhand.
- `const ItemStack& getSelectedItem() const`: Returns the item in the active hotbar slot.
- `int getSelectedItemSlot() const noexcept`: Returns hotbar index `[0..8]`.
- `bool isOffhandItemAllowed(const ItemStack& item) const`: Tests if item can be placed in offhand.
- `virtual BlockSource& getDimensionBlockSource() const`: Returns world block source for player's dimension.
- `virtual Dimension& getDimension() const`: Returns dimension object (Overworld, Nether, End).
- `virtual int32_t getDimensionId() const`: Returns dimension ID (0=Overworld, 1=Nether, 2=The End).
- `virtual Level& getLevel() const`: Returns world level instance.
- `virtual Inventory& getInventory() const`: Returns player inventory container.

#### `LocalPlayer` Methods
- `virtual void move(const Vec3& delta)`: Applies movement displacement to local player.
- `virtual void tickWorld(Tick const& currentTick)`: Client-side tick update.
- `void setSelectedSlot(int slot) noexcept`: Changes active hotbar selection.
- `Vec3 getEyePos() const noexcept`: Returns exact 3D floating-point eye coordinates.
- `Vec3 getViewVector(float a = 1.0f) const noexcept`: Returns forward-facing look unit vector.
- `float getPickRange() const noexcept`: Returns block reach distance (default 5.0 blocks).
- `BlockPos getFeetBlockPos() const noexcept`: Returns integer block coordinates at feet.
- `std::string getLocaleCode() const`: Returns client language code (e.g. `"en_US"`).

---

### 17.3 Inventory & Equipment Slots
Header: `#include <mc/world/actor/player/Inventory.h>`

```cpp
class Inventory {
public:
    virtual ~Inventory() = default;
    [[nodiscard]] virtual ItemStack const& getItem(int32_t slot) const;
    [[nodiscard]] virtual ItemStack& getItem(int32_t slot);
    virtual void setItem(int32_t slot, ItemStack const& item);
    [[nodiscard]] virtual int32_t getContainerSize() const noexcept;
};
```

- `getItem(int32_t slot)`: Returns item in inventory slot (hotbar is slots 0–8).
- `setItem(int32_t slot, ItemStack const& item)`: Places item in specified slot.
- `getContainerSize()`: Total container capacity (36 for main player inventory).

---

### 17.4 BlockSource & Chunk Management
Header: `#include <mc/world/level/BlockSource.h>`

World block access interface.

```cpp
class BlockSource {
public:
    Block const& getBlock(BlockPos const& pos) const;
    Block const& getBlock(int32_t x, int32_t y, int32_t z) const;
    Block const& getExtraBlock(BlockPos const& pos) const;
    Block const& getLiquidBlock(BlockPos const& pos) const;
    BlockActor* getBlockEntity(BlockPos const& pos) const;
    bool areChunksFullyLoaded(BlockPos const& min, BlockPos const& max) const;
    bool hasChunksAt(BlockPos const& min, BlockPos const& max) const;
    ChunkSource& getChunkSource() const;
    bool setBlock(BlockPos const& pos, Block const& block, int updateFlags = 3, void* actor = nullptr);
};
```

- `getBlock(pos)`: Returns the discrete `Block` permutation at the coordinate.
- `getExtraBlock(pos)`: Returns secondary layer block (e.g. water inside waterlogged stairs).
- `getLiquidBlock(pos)`: Returns fluid block permutation.
- `getBlockEntity(pos)`: Returns tile entity `BlockActor*` (chests, signs, furnaces), or `nullptr`.
- `setBlock(pos, block, updateFlags)`: Places block into the world. `updateFlags=3` triggers block update and network broadcast.

---

### 17.5 Block, BlockLegacy & BlockType
Header: `#include <mc/world/level/block/Block.h>` and `<mcpe/BlockTypeRegistry.h>`

#### `Block` Methods
- `uint32_t getRuntimeId() const noexcept`: Palette network runtime ID used in chunk packets.
- `uint32_t getNetworkId() const noexcept`: Network type identifier.
- `std::string const& getTypeName() const noexcept`: Full block name string (e.g. `"minecraft:stone"`).
- `HashedString const& getHashedTypeName() const noexcept`: 64-bit hashed string name.
- `bool isAir() const noexcept`: Returns `true` if block is air.
- `int16_t getBlockItemId() const noexcept`: Associated item identifier.
- `BlockType const& getBlockType() const noexcept`: Base block type definition.
- `bool hasProperty(BlockProperty prop) const noexcept`: Checks properties (Door, Stair, Slab, Button).
- `static Block const* tryGetFromRegistry(HashedString const& name)`: Looks up registered block permutation by name.

#### `BlockLegacy` Methods
- `const HashedString& getRawNameId() const noexcept`: Raw identifier without namespace.
- `const HashedString& getFullName() const noexcept`: Complete identifier (e.g. `"minecraft:iron_ore"`).
- `bool isSolid() const noexcept`: Returns `true` if solid.
- `float getExplosionResistance() const noexcept`: Blast resistance value.
- `float getFriction() const noexcept`: Surface friction multiplier (e.g. 0.6 for normal blocks, 0.98 for ice).
- `uint8_t getLightEmission() const noexcept`: Light level emitted `[0..15]`.
- `bool mayPlace(BlockSource& region, const BlockPos& pos) const`: Placement validation.

---

### 17.6 BlockTypeRegistry & Direct Access Blocks
Header: `#include <mcpe/BlockTypeRegistry.h>`

Provides pre-cached zero-lookup pointers to high-frequency vanilla blocks:

```cpp
#include <mcpe/BlockTypeRegistry.h>

const Block* airBlock = registry.getAir();
const Block* stoneBlock = registry.getStone();
const Block* dirtBlock = registry.getDirt();
```

---

### 17.7 ItemStack & ItemInstance
Headers: `#include <mc/world/item/ItemStack.h>` and `<mc/world/item/ItemInstance.h>`

```cpp
class ItemStack : public ItemInstance {
public:
    ItemStack();
    ItemStack(Block const& block, int count = 1, int aux = 0);
    void reinit(std::string_view name, uint8_t count = 1, uint16_t aux = 0);
    [[nodiscard]] std::string getName() const;
    static ItemStack const& getEmptyItem();
};
```

#### Member Functions
- `bool isNull() const noexcept`: Returns `true` if stack count is 0 or item pointer is null.
- `uint8_t getCount() const noexcept`: Stack count `[0..64]`.
- `void setCount(uint8_t count) noexcept`: Sets stack count.
- `uint16_t getAuxValue() const noexcept`: Metadata / damage value.
- `void setAuxValue(uint16_t aux) noexcept`: Sets metadata / damage.
- `CompoundTag const* getUserData() const noexcept`: Returns custom NBT tag dictionary, or `nullptr`.
- `bool hasUserData() const noexcept`: Returns `true` if item has NBT data (enchantments, custom lore).
- `std::string const& getTypeName() const noexcept`: Item identifier string.
- `std::string getCustomName() const`: Returns custom display name from anvil renaming.
- `int getMaxStackSize() const noexcept`: Max stack capacity (usually 64, 16 for ender pearls, 1 for tools).

---

### 17.8 CompoundTag, Tag & Binary NBT Serialization
Header: `#include <mc/deps/nbt/CompoundTag.h>`

#### `CompoundTag` Methods
- `size_t size() const`: Number of tags in dictionary.
- `bool empty() const`: Returns `true` if empty.
- `bool contains(std::string_view name) const`: Checks if key exists.
- `Tag* put(std::string name, std::unique_ptr<Tag> tag)`: Inserts generic tag.
- `void putInt(std::string name, int32_t val)`: Inserts 32-bit integer.
- `void putString(std::string name, std::string val)`: Inserts string.
- `void putByte(std::string name, int8_t val)`: Inserts 8-bit byte.
- `void putShort(std::string name, int16_t val)`: Inserts 16-bit short.
- `void putFloat(std::string name, float val)`: Inserts 32-bit float.
- `void putDouble(std::string name, double val)`: Inserts 64-bit double.
- `void putCompound(std::string name, std::unique_ptr<CompoundTag> tag)`: Inserts nested compound.
- `std::unique_ptr<CompoundTag> clone() const`: Deep copy.

#### Binary NBT Parsing with Auto-Gzip Decompression
```cpp
// Parses raw .mcstructure or NBT data bytes (decompresses Gzip/Zlib automatically)
CompoundTag::BinaryNbtResult result = CompoundTag::fromBinaryNbt(rawBytes);
if (result) {
    CompoundTag* root = result.get();
    int32_t version = root->contains("format_version") ? 1 : 0;
} else {
    std::string error = result.error().message();
}
```

---

### 17.9 GameMode
Header: `#include <mc/world/gamemode/GameMode.h>`

Controls player interaction, mining, item usage, and attacks:

- `bool useItem(ItemStack& item, HandSlot handSlot = HandSlot::MainHand)`: Right-click / tap item use (eating food, throwing pearls, firing bows).
- `bool buildBlock(BlockPos const& pos, unsigned char face, HandSlot handSlot = HandSlot::MainHand, bool isSimTick = false)`: Places held block at target position.
- `void startBuildBlock(BlockPos const& pos, unsigned char face)`: Begins block breaking.
- `void stopBuildBlock()`: Cancels block breaking.
- `bool useItemOn(ItemStack& item, BlockPos const& pos, uint8_t face, Vec3 const& hitPos, void const* block)`: Uses item targeting a block surface.
- `bool attack(Player& player, Actor& target)`: Attacks an actor or mob entity.

---

### 17.10 StructureTemplate & StructureManager
Headers: `#include <mc/world/level/levelgen/structure/StructureTemplate.h>` and `<mc/world/level/levelgen/structure/StructureManager.h>`

Used for `.mcstructure` and schematic processing:

- `bool load(CompoundTag const& tag)`: Loads structure from NBT compound.
- `bool save(CompoundTag& tag) const`: Serializes structure into NBT compound.
- `void fillFromWorld(BlockSource& region, BlockPos const& pos, LegacyStructureSettings const& settings)`: Captures world region into structure.
- `void placeInWorld(BlockSource& region, BlockPos const& pos, LegacyStructureSettings const& settings)`: Pastes structure into world.
- `static bool exportStructure(StructureTemplate const& structure, Core::Path const& outputPath)`: Writes structure file to disk.
- `static std::unique_ptr<StructureTemplate> importStructure(Core::Path const& inputPath)`: Reads structure file from disk.

---

### 17.11 ChestBlockActor & BlockActor
Header: `#include <mc/world/level/block/actor/ChestBlockActor.h>`

- `ChestBlockActor* mLargeChestPaired`: Pointer to paired double-chest half.
- `bool mIsLarge`: True if part of a double chest.
- `void _tryToPairWith(BlockSource& region, BlockPos const& neighbor)`: Attempts double chest pairing.
- `void unpair()`: Splits double chest into single chest.

---

### 17.12 CommandRegistry
Header: `#include <mc/server/commands/CommandRegistry.h>`

Registers native slash commands into Minecraft Bedrock's command engine:

```cpp
#include <mc/server/commands/CommandRegistry.h>

CommandRegistry::registerCommand(
    registryInstance,
    "mymod",
    "My Mod custom command description",
    CommandPermissionLevel::Any,
    CommandFlag::None,
    CommandFlag::None
);
```

---

### 17.13 MinecraftUIRenderContext & ScreenContext
Header: `#include <mc/client/renderer/screen/MinecraftUIRenderContext.h>`

UI drawing canvas interface passed during screen render passes:

- `ScreenContext* getScreenContext()`: Returns underlying native screen context.
- `void fillRectangle(const RectangleArea& area, mce::Color color, float alpha)`: Draws solid colored rectangle.
- `void drawImage(void* texture, glm::vec2 pos, glm::vec2 size, glm::vec2 uvPos, glm::vec2 uvSize, bool flush)`: Renders 2D texture quad.
- `void flushImages(mce::Color color, float alpha, const HashedString& material)`: Flushes batched image render commands.
- `mce::TexturePtr getTexture(const ResourceLocation& loc, bool fallback)`: Retrieves GPU texture handle.

---

### 17.14 PacketSender & TextPacket
Headers: `#include <mc/network/PacketSender.h>` and `<mc/network/packet/TextPacket.h>`

#### `TextPacket` Factory Helpers
- `static TextPacket createChatPacket(std::string author, std::string message)`: Creates player chat message.
- `static TextPacket createRawPacket(std::string message)`: Creates raw system broadcast message.

#### `PacketSender` Methods
- `virtual void send(Packet& packet) = 0`
- `virtual void sendToServer(Packet& packet) = 0`: Dispatches packet to connected server.
- `virtual void sendToClient(void const* target, Packet const& packet, unsigned char channel) = 0`
- `virtual void sendBroadcast(Packet const& packet) = 0`: Broadcasts packet to all connected clients.

---

### 17.15 I18n & Localization
Header: `#include <mc/locale/I18n.h>`

```cpp
#include <mc/locale/I18n.h>

// Translate Minecraft localization key
std::string playText = I18n::get("menu.play");
std::string custom = I18n::get("chat.type.text", {"PlayerName", "Hello world"});
```

- `static std::string get(std::string_view key, std::vector<std::string> const& params = {})`
- `static std::string get(std::string const& key)`
- `Localization* getLocaleFor(std::string const& code) const`

---

### 17.16 Math & Spatial Coordinate Types (`BlockPos`, `Vec3`, `HashedString`, `BoundingBox`)

- **`BlockPos`** (`#include <mc/world/level/BlockPos.h>`):  
  Integer 3D world coordinates: `int32_t x`, `int32_t y`, `int32_t z`. Supports `==`, `!=`, and addition operators.
- **`Vec3`** (`#include <mc/deps/core/math/Vec3.h>`):  
  Floating-point 3D vector: `float x`, `float y`, `float z`.
- **`HashedString`** (`#include <mc/deps/core/string/HashedString.h>`):  
  Cached 64-bit FNV-1a hash paired with `std::string`. `getHash()` returns `uint64_t`, `getString()` returns `const std::string&`.
- **`BoundingBox`** (`#include <mc/world/level/levelgen/structure/BoundingBox.h>`):  
  Axis-aligned integer cuboid defined by `BlockPos min` and `BlockPos max`.

---

## 18. Bundled Third-Party Utilities (`nlohmann::json`, `glm`)

No external package manager is required for JSON or math:
- `#include <nlohmann/json.hpp>`: Full modern C++ JSON serializer and deserializer.
- `#include <glm/glm.hpp>`: OpenGL Mathematics vectors (`glm::vec2`, `glm::vec3`, `glm::vec4`), matrices (`glm::mat4`), and quaternion math.

---

## 19. Version Policy & Starter Template (`v1.0.0`)

### 19.1 Strict Initial Versioning Rule
**All mods built for LeviLamroid must initialize their version to `v1.0.0` (or `1.0.0`).**  
Do not bump or modify the version beyond `1.0.0` unless explicitly requested by the user.

### 19.2 Starter Template Structure
A complete, turnkey starter template is provided in `template/`:

```
template/
├── CMakeLists.txt         # Configured for Android ARM64
├── manifest.json          # Initialized with version [1, 0, 0]
├── package.sh             # One-click build, .levipack & .zip packager
├── README.md              # Template documentation
└── src/
    ├── ExampleMod.h       # Mod class inheriting ll::mod::NativeMod
    └── ExampleMod.cpp     # Full implementation with HUD button & event listeners
```

### 19.3 Quick Build & Package Command
```bash
cd template
bash package.sh
```
This generates:
- `dist/libexample_mod.so`
- `dist/example_mod.levipack`
- `dist/ExampleMod-1.0.0-android-arm64.zip`
- Deploys automatically to `/sdcard/Android/media/org.levimc.launcher/minecraft/<profile>/mods/`.

---

## 20. Client Command System (`ll::command::CommandRegistrar`)

Located in `#include <ll/api/command/CommandRegistrar.h>`.  
Provides LeviLamina-compatible client-side slash command registration:

```cpp
#include <ll/api/command/CommandRegistrar.h>

// Register /mycommand with alias /mc
auto& cmd = ll::command::CommandRegistrar::getInstance().getOrCreateCommand(
    "mycommand",
    "Description of my custom command",
    ll::command::CommandPermissionLevel::Any
);

cmd.alias("mc");
cmd.execute([](const std::vector<std::string>& args) {
    ll::Logger::getInstance().info("Command executed with ", args.size(), " arguments.");
});
```

---

## 21. GameTick Task Scheduler (`ll::chrono::GameTickScheduler`)

Located in `#include <ll/api/chrono/GameTickScheduler.h>`.  
Executes delayed or repeating tasks synchronized with Minecraft's main game tick loop:

```cpp
#include <ll/api/chrono/GameTickScheduler.h>

// 1. One-shot task: run 40 ticks (~2 seconds) in the future
ll::chrono::scheduleTask(40, []() {
    ll::Logger::getInstance().info("2 seconds have passed since command!");
});

// 2. Repeating task: run every 20 ticks (1 second)
auto taskId = ll::chrono::scheduleRepeatingTask(20, []() {
    // Tick heartbeat
});

// 3. Cancel task
ll::chrono::cancelTask(taskId);
```

---

## 22. Form & Modal Dialogs (`ll::form`)

Located in `#include <ll/api/form/Form.h>`.  
Construct native Bedrock interactive UI dialogs with callback responses:

### 22.1 Simple Form
```cpp
#include <ll/api/form/Form.h>

ll::form::SimpleForm form("Mod Menu", "Select an option:");
form.addButton("God Mode", []() {
    ll::Logger::getInstance().info("God Mode toggled");
});
form.addButton("Teleport to Spawn", []() {
    ll::Logger::getInstance().info("Teleported to spawn");
});

// Serialize JSON payload for Bedrock client
std::string jsonPayload = form.serialize();
```

### 22.2 Modal Form (Yes / No)
```cpp
ll::form::ModalForm modal("Confirm Action", "Do you want to clear your inventory?");
modal.setUpperButton("Confirm");
modal.setLowerButton("Cancel");
modal.onResponse([](bool confirmed) {
    if (confirmed) {
        // Handle confirm
    }
});
```

---

## 23. LeviLamina Migration Guide & Android Parity Matrix

For a complete guide on porting desktop LeviLamina mods to Android using LeviLamroid, see:
[`PORTING_FROM_LEVI_LAMINA.md`](file:///home/devroom/LeviLamroid/PORTING_FROM_LEVI_LAMINA.md)

It details:
- **Build configuration changes** (CMake / Clang AArch64)
- **Calling convention differences** (AAPCS64 registers X0-X7 vs MSVC x64)
- **Hooking conversions** (RVA and Itanium VTable slot indexing)
- **Android mobile input** (`pl::input` and `pl::modmenu`)
- **What cannot exist on Android** (Windows SEH, PDB runtime parsing, Win32 APIs)

---

## 24. High-Level Human Engine APIs (`ll::ui`, `ll::render`, `ll::isInWorld`)

To allow client mods to function seamlessly without hardcoding version-specific memory RVAs or managing manual memory detours, LeviLamroid provides high-level human helper APIs:

### 24.1 UI Human Functions (`ll::ui`)
Header: `#include "ll/api/ui/UI.h"`

- `void ll::ui::forceJsonUIForScreen(const std::string& screenName)`: Forces the specified screen (e.g. `"/main-menu"`, `"start_screen"`) to use the legacy JSON UI tech stack instead of OreUI.
- `void ll::ui::onStartMenuOpen(void (*cb)())`: Registers a callback invoked immediately when the start menu controller opens.
- `void ll::ui::onPlayButtonClicked(void (*cb)())`: Registers a callback invoked when the user taps the Play button on the main menu.
- `void ll::ui::onSettingsButtonClicked(void (*cb)())`: Registers a callback invoked when Settings is tapped.
- `void ll::ui::onRealmsButtonClicked(void (*cb)())`: Registers a callback invoked when Realms is tapped.
- `void ll::ui::onDressingRoomButtonClicked(void (*cb)())`: Registers a callback invoked when the Dressing Room / Profile button is tapped.
- `void ll::ui::onMarketplaceButtonClicked(void (*cb)())`: Registers a callback invoked when Marketplace is tapped.
- `void ll::ui::onMenuExit(void (*cb)())`: Registers a callback invoked when returning or exiting a sub-menu back to the start menu.

### 24.2 Render Human Functions (`ll::render`)
Header: `#include "ll/api/render/Render.h"`

- `void ll::render::suppressVanillaBackground(bool suppress)`: Suppresses vanilla cubemap panorama drawing strictly on menus. Automatically disabled when in-world so gameplay skies are untouched.
- `void ll::render::registerPostClearCallback(void (*cb)(int width, int height))`: Registers a GPU render callback executed immediately after the OpenGL ES `glClear` color pass. Strictly active on menus and disabled in-world.

### 24.3 Engine State Queries (`ll`)
Header: `#include "levilamroid/LeviLamroid.h"`

- `bool ll::isInWorld()`: Returns `true` when playing in an active world. Continuously affirmed by golden tick hooks (`Player::normalTick`, `Level::tick`, `Actor::normalTick`, and `LocalPlayer::move`). Automatically resets to `false` when the start menu opens.
- `bool ll::isMainMenu()`: Returns `true` strictly while idling on the start menu (`StartMenuScreenController::$onOpen`). Automatically set to `false` as soon as world loading begins (`ProgressBarRenderer`) or world ticks start. Defaults safely to `false` if uninitialized.

