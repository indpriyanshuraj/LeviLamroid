# LeviLamroid (Android ARM64)

[![Platform](https://img.shields.io/badge/Platform-Android%20ARM64-green.svg)](https://github.com/spygamingog)
[![Target](https://img.shields.io/badge/Minecraft-Bedrock%20v1.26.x-blue.svg)](https://minecraft.net)
[![Role](https://img.shields.io/badge/Role-Core%20Mod%20Loader%20Runtime-orange.svg)](src/)
[![Version](https://img.shields.io/badge/Version-1.0.3--beta-purple.svg)](https://github.com/spygamingog/LeviLamroid/releases)
[![Status](https://img.shields.io/badge/Status-Beta%20Testing-yellow.svg)](https://github.com/spygamingog/LeviLamroid)

A next-generation native C++ mod loader runtime and reverse-engineering hook hub for **Minecraft Bedrock Edition (Android ARM64)**.

Developed and maintained by **[SpyGamingOG](https://github.com/spygamingog)**.  
Inspired by the architecture of Windows [LeviLamina](https://github.com/LiteLDev/LeviLamina), redesigned from the ground up for Android ARM64 Linux kernels and **LeviLauncher (LeviLaunchroid)**.

> [!WARNING]
> ### ⚠️ Under Active Development — Public Beta Release (`v1.0.3-beta`)
> **LeviLamroid is currently in active public beta testing.**
> - **Reverse Engineering Status**: Core hooking, UI screen navigation, RenderDragon graphics passes, and audio are verified and stable. **Internal Item & Block C++ Component Graphs (`FoodItemComponent`, `SpawnEggItem`, `BlockDefinitionGroup` state permutation schemas) are NOT completely discovered** and are undergoing active reverse engineering.
> - **Crash Mitigation**: LeviLamroid safely intercepts `GameMode::useItem` and `GameMode::useItemOn` to handle consumption and placement via command bridges and stack decrementing, preventing engine crashes while component schemas continue to be mapped.
> - **Multi-Version Coverage**: Native verified support across Bedrock 1.26.44.3, 1.26.45.1, 1.26.51.1, and 1.26.52.3 on LeviLauncher.
> - **Bug Reports Welcome**: If you encounter an unmapped signature, a broken hook, or an unexpected crash, please capture a logcat (`adb logcat`) and open an issue on the [GitHub Issue Tracker](https://github.com/spygamingog/LeviLamroid/issues).

---

## Why LeviLamroid?

Traditionally on Android Bedrock, each individual client mod had to:
1. Re-discover `libminecraftpe.so` memory base manually via `/proc/self/maps`.
2. Hardcode its own RVA offsets and duplicate inline hooking engines.
3. Write custom GOT table patchers.
4. Risk symbol collision and hook conflicts between multiple mods.
5. Ship separate versions of `.so` files for different Minecraft versions.

**LeviLamroid** centralizes all engine reverse-engineering, hooking, and lifecycle dispatch into a single unified runtime. Individual mods link against LeviLamroid or listen to its event bus, eliminating duplicate code, binary bloat, and hook thrashing.

---

## Core Capabilities

1. **Universal Hook Engine (`ll::memory`)**:
   - `hook(target, detour, original)`: Inline detour hooking with automated AArch64 trampoline allocation and CPU instruction cache invalidation (`__builtin___clear_cache`).
   - `hookRva(rva, detour, original)`: RVA-based hooking relative to `libminecraftpe.so` base.
   - `hookSignature(pattern, detour, original, fallbackRva)`: Hybrid Signature Hooking Engine with $O(1)$ fast-path direct memory verification and automated `memchr`-accelerated ELF `.text` pattern scanning fallback.
   - `hookGotSlot(gotRva, detour, original)`: Global Offset Table (GOT) relocation hooking for GLES (`glClear`) / libc functions.
   - LeviLaunchroid PreLoader ABI bridge.

2. **Dynamic Multi-Version Detection Engine (`ll::mcpe`)**:
   - **Zero-Latency GNU Build ID Resolution**: Scans the loaded ELF `PT_NOTE` segment via `dl_iterate_phdr` to extract the unique 20-byte SHA-1 Build ID:
     - `56de9eed077631e03a31f4f58eb2f0e00071d335` $\rightarrow$ Bedrock `1.26.52.3` (100% complete RVAs across gameplay, UI, render, and virtual resource pack access strategies)
     - `19d675681604a80ce6a8ff35dfb66f287e0766e0` $\rightarrow$ Bedrock `1.26.51.1` (100% complete RVAs across gameplay, UI, render, and virtual resource pack access strategies)
     - `868e275cb295e9a275bb29d2258edc2f7dc48761` $\rightarrow$ Bedrock `1.26.45.1` (100% complete RVAs across gameplay, UI, render, audio, commands, and virtual resource pack access strategies)
     - `7ff153b6cba8d148e64c2ec582f3484f9ea4457e` $\rightarrow$ Bedrock `1.26.44.3` (100% complete RVAs across gameplay, UI, render, and virtual resource pack access strategies)
   - **Zero Binary Bloat**: Each supported version adds only ~200 bytes of RVA table entries in `.rodata`, requiring no duplicated hook machinery or bloated codebases.
   - Dynamic pattern scanner (`scanMinecraft`) for automated signature matching on newer releases.

3. **Event-Driven Architecture (`ll::event::EventBus`)**:
   - **UI Navigation Events**: `StartMenuOpenEvent`, `PauseMenuOpenEvent`, `PauseMenuCloseEvent`, `PlayButtonClickedEvent`, `SettingsButtonClickedEvent`, `RealmsButtonClickedEvent`, `MarketplaceButtonClickedEvent`, `DressingRoomButtonClickedEvent`, `MenuExitEvent`, `ScreenTechStackQueryEvent`.
   - **Render Events**: `GlClearEvent` (GPU render thread interception), `CubemapRenderEvent` (skybox suppression & overriding), `ProgressBarRenderEvent`.
   - **Gameplay Events**: `PlayerMoveEvent`, `PlayerTickEvent`, `PlayerUseItemEvent`, `LevelTickEvent`, `BlockBreakEvent`.

4. **Dynamic Plugin Manager (`ll::plugin::PluginManager`)**:
   - Auto-discovers and loads `.so` client mods from LeviLauncher `mods/` directories.
   - Supports both `PLGetModRegistration` (LeviLaunchroid PreLoader ABI with aligned 64-bit function pointers) and `ll_mod_load` (LeviLamina) C ABIs.

5. **Resourcepack-Free Native Audio Architecture (`levilamroid::sound`)**:
   - **Method A (Direct In-Memory FMOD Audio)**:
     - Directly interfaces with `libfmod.so` to play raw WAV/OGG/MP3 byte arrays in memory without requiring a `sounds.json` definition or an `.mcpack` file on disk.
     - *Best for*: Custom UI clicks, narration, menu background music, mod-specific sound alerts.
   - **Method B (Engine Core `SoundEngine` & Sound Physics)**:
     - Native C++ engine interfaces for `SoundEngine::play(name, pos, vol, pitch)`, `SoundEngine::playUI`, and `SoundEngine::playAt(actor, ...)`.
     - Canonical `LevelSoundEvent` enum coverage matching Bedrock 1.26.x.
     - `ll::event::SoundPlayEvent` and `ll::event::LevelSoundEventOccurred` event bus subscriptions.
     - *Best for*: In-game gameplay mods like **Sound Physics** (calculating block raycast acoustic occlusion, cavern reverberation, water muffling, and 3D spatial attenuation).

6. **Resourcepack-Free Native Texture Architecture (`levilamroid::mcpe::mce` & `levilamroid::resource`)**:
   - **Option A (Direct GPU VRAM Upload via `mce::TextureGroup`)**:
     - Uploads raw RGBA8888 pixel buffers directly into RenderDragon VRAM via `mce::TextureGroup::uploadTexture`.
     - *Best for*: Dynamic HUDs, minimaps, procedural textures, custom UI icons, and in-game 3D rendering.
   - **Option B (In-Memory Virtual Resource Pack via `VirtualPackHub`)**:
     - Mounts an `InMemoryPackAccessStrategy` on top of Minecraft's `ResourcePackStack`.
     - Overrides existing vanilla block textures (`textures/blocks/stone.png`), item textures (`textures/items/diamond_sword.png`), or `sounds/sound_definitions.json` dynamically in RAM without modifying APK assets or creating disk files.
     - *Best for*: Texture replacement mods, dynamic item skins, vanilla asset overrides.

7. **Client Fabric API & RenderDragon In-Game Graphics Pipeline (`ll::event`)**:
   - **3D World Render Pass (`ll::event::RenderLevelEvent`)**:
     - Hooked directly into `InGamePlayScreen::_renderLevel` (`0x09ACD4C8`).
     - Enables in-world 3D drawing: entity ESP, tracers, waypoint markers, custom mob bounding boxes, and block outlines.
   - **2D In-Game HUD & UI Pass (`ll::event::UIRenderEvent`)**:
     - Hooked into `InGamePlayScreen::render` (`0x09ACC828`) and `MinecraftUIRenderContext::flushImages` (`0x0B251D70`).
     - Enables 2D in-game HUDs: armor status overlays, keystrokes, coordinate displays, custom crosshairs, and minimaps.
   - **Frame Preparation (`ll::event::RenderFramePrepareEvent`)**:
     - Hooked into `GameRenderer::_prepareFrame` (`0x0B2EB50C`) for matrix transformations and camera projection updates.
   - **Camera Interpolation (`LevelRendererPlayer::preRenderUpdate`)**:
     - Hooked into `LevelRendererPlayer::preRenderUpdate` (`0x0B29B620`) for smooth camera motion, view bobbing, and free-cam.
   - **Network & Transaction Hooks**:
     - Intercepts `ClientNetworkHandler::handle` (`0x09EE5908`), `LoopbackPacketSender::sendToServer` (`0x0C68C410`), and `LocalPlayer::sendComplexInventoryTransaction` (`0x0AACF1A0`).

8. **Dynamic Boot Signature Scanner Engine (`ll::memory::SignatureEngine`)**:
   - High-performance, multi-tier signature engine scanning the 205.8 MB `.text` section of `libminecraftpe.so`.
   - **Fast-Path**: Verifies the known RVA against the 100% unique AOB signature in $O(1)$ time.
   - **Localized Search**: Searches a $\pm 64\text{ KB}$ window around the RVA if minor binary drift occurred.
   - **Full `.text` Scan**: Scans with `scanMinecraftText` using `memchr` vector acceleration.
   - Thread-safe RVA caching guarantees that once resolved, lookups have zero runtime overhead.

9. **Native Custom Blocks & Placement Interception (`ll::block`)**:
   - `ll_block_register(identifier, displayName, textureName, ...)` and `ll_block_register_with_base(...)`.
   - Reverse-engineered base constructor `BlockItem::BlockItem` (`0x10009DC4`), vtable `0x13099938`, and placement routine `BlockItem::_useOn` (`0x1000B300`).
   - *Technical Note*: Full runtime block state permutation graphs and schema registration remain under active reverse engineering. LeviLamroid currently intercepts placement in `GameMode::useItemOn` to safely trigger block placement without memory corruption.

10. **100% LeviLamina API Parity (`include/ll/api/`)**:
   - Provides drop-in compatibility headers for Windows LeviLamina client mods, allowing seamless compilation and porting to Android.

11. **Unified BP-in-RP Addon Ingestion Architecture (`ll::resource` & `ll::addon`)**:
   - **Zero Behavior Pack Overhead**: Merges Behavior Pack JSON definitions (`items/*.json`, `blocks/*.json`, `recipes/*.json`) directly inside the mod's single Resource Pack directory.
   - **Safe Ingestion & Component Sanitization**: Automatically scans `blocks/` before `items/`, strips `"minecraft:block_placer"` to avoid Bedrock 1.21+ schema validator discards, and remaps unsafe creative categories to ensure clean inventory visibility while native components are being reversed.
   - **No Experimental Warnings**: Bypasses the need for world-level Behavior Pack activation, avoiding "Holiday Creator Features" experimental prompts and keeping Xbox achievements intact.

---

## Installation

### Automatic Installation
Run the build & packaging script:
```bash
bash package.sh
```
This automatically compiles `liblevilamroid.so`, generates `levilamroid.levipack`, and deploys LeviLamroid to your active LeviLauncher profiles with load order `0` (highest priority).

### Manual Installation
1. Copy `liblevilamroid.so` and `manifest.json` into:
   ```
   /sdcard/Android/media/org.levimc.launcher/minecraft/<profile>/mods/LeviLamroid/
   ```
2. In `mods_config.json`, ensure `LeviLamroid` is present and enabled with `"order": 0`.

---

## Developing Mods with LeviLamroid

Writing a mod for Android Bedrock with LeviLamroid is as simple as subscribing to events:

```cpp
#include <levilamroid/LeviLamroid.h>

extern "C" __attribute__((visibility("default")))
bool ll_mod_load(void*) {
    // 1. UI Hooking: Listen for Start Menu Open via ll event bus
    ll::event::EventBus::getInstance().subscribe<ll::event::StartMenuOpenEvent>([](auto& ev) {
        ll::Logger::getInstance().info("Start Menu opened!");
    });

    // 2. Sound Method B (Sound Physics): Intercept 3D in-game sounds
    ll::event::EventBus::getInstance().subscribe<ll::event::SoundPlayEvent>([](auto& ev) {
        // Calculate raycast occlusion between sound pos and player camera
        // Adjust volume or pitch dynamically for acoustics
        if (ev.getSoundName() == "random.explode") {
            ev.setPitch(0.85f); // Deep explosion reverberation
        }
    });

    // 3. Sound Method A (Direct FMOD): Play in-memory raw sound with NO resourcepack
    // levilamroid::sound::FmodAudioEngine::getInstance().playMemory(rawWavBytes, wavSize);

    // 4. Texture Option B (Virtual Pack): Override vanilla block texture dynamically
    // levilamroid::resource::VirtualPackHub::getInstance().overrideBlockTexture("stone", customPngBytes);

    // 5. Render Interception: Intercept GPU rendering before UI draws
    ll::event::EventBus::getInstance().subscribe<ll::event::GlClearEvent>([](auto& ev) {
        // Render custom 3D scenes or HUDs here (OpenGL ES 3.0)
    });

    // 6. High-Level Human Functions (Zero Hardcoded Offsets)
    ll::ui::forceJsonUIForScreen("/main-menu");
    ll::ui::onStartMenuOpen([]() {
        ll::Logger::getInstance().info("Start Menu Opened via human callback!");
    });
    ll::ui::onPauseMenuOpen([](void* pauseController) {
        ll::Logger::getInstance().info("In-Game Pause Menu Opened!");
    });
    ll::ui::onPauseMenuClose([]() {
        ll::Logger::getInstance().info("In-Game Pause Menu Closed!");
    });
    ll::ui::onPlayButtonClicked([]() {
        ll::Logger::getInstance().info("Play button clicked!");
    });
    ll::render::suppressVanillaBackground(true); // Suppresses vanilla cubemap/panorama ONLY on menu, never in world
    ll::render::registerPostClearCallback([](int width, int height) {
        // Direct GPU thread render pass immediately after glClear
    });

    if (ll::isInWorld()) {
        // Safe check: player is active in world, custom menu scenes are disabled
    }

    return true;
}
```

---

## Complete API & Function Reference

For the comprehensive dictionary of every function, class, struct, macro, and header provided by LeviLamroid:
👉 **[Read the Complete API Reference in FUNCTIONS.md](FUNCTIONS.md)**

---

## Mod Template

A ready-to-use project template is available in [`template/`](template/):
- **Full C++20 Project Setup** (`CMakeLists.txt`)
- **Mod Manifest** (`manifest.json`)
- **Lifecycle Implementation** (`LL_REGISTER_MOD`)
- **Event Bus Listener & HUD Button Integration**
- **One-Click Packaging & Deployment Script** (`package.sh`)

Check out [`template/README.md`](template/README.md) to start building your own LeviLamroid mod in minutes!

## Credits & Attribution

- **Author & Maintainer**: [SpyGamingOG](https://github.com/spygamingog)
- **Architectural Inspiration**: [LiteLDev/LeviLamina](https://github.com/LiteLDev/LeviLamina)
- **Platform**: Minecraft Bedrock Edition on LeviLauncher (Android ARM64).

