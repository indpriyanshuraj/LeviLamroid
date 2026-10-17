# Changelog

All notable changes to **LeviLamroid** are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and release versions follow [Semantic Versioning](https://semver.org/).

## [1.0.2-beta] - 2026-10-17

### Added
- **Native C++ Item Subsystem (`ItemRegistry` & `Item`)**:
  - Direct in-memory item registration via hooked `ItemRegistry::registerItem` (`0x0FBB0064` on 1.26.52.3 ARM64) and native `Item::Item` constructor (`0x0FF7D35C`).
  - Strict AArch64 in-memory structure field alignment: `mMaxStackSize` (+0x88), `mCreativeCategory` (+0x140), `mCreativeGroup` (+0x148 as `HashedString`), `mAllowOffhand` (+0x1C8), `mIsHandEquipped` (+0x1CA), and `mMaxDamage` (+0x1D0).
  - Robust Bedrock `SharedPtr<Item>` allocation with heap-allocated `BedrockControlBlock` (`mWeakRef = 1`, `mStrongRef = 1`), completely eliminating crashes on world entry.
  - Hooked `ItemRegistry::postInit` (`0x0FBB77DC`) for thread-safe item injection upon registry creation.
  - Exported canonical C APIs `ll_item_register_full` and `ll_item_give_player` for cross-module mod linking.
- **Dynamic Asset & Texture Pipeline Interception**:
  - Hooked `DirectoryPackAccessStrategy::getAsset` / `hasAsset` and `ZipPackAccessStrategy::getAsset` / `hasAsset` across supported Bedrock versions.
  - Intercepts requests for `textures/item_texture.json` and `textures/terrain_texture.json`, dynamically appending texture definitions with `"resource_pack_name": "vanilla"` so custom textures resolve in both the 2D UI and 3D RenderDragon pipelines.
  - Native `Item::setIcon` call (`0x0FF83DDC`) linking item icon strings directly to registered atlas keys.
  - Dynamic `.lang` localization merging for in-game item and block display names (`item.<id>.name`, `tile.<id>.name`).
  - Automatic asset mounting: `PluginManager::loadPlugin` automatically indexes `resource_packs/` and `resources/` within mod directories and registers their assets into the dynamic asset pipeline.
- **Creative Inventory & Category Injection**:
  - Hooked `serverInitCreativeItemsCallback` (`0x0FBC3604` / `VanillaItems::serverInitCreativeItemsCallback`) to register custom items directly into Bedrock's `CreativeItemRegistry` alongside vanilla items.
  - Single items (weapons/swords) and stackable items (materials/blocks) properly respect category (Construction, Equipment, Items) and stack counts.
- **Custom Block System Foundation & `BlockItem` VTable ABI**:
  - Introduced `BlockHooks.cpp` and `levilamroid::block::registerCustomBlock` (`ll_block_register`).
  - Reconstructed custom `_ZTV9BlockItem` VTable (`sCustomBlockItemVtableStorage` derived from native `0x13099938`) with overridden virtual slots:
    - Slot 95: `buildDescriptionId` -> returns `"tile.<id>.name"`.
    - Slot 118: `getIconInfo` -> returns `ItemIconInfo(textureKey, 0, 1)`.
    - Slot 131: `_useOn` -> custom block placement logic (`BlockItem::_useOn` at `0x1000B300`).
  - Backing `BlockType` resolution via `BlockTypeRegistry::getInstance` (`0x103E33A8`) and `BlockTypeRegistry::lookupByName` (`0x103E5C9C`).
- **Client-Side Command Interception (`LoopbackPacketSender::send`)**:
  - Hooked `LoopbackPacketSender::send` for Packet ID 77 (`CommandRequestPacket`), allowing client-registered commands (e.g. `/levi give`, `/levi fx`, `/levi camlock`) to be processed locally and canceled before network transmission, preventing "Unknown command" server rejections.
  - Camera orientation control via hooked `LocalPlayer::applyTurnDelta` (`0x0AAC9774`) with `ll_camera_set_locked`.
  - Continuous local player tracking via hooked `ClientInstance::update` (`0x098036A4`) and `ClientInstance::getLocalPlayer` (`0x09808050`).
- **Gameplay Action Hooks & 3D World FX**:
  - Hooked `SurvivalMode::startDestroyBlock` (`0x0F8A4048`), `SurvivalMode::useItem` (`0x0F8A4244`), and `GameMode::attack` (`0x0F89FA74`).
  - Added `PlayerAttackEvent` in `include/ll/api/event/player/PlayerEvents.h`.
  - Mathematical 3D Celestial Double-Helix particle vortex generator around the player in real-time world space.
- **Build System & Packaging Automation**:
  - Added `xmake.lua` configurations for both `LeviLamroid` and `LeviItems` enabling sub-second incremental builds on Termux / Linux ARM64.
  - Packaging automation (`package.sh`) producing `LeviLamroid-1.0.2-beta-android-arm64.zip` and auto-deploying to active LeviLauncher profiles.

---

## [1.0.1-beta] - 2026-10-16

### Added
- **100% Signature-Based Engine Hooking**:
  - All core engine hooks across gameplay (`LocalPlayer::move`, `Player::normalTick`, `Level::tick`, `Actor::normalTick`, `GameMode::destroyBlock`, `GameMode::useItemOn`), rendering (`PanoramaRenderer::render`, `CubemapBackgroundScreen::render`, `ProgressBarRenderer::render`), and UI (`StartMenuScreenController::tick`, `_onOpen`, buttons, exit) now use `ll::memory::hookSignature`.
  - Built-in $O(1)$ fast-path verification checks against known RVAs in under 1 µs, falling back to automated `memchr`-accelerated ELF `.text` wildcard scanning for seamless resilience across point updates.
- **Expanded Multi-Version Bedrock Support**:
  - Full native support for **Minecraft Bedrock 1.26.44.3**, **1.26.45.1**, **1.26.51.1**, and **1.26.52.3** on LeviLauncher (Android ARM64).
  - Centralized version resolution in `Offsets.cpp` using ELF `PT_NOTE` segment GNU Build ID hashing with zero version coupling in mod runtime headers.
- **Dynamic Profile & Configuration Discovery**:
  - `NativeMod::getConfigDir()` and packaging automation now dynamically scan all active LeviLauncher profile directories under `/sdcard/Android/media/org.levimc.launcher/minecraft/*/mods`.
  - Exported `ll_mcpe_get_detected_version()` for clean dynamic version querying across loaded native mods.

### Fixed & Improved
- **Start Menu Lifecycle & Sub-Screen Return Synchronization**:
  - Hooked `StartMenuScreenController::tick` to create a robust, universal lifecycle tracker for the start screen across all Bedrock versions.
  - Correctly defers initial menu entry events and callbacks until Bedrock finishes loading assets and dismisses the loading screen (`!isLoading()`).
  - Automatically detects when returning/popping back to the start menu from any sub-screen (Play, Settings, Realms, Dressing Room, Marketplace), reliably dispatching `onStartMenuOpen()` and publishing `StartMenuOpenEvent` so client mods can seamlessly restore main menu state, background effects, or custom camera orientations.
- **Backbuffer GPU FBO Filtering (`hooked_glClear`)**:
  - Post-clear rendering callbacks strictly target the default display framebuffer (`GL_FRAMEBUFFER_BINDING == 0`), preventing redundant draw cycles and texture corruption during off-screen UI composition passes.
- **Purged Hardcoded Profile Paths**:
  - Eliminated legacy hardcoded profile references (`Minecraft_1.26.45.1`) across mod headers and scripts, ensuring complete version-agnostic profile management.

---

## [1.0.0-beta] - 2026-10-15


### Initial Beta Testing Release (by SpyGamingOG)

This marks the **Initial Beta Testing Release** of **LeviLamroid** (`v1.0.0-beta`), a next-generation native C++ mod loader runtime and reverse-engineering hook hub engineered for **Minecraft Bedrock Edition (Android ARM64)** on **LeviLauncher (LeviLaunchroid)**.

> [!WARNING]
> **Active Development & Beta Testing Notice**  
> LeviLamroid is currently in an active **Beta Testing** phase. Because Android Bedrock is a rapidly evolving closed-source client binary with frequent updates and device-specific graphics drivers:
> - Certain hooks, functions, and internal Bedrock class layouts are experimental and subject to change.
> - Some game hooks or event callbacks may not function correctly across all minor engine patches or modified APKs.
> - Developers and testers may encounter bugs, crashes, or unmapped RVAs.
> - Please report any unexpected behavior or crashes with logcat traces on the official issue tracker: [GitHub Issues](https://github.com/spygamingog/LeviLamroid/issues).

---

### Key Capabilities & Architecture

#### 1. Core Architecture & LeviLamina C++ Source Parity
- **Canonical `ll` Namespace**: Standardized `include/ll/api/` as the single canonical implementation layer for LeviLamina abstractions, allowing client mods written for Windows LeviLamina to compile and run on Android ARM64 with minimal adjustments.
- **Master Header**: Single-include convenience via `#include <levilamroid/LeviLamroid.h>` providing the complete SDK surface.
- **Service Locator**: Added `ll::service::Bedrock` providing thread-safe static access to the primary `Level` pointer and game services.
- **Chrono & Tick Scheduler**: Implemented `ll::chrono::GameTickScheduler` (`scheduleTask`, `scheduleRepeatingTask`) synced to Minecraft's 20Hz tick loop.
- **Command Registration**: Added `ll::command::CommandRegistrar` and `CommandHandle` for registering custom in-game slash commands.
- **Form UI Dialogs**: Implemented `ll::form::SimpleForm` and `ModalForm` dialog wrappers for interactive in-game menus.

#### 2. Universal Memory Detour & Hook Engines
- **Atomic AArch64 Inline Detours (`ll::memory::hook`)**: 16-byte atomic jump generation (`LDR X16, #8; BR X16`) with executable trampoline pooling and automated instruction cache flushing (`__builtin___clear_cache`).
- **Relative Virtual Address Hooking (`ll::memory::hookRva`)**: Direct hook binding relative to the dynamic base address of `libminecraftpe.so`.
- **Global Offset Table Patching (`ll::memory::hookGotSlot`)**: Dynamic GOT relocation hooking for intercepting imported shared library calls (e.g. `glClear`, `glClearColor`, `eglSwapBuffers`).
- **Wildcard Pattern Scanner (`levilamroid::memory::scanMinecraft`)**: Automated memory page scanning for IDA-style byte patterns with wildcards.

#### 3. Dynamic Engine Version Detection & Golden RVA Tables
- **Zero-Latency GNU Build ID Resolution**: Scans the loaded ELF `PT_NOTE` segment in memory via `dl_iterate_phdr` to match the exact 20-byte SHA-1 Build ID:
  - `56de9eed077631e03a31f4f58eb2f0e00071d335` $\rightarrow$ Bedrock `1.26.52.3`
  - `868e275cb295e9a275bb29d2258edc2f7dc48761` $\rightarrow$ Bedrock `1.26.45.1`
- **Zero Binary Bloat**: Each supported version adds only ~200 bytes of RVA table entries in `.rodata`, avoiding redundant code duplication.

#### 4. Thread-Safe Event Bus System
- **Event Dispatcher (`ll::event::EventBus`)**: Lock-guarded publish/subscribe event bus supporting lambda listeners and priority ordering.
- **UI Navigation Events**: Interception for start screen opening, menu buttons (Play, Settings, Realms, Dressing Room, Marketplace), and screen exit.
- **Render Events**: Interception of OpenGL ES `glClear`, cubemap background rendering, and progress bar drawing.
- **Entity & Player Events**: Real-time events for player movement, normal tick, respawn, item usage, actor hurt, and actor death.
- **World & Audio Events**: Block breaking, block placement, level tick, and 3D sound playback interception.

#### 5. PreLoader Lifecycle & Mod Management
- **LeviLauncher PreLoader ABI**: Full implementation of `PLGetModRegistration` with aligned 64-bit function pointers (`load`, `enable`, `disable`, `unload`) matching LeviLauncher's native lifecycle.
- **Dynamic Plugin Manager (`ll::plugin::PluginManager`)**: Discovers and loads `.so` client mods from LeviLauncher `mods/` directories at runtime.
- **LeviLamina ABI Fallback**: Also exports `ll_mod_load` for compatibility with LeviLamina C ABI plugins.

#### 6. Mobile Touch & In-Game HUD Systems
- **Touch & Hardware Input Interception (`pl::input`)**: Captures touch coordinates, physical keyboard keycodes, and controller inputs without locking the player camera.
- **On-Screen Floating HUD Buttons (`pl::modmenu::registerButton`)**: Creates touch-friendly floating action buttons with customizable geometry, colors, labels, and callbacks.
- **Dear ImGui v1.91.x Integration**: Bundled Dear ImGui with an optimized OpenGL ES 3.0 Android rendering backend.

#### 7. In-Memory Audio & Virtual Resource Pack Hub
- **Direct In-Memory FMOD Audio (`levilamroid::sound::FmodAudioEngine`)**: Direct playback of raw WAV/OGG/MP3 byte streams in memory via FMOD without creating disk files or resource packs.
- **Bedrock Sound Physics Hooking (`levilamroid::mcpe::SoundEngine`)**: C++ class interfaces and hooks for `play`, `playUI`, and `playAt` to power spatial audio and reverberation mods.
- **Virtual Pack Hub (`levilamroid::resource::VirtualPackHub`)**: Mounts an `InMemoryPackAccessStrategy` to override vanilla textures and audio definitions dynamically in RAM.

#### 8. Official C++20 Starter Template
- Complete ready-to-build mod template in `template/` including `CMakeLists.txt`, `manifest.json`, HUD button demo, event bus listeners, and one-click packaging (`package.sh`).

#### 9. High-Level Human Engine APIs & Dynamic Symbol Linkage
- **Human-Friendly Engine APIs**: Added `ll::ui` and `ll::render` namespaces with convenient callbacks (`ll::ui::onStartMenuOpen`, `ll::ui::onPlayButtonClicked`, `ll::ui::forceJsonUIForScreen`, `ll::render::suppressVanillaBackground`, `ll::render::registerPostClearCallback`, `ll::isInWorld()`, `ll::isMainMenu()`), removing the need for client mods to hardcode raw hex addresses or manage complex memory detours.
- **RTLD_GLOBAL Self-Promotion**: Automatically promotes `liblevilamroid.so` to `RTLD_GLOBAL` at initialization so subsequent mods can resolve symbols seamlessly across dlopen boundaries.
- **Dynamic Symbol Dispatch**: Client headers resolve functions dynamically via `dlsym`, preventing launcher load-time `dlopen` failures.

#### 10. Robust In-World Detection & Skybox Pass Protection
- **Verified Golden RVAs for Tick Cycles**: Corrected `LocalPlayer::move`, `Player::normalTick`, `Level::tick`, `GameMode::destroyBlock`, and `GameMode::useItemOn` for Bedrock 1.26.52.3 (`0x0F9E26C0`, `0x0FB313C4`, `0x0F287188`, `0x0F8A03AC`, `0x0F89FE18`) and 1.26.45.1 (`0x0F0B1680`, `0x0F222D0C`, `0x0EA8DE1C`, `0x0EF72C18`, `0x0EF72684`), replacing stale placeholder addresses.
- **`Actor::normalTick` Interception**: Added hook on `Actor::normalTick` (`0x0F27DA94` / `0x0EA412D4`) guaranteeing that entity ticks in world continuously affirm `setInWorld(true)` and `setIsMainMenu(false)`.
- **Screen Exit Guarding**: Purged rogue `setIsMainMenu(true)` call from `hooked_menuExit`, ensuring screen transitions (exiting menus to load worlds, closing pause menus) never falsely report being on the main menu.
- **Dual-State Render Pass Protection**: OpenGL ES `glClear` callbacks and cubemap/panorama suppressions now require both `isMainMenu() && !isInWorld()`. In-world skyboxes and RenderDragon passes are 100% untouched.
- **Safe Fallback Semantics**: `ll::isMainMenu()` now defaults to `false` when unbound, preventing false positive menu states during engine initialization and world loading.
