#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/Logger.h"
#include "levilamroid/api/memory/Hook.h"
#include <mutex>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <link.h>
#include <elf.h>

namespace levilamroid::mcpe {

// Verified Golden Offsets for Minecraft Bedrock 1.26.52.3 (Android ARM64)
static const EngineOffsets kOffsets_1_26_52_3 = {
    // UI Screen Tech Stack
    .getTechStackForScreen    = 0x0974E844,

    // Start Menu Screen Controller
    .startMenuOnOpen          = 0x0A2DF314,
    .startMenuTick            = 0x0A2E1FF4,

    // Direct Menu Button Handlers
    .playButton               = 0x0A31344C,
    .settingsButton           = 0x0A3171AC,
    .realmsButton             = 0x0A3174DC,
    .dressingRoomButton       = 0x0A31815C,
    .marketplaceButton        = 0x0A3187A4,
    .menuExit                 = 0x0A567540,

    // RenderDragon & Skybox Passes
    .progressBarRender        = 0x09685F70,
    .panoramaRender           = 0x0967FA98,
    .cubemapBackgroundRender  = 0x09B7C9E0,
    .cubemapFaceSubmit        = 0, // In-world skybox sharing; do not suppress
    .cubemapRendererRender    = 0, // In-world skybox/environment renderer; do not hook or suppress!
    .mainMenuCubemapPass      = 0, // False RVA: was LevelRendererCamera::setupViewArea (0x0B2DC860)
    .cubemapDraw              = 0, // False RVA: was LevelRenderer internal (0x0B29A6AC)

    // Verified Client-Side Fabric Engine & RenderDragon RVAs (Step B & C 1.26.52.3)
    .inGamePlayScreenRender              = 0x09ACC828,
    .inGamePlayScreenRenderLevel         = 0x09ACD4C8,
    .inGamePlayScreenRenderLevelPrep     = 0x09ACC534,
    .gameRendererPrepareFrame            = 0x0B2EB50C,
    .levelRendererPlayerPreRenderUpdate  = 0x0B29B620,
    .oreUiSceneRender                    = 0x095E613C,
    .uiSceneRender                       = 0x09AA0090,
    .whiskerDisplayRender                = 0x0B33126C,
    .minecraftGameClientUpdate           = 0x099D34D0,
    .clientInstanceRequestLeaveGame      = 0x09800FF8,
    .clientNetworkHandlerHandle          = 0x09EE5908,
    .loopbackPacketSenderSendToServer    = 0x0C68C410,

    // GPU / libGLESv2 Hooks
    .glClearGotRva            = 0x132893d8, // Hooked with strict sIsOnMainMenu guard to prevent in-world interference

    // MinecraftUIRenderContext Virtual Method RVAs
    .uiDrawImage              = 0x0B25099C,
    .uiFlushImages            = 0x0B251D70,
    .uiFillRectangle          = 0x0B25232C,
    .uiDrawRectangle          = 0x0B2536FC,
    .uiDrawText               = 0x0B2507DC,
    .uiFlushText              = 0x0B2508DC,
    .uiDrawNineslice          = 0x0B25198C,
    .uiGetScreenContext       = 0x0B2541E8,

    // Player / GameMode / World
    .localPlayerMove          = 0x0F287188,
    .playerNormalTick         = 0x0F9E26C0,
    .levelTick                = 0x0FB313C4,
    .gameModeDestroyBlock     = 0x0F8A03AC,
    .gameModeUseItem          = 0x0F8A2EB0,
    .gameModeUseItemOn        = 0x0F89FE18,
    .gameModeAttack           = 0x0F8A073C,

    // Verified Engine & Graphics RVAs
    .clientInstanceUpdate          = 0x098036A4,
    .clientInstanceGetLocalPlayer  = 0, // Hidden struct return buffer; use playerNormalTick
    .localPlayerApplyTurnDelta     = 0x0AAC9774,
    .tessellatorBegin              = 0x0A76E7F8,
    .blockSourceGetBlock           = 0x0FB91918,
    .blockSourceGetBrightness      = 0x0FB92F9C,
    .commandAutocomplete          = 0x0EBEAE44,
    .commandRequestPacketWrite     = 0,

    .soundEnginePlay          = 0x0B79F7F4,
    .soundEnginePlayUI        = 0x0B79F978,
    .soundEnginePlayAt        = 0x0B79FDE0,
    .soundEngineStop          = 0x0B79FF1C,

    // Resource Pack & Asset Access Strategy RVAs (Audited 1.26.52.3)
    .directoryPackGetAsset    = 0x111EEE94,
    .zipPackGetAsset          = 0x11158670,
    .directoryPackHasAsset    = 0x111EE8A8,
    .zipPackHasAsset          = 0x11157F00,
    .packForEachInAssetSet    = 0x1117FD58,
    .resourcePackManagerLoad  = 0x0C92D4B8,
    .resourcePackManagerGetStack = 0x0C92D268,
    .resourcePackManagerDoStackOp = 0x0C930460,
    .resourcePackGetResource  = 0x06A3DEA0,
    .textureGroupLoadAsync    = 0x0A777500,
    .textureGroupEnableLRU    = 0x0A778998,
    .textureHotReloaderRegister = 0x0B1FC23C,
    .textureHotReloaderRegisterAtlas = 0x0B1FDBC4,
    .blockGraphicsGetTextureItem = 0x0A66DC10,
    .cubemapLoadPanoramas     = 0x09B79A28,
    .resourcePackRepositoryGetResourcePackInPath = 0x0C9356E4,
    .repositoryLoadingFindVanillaPacks           = 0x0C96C79C,

    // OreUI (Cohtml Engine) RVAs
    .cohtmlLoadURL               = 0x06D6F97C,
    .cohtmlLoadURLImpl           = 0x06D6F984,
    .cohtmlExecuteScript         = 0x06D7E008,
    .cohtmlAddInitialScript      = 0x06D7E41C,
    .cohtmlTriggerEvent          = 0x06D8590C,
    .cohtmlRegisterForEvent      = 0x06D81B78,
    .cohtmlUnregisterFromEvent   = 0x06D820EC,
    .cohtmlTryDispatchClickEvent = 0x06D7C03C,
    .cohtmlMouseEvent            = 0x06D7B5A8,
    .cohtmlTouchEvent            = 0x06D7A048,
    .oreUiInit                   = 0x096F742C,

    // JSON UI & Screen Controller RVAs
    .screenControllerTick                = 0x09B14FEC,
    .screenControllerHandleEvent         = 0x09B14870,
    .screenControllerBind                = 0x09B1481C,
    .screenControllerAddStaticScreenVars = 0x09B15FA4,
    .screenControllerSetScreenState      = 0x09B16274,
    .screenControllerCanExit             = 0x09B169DC,
    .screenControllerGetScreenModel      = 0x09B23714,
    .sceneFactoryCreateScreenController  = 0x09AEF950,
    .sceneFactoryCreateScreen            = 0x09AFD430,
    .buttonComponentHandleEvent          = 0x096FB8E8,
    .buttonComponentSetPressed           = 0x096FC22C,
    .buttonComponentGetButtonState       = 0x096FC208,
    .dataDrivenUILayout                  = 0x09C356D0,
    .dataDrivenUIUIRoot                  = 0x0B868FA0,
    .guiDataDisplayClientMessage         = 0x0A10391C,

    // Chat, Command & Packet System RVAs
    .chatScreenControllerHandleEvent     = 0x09A0F020,
    .chatScreenControllerPropertyBind    = 0x09A0F948,
    .commandRegistryRegisterCommand      = 0x0867F848,
    .loopbackPacketSenderSend            = 0x0C68FAC4,
    .serverNetworkHandlerHandle          = 0x0C58394C,

    // Behavior, Recipes, Items, Blocks & Scripting RVAs
    .recipesAddShapedRecipe              = 0x0FC3D3D8,
    .recipesAddShapelessRecipe            = 0x0FC3E1D4,
    .itemRegistryTryRegisterAndInitItem  = 0x0FBB3014,
    .itemRegistryParseItemIdentifier     = 0x0FBB5A54,
    .blockTypeRegistryInit               = 0x0AD6D44C,
    .blockDefinitionLoaderProcessCustomComponents = 0x10491FF8,
    .actorNormalTick                     = 0x0F27DA98,
    .actorBaseTick                       = 0x0F560820,
    .actorHurt                           = 0x0F5609A8,
    .actorDie                            = 0x0F564E7C,
    .actorTeleportTo                     = 0x0F5746BC,
    .actorHasComponent                   = 0x0F583390,
    .actorSetCustomName                  = 0x0F57E694,
    .scriptingQuickJS                    = 0x11432150,
    .scriptModuleMinecraftScriptSystem   = 0x0E069FE8,

    // Item & Creative Inventory Subsystem RVAs (Audited 1.26.52.3 ARM64)
    .itemRegistryRegisterItem               = 0x0FBB0064,
    .itemRegistryGetItem                    = 0x0FBC1640,
    .itemRegistryPostInitHook               = 0x0FBB77DC,
    .itemConstructor                        = 0x0FF7D35C,
    .itemSetIconInfo                        = 0x0FF83DDC, // Real Item::setIcon(std::string const& name, int frame) in 1.26.52.3
    .itemSetDescriptionId                   = 0, // Inlined in Item::Item; 0x0FF81EA4 was engine panic assertion
    .itemVtable                             = 0x1308A650,
    .itemStackVtable                        = 0x1308BE60,
    .itemInstanceVtable                     = 0x1308AC00,
    .itemStackCopyCtor                      = 0x0FF9DB08,
    .vanillaItemsRegisterItems              = 0x0FF05E14,
    .vanillaItemsServerInitCreativeCallback = 0x0FF5A2D4,
    .vanillaItemsApplyHardcodedLegacyIcons  = 0x0FF43810,
    .creativeItemRegistryReset              = 0x0FBC3604,
    .creativeItemInitializerCtor            = 0x0FFFC988,
    .creativeItemInitializerAddCreativeServer = 0x1002227C,
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
    .screenControllerRegisterButtonEventHandler = 0x09B18244,
    .screenControllerBindString             = 0x09B1CBA0,
    .screenControllerBindStringForAnyCollection = 0x09B1D084,

    // VTable Slots
    .clientInstanceLocalPlayerSlot = 32,
    .clientInstanceIsShowingMenuSlot = 151,
    .levelGetBlockDefinitionGroupSlot = 28,

    // Scene Factory & Scene Stack RVAs and Slots (1.26.52.3 Audited)
    .sceneFactoryCreateUIScene = 0x09AE87E8,
    .sceneFactoryCreateScreenAbstract = 0x09AEB298,
    .clientInstanceSceneFactorySlot = 226, // 0x710
    .clientInstanceSceneStackSlot = 236,   // 0x760
    .sceneStackPushScreenSlot = 26,        // 0xD0
    .sceneStackSchedulePopSlot = 27        // 0xD8
};

// Verified Golden Offsets for Minecraft Bedrock 1.26.44.3 (Android ARM64)
static const EngineOffsets kOffsets_1_26_44_3 = {
    // UI Screen Tech Stack
    .getTechStackForScreen    = 0x0938B7B4,

    // Start Menu Screen Controller
    .startMenuOnOpen          = 0x09EBD9FC,
    .startMenuTick            = 0x09EC0678,

    // Direct Menu Button Handlers
    .playButton               = 0x09EF3E04,
    .settingsButton           = 0x09EF7360,
    .realmsButton             = 0x09EF7690,
    .dressingRoomButton       = 0x09EF8310,
    .marketplaceButton        = 0x09EF8944,
    .menuExit                 = 0x0A15252C,

    // RenderDragon & Skybox Passes
    .progressBarRender        = 0x092BECCC,
    .panoramaRender           = 0x092B8E60,
    .cubemapBackgroundRender  = 0x097C9A20,
    .cubemapFaceSubmit        = 0,
    .cubemapRendererRender    = 0,
    .mainMenuCubemapPass      = 0,
    .cubemapDraw              = 0,

    .inGamePlayScreenRender              = 0,
    .inGamePlayScreenRenderLevel         = 0,
    .inGamePlayScreenRenderLevelPrep     = 0,
    .gameRendererPrepareFrame            = 0,
    .levelRendererPlayerPreRenderUpdate  = 0,
    .oreUiSceneRender                    = 0,
    .uiSceneRender                       = 0,
    .whiskerDisplayRender                = 0,
    .minecraftGameClientUpdate           = 0,
    .clientInstanceRequestLeaveGame      = 0,
    .clientNetworkHandlerHandle          = 0,
    .loopbackPacketSenderSendToServer    = 0,

    // GPU / libGLESv2 Hooks
    .glClearGotRva            = 0x124C9D28,

    // MinecraftUIRenderContext Virtual Method RVAs
    .uiDrawImage              = 0x0AD84690,
    .uiFlushImages            = 0x0AD85A64,
    .uiFillRectangle          = 0x0AD86020,
    .uiDrawRectangle          = 0x0AD873F0,
    .uiDrawText               = 0x0AD844D0,
    .uiFlushText              = 0x0AD845D0,
    .uiDrawNineslice          = 0x0AD85680,
    .uiGetScreenContext       = 0x0AD87EDC,

    // Player / GameMode / World
    .localPlayerMove          = 0x0EA8E37C,
    .playerNormalTick         = 0x0F0B1BE0,
    .levelTick                = 0x0F22326C,
    .gameModeDestroyBlock     = 0,
    .gameModeUseItem          = 0,
    .gameModeUseItemOn        = 0,
    .gameModeAttack           = 0,
    .clientInstanceUpdate          = 0,
    .clientInstanceGetLocalPlayer  = 0,
    .localPlayerApplyTurnDelta     = 0,
    .tessellatorBegin              = 0,
    .blockSourceGetBlock           = 0,
    .blockSourceGetBrightness      = 0,
    .commandAutocomplete          = 0,
    .commandRequestPacketWrite     = 0,
    .soundEnginePlay          = 0x0B284980,
    .soundEnginePlayUI        = 0x0B284B04,
    .soundEnginePlayAt        = 0x0B284F6C,
    .soundEngineStop          = 0x0B2850A8,

    // Resource Pack & Asset Access Strategy RVAs
    .directoryPackGetAsset    = 0x1042D8C0,
    .zipPackGetAsset          = 0x1040D318,
    .directoryPackHasAsset    = 0x1042D0C4,
    .zipPackHasAsset          = 0x1040DA6C,
    .packForEachInAssetSet    = 0x10437E7C,
    .resourcePackManagerLoad  = 0x0C594BB8,
    .resourcePackManagerGetStack = 0x0C594960,
    .resourcePackManagerDoStackOp = 0x0C59B36C,
    .resourcePackGetResource  = 0x06695294,
    .textureGroupLoadAsync    = 0x0A403730,
    .textureGroupEnableLRU    = 0x0A404BC8,
    .textureHotReloaderRegister = 0x0ACF87B8,
    .textureHotReloaderRegisterAtlas = 0x0ACFA140,
    .blockGraphicsGetTextureItem = 0x0A2246D4,
    .cubemapLoadPanoramas     = 0x097C66E0,
    .resourcePackRepositoryGetResourcePackInPath = 0x0C59FF90,
    .repositoryLoadingFindVanillaPacks           = 0x0C5D9224,

    // OreUI (Cohtml Engine) RVAs
    .cohtmlLoadURL               = 0x069C1B1C,
    .cohtmlLoadURLImpl           = 0x069C1B24,
    .cohtmlExecuteScript         = 0x069D0134,
    .cohtmlAddInitialScript      = 0x069D0548,
    .cohtmlTriggerEvent          = 0x069D7A38,
    .cohtmlRegisterForEvent      = 0x069D3CA4,
    .cohtmlUnregisterFromEvent   = 0x069D4218,
    .cohtmlTryDispatchClickEvent = 0x069CE170,
    .cohtmlMouseEvent            = 0x069CD6DC,
    .cohtmlTouchEvent            = 0x069CC180,
    .oreUiInit                   = 0x09230524,

    // JSON UI & Screen Controller RVAs
    .screenControllerTick                = 0x096D6F48,
    .screenControllerHandleEvent         = 0x096D747C,
    .screenControllerBind                = 0x096D6EF4,
    .screenControllerAddStaticScreenVars = 0x096D8434,
    .screenControllerSetScreenState      = 0x096D8704,
    .screenControllerCanExit             = 0x096D8E6C,
    .screenControllerGetScreenModel      = 0x096E5AF0,
    .sceneFactoryCreateScreenController  = 0x09765630,
    .sceneFactoryCreateScreen            = 0x097836D8,
    .buttonComponentHandleEvent          = 0x09338E0C,
    .buttonComponentSetPressed           = 0x09339750,
    .buttonComponentGetButtonState       = 0x0933972C,
    .dataDrivenUILayout                  = 0x091998F8,
    .dataDrivenUIUIRoot                  = 0x0C59D404,
    .guiDataDisplayClientMessage         = 0x09CFBC70,

    // Chat, Command & Packet System RVAs
    .chatScreenControllerHandleEvent     = 0x09684918,
    .chatScreenControllerPropertyBind    = 0x09685238,
    .commandRegistryRegisterCommand      = 0x0D607B04,
    .loopbackPacketSenderSend            = 0,
    .serverNetworkHandlerHandle          = 0x0C1E5BEC,

    // Behavior, Recipes, Items, Blocks & Scripting RVAs
    .recipesAddShapedRecipe              = 0x0F2FC668,
    .recipesAddShapelessRecipe            = 0x0F2FD534,
    .itemRegistryTryRegisterAndInitItem  = 0x0F2B00C8,
    .itemRegistryParseItemIdentifier     = 0x0F2B2AE0,
    .blockTypeRegistryInit               = 0x0954DA44,
    .blockDefinitionLoaderProcessCustomComponents = 0x0FB27464,
    .actorNormalTick                     = 0x0EA41838,
    .actorBaseTick                       = 0x0EC84B80,
    .actorHurt                           = 0x0EC84D08,
    .actorDie                            = 0x0EC891B0,
    .actorTeleportTo                     = 0x0EC984CC,
    .actorHasComponent                   = 0x0D6C20E8,
    .actorSetCustomName                  = 0x0ECA2184,
    .scriptingQuickJS                    = 0x10918134,
    .scriptModuleMinecraftScriptSystem   = 0x0DA5509C,

    // Item & Creative Inventory Subsystem RVAs
    .itemRegistryRegisterItem               = 0,
    .itemRegistryGetItem                    = 0,
    .itemRegistryPostInitHook               = 0,
    .itemConstructor                        = 0,
    .itemSetIconInfo                        = 0x0F65DD84,
    .itemSetDescriptionId                   = 0,
    .itemVtable                             = 0,
    .itemStackVtable                        = 0,
    .itemInstanceVtable                     = 0,
    .itemStackCopyCtor                      = 0,
    .vanillaItemsRegisterItems              = 0,
    .vanillaItemsServerInitCreativeCallback = 0,
    .vanillaItemsApplyHardcodedLegacyIcons  = 0,
    .creativeItemRegistryReset              = 0,
    .creativeItemInitializerCtor            = 0,
    .creativeItemInitializerAddCreativeServer = 0,
    .playerGetSelectedItem                  = 0,
    .containerGetItem                       = 0,
    .containerSetItem                       = 0,
    .blockItemConstructor                   = 0,
    .blockItemVtable                        = 0,
    .blockItemUseOn                         = 0,
    .blockSourceSetBlock                    = 0,
    .blockTypeRegistryGetInstance           = 0,
    .blockTypeRegistryLookupByName          = 0,
    .blockTypeRegistryLookupByNameImpl      = 0,
    .blockTypeRegistryGetDirectAccessBlocks = 0,
    .blockDefinitionGroupInitBlockTypeFromDefinition = 0,
    .blockDefinitionGroupInitBlockFromDefinition     = 0,
    .registerItemAndBlockDefinitions        = 0,
    .screenControllerRegisterButtonEventHandler = 0,
    .screenControllerBindString             = 0,
    .screenControllerBindStringForAnyCollection = 0,

    // VTable Slots
    .clientInstanceLocalPlayerSlot = 32,
    .clientInstanceIsShowingMenuSlot = 150,
    .levelGetBlockDefinitionGroupSlot = 28,
    .sceneFactoryCreateUIScene = 0,
    .sceneFactoryCreateScreenAbstract = 0,
    .clientInstanceSceneFactorySlot = 0,
    .clientInstanceSceneStackSlot = 0,
    .sceneStackPushScreenSlot = 0,
    .sceneStackSchedulePopSlot = 0
};

// Verified Golden Offsets for Minecraft Bedrock 1.26.45.1 (Android ARM64)
static const EngineOffsets kOffsets_1_26_45_1 = {
    .getTechStackForScreen    = 0x0938B714,
    .startMenuOnOpen          = 0x09EBD95C,
    .startMenuTick            = 0x09EC05D8,
    .playButton               = 0x09EF3D64,
    .settingsButton           = 0x09EF72C0,
    .realmsButton             = 0x09EF75F0,
    .dressingRoomButton       = 0x09EF8270,
    .marketplaceButton        = 0x09EF88A4,
    .menuExit                 = 0x0A15248C,
    .progressBarRender        = 0x092BEC2C,
    .panoramaRender           = 0x092B8DC0,
    .cubemapBackgroundRender  = 0x097C99A4,
    .cubemapFaceSubmit        = 0, // In-world skybox sharing; do not suppress
    .cubemapRendererRender    = 0,
    .mainMenuCubemapPass      = 0,
    .cubemapDraw              = 0,

    .inGamePlayScreenRender              = 0x0974066C,
    .inGamePlayScreenRenderLevel         = 0x0974097C,
    .inGamePlayScreenRenderLevelPrep     = 0x0973E864,
    .gameRendererPrepareFrame            = 0x0ADD98DC,
    .levelRendererPlayerPreRenderUpdate  = 0x0AE27C64,
    .oreUiSceneRender                    = 0x09220244,
    .uiSceneRender                       = 0x09704394,
    .whiskerDisplayRender                = 0x0AE70680,
    .minecraftGameClientUpdate           = 0x095FB3E8,
    .clientInstanceRequestLeaveGame      = 0x0943C430,
    .clientNetworkHandlerHandle          = 0x09ADC770,
    .loopbackPacketSenderSendToServer    = 0x0C2E26FC,

    .glClearGotRva            = 0x124C9688,
    .uiDrawImage              = 0x0AD845F0,
    .uiFlushImages            = 0x0AD859C4,
    .uiFillRectangle          = 0x0AD85F80,
    .uiDrawRectangle          = 0x0AD87350,
    .uiDrawText               = 0x0AD84430,
    .uiFlushText              = 0x0AD84530,
    .uiDrawNineslice          = 0x0AD855E0,
    .uiGetScreenContext       = 0x0AD87E3C,
    .localPlayerMove          = 0x0EA8DE1C,
    .playerNormalTick         = 0x0F0B1680,
    .levelTick                = 0x0F222D0C,
    .gameModeDestroyBlock     = 0x0EF72C18,
    .gameModeUseItem          = 0x0EF75F40,
    .gameModeUseItemOn        = 0x0EF72684,
    .gameModeAttack           = 0x0EF72F9C,
    .clientInstanceUpdate          = 0x0943EC54,
    .clientInstanceGetLocalPlayer  = 0, // Hidden struct return buffer; use playerNormalTick
    .localPlayerApplyTurnDelta     = 0x0A6430D8,
    .tessellatorBegin              = 0x0A76E780,
    .blockSourceGetBlock           = 0x0F253C8C,
    .blockSourceGetBrightness      = 0x0F253ECC,
    .commandAutocomplete          = 0x0E536964,
    .commandRequestPacketWrite     = 0,
    .soundEnginePlay          = 0x0B2848E0,
    .soundEnginePlayUI        = 0x0B284A5C,
    .soundEnginePlayAt        = 0x0B284EC4,
    .soundEngineStop          = 0x0B285000,
    .directoryPackGetAsset    = 0x1042D360,
    .zipPackGetAsset          = 0x1040CDB8,
    .directoryPackHasAsset    = 0x1042CB64,
    .zipPackHasAsset          = 0x1040D50C,
    .packForEachInAssetSet    = 0x1043791C,
    .resourcePackManagerLoad  = 0x0C594B18,
    .resourcePackManagerGetStack = 0x0C5948C0,
    .resourcePackManagerDoStackOp = 0x0C59B2CC,
    .resourcePackGetResource  = 0x066951F4,
    .textureGroupLoadAsync    = 0x0A403690,
    .textureGroupEnableLRU    = 0x0A404B28,
    .textureHotReloaderRegister = 0x0ACF8718,
    .textureHotReloaderRegisterAtlas = 0x0ACFA0A0,
    .blockGraphicsGetTextureItem = 0x0A224634,
    .cubemapLoadPanoramas     = 0x097C6640,
    .resourcePackRepositoryGetResourcePackInPath = 0x0C59FEF0,
    .repositoryLoadingFindVanillaPacks           = 0x0C5D9184,

    // OreUI (Cohtml Engine) RVAs
    .cohtmlLoadURL               = 0x069C1A7C,
    .cohtmlLoadURLImpl           = 0x069C1A84,
    .cohtmlExecuteScript         = 0x069D0094,
    .cohtmlAddInitialScript      = 0x069D04A8,
    .cohtmlTriggerEvent          = 0x069D7998,
    .cohtmlRegisterForEvent      = 0x069D3C04,
    .cohtmlUnregisterFromEvent   = 0x069D4178,
    .cohtmlTryDispatchClickEvent = 0x069CE0D0,
    .cohtmlMouseEvent            = 0x069CD63C,
    .cohtmlTouchEvent            = 0x069CC0E0,
    .oreUiInit                   = 0x09230484,

    // JSON UI & Screen Controller RVAs
    .screenControllerTick                = 0x096D6EA8,
    .screenControllerHandleEvent         = 0x096D73DC,
    .screenControllerBind                = 0x096D6E54,
    .screenControllerAddStaticScreenVars = 0x096D8394,
    .screenControllerSetScreenState      = 0x096D8664,
    .screenControllerCanExit             = 0x096D8DCC,
    .screenControllerGetScreenModel      = 0x096E5A50,
    .sceneFactoryCreateScreenController  = 0x09765590,
    .sceneFactoryCreateScreen            = 0x09783638,
    .buttonComponentHandleEvent          = 0x09338D6C,
    .buttonComponentSetPressed           = 0x093396B0,
    .buttonComponentGetButtonState       = 0x0933968C,
    .dataDrivenUILayout                  = 0x09199858,
    .dataDrivenUIUIRoot                  = 0x0C59D364,
    .guiDataDisplayClientMessage         = 0x09CFBBD0,

    // Chat, Command & Packet System RVAs
    .chatScreenControllerHandleEvent     = 0x09684878,
    .chatScreenControllerPropertyBind    = 0x09685198,
    .commandRegistryRegisterCommand      = 0x0D6075CC,
    .loopbackPacketSenderSend            = 0x0C2E26EC,
    .serverNetworkHandlerHandle          = 0x0C1E5B4C,

    // Behavior, Recipes, Items, Blocks & Scripting RVAs
    .recipesAddShapedRecipe              = 0x0F2FC108,
    .recipesAddShapelessRecipe            = 0x0F2FCFD4,
    .itemRegistryTryRegisterAndInitItem  = 0x0F2AFB68,
    .itemRegistryParseItemIdentifier     = 0x0F2B2580,
    .blockTypeRegistryInit               = 0x0954D9A4,
    .blockDefinitionLoaderProcessCustomComponents = 0x0FB26F04,
    .actorNormalTick                     = 0x0EA412D8,
    .actorBaseTick                       = 0x0EC84620,
    .actorHurt                           = 0x0EC847A8,
    .actorDie                            = 0x0EC88C50,
    .actorTeleportTo                     = 0x0EC97F6C,
    .actorHasComponent                   = 0x0D6C1B88,
    .actorSetCustomName                  = 0x0ECA1C24,
    .scriptingQuickJS                    = 0x10917BD4,
    .scriptModuleMinecraftScriptSystem   = 0x0DA54B3C,

    // Item & Creative Inventory Subsystem RVAs
    .itemRegistryRegisterItem               = 0x0F2AC5DC,
    .itemRegistryGetItem                    = 0x0F27CE28,
    .itemRegistryPostInitHook               = 0x0F2B41D8,
    .itemConstructor                        = 0x0F65A2D4,
    .itemSetIconInfo                        = 0x0F65D824,
    .itemSetDescriptionId                   = 0,
    .itemVtable                             = 0x122E1F18,
    .itemStackVtable                        = 0x122DE850,
    .itemInstanceVtable                     = 0x122DB0F8,
    .itemStackCopyCtor                      = 0x0F63AB48,
    .vanillaItemsRegisterItems              = 0x0F470C88,
    .vanillaItemsServerInitCreativeCallback = 0x0F6370A4,
    .vanillaItemsApplyHardcodedLegacyIcons  = 0x0F62065C,
    .creativeItemRegistryReset              = 0x0F2BF7BC,
    .creativeItemInitializerCtor            = 0x0F6D93AC,
    .creativeItemInitializerAddCreativeServer = 0x0F6D9374,
    .playerGetSelectedItem                  = 0x0F0C67A8,
    .containerGetItem                       = 0x0EF7AA90,
    .containerSetItem                       = 0x0EF7A3F4,
    .blockItemConstructor                   = 0x0F6E4D0C,
    .blockItemVtable                        = 0x122F10F0,
    .blockItemUseOn                         = 0x0F6E645C,
    .blockSourceSetBlock                    = 0x0F25B948,
    .blockTypeRegistryGetInstance           = 0x0FB149D8,
    .blockTypeRegistryLookupByName          = 0x0FB33684,
    .blockTypeRegistryLookupByNameImpl      = 0x0FB337B0,
    .blockTypeRegistryGetDirectAccessBlocks = 0x0FB33BDC,
    .blockDefinitionGroupInitBlockTypeFromDefinition = 0x0FB13E64,
    .blockDefinitionGroupInitBlockFromDefinition     = 0x0FB1429C,
    .registerItemAndBlockDefinitions        = 0x0F2BF9F0,
    .screenControllerRegisterButtonEventHandler = 0x096D9D38,
    .screenControllerBindString             = 0x096DED60,
    .screenControllerBindStringForAnyCollection = 0x096DEF90,

    // VTable Slots
    .clientInstanceLocalPlayerSlot = 32,
    .clientInstanceIsShowingMenuSlot = 150, // Slot 150 in 1.26.4x
    .levelGetBlockDefinitionGroupSlot = 28,
    .sceneFactoryCreateUIScene = 0x0975E3B0,
    .sceneFactoryCreateScreenAbstract = 0x097609CC,
    .clientInstanceSceneFactorySlot = 224,
    .clientInstanceSceneStackSlot = 235,
    .sceneStackPushScreenSlot = 26,
    .sceneStackSchedulePopSlot = 27
};

// Verified Golden Offsets for Minecraft Bedrock 1.26.51.1 (Android ARM64)
static const EngineOffsets kOffsets_1_26_51_1 = {
    // UI Screen Tech Stack
    .getTechStackForScreen    = 0x0974E7A4,

    // Start Menu Screen Controller
    .startMenuOnOpen          = 0x0A2DF2A4,
    .startMenuTick            = 0x0A2E1F84,

    // Direct Menu Button Handlers
    .playButton               = 0x0A3133DC,
    .settingsButton           = 0x0A31713C,
    .realmsButton             = 0x0A31746C,
    .dressingRoomButton       = 0x0A3180EC,
    .marketplaceButton        = 0x0A318734,
    .menuExit                 = 0x0A5674D0,

    // RenderDragon & Skybox Passes
    .progressBarRender        = 0x09685ED0,
    .panoramaRender           = 0x0967F9F8,
    .cubemapBackgroundRender  = 0x09B7C960,
    .cubemapFaceSubmit        = 0,
    .cubemapRendererRender    = 0,
    .mainMenuCubemapPass      = 0,
    .cubemapDraw              = 0,

    .inGamePlayScreenRender              = 0,
    .inGamePlayScreenRenderLevel         = 0,
    .inGamePlayScreenRenderLevelPrep     = 0,
    .gameRendererPrepareFrame            = 0,
    .levelRendererPlayerPreRenderUpdate  = 0,
    .oreUiSceneRender                    = 0,
    .uiSceneRender                       = 0,
    .whiskerDisplayRender                = 0,
    .minecraftGameClientUpdate           = 0,
    .clientInstanceRequestLeaveGame      = 0,
    .clientNetworkHandlerHandle          = 0,
    .loopbackPacketSenderSendToServer    = 0,

    // GPU / libGLESv2 Hooks
    .glClearGotRva            = 0x13289078,

    // MinecraftUIRenderContext Virtual Method RVAs
    .uiDrawImage              = 0x0B2508C0,
    .uiFlushImages            = 0x0B251C94,
    .uiFillRectangle          = 0x0B252250,
    .uiDrawRectangle          = 0x0B253620,
    .uiDrawText               = 0x0B250700,
    .uiFlushText              = 0x0B250800,
    .uiDrawNineslice          = 0x0B2518B0,
    .uiGetScreenContext       = 0x0B25410C,

    // Player / GameMode / World
    .localPlayerMove          = 0x0F286DF8,
    .playerNormalTick         = 0x0F9E2300,
    .levelTick                = 0x0FB31004,
    .gameModeDestroyBlock     = 0,
    .gameModeUseItem          = 0,
    .gameModeUseItemOn        = 0x0E3AF224,
    .gameModeAttack           = 0,
    .clientInstanceUpdate          = 0,
    .clientInstanceGetLocalPlayer  = 0,
    .localPlayerApplyTurnDelta     = 0,
    .tessellatorBegin              = 0,
    .blockSourceGetBlock           = 0,
    .blockSourceGetBrightness      = 0,
    .commandAutocomplete          = 0,
    .commandRequestPacketWrite     = 0,
    .soundEnginePlay          = 0x0B79F720,
    .soundEnginePlayUI        = 0x0B79F8A4,
    .soundEnginePlayAt        = 0x0B79FD0C,
    .soundEngineStop          = 0x0B79FE48,

    // Resource Pack & Asset Access Strategy RVAs
    .directoryPackGetAsset    = 0x111EEA54,
    .zipPackGetAsset          = 0x11157AC0,
    .directoryPackHasAsset    = 0x111EE468,
    .zipPackHasAsset          = 0x11158214,
    .packForEachInAssetSet    = 0x1117F918,
    .resourcePackManagerLoad  = 0x0C92D160,
    .resourcePackManagerGetStack = 0x0C92CF10,
    .resourcePackManagerDoStackOp = 0x0C930108,
    .resourcePackGetResource  = 0x06A3DE00,
    .textureGroupLoadAsync    = 0x0A777490,
    .textureGroupEnableLRU    = 0x0A778928,
    .textureHotReloaderRegister = 0x0B1FC160,
    .textureHotReloaderRegisterAtlas = 0x0B1FDAE8,
    .blockGraphicsGetTextureItem = 0x0A66DBA0,
    .cubemapLoadPanoramas     = 0x09B799A8,
    .resourcePackRepositoryGetResourcePackInPath = 0x0C93538C,
    .repositoryLoadingFindVanillaPacks           = 0x0C96C444,

    // OreUI (Cohtml Engine) RVAs
    .cohtmlLoadURL               = 0x06D6F8DC,
    .cohtmlLoadURLImpl           = 0x06D6F8E4,
    .cohtmlExecuteScript         = 0x06D7DF68,
    .cohtmlAddInitialScript      = 0x06D7E37C,
    .cohtmlTriggerEvent          = 0x06D8586C,
    .cohtmlRegisterForEvent      = 0x06D81AD8,
    .cohtmlUnregisterFromEvent   = 0x06D81B54,
    .cohtmlTryDispatchClickEvent = 0x06D7BF9C,
    .cohtmlMouseEvent            = 0x06D7B508,
    .cohtmlTouchEvent            = 0x06D79FA8,
    .oreUiInit                   = 0x096F738C,

    // JSON UI & Screen Controller RVAs
    .screenControllerTick                = 0x09B14FEC,
    .screenControllerHandleEvent         = 0x09B14870,
    .screenControllerBind                = 0x09B1481C,
    .screenControllerAddStaticScreenVars = 0x09B15014,
    .screenControllerSetScreenState      = 0x09B161F4,
    .screenControllerCanExit             = 0x09B1695C,
    .screenControllerGetScreenModel      = 0x09B23694,
    .sceneFactoryCreateScreenController  = 0x09AEE67C,
    .sceneFactoryCreateScreen            = 0x09AFD3B0,
    .buttonComponentHandleEvent          = 0x096FB848,
    .buttonComponentSetPressed           = 0x096FC18C,
    .buttonComponentGetButtonState       = 0x096FC168,
    .dataDrivenUILayout                  = 0x09C35650,
    .dataDrivenUIUIRoot                  = 0x0B868ECC,
    .guiDataDisplayClientMessage         = 0x0A1038AC,

    // Chat, Command & Packet System RVAs
    .chatScreenControllerHandleEvent     = 0x09A0EFA0,
    .chatScreenControllerPropertyBind    = 0x09A0F8C8,
    .commandRegistryRegisterCommand      = 0x0DC4A018,
    .loopbackPacketSenderSend            = 0,
    .serverNetworkHandlerHandle          = 0x0C583850,

    // Behavior, Recipes, Items, Blocks & Scripting RVAs
    .recipesAddShapedRecipe              = 0x0FC3D018,
    .recipesAddShapelessRecipe            = 0x0FC3DE14,
    .itemRegistryTryRegisterAndInitItem  = 0x0FBB2C54,
    .itemRegistryParseItemIdentifier     = 0x0FBB5694,
    .blockTypeRegistryInit               = 0x0AD6C1FC,
    .blockDefinitionLoaderProcessCustomComponents = 0x10491C38,
    .actorNormalTick                     = 0x0F27D708,
    .actorBaseTick                       = 0x0F560460,
    .actorHurt                           = 0x0F5605E8,
    .actorDie                            = 0x0F564ABC,
    .actorTeleportTo                     = 0x0F5742FC,
    .actorHasComponent                   = 0x0F582FD0,
    .actorSetCustomName                  = 0x0F57E2D4,
    .scriptingQuickJS                    = 0x11431518,
    .scriptModuleMinecraftScriptSystem   = 0x0E069C90,

    // Item & Creative Inventory Subsystem RVAs
    .itemRegistryRegisterItem               = 0,
    .itemRegistryGetItem                    = 0,
    .itemRegistryPostInitHook               = 0,
    .itemConstructor                        = 0,
    .itemSetIconInfo                        = 0x0FF7C448,
    .itemSetDescriptionId                   = 0,
    .itemVtable                             = 0,
    .itemStackVtable                        = 0,
    .itemInstanceVtable                     = 0,
    .itemStackCopyCtor                      = 0,
    .vanillaItemsRegisterItems              = 0,
    .vanillaItemsServerInitCreativeCallback = 0,
    .vanillaItemsApplyHardcodedLegacyIcons  = 0,
    .creativeItemRegistryReset              = 0,
    .creativeItemInitializerCtor            = 0,
    .creativeItemInitializerAddCreativeServer = 0,
    .playerGetSelectedItem                  = 0,
    .containerGetItem                       = 0,
    .containerSetItem                       = 0,
    .blockItemConstructor                   = 0,
    .blockItemVtable                        = 0,
    .blockItemUseOn                         = 0,
    .blockSourceSetBlock                    = 0x0E389AA0,
    .blockTypeRegistryGetInstance           = 0,
    .blockTypeRegistryLookupByName          = 0,
    .blockTypeRegistryLookupByNameImpl      = 0,
    .blockTypeRegistryGetDirectAccessBlocks = 0,
    .blockDefinitionGroupInitBlockTypeFromDefinition = 0,
    .blockDefinitionGroupInitBlockFromDefinition     = 0,
    .registerItemAndBlockDefinitions        = 0,
    .screenControllerRegisterButtonEventHandler = 0,
    .screenControllerBindString             = 0,
    .screenControllerBindStringForAnyCollection = 0,

    // VTable Slots
    .clientInstanceLocalPlayerSlot = 32,
    .clientInstanceIsShowingMenuSlot = 151,
    .levelGetBlockDefinitionGroupSlot = 28,
    .sceneFactoryCreateUIScene = 0,
    .sceneFactoryCreateScreenAbstract = 0,
    .clientInstanceSceneFactorySlot = 0,
    .clientInstanceSceneStackSlot = 0,
    .sceneStackPushScreenSlot = 0,
    .sceneStackSchedulePopSlot = 0
};

static std::string sDetectedVersion = "1.26.52.3";
static EngineOffsets sActiveOffsets = kOffsets_1_26_52_3;
static std::mutex sOffsetMutex;

const EngineOffsets& getOffsets() {
    std::lock_guard<std::mutex> lock(sOffsetMutex);
    return sActiveOffsets;
}

std::string getDetectedVersion() {
    std::lock_guard<std::mutex> lock(sOffsetMutex);
    return sDetectedVersion;
}

void setTargetVersion(const std::string& version) {
    std::lock_guard<std::mutex> lock(sOffsetMutex);
    sDetectedVersion = version;
    if (version.find("1.26.44") != std::string::npos) {
        sActiveOffsets = kOffsets_1_26_44_3;
    } else if (version.find("1.26.45") != std::string::npos) {
        sActiveOffsets = kOffsets_1_26_45_1;
    } else if (version.find("1.26.51") != std::string::npos) {
        sActiveOffsets = kOffsets_1_26_51_1;
    } else {
        sActiveOffsets = kOffsets_1_26_52_3;
    }
    ll::Logger::getInstance().info("LeviLamroid bound offset dictionary to Minecraft Bedrock ", sDetectedVersion);
}

void autoDetectVersion() {
    // 1. Try exact ELF GNU Build ID matching from memory
    std::string buildId;
    dl_iterate_phdr([](struct dl_phdr_info* info, size_t, void* data) -> int {
        auto* outId = static_cast<std::string*>(data);
        if (!info->dlpi_name || !strstr(info->dlpi_name, "libminecraftpe.so")) {
            return 0;
        }

        for (int i = 0; i < info->dlpi_phnum; ++i) {
            if (info->dlpi_phdr[i].p_type == PT_NOTE) {
                const uint8_t* notePtr = reinterpret_cast<const uint8_t*>(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
                size_t remaining = info->dlpi_phdr[i].p_memsz;
                while (remaining >= sizeof(Elf64_Nhdr)) {
                    const auto* nhdr = reinterpret_cast<const Elf64_Nhdr*>(notePtr);
                    size_t nameLen = (nhdr->n_namesz + 3) & ~3;
                    size_t descLen = (nhdr->n_descsz + 3) & ~3;
                    if (sizeof(Elf64_Nhdr) + nameLen + descLen > remaining) break;

                    const char* name = reinterpret_cast<const char*>(notePtr + sizeof(Elf64_Nhdr));
                    if (nhdr->n_type == NT_GNU_BUILD_ID && nhdr->n_namesz == 4 && memcmp(name, "GNU", 3) == 0) {
                        const uint8_t* desc = notePtr + sizeof(Elf64_Nhdr) + nameLen;
                        char hex[64];
                        for (size_t b = 0; b < nhdr->n_descsz && b < 32; ++b) {
                            snprintf(hex + b * 2, sizeof(hex) - b * 2, "%02x", desc[b]);
                        }
                        *outId = hex;
                        return 1;
                    }
                    notePtr += sizeof(Elf64_Nhdr) + nameLen + descLen;
                    remaining -= sizeof(Elf64_Nhdr) + nameLen + descLen;
                }
            }
        }
        return 0;
    }, &buildId);

    if (!buildId.empty()) {
        ll::Logger::getInstance().info("Discovered libminecraftpe.so GNU Build ID: ", buildId);
        if (buildId.starts_with("b480c79a")) {
            setTargetVersion("1.26.44.3");
            return;
        } else if (buildId.starts_with("56de9eed")) {
            setTargetVersion("1.26.52.3");
            return;
        } else if (buildId.starts_with("868e275c") || buildId.starts_with("86895c02")) {
            setTargetVersion("1.26.45.1");
            return;
        } else if (buildId.starts_with("712509dc")) {
            setTargetVersion("1.26.51.1");
            return;
        }
    }

    // 2. Check memory maps for profile path indications
    FILE* fp = fopen("/proc/self/maps", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "libminecraftpe.so")) {
                if (strstr(line, "1.26.44")) {
                    fclose(fp);
                    setTargetVersion("1.26.44.3");
                    return;
                } else if (strstr(line, "1.26.45")) {
                    fclose(fp);
                    setTargetVersion("1.26.45.1");
                    return;
                } else if (strstr(line, "1.26.51")) {
                    fclose(fp);
                    setTargetVersion("1.26.51.1");
                    return;
                }
            }
        }
        fclose(fp);
    }

    // 3. Fallback to /proc/self/cmdline
    std::ifstream cmdline("/proc/self/cmdline");
    std::string cmd;
    if (std::getline(cmdline, cmd)) {
        if (cmd.find("1.26.44") != std::string::npos) {
            setTargetVersion("1.26.44.3");
            return;
        } else if (cmd.find("1.26.45") != std::string::npos) {
            setTargetVersion("1.26.45.1");
            return;
        } else if (cmd.find("1.26.51") != std::string::npos) {
            setTargetVersion("1.26.51.1");
            return;
        }
    }

    // Default to primary active version
    setTargetVersion("1.26.52.3");
}

uintptr_t findFunctionRva(std::string_view name) {
    const auto& off = getOffsets();
    if (name == "getTechStackForScreen" || name == "ScreenTechStackSelector::getTechStackForScreen") return off.getTechStackForScreen;
    if (name == "startMenuOnOpen" || name == "StartMenuScreenController::_onOpen" || name == "StartMenuScreenController::onOpen") return off.startMenuOnOpen;
    if (name == "startMenuTick" || name == "StartMenuScreenController::tick") return off.startMenuTick;
    if (name == "playButton") return off.playButton;
    if (name == "settingsButton") return off.settingsButton;
    if (name == "realmsButton") return off.realmsButton;
    if (name == "dressingRoomButton") return off.dressingRoomButton;
    if (name == "marketplaceButton") return off.marketplaceButton;
    if (name == "menuExit") return off.menuExit;
    if (name == "progressBarRender") return off.progressBarRender;
    if (name == "panoramaRender") return off.panoramaRender;
    if (name == "cubemapBackgroundRender") return off.cubemapBackgroundRender;
    if (name == "cubemapFaceSubmit") return off.cubemapFaceSubmit;
    if (name == "cubemapRendererRender") return off.cubemapRendererRender;
    if (name == "mainMenuCubemapPass") return off.mainMenuCubemapPass;
    if (name == "cubemapDraw") return off.cubemapDraw;
    if (name == "glClearGotRva") return off.glClearGotRva;

    // UI Virtual Methods
    if (name == "uiDrawImage" || name == "MinecraftUIRenderContext::drawImage") return off.uiDrawImage;
    if (name == "uiFlushImages" || name == "MinecraftUIRenderContext::flushImages") return off.uiFlushImages;
    if (name == "uiFillRectangle" || name == "MinecraftUIRenderContext::fillRectangle") return off.uiFillRectangle;
    if (name == "uiDrawRectangle" || name == "MinecraftUIRenderContext::drawRectangle") return off.uiDrawRectangle;
    if (name == "uiDrawText" || name == "MinecraftUIRenderContext::drawText") return off.uiDrawText;
    if (name == "uiFlushText" || name == "MinecraftUIRenderContext::flushText") return off.uiFlushText;
    if (name == "uiDrawNineslice" || name == "MinecraftUIRenderContext::drawNineslice") return off.uiDrawNineslice;
    if (name == "uiGetScreenContext" || name == "MinecraftUIRenderContext::getScreenContext") return off.uiGetScreenContext;

    // Player, World & GameMode
    if (name == "localPlayerMove" || name == "LocalPlayer::move" || name == "LocalPlayerMove") return off.localPlayerMove;
    if (name == "playerNormalTick" || name == "Player::normalTick" || name == "PlayerNormalTick") return off.playerNormalTick;
    if (name == "levelTick" || name == "Level::tick" || name == "LevelTick") return off.levelTick;
    if (name == "gameModeDestroyBlock" || name == "GameMode::destroyBlock") return off.gameModeDestroyBlock;
    if (name == "gameModeUseItem" || name == "GameMode::useItem") return off.gameModeUseItem;
    if (name == "gameModeUseItemOn" || name == "GameMode::useItemOn") return off.gameModeUseItemOn;
    if (name == "gameModeAttack" || name == "GameMode::attack") return off.gameModeAttack;

    // Audio / SoundEngine
    if (name == "soundEnginePlay" || name == "SoundEngine::play") return off.soundEnginePlay;
    if (name == "soundEnginePlayUI" || name == "SoundEngine::playUI") return off.soundEnginePlayUI;
    if (name == "soundEnginePlayAt" || name == "SoundEngine::playAt") return off.soundEnginePlayAt;
    if (name == "soundEngineStop" || name == "SoundEngine::stop") return off.soundEngineStop;

    // Resource Packs & Assets
    if (name == "directoryPackGetAsset" || name == "DirectoryPackAccessStrategy::getAsset") return off.directoryPackGetAsset;
    if (name == "zipPackGetAsset" || name == "ZipPackAccessStrategy::getAsset") return off.zipPackGetAsset;
    if (name == "directoryPackHasAsset" || name == "DirectoryPackAccessStrategy::hasAsset") return off.directoryPackHasAsset;
    if (name == "zipPackHasAsset" || name == "ZipPackAccessStrategy::hasAsset") return off.zipPackHasAsset;
    if (name == "packForEachInAssetSet") return off.packForEachInAssetSet;
    if (name == "resourcePackManagerLoad" || name == "ResourcePackManager::load") return off.resourcePackManagerLoad;
    if (name == "resourcePackManagerGetStack" || name == "ResourcePackManager::getStack") return off.resourcePackManagerGetStack;
    if (name == "resourcePackManagerDoStackOp") return off.resourcePackManagerDoStackOp;
    if (name == "resourcePackGetResource" || name == "ResourcePack::getResource") return off.resourcePackGetResource;
    if (name == "textureGroupLoadAsync" || name == "TextureGroup::loadAsync") return off.textureGroupLoadAsync;
    if (name == "textureGroupEnableLRU") return off.textureGroupEnableLRU;
    if (name == "textureHotReloaderRegister") return off.textureHotReloaderRegister;
    if (name == "textureHotReloaderRegisterAtlas") return off.textureHotReloaderRegisterAtlas;
    if (name == "blockGraphicsGetTextureItem") return off.blockGraphicsGetTextureItem;
    if (name == "cubemapLoadPanoramas") return off.cubemapLoadPanoramas;
    if (name == "resourcePackRepositoryGetResourcePackInPath") return off.resourcePackRepositoryGetResourcePackInPath;
    if (name == "repositoryLoadingFindVanillaPacks") return off.repositoryLoadingFindVanillaPacks;

    // OreUI (Cohtml Engine)
    if (name == "cohtmlLoadURL" || name == "cohtml::View::LoadURL") return off.cohtmlLoadURL;
    if (name == "cohtmlLoadURLImpl") return off.cohtmlLoadURLImpl;
    if (name == "cohtmlExecuteScript" || name == "cohtml::View::ExecuteScript") return off.cohtmlExecuteScript;
    if (name == "cohtmlAddInitialScript") return off.cohtmlAddInitialScript;
    if (name == "cohtmlTriggerEvent" || name == "cohtml::View::TriggerEvent") return off.cohtmlTriggerEvent;
    if (name == "cohtmlRegisterForEvent") return off.cohtmlRegisterForEvent;
    if (name == "cohtmlUnregisterFromEvent") return off.cohtmlUnregisterFromEvent;
    if (name == "cohtmlTryDispatchClickEvent") return off.cohtmlTryDispatchClickEvent;
    if (name == "cohtmlMouseEvent" || name == "cohtml::ViewImpl::MouseEvent") return off.cohtmlMouseEvent;
    if (name == "cohtmlTouchEvent" || name == "cohtml::ViewImpl::TouchEvent") return off.cohtmlTouchEvent;
    if (name == "oreUiInit") return off.oreUiInit;

    // JSON UI & Screen Controller
    if (name == "screenControllerTick" || name == "ScreenController::tick") return off.screenControllerTick;
    if (name == "screenControllerHandleEvent" || name == "ScreenController::handleEvent") return off.screenControllerHandleEvent;
    if (name == "screenControllerBind" || name == "ScreenController::bind") return off.screenControllerBind;
    if (name == "screenControllerAddStaticScreenVars") return off.screenControllerAddStaticScreenVars;
    if (name == "screenControllerSetScreenState") return off.screenControllerSetScreenState;
    if (name == "screenControllerCanExit") return off.screenControllerCanExit;
    if (name == "screenControllerGetScreenModel") return off.screenControllerGetScreenModel;
    if (name == "sceneFactoryCreateScreenController") return off.sceneFactoryCreateScreenController;
    if (name == "sceneFactoryCreateScreen") return off.sceneFactoryCreateScreen;
    if (name == "buttonComponentHandleEvent") return off.buttonComponentHandleEvent;
    if (name == "buttonComponentSetPressed") return off.buttonComponentSetPressed;
    if (name == "buttonComponentGetButtonState") return off.buttonComponentGetButtonState;
    if (name == "dataDrivenUILayout") return off.dataDrivenUILayout;
    if (name == "dataDrivenUIUIRoot") return off.dataDrivenUIUIRoot;
    if (name == "guiDataDisplayClientMessage" || name == "GuiData::displayClientMessage") return off.guiDataDisplayClientMessage;

    // Chat, Command & Packet System
    if (name == "chatScreenControllerHandleEvent" || name == "ChatScreenController::handleEvent") return off.chatScreenControllerHandleEvent;
    if (name == "chatScreenControllerPropertyBind") return off.chatScreenControllerPropertyBind;
    if (name == "commandRegistryRegisterCommand" || name == "CommandRegistry::registerCommand") return off.commandRegistryRegisterCommand;
    if (name == "loopbackPacketSenderSend" || name == "LoopbackPacketSender::send") return off.loopbackPacketSenderSend;
    if (name == "serverNetworkHandlerHandle" || name == "ServerNetworkHandler::handle") return off.serverNetworkHandlerHandle;

    // Behavior, Recipes, Items, Blocks & Scripting
    if (name == "recipesAddShapedRecipe" || name == "Recipes::addShapedRecipe") return off.recipesAddShapedRecipe;
    if (name == "recipesAddShapelessRecipe" || name == "Recipes::addShapelessRecipe") return off.recipesAddShapelessRecipe;
    if (name == "itemRegistryTryRegisterAndInitItem" || name == "ItemRegistry::tryRegisterAndInitItem") return off.itemRegistryTryRegisterAndInitItem;
    if (name == "itemRegistryParseItemIdentifier") return off.itemRegistryParseItemIdentifier;
    if (name == "blockTypeRegistryInit" || name == "BlockTypeRegistry::init") return off.blockTypeRegistryInit;
    if (name == "blockTypeRegistryGetInstance" || name == "BlockTypeRegistry::getInstance") return off.blockTypeRegistryGetInstance;
    if (name == "blockTypeRegistryLookupByName" || name == "BlockTypeRegistry::lookupByName") return off.blockTypeRegistryLookupByName;
    if (name == "blockTypeRegistryLookupByNameImpl" || name == "BlockTypeRegistry::_lookupByNameImpl") return off.blockTypeRegistryLookupByNameImpl;
    if (name == "blockDefinitionLoaderProcessCustomComponents") return off.blockDefinitionLoaderProcessCustomComponents;
    if (name == "actorNormalTick" || name == "Actor::normalTick") return off.actorNormalTick;
    if (name == "actorBaseTick" || name == "Actor::baseTick") return off.actorBaseTick;
    if (name == "actorHurt" || name == "Actor::hurt") return off.actorHurt;
    if (name == "actorDie" || name == "Actor::die") return off.actorDie;
    if (name == "actorTeleportTo" || name == "Actor::teleportTo") return off.actorTeleportTo;
    if (name == "actorHasComponent" || name == "Actor::hasComponent") return off.actorHasComponent;
    if (name == "actorSetCustomName" || name == "Actor::setCustomName") return off.actorSetCustomName;
    if (name == "itemSetIconInfo" || name == "Item::setIconInfo") return off.itemSetIconInfo;
    if (name == "itemConstructor" || name == "Item::Item") return off.itemConstructor;
    if (name == "scriptingQuickJS") return off.scriptingQuickJS;
    if (name == "scriptModuleMinecraftScriptSystem") return off.scriptModuleMinecraftScriptSystem;
    return 0;
}

} // namespace levilamroid::mcpe

extern "C" {
LL_API const char* ll_mcpe_get_detected_version() {
    static std::string ver;
    ver = ::levilamroid::mcpe::getDetectedVersion();
    return ver.c_str();
}
}
