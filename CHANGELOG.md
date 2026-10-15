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
