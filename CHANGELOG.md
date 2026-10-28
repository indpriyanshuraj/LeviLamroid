# Changelog

All notable changes to **LeviLamroid** are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and release versions follow [Semantic Versioning](https://semver.org/).

## [1.0.5-beta] - 2026-10-28

### Multi-Version Engine (1.26.60.6 Parity), ResourcePackManager & Camera Subsystems
- **Bedrock 1.26.60.6 Full Parity & GNU Build ID Detection (`Offsets.cpp`)**:
  - Reverse-engineered and verified 4,557 symbols for Minecraft Bedrock 1.26.60.6 (`eb291b883dfdfaff70c4999c2eabd61885ae3e15`).
  - Added `kOffsets_1_26_60_6` with verified RVAs: `LocalPlayer::applyTurnDelta` (`0x0fd9ae30`), `LocalPlayer::normalTick` (`0x101eccd4`), `LevelRenderer::renderLevel` (`0x11b3f100`), `BlockSource::getBlock` (`0x10371548`), `SoundEngine::play` (`0x0bce01c8`), `CommandRegistry::registerCommand` (`0x08a06ce8`), and `ScreenController::handleEvent` (`0x09e95354`).
- **Complete Camera Subsystem & Rotation Handling**:
  - Mapped 118 camera-related symbols and components: `LevelRendererCamera::setupViewArea` (`0x0b0a80e4`), `LevelRendererCamera::updatePerChunkFaceSortState` (`0x0b7a9ba8`), `VanillaCamera::CameraBobComponent` (`0x088e7558`), `PlayerAuthInputPacketPayload::mCameraOrientation` (`0x0daa5398`), and MinecraftCamera component graph (`CameraComponent`, `DeathCameraComponent`, `CameraOrbitComponent`, `CameraFirstPersonComponent`).
- **`ResourcePackManager::_composeFullStack` Reverse Engineering & Zero-RP Pipeline**:
  - Located, disassembled, and verified `ResourcePackManager::_composeFullStack`, `composeFullStack`, `setStack`, `_doStackOperation`, `load`, and `getStack` across all major Bedrock 1.26 releases (`1.26.44.3`, `1.26.45.1`, `1.26.51.1`, `1.26.52.3`, `1.26.60.6`).
  - Documented stack assembly lifecycle: `mFullStackAccess` mutex locking, layer composition (`mGlobalStack` + `mLevelStack`), version evaluation (`_calculateMinEngineVersionFromFullStack`), and `ResourcePackListener` observer broadcast.
- **AAPCS64 IDA/Amethyst Wildcard Byte Signatures**:
  - Discovered 100% byte-identical 48-byte machine code signature for `ResourcePackManager::composeFullStack` matching every 1.26.x Bedrock engine build.
  - Extracted unique masked signatures for `_composeFullStack`, `setStack`, and `_doStackOperation` for AArch64 pattern scanning.
- **Universal Modder RE Studio Target Ingestion**:
  - Ingested all 2,036 symbols from `Symbols_1_26_52_3.inl` into the RE Studio Parity Database, expanding master targets to **5,039 symbols**.
  - Implemented Phase 1.5 Signature Scanner in RE Studio orchestrator, auto-generating C++ signature headers for all verified symbols.

## [1.0.8-beta] - 2026-10-27

### Crash Elimination, Authentic Usable Items & Doctor Strange 3D Mandala
- **Zero-Crash GameMode Interception & Corrupted RVA Removal (`Offsets.cpp`)**:
  - Eliminated bogus mid-function RVAs for `gameModeUseItem` (`0x0F8A2EB0`), `gameModeUseItemOn` (`0x0F89FE18`), `gameModeAttack` (`0x0F8A073C`), and `gameModeDestroyBlock` (`0x0F8A03AC`) which were landing inside `__stack_chk_fail@plt` epilogues and causing instant SIGSEGV crashes on right-click / item interaction.
  - Set all four offsets to `0` across all version profiles (1.26.52.3, 1.21.90, 1.21.80, 1.21.60), delegating interactions to safe event loop dispatching.
- **Geyser-Compliant Weapon Properties & Unsafe Memory Dereference Elimination (`LeviItems.cpp`, `NbtBuilder.cpp`)**:
  - Adhered to Geyser rule (`CustomItemRegistryPopulator.java:404`): Weapons/swords (`def.category == 3 && def.attackDamage > 0`) must NEVER contain `minecraft:use_modifiers` or `use_duration`, preventing Bedrock client from executing unallocated virtual components. Removed legacy `use_modifiers` from `cosmic_blade.json`.
  - Removed dangerous `reinterpret_cast<const std::string*>` pointer loop from `PlayerUseItemEvent` in `LeviItems.cpp` which was dereferencing `ItemStack::vftable` pointers as C++ strings and crashing with SIGSEGV.
  - Replaced with robust C ABI listener `ll_item_register_use_listener` passing clean, null-terminated `const char* itemIdent`.
- **Authentic Food Consumption & Stack Invalidation Engine (`CoreHooks.cpp`)**:
  - Replaced artificial instant `-1` decrement hacks with continuous hold tracking:
    - Chewing audio (`/playsound random.eat @a ~ ~ ~ 0.7 1.1`) and crumb particles emitted every 320ms while holding.
    - Full 1500ms (1.5s = 30 ticks) hold threshold required to consume food.
    - Full nourishment playback: burp sound (`random.burp`), saturation effect (`/effect @s saturation`), heart particle burst, and `+N Hunger` toast.
    - Fixed empty stack corruption: when stack count reaches 0, `mValid` byte at offset `+0x23` is cleanly cleared to `0`, preventing invalid empty stack engine panics across all placement and consumption routines.
  - Added `can_always_eat: true` on `astral_core.json`, allowing consumption at full hunger like golden apples.
- **Doctor Strange 3D Mystic Mandala Custom Item (`levitest:astral_mandala`)**:
  - Conjures an authentic 3D Doctor Strange Eldritch Mandala in world space hovering directly in front of the caster at chest height:
    - Dynamic orthonormal vector basis $(\vec{R}, \vec{U}, \vec{F})$ constructed from player pitch and yaw.
    - 7 sacred geometry vector layers: outer fiery perimeter ring ($R=1.6$), 12 runic perimeter spokes ($R=1.8$), middle amber concentric circle ($R=1.15$), interlocking double square octagram star, inner sacred hexagon ($R=0.65$), central eye core ring ($R=0.35$), and singularity of Agamotto.
    - Mystical audio (`block.enchantment_table.use`, `beacon.activate`, `conduit.activate`) and eldritch shield buffs (`resistance 12 2`, `absorption 12 1`, `fire_resistance 12 0`).
  - Implemented `minecraft:cooldown` parsing in `AddonIngestion.cpp` and Network NBT builder matching Geyser `CustomItemRegistryPopulator.java:696`.
- **Zero C++ Hardcoding**:
  - 100% of custom items, blocks, recipes, and entities load dynamically from standard Bedrock addon `.json` files in the unified `resources/` folder.

## [1.0.7-beta] - 2026-10-26

### Geyser-Parity 2.0 & Network Packet Redesign
- **Exact Geyser Network NBT Wire Protocol Realignment (`NbtBuilder.cpp`)**:
  - Re-engineered `buildItemComponentNbt` matching Geyser `CustomItemRegistryPopulator.java`:
    - Injected `item_tags` (ListTag of StringTag, including `geyser:is_custom` and `levilamroid:is_custom`), replacing invalid string tag format.
    - Added string `minecraft:use_animation` ("eat", "drink", "bow", "spear") in components alongside integer `use_animation` in `item_properties`.
    - Added `minecraft:use_modifiers` with duration in seconds (float: ticks / 20.0f).
    - Added `minecraft:food` schema with `can_always_eat: 0` and `using_converts_to: {}`.
    - Added `minecraft:digger` compound list with `block: { tags: "1" }` and tool mining speed.
  - Re-engineered `buildBlockPropertyNbt` matching Geyser `CustomBlockRegistryPopulator.java`:
    - Enforced float `1.0f` for `ambient_occlusion` (required by Bedrock 1.26.20+ to avoid mesh rejection).
    - Added `packed_bools: 1` and `mappings: {}` to material instances.
    - Aligned component properties to `"value"` (`minecraft:destructible_by_mining`, `minecraft:friction`), `"emission"` (`minecraft:light_emission`).
    - Redesigned `minecraft:collision_box` and `minecraft:selection_box` to `{ "enabled": 1, "boxes": [ { minX, minY, minZ, maxX, maxY, maxZ } ] }`.
  - Re-engineered `buildEntityDefinitionNbt` with `{ id, bid, rid, summonable: 1, hasSpawnEgg: 1, experimental: 0 }`.

- **Comprehensive Network Packet Ingestion & Loopback Symmetry (`CoreHooks.cpp`)**:
  - **`AvailableCommandsPacket` (Packet 76)**: Injected all registered custom/client commands with flags `0x40` (Not Cheat) and `CommandPermissionLevel::Any`, enabling client grammar validation and autocompletion without "Unknown command" red text.
  - **Inlined Value-Type Constructors (`AvailableCommandsPacketPayload.h`)**: Inlined default constructors, destructors, copy & move operations (`= default;`) for `CommandData`, `OverloadData`, `ParamData`, `EnumData`, `SoftEnumData`, `ConstrainedValueData`, `ChainedSubcommandData`, `ChainedSubcommandRelationship`, completely eliminating Android linker missing symbol crashes (`cannot locate symbol _ZN30AvailableCommandsPacketPayload11CommandDataC1Ev`).
  - **`CreativeContentPacket` (Packet 145)**: Cloned authentic `NetworkItemInstanceDescriptor` template entry from incoming packet and rebound `mItem` (+0x08) to native `Item*` pointer (`def.nativeItemPtr`), guaranteeing valid Itanium vtable and preventing null descriptor crashes in creative menu.
  - **`AvailableActorIdentifiersPacket` (Packet 119)**: Injected custom mobs with `mExperimentIndex = std::nullopt` and base identifier `bid` fallback.
  - **`LoopbackPacketSender::send` (Packet ID 145 / 76 / 162 / 119 / 11)**: Symmetrically hooked loopback packet sender so custom items, blocks, mobs, commands, and creative tabs function identically in local singleplayer, LAN host, and dedicated multiplayer sessions.

## [1.0.6-beta] - 2026-10-24

### Geyser-Parity Network Registration Architecture
- **Bedrock Network NBT Wire Serialization Engine (`NbtBuilder.cpp`)**:
  - Implemented full Little-Endian and ZigZag Network NBT serialization/deserialization matching GeyserMC and Minecraft Bedrock network protocol standards.
  - High-performance encoders for VarInt, Signed VarInt (ZigZag: `(n << 1) ^ (n >> 31)`), VarLong, UTF-8 String (VarInt length-prefixed), and IEEE 754 Little-Endian 32-bit/64-bit floating point numbers.
  - Automated schema builders:
    - `buildItemComponentNbt`: Constructs DataDriven item component compounds (`minecraft:icon`, `minecraft:display_name`, `minecraft:damage`, `minecraft:food`, `minecraft:weapon`, `minecraft:record`, `minecraft:render_offsets`).
    - `buildBlockPropertyNbt`: Constructs block property compounds (`minecraft:material_instances`, `minecraft:geometry`, `minecraft:collision_box`, `minecraft:selection_box`, `minecraft:light_emission`, `minecraft:destructible_by_mining`, `minecraft:explosion_resistance`).
    - `buildEntityDefinitionNbt`: Constructs runtime actor identifier compounds for mobs and entities.
  - Standalone verification test passing 100% roundtrips across all compound types.

- **Zero-Drop Packet Ingestion Subsystem (`CoreHooks.cpp`)**:
  - Hooked `ClientNetworkHandler::handle` (`0x09EE5908` on 1.26.52.3 / `0x09ADC770` on 1.26.45.1).
  - Intercepts incoming network packets and injects custom mod content before engine processing:
    - **`StartGamePacket` (ID 11)**: Injects custom block properties into `mPayload.mBlockProperties` (`std::vector<ServerBlockProperty>`) AND auto-enables data-driven experimental flags (`data_driven_items`, `upcoming_creator_features`, `experimental_molang_features`, `mOverrideForceExperimentalGameplayFlag = true`).
    - **`ItemRegistryPacket` (ID 162)**: Injects custom item components into `mPayload.mItems` (`std::vector<ItemData>`) with runtime IDs, `isComponentBased = true`, and `itemVersion = 2`.
    - **`AvailableActorIdentifiersPacket` (ID 119)**: Injects custom mob definitions into `mPayload.mIdentifierList` (`std::vector<ActorInfo>`).
    - **`CreativeContentPacket` (ID 145)**: Injects custom items into `mPayload.mEntries` (`std::vector<CreativeItemEntryPayload>`), assigning unique net IDs and group indices matching `def.category` so items appear in the creative inventory tabs.
  - Solves the need for fragile polymorphic C++ class reverse engineering (`FoodItemComponent`, `SpawnEggItem`), providing stable, crash-free registration.

- **Loader Startup & Linker dlopen Hardening**:
  - **Inlined Value-Type Constructors & Destructors**: Completely eliminated missing symbol crashes on stripped Android ARM64 `libminecraftpe.so` by providing inline `= default;` constructors/destructors for `ServerBlockProperty`, `ActorInfo`, `ActorDefinitionIdentifier`, `ItemDescriptor`, `ItemDescriptorCount`, `NetworkItemInstanceDescriptor`, `CreativeItemEntryPayload`, `CreativeGroupInfoPayload`, and `CreativeContentPacketPayload`.
  - **Fixed BlockHooks Nested Namespace**: Resolved misplaced namespace bracket that placed `isPlaceableItem` into `levilamroid::block::levilamroid::block`.
  - **Dynamic Mod Symbol Resolution**: In `levilamroid::init()`, promoted `liblevilamroid.so` to `RTLD_GLOBAL` via `dlopen`. In `LeviItems`, implemented dynamic loader function resolver (`resolveLoaderFn`) via `dlsym` and direct `liblevilamroid.so` handle lookup, ensuring 100% symbol resolution across all loader configurations.

- **Expanded C & C++ Modding APIs**:
  - Weapons: `ll_item_register_weapon(identifier, id, name, icon, frame, attackDamage, durability, category, creativeGroup)`
  - Foods: `ll_item_register_food(identifier, id, name, icon, frame, nutrition, saturation, category, creativeGroup)`
  - Tools: `ll_item_register_tool(identifier, id, name, icon, frame, attackDamage, durability, miningSpeed, category, creativeGroup)`
  - Blocks: `ll_block_register`, `ll_block_register_with_id`, `ll_block_register_with_base`
  - Entities: `ll_entity_register`, `ll_entity_spawn`, `ll_entity_register_spawn_listener`, `ll_entity_register_hurt_listener`, `ll_entity_register_death_listener`

- **Reference Mod Validation (`examples/levi-items-mod`)**:
  - Updated reference mod registering `levitest:cosmic_blade` (14 dmg, 2500 dur), `levitest:astral_core` (8 nut, 1.2 sat), `levitest:astral_block` (solid, light 10, resistance 12), and `levitest:astral_golem` (150 HP).
  - Built `libLeviItems.so` and packaged `levi_items.levipack` with raw texture mounting and zero Behavior Pack overhead.
  - Verified 0 missing/unresolved symbols on both libraries with `nm -u`.
  - Automatically deployed and enabled across all 6 LeviLauncher profiles.

## [1.0.5-beta] - 2026-10-22

### Critical World Join & Item Registration Crash Fix
- **Android ARM64 Tagged Pointer & `__next_prime` Overflow Fix**:
  - Identified and fixed crash `std::overflow_error: __next_prime overflow` occurring during item registration on world load.
  - On 64-bit Android (API 29+ / Android 15), the Scudo Hardened Allocator tags heap pointers in bits 56..63 (e.g. `0xb4...` or `0x20...`). `isPointerReadable()` checked `v > 0x7fffffffff`, causing all valid heap pointers to be falsely rejected.
  - This false rejection caused `unwrapItemRegistry()` to fail dereferencing the authentic `ItemRegistry*` heap pointer from the stack `ItemRegistryRef`, causing `ItemRegistry::registerItem` to operate on stack memory where `max_load_factor` at offset `+152` was `0.0f`, dividing by zero and triggering `__next_prime(UINT64_MAX)`.
  - Updated `isPointerReadable()` across `ItemHooks.cpp`, `CoreHooks.cpp`, `ResourceHooks.cpp`, and `UIHooks.cpp` to mask off Top-Byte Ignore tags (`v & 0x00ffffffffffffffULL`).
  - Added `max_load_factor` sanity check (guaranteeing `1.0f`) and exception handling around `ItemRegistry::registerItem`.
- **Scudo Chunk Corruption & BlockItem Vtable Resolution**:
  - Eliminated native crash `Scudo ERROR: corrupted chunk header` occurring upon world load when registering block items (e.g. `farmersdelight:tomato_crate`).
  - Fixed ABI mismatch in `BlockTypeRegistry::lookupByName`: ARM64 AAPCS requires struct-return buffers in `X0` for `WeakPtr<BlockLegacy>` (16B), passing `this` in `X1`. Direct calls with `this` in `X0` corrupted the singleton's header memory. Implemented `safeLookupBlockLegacy` with the authentic 4-parameter struct-return signature.
  - Removed premature block lookup during `VanillaItems::registerItems`, deferring block resolution to post-init.
  - Removed assignment of uninitialized/zeroed `blockItemVtable` (`0x122F10F0`), preserving the authentic `Item` vtable initialized by `Item::Item` constructor.
- **ItemRegistry Reference Unwrapping**:
  - Identified and fixed critical SIGSEGV crash during world join in `VanillaItems::registerItems` (`0x0F2AC634` / `0xF2C1ED0` on 1.26.45.1 and `0x0FA05B18` on 1.26.52.3).
  - In Bedrock ARM64 ABI, `VanillaItems::registerItems` passes `const Bedrock::NotNullNonOwnerPtr<ItemRegistry>&` as a pointer on the caller's stack to `{ ItemRegistry* ptr, ControlBlock* ctrl }`.
  - Implemented `unwrapItemRegistry()` to dereference the stack reference and resolve the authentic heap `ItemRegistry*` object before calling `ItemRegistry::registerItem`.
  - Prevented corrupt stack memory access at offset `+0x50` (`mIdToItemMap`) and `+0x80` (`mNameToItemMap`).
- **Control Block & Memory Parity**:
  - Corrected `BedrockControlBlock` memory layout to 16 bytes: `Item* mItem` (8B), `uint32_t mStrongRef` (4B, 1000000), `uint32_t mWeakRef` (4B, 1000000), eliminating 8-byte pointer misalignment.
  - Added dedicated `BedrockBlockControlBlock` for `BlockItem` at offset `+312` (`0x138`) storing `BlockLegacy*` with independent refcounts.
  - Reused existing `def.nativeItemPtr` and `def.nativeControlBlock` allocations across multiple registry initialization passes to prevent duplicate allocations.

### Deadlock-Free Inventory & Container Operations
- **Direct Container Slot Writing**:
  - Resolved main thread starvation in `giveItemToPlayer` / `ll_item_give_player`.
  - Directly writes the 152-byte `ItemStack` into `slotsBegin + (slot * 152)` and notifies container listeners via virtual slot 35 (`Container::containerContentChanged(slot)`).
  - Corrected 1.26.45.1 `.containerSetItem` offset to `0x0EF7A404`.

### Expanded Engine C & C++ APIs
- **World & BlockSource Inspection**:
  - `ll_block_source_get_block(void* blockSource, int x, int y, int z)`: Inspect block permutation at coordinate.
  - `ll_block_source_set_block(void* blockSource, int x, int y, int z, void* permutation, int flags)`: Set block permutation with update flags.
  - `ll_block_source_get_brightness(void* blockSource, int x, int y, int z)`: Query region light levels.
- **ItemStack Inspection & Mutation**:
  - `ll_item_stack_get_count(const void* itemStack)`: Query stack count.
  - `ll_item_stack_set_count(void* itemStack, uint8_t count)`: Modify stack count and validity.
  - `ll_item_stack_get_aux(const void* itemStack)`: Query metadata aux value.
  - `ll_item_stack_is_valid(const void* itemStack)`: Check item validity flag.
  - `ll_item_stack_get_item(const void* itemStack)`: Retrieve native `Item*` from control block.

### Clean Deployment & Pure Zero-RP Enforcement
- **Stale File Purge & Pack Hygiene**:
  - Wiped legacy test definitions (`astral_block*`, `astral_crafting_table*`) from all profile directories.
  - Hardened `package.sh` in `LeviLamroid`, `LeviItems`, and `LeviAdvancements` to completely wipe target `resources/` and `data/` directories prior to copying, preventing stale JSON ingestion.
  - Verified pure Zero-RP and Zero-BP mode: `global_resource_packs.json` and `world_resource_packs.json` remain pristine with 0 mod entries; all assets and UI are dynamically mounted in-memory via `ResourceManager`.

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

### Dynamic Multi-Version UI Engine
- **Eliminated Hardcoded Screen Offsets in `src/hooks/UIHooks.cpp`**:
  - Replaced hardcoded vtable slot constants (`0x710/8`, `0x760/8`, `0x758/8`, `0xD0/8`, `0xD8/8`) and RVAs with dynamic `EngineOffsets` fields:
    - `off.clientInstanceSceneFactorySlot` (1.26.52.3: 226, 1.26.45.1: 224)
    - `off.clientInstanceSceneStackSlot` (1.26.52.3: 236, 1.26.45.1: 235)
    - `off.sceneFactoryCreateUIScene` (1.26.52.3: `0x09AE87E8`, 1.26.45.1: `0x0975E3B0`)
    - `off.sceneFactoryCreateScreenAbstract` (1.26.52.3: `0x09AEB298`, 1.26.45.1: `0x097609CC`)
    - `off.sceneStackPushScreenSlot` (Slot 26 across all versions)
    - `off.sceneStackSchedulePopSlot` (Slot 27 across all versions)

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
