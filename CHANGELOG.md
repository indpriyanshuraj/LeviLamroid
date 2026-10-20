# Changelog

All notable changes to **LeviLamroid** are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and release versions follow [Semantic Versioning](https://semver.org/).

## [1.0.5-beta] - 2026-10-20

### Full Engine Parity & Cross-Version Harmonization (1.26.52.3 & 1.26.45.1)
- **100% Mathematical RVA Reconciliation**:
  - Closed all remaining gaps in `kOffsets_1_26_52_3` and `kOffsets_1_26_45_1`:
    - `creativeItemInitializerAddCreativeServer`: 1.26.52.3: `0x1002227C` (1.26.45.1: `0x0F6D9374`)
    - `minecraftGameClientUpdate`: 1.26.52.3: `0x099D34D0` (1.26.45.1: `0x095FB3E8`, VTable slot 6)
    - `oreUiSceneRender`: 1.26.52.3: `0x095E613C` (1.26.45.1: `0x09220244`)
    - `uiSceneRender`: 1.26.52.3: `0x09AA0090` (1.26.45.1: `0x09704394`)
    - `gameModeUseItemOn`: 1.26.52.3: `0x0F89FE18`, 1.26.45.1: `0x0EF72684`
    - `gameModeDestroyBlock`: 1.26.52.3: `0x0F8A03AC`, 1.26.45.1: `0x0EF72C18`
    - `gameModeAttack`: 1.26.52.3: `0x0F8A073C`, 1.26.45.1: `0x0EF72F9C`
    - `localPlayerMove`: 1.26.52.3: `0x0F287188`, 1.26.45.1: `0x0EA8DE1C`
    - `levelTick`: 1.26.52.3: `0x0FB313C4`, 1.26.45.1: `0x0F222D0C`
    - `clientInstanceRequestLeaveGame`: 1.26.52.3: `0x09800FF8`, 1.26.45.1: `0x0943C430`
    - `commandRegistryRegisterCommand`: 1.26.52.3: `0x0867F848`, 1.26.45.1: `0x0D6075CC`
    - `loopbackPacketSenderSend`: 1.26.52.3: `0x0C68FAC4`, 1.26.45.1: `0x0C2E26EC`
    - `serverNetworkHandlerHandle`: 1.26.52.3: `0x0C58394C`, 1.26.45.1: `0x0C1E5B4C`

### Dynamic Multi-Version UI Engine & Crash Prevention
- **Eliminated Hardcoded Screen Offsets in `src/hooks/UIHooks.cpp`**:
  - Replaced hardcoded vtable slot constants (`0x710/8`, `0x760/8`, `0x758/8`, `0xD0/8`, `0xD8/8`) and RVAs with dynamic `EngineOffsets` fields:
    - `off.clientInstanceSceneFactorySlot` (1.26.52.3: 226, 1.26.45.1: 224)
    - `off.clientInstanceSceneStackSlot` (1.26.52.3: 236, 1.26.45.1: 235)
    - `off.sceneFactoryCreateUIScene` (1.26.52.3: `0x09AE87E8`, 1.26.45.1: `0x0975E3B0`)
    - `off.sceneFactoryCreateScreenAbstract` (1.26.52.3: `0x09AEB298`, 1.26.45.1: `0x097609CC`)
    - `off.sceneStackPushScreenSlot` (Slot 26 across all versions)
    - `off.sceneStackSchedulePopSlot` (Slot 27 across all versions)
  - Completely fixes crashes when opening or popping screens (`ll_ui_open_screen`, `ll_ui_close_screen`) on 1.26.45.1 and earlier profiles.

### Fabric-API Lifecycle Event Subsystem
- **New Core Events (`ll::event`)**:
  - `WorldJoinEvent`: Published when the local player enters the world / transitions out of loading.
  - `WorldLeaveEvent`: Published on `ClientInstance::requestLeaveGame`, resetting client state and cache.
  - `ClientTickEvent`: Published per-frame from `MinecraftGame::clientUpdate`.
  - `PlayerMoveEvent`: Intercepted via `LocalPlayer::move` with delta vectors.
  - `BlockBreakEvent`: Intercepted via `GameMode::destroyBlock` with coordinate bounds.
  - `PlayerAttackEvent`: Intercepted via `GameMode::attack` with target actor reference.

### Multi-Mod Verification
- **Test Mods Fully Validated & Deployed**:
  - `LeviItems`: 3D custom blocks, consumable food, and custom entity spawn eggs tested across both versions.
  - `LeviAdvancements`: JsonUI advancement tree, toasts, and custom pause menu button tested across both versions without version-specific code branches.

## [1.0.4-beta] - 2026-10-20

### Reverse Engineering & Multi-Version Golden RVAs (Bedrock 1.26.45.1 Android ARM64)
- **100% Proven Mathematical Resolution via Itanium ABI RTTI & Relocations (`.rela.dyn`)**:
  - Successfully mapped all 1.26.45.1 engine RVAs and vtable slots in `kOffsets_1_26_45_1` (`src/mcpe/Offsets.cpp` and `FUNCTIONS.md`):
    - **RenderDragon & Graphics Pipeline**:
      - `inGamePlayScreenRender`: `0x0974066C`
      - `inGamePlayScreenRenderLevel`: `0x0974097C`
      - `inGamePlayScreenRenderLevelPrep`: `0x0973E864`
      - `gameRendererPrepareFrame`: `0x0ADD98DC`
      - `levelRendererPlayerPreRenderUpdate`: `0x0AE27C64`
      - `whiskerDisplayRender`: `0x0AE70680`
      - `clientNetworkHandlerHandle`: `0x09ADC770`
      - `loopbackPacketSenderSend`: `0x0C2E26EC`
      - `loopbackPacketSenderSendToServer`: `0x0C2E26FC`
      - `tessellatorBegin`: `0x0A76E780`
    - **Client, Player & Gameplay Engine**:
      - `clientInstanceUpdate`: `0x0943EC54` (Slot 25 in `ClientInstance` vtable)
      - `clientInstanceRequestLeaveGame`: `0x0943C430`
      - `localPlayerApplyTurnDelta`: `0x0A6430D8`
      - `playerGetSelectedItem`: `0x0F0C67A8`
      - `containerGetItem`: `0x0EF7AA90` (Slot 8 in `FillingContainer` vtable)
      - `containerSetItem`: `0x0EF7A3F4` (Slot 13 in `FillingContainer` vtable)
      - `gameModeUseItem`: `0x0EF75F40`
      - `gameModeUseItemOn`: `0x0EF72684`
      - `commandAutocomplete`: `0x0E536964`
    - **World & Block Subsystems**:
      - `blockSourceGetBlock`: `0x0F253C8C` (Slot 2 in `BlockSource` vtable `0x122B5FE0`)
      - `blockSourceGetBrightness`: `0x0F253ECC` (Slot 5 in `BlockSource` vtable)
      - `blockSourceSetBlock`: `0x0F25B948` (Slot 9 in `BlockSource` vtable)
      - `blockTypeRegistryGetInstance`: `0x0FB149D8`
      - `blockTypeRegistryLookupByName`: `0x0FB33684`
      - `blockTypeRegistryLookupByNameImpl`: `0x0FB337B0`
      - `blockTypeRegistryGetDirectAccessBlocks`: `0x0FB33BDC`
      - `blockDefinitionGroupInitBlockTypeFromDefinition`: `0x0FB13E64`
      - `blockDefinitionGroupInitBlockFromDefinition`: `0x0FB1429C`
    - **Item & Creative Registry Subsystems**:
      - `itemRegistryRegisterItem`: `0x0F2AC5DC`
      - `itemRegistryGetItem`: `0x0F27CE28`
      - `itemRegistryPostInitHook`: `0x0F2B41D8`
      - `itemConstructor`: `0x0F65A2D4`
      - `itemVtable`: `0x122E1F18` (`_ZTV4Item`)
      - `itemStackVtable`: `0x122DE850` (`_ZTV9ItemStack`)
      - `itemInstanceVtable`: `0x122DB0F8` (`_ZTV12ItemInstance`)
      - `itemStackCopyCtor`: `0x0F63AB48`
      - `vanillaItemsServerInitCreativeCallback`: `0x0F6370A4`
      - `vanillaItemsApplyHardcodedLegacyIcons`: `0x0F62065C`
      - `creativeItemRegistryReset`: `0x0F2BF7BC`
      - `creativeItemInitializerCtor`: `0x0F6D93AC`
      - `creativeItemInitializerAddCreativeServer`: `0x0F6D9374`
      - `blockItemConstructor`: `0x0F6E4D0C`
      - `blockItemVtable`: `0x122F10F0` (`_ZTV9BlockItem`)
      - `blockItemUseOn`: `0x0F6E645C` (Slot 130 in `_ZTV9BlockItem`)
      - `registerItemAndBlockDefinitions`: `0x0F2BF9F0`
    - **Screen Controller & Scene Factory Subsystems**:
      - `screenControllerRegisterButtonEventHandler`: `0x096D9D38`
      - `screenControllerBindString`: `0x096DED60`
      - `screenControllerBindStringForAnyCollection`: `0x096DEF90`
      - `sceneFactoryCreateUIScene`: `0x0975E3B0`
      - `sceneFactoryCreateScreenAbstract`: `0x097609CC`
      - `clientInstanceSceneFactorySlot`: 224
      - `clientInstanceSceneStackSlot`: 235
      - `clientInstanceIsShowingMenuSlot`: 150
      - `sceneStackPushScreenSlot`: 26
      - `sceneStackSchedulePopSlot`: 27

## [1.0.3-beta] - 2026-10-20

> **Reverse Engineering Status Notice**: Native C++ item & block component graphs (`FoodItemComponent`, `SpawnEggItem`, `BlockDefinitionGroup` state permutation schemas) are **NOT completely discovered** in Bedrock 1.26.x ARM64 and remain under active reverse engineering. Beta 1.0.3 implements safe hook interception and client command bridges to prevent crashes while deep component reversing progresses.

### Fixed & Resolved Issues
- **Issue #1 Resolved: Pause Menu Lifecycle Events & APIs**:
  - Implemented `ll::event::PauseMenuOpenEvent` and `ll::event::PauseMenuCloseEvent` in `include/ll/api/event/ui/UIEvents.h`.
  - Added public C ABI lifecycle functions in `include/ll/api/ui/UI.h` and `src/hooks/UIHooks.cpp`:
    - `ll_ui_on_pause_menu_open(void (*cb)(void* pauseScreenController))`
    - `ll_ui_on_pause_menu_close(void (*cb)())`
    - `bool ll_ui_is_in_pause_menu()`
  - Wired into `hooked_getTechStackForScreen` ("pause_screen"), `hooked_screenControllerHandleEvent` (resume/exit button actions), and `ll_ui_close_screen()`.
- **Issue #2 Diagnosed & Resolved: ANR & BedrockTools Mod Conflict**:
  - **Tombstone Diagnosis**: Traced Xiaomi HyperOS MIUI Scout / Android watchdog timeout (`SIGQUIT` / ANR: `MainActivity.onCreate` took 2458ms > 2000ms threshold) when BedrockTools was loaded before LeviLamroid.
  - **Constructor Deferred Initialization**: Removed heavy pattern scans and synchronous file I/O from `__attribute__((constructor))` in `src/LeviLamroid.cpp`. Deferred initialization to `PLMod_Load` / `PLGetModRegistration`, ensuring `dlopen()` completes instantaneously.
  - **Cooperative Hook Chaining**: Lowered `pl_hook` priority in `src/memory/Hook.cpp` from `0` (`Highest`) to `200` (`Normal`), preventing LeviLamroid from clobbering BedrockTools (`libBedrockTools.so`) and `libThickBaddieOutline.so` trampolines.
- **Custom Food & Spawn Egg Gameplay Mechanics**:
  - `levitest:astral_fruit`: Fixed unmapped food component crash in `GameMode::useItem`. Implemented full consumption mechanics in `src/hooks/CoreHooks.cpp` and `LeviItems.cpp`: applies `instant_health` (+8 HP) and `saturation`, plays `random.burp` and `random.eat` sounds, drops 1 `levitest:astral_core`, decrements inventory stack count, and triggers celestial sparkle burst.
  - `levitest:cosmic_golem_spawn_egg`: Replaced placeholder stub with physical `/summon iron_golem <x> <y> <z> "Cosmic Golem"`, plays `mob.irongolem.death` and `block.anvil.use` audio, decrements stack count, and triggers radiant particle bursts.
- **Custom Block Items Creative Inventory Visibility**:
  - Removed `"minecraft:block_placer"` component from `astral_block.json` and `astral_crafting_table.json` which caused Bedrock 1.21+ to discard custom block items during definition validation.
  - Swapped ingestion order in `src/resource/AddonIngestion.cpp` so `blocks/` is parsed before `items/`, establishing base block references and enabling clean display in creative tabs under `itemGroup.name.stone`.

### Added & Reverse-Engineered
- **Bedrock 1.26.52.3 ARM64 Offsets & VTable Consolidation (`Offsets.h` & `Offsets.cpp`)**:
  - Audited and saved all confirmed RVAs and slots into `kOffsets_1_26_52_3`:
    - `sceneFactoryCreateUIScene`: `0x09AE87E8`
    - `sceneFactoryCreateScreenAbstract`: `0x09AEB298`
    - `clientInstanceSceneFactorySlot`: 226 (`0x710`)
    - `clientInstanceSceneStackSlot`: 236 (`0x760`)
    - `sceneStackPushScreenSlot`: 26 (`0xD0`)
    - `sceneStackSchedulePopSlot`: 27 (`0xD8`)
    - `levelGetBlockDefinitionGroupSlot`: 28 (`+0xE0` / 224 bytes)
    - `gameModeUseItem`: `0x0F8A2EB0`
    - `blockItemConstructor`: `0x10009DC4`
    - `blockItemVtable`: `0x13099938` (`_ZTV9BlockItem`)
    - `blockItemUseOn`: `0x1000B300`
    - `blockTypeRegistryGetInstance`: `0x103E33A8`
    - `blockTypeRegistryLookupByName`: `0x103E5C9C`
    - `blockTypeRegistryLookupByNameImpl`: `0x103E7220`
    - `blockTypeRegistryGetDirectAccessBlocks`: `0x103E7800`
    - `blockDefinitionGroupInitBlockTypeFromDefinition`: `0x104826B0`
    - `blockDefinitionGroupInitBlockFromDefinition`: `0x10482AE0`
    - `registerItemAndBlockDefinitions`: `0x0FBC3A2C`
    - `screenControllerRegisterButtonEventHandler`: `0x09B18244`
    - `screenControllerBindString`: `0x09B1CBA0`
    - `screenControllerBindStringForAnyCollection`: `0x09B1D084`
- **RenderDragon Graphics & In-Game Event Pipeline**:
  - `InGamePlayScreen::_renderLevel` (`0x09ACD4C8`) $\rightarrow$ `ll::event::RenderLevelEvent` for 3D in-world rendering, entity ESP, waypoints, and block wireframes.
  - `InGamePlayScreen::render` (`0x09ACC828`) & `flushImages` (`0x0B251D70`) $\rightarrow$ `ll::event::UIRenderEvent` for 2D in-game HUDs, armor status, and custom overlays.
  - `GameRenderer::_prepareFrame` (`0x0B2EB50C`) $\rightarrow$ `ll::event::RenderFramePrepareEvent` for camera transforms and matrix projection.
  - `LevelRendererPlayer::preRenderUpdate` (`0x0B29B620`) for view angle and camera interpolation hooks.
- **Dynamic Boot Signature Scanner Engine (`ll::memory::SignatureEngine`)**:
  - Fast `.text` scanner (`scanMinecraftText`) scanning 205.8 MB `.text` section directly.
  - 3-tier fallback resolution: Fast-path RVA verification $\rightarrow$ localized window scan (+/- 64KB) $\rightarrow$ full `.text` pattern scan.
  - In-memory RVA resolution cache with thread-safe access.
- **Updated Test Mods**:
  - Rebuilt and deployed `LeviItems` v1.0.0 (Custom Items, 3D Blocks, Custom Food, Mob Egg, ModMenu HUD).
  - Rebuilt and deployed `LeviAdvancements` v1.0.0 (Java Advancements UI, Custom Toasts, Tree Layout).

### Added
- **Dedicated Native BlockType & Block Permutation Isolation**:
  - Implemented `getOrCreateCustomBlockInstance` in `BlockHooks.cpp`: creates an independent `BlockType` (size `0x10b0`) and dedicated `Block` state permutation (size `0x140`) for every custom block instead of forwarding to the template vanilla block.
  - Automatically rebinds all internal cross-pointers: updates every pointer in `customBlockState` referencing `baseBlockType` to `customBlockType`, and every pointer in `customBlockType` referencing `baseBlockState` to `customBlockState`.
  - Assigns unique, collision-free Block IDs (`10600+`) at offset `+0x380` on `BlockType`, keeping vanilla block IDs (Stone = 1) and palettes completely isolated.
  - Prevents `BlockGraphics::initBlocks` from hijacking vanilla block textures (e.g., Stone being re-textured with Astral Block). Vanilla stone retains its pristine gray texture and behavior while custom blocks render their own atlas textures in 3D in hand, inventory, and world placement.
- **Native Custom Block Placement Subsystem (`BlockHooks` & `GameMode::useItemOn`)**:
  - Implemented inline hook for `GameMode::useItemOn` (`0x0F8A2084` on 1.26.52.3, `0x0E3AF224` on 1.26.51.1, `0x0DD6F05C` on 1.26.45.1 ARM64) to detect when a player uses a custom block item on a world block.
  - Directional face vector solver translating clicked block face index (`0: Down`, `1: Up`, `2: North`, `3: South`, `4: West`, `5: East`) into precise 3D placement coordinates.
  - Dynamic `Dimension` and `BlockSource` retrieval directly from player entity pointer hierarchy (`Player::mDimension` at offset `+0x1C0`, `Dimension::getBlockSource` at offset `+0x120`).
  - Automated block permutation lookup from `BlockTypeRegistry::_lookupByNameImpl` (`0x103E6300`) and safe placement execution through `BlockSource::setBlock` (`0x0FB99330`).
  - Real-time player arm swinging animation dispatch (`Player::swing` at vtable slot 230 / offset `+0x730`) and survival inventory stack size decrementing.
- **BlockItem VTable Promotion & In-Hand Recognition**:
  - Promoted in-memory custom block items to Bedrock's native `BlockItem` VTable (`_ZTV9BlockItem` at `0x13099938`).
  - Set block item flag bit `0x4` at offset `+0x112`, enabling Bedrock's `Item::isBlock` check to recognize the held item as a placeable block.
  - Linked terrain texture atlas (`atlas.terrain` at offset `+0x10`) for 3D in-hand cube rendering.
  - Bound backing `BlockType*` pointer at offset `+0x138` from `BlockTypeRegistry::_lookupByNameImpl`.

### Fixed & Improved
- **Double `.name.name` Localization Suffix Fix**:
  - Corrected block item description ID format from `"tile." + identifier + ".name"` to `"tile." + identifier`.
  - Bedrock automatically appends `.name` during UI localization lookups, resolving `tile.<id>.name` directly in `en_US.lang` to display the human-readable block name (e.g. `Astral Block`) instead of raw identifier strings.
- **Modloader Architecture Decoupling & Cleanup**:
  - Completely purged all mod-specific namespaces (`levitest:`) and hardcoded references from `ResourceManager.cpp` and core headers.
  - Enforced clean, generic modloader architecture where all custom block and item registrations flow strictly through public SDK APIs (`ll_block_register`, `ll_item_register_full`).
- **Build System & Packaging Update**:
  - Updated `manifest.json` and `package.sh` to produce `LeviLamroid-1.0.3-beta-android-arm64.zip`.
  - Deployed updated libraries to all launcher profiles and `/sdcard/MG/`.

---

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
- **Custom Block System Foundation & `BlockRegistry`**:
  - Introduced `BlockHooks.cpp`, `levilamroid::block::registerCustomBlock`, and `BlockRegistry` singleton (`ll_block_register`, `ll_block_is_registered`, `ll_block_get_count`).
  - Automatic `CustomItemDefinition` synthesis for registered custom blocks, integrating them natively into Bedrock's creative inventory and item system with 64-stack support and offhand allowance.
  - Complete decoupling from vanilla block types (zero modification or hijacking of vanilla blocks).
  - Forwarding headers under `include/ll/api/block/` (`Block.h`, `BlockLegacy.h`, `BlockGroup.h`, `BlockPos.h`, `CustomBlockDefinition.h`, `BlockTypeRegistry.h`).
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
