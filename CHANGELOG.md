# Changelog

All notable changes to **LeviLamroid** are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and release versions follow [Semantic Versioning](https://semver.org/).

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
