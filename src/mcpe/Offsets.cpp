#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/Logger.h"
#include "levilamroid/api/memory/Hook.h"
#include <mutex>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <link.h>
#include <elf.h>
#include <dlfcn.h>

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

    // Client-Side Fabric Engine & RenderDragon RVAs (Verified Golden on 1.26.52.3)
    .inGamePlayScreenRender              = 0, // Disabled: avoid invalid RVA crashing on world join
    .inGamePlayScreenRenderLevel         = 0x0B2F53BC, // LevelRenderer::renderLevel (Verified Golden Pass 10 on 1.26.52.3)
    .inGamePlayScreenRenderLevelPrep     = 0, // Disabled: avoid mid-function ADRP
    .gameRendererPrepareFrame            = 0, // Disabled: avoid unverified hook
    .levelRendererPlayerPreRenderUpdate  = 0x0B2F53BC, // LevelRenderer::renderLevel (Verified Golden Pass 10 on 1.26.52.3)
    .oreUiSceneRender                    = 0,
    .uiSceneRender                       = 0,
    .whiskerDisplayRender                = 0,
    .minecraftGameClientUpdate           = 0, // Disabled
    .clientInstanceRequestLeaveGame      = 0x09800FF8,
    .clientNetworkHandlerHandle          = 0, // Disabled: unverified
    .loopbackPacketSenderSendToServer    = 0,

    // GPU / libGLESv2 Hooks
    .glClearGotRva            = 0x132893d8, // Hooked with strict sIsOnMainMenu guard to prevent in-world interference

    // MinecraftUIRenderContext Virtual Method RVAs (Disabled direct hooking to prevent UI text corruption)
    .uiDrawImage              = 0,
    .uiFlushImages            = 0,
    .uiFillRectangle          = 0,
    .uiDrawRectangle          = 0,
    .uiDrawText               = 0,
    .uiFlushText              = 0,
    .uiDrawNineslice          = 0,
    .uiGetScreenContext       = 0,

    // Player / GameMode / World
    .localPlayerMove          = 0,
    .playerNormalTick         = 0x0F9E21B0, // Verified golden LocalPlayer::normalTick (LocalPlayer::vslot_03)
    .levelTick                = 0,
    .gameModeDestroyBlock     = 0,
    .gameModeUseItem          = 0,
    .gameModeUseItemOn        = 0,
    .gameModeAttack           = 0,
    .gameModeBuildBlock       = 0,
    .gameModeStartBuildBlock  = 0,

    // Touch & Mouse Input Subsystem (Audited from LeviMap on 1.26.52.3)
    .mouseHandlerAndroidVtable            = 0x12DB3348,
    .clientInstanceVtable                 = 0x12BC3300,
    .minecraftGameVtable                  = 0x12BCE6F8,
    .blockSourceVtable                    = 0x12ACC870,
    .mouseHandlerAndroidButtonInput       = 0x0B9C412C,
    .mouseHandlerAndroidAbsoluteMouseInput = 0x0B9C40D8,
    .mouseHandlerAndroidScrollInput       = 0x0B9C4148,
    .mainActivityNativeKeyHandler         = 0x0852FDD8,

    // Verified Engine & Graphics RVAs
    .clientInstanceUpdate          = 0,
    .clientInstanceGetLocalPlayer  = 0,
    .localPlayerApplyTurnDelta     = 0x0A6430D8, // LocalPlayer::applyTurnDelta (Sig 62)
    .tessellatorBegin              = 0,
    .blockSourceGetBlock           = 0x0FB91918,
    .blockSourceGetBrightness      = 0x0FB9A55C, // BlockSource::getBrightness (Verified Golden Pass 10 on 1.26.52.3)
    .dimensionGetBlockSource       = 0x102F4538,
    .commandAutocomplete          = 0,
    .commandRequestPacketWrite    = 0,

    .soundEnginePlay          = 0x0B79F7F4,
    .soundEnginePlayUI        = 0x0B79F978,
    .soundEnginePlayAt        = 0x0B79FDE0,
    .soundEngineStop          = 0x0B79FF1C,

    // Resource Pack & Asset Access Strategy RVAs (Zeroed out to prevent world-join crash)
    .directoryPackGetAsset    = 0,
    .zipPackGetAsset          = 0,
    .directoryPackHasAsset    = 0,
    .zipPackHasAsset          = 0,
    .packForEachInAssetSet    = 0,
    .resourcePackManagerLoad  = 0x0C92D4B8,
    .resourcePackManagerGetStack = 0x0C92D268,
    .resourcePackManagerDoStackOp = 0x0C930460,
    .resourcePackManagerComposeFullStack = 0x0C931D34,
    .resourcePackManager_composeFullStack = 0x0C931828,
    .resourcePackManagerSetStack = 0x0C930958,
    .resourcePackGetResource  = 0,
    .textureGroupLoadAsync    = 0,
    .textureGroupEnableLRU    = 0,
    .textureHotReloaderRegister = 0,
    .textureHotReloaderRegisterAtlas = 0,
    .blockGraphicsGetTextureItem = 0,
    .cubemapLoadPanoramas     = 0,
    .resourcePackRepositoryGetResourcePackInPath = 0,
    .repositoryLoadingFindVanillaPacks           = 0,

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
    .commandRegistryRegisterCommand      = 0x0DC4A370,
    .loopbackPacketSenderSend            = 0, // Disabled: prevents outbound packet destruction and world-loading stall
    .serverNetworkHandlerHandle          = 0, // Disabled: mid-function ADRP causes crash on world join

    // Behavior, Recipes, Items, Blocks & Scripting RVAs
    .recipesAddShapedRecipe              = 0x0FC3D3D8,
    .recipesAddShapelessRecipe            = 0x0FC3E1D4,
    .itemRegistryTryRegisterAndInitItem  = 0x0FBB3014,
    .itemRegistryParseItemIdentifier     = 0x0FBB5A54,
    .blockTypeRegistryInit               = 0, // Disabled: blockTypeRegistryGetInstance() resolves registry directly and crash-free
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
    .itemRegistryRegisterItem               = 0,
    .itemRegistryGetItem                    = 0,
    .itemRegistryPostInitHook               = 0,
    .itemConstructor                        = 0,
    .itemSetIconInfo                        = 0,
    .itemSetDescriptionId                   = 0,
    .itemVtable                             = 0x1308A650,
    .itemStackVtable                        = 0x1308BE60,
    .itemInstanceVtable                     = 0x1308AC00,
    .itemStackCopyCtor                      = 0x0FF9DB08,
    .vanillaItemsRegisterItems              = 0,
    .vanillaItemsServerInitCreativeCallback = 0,
    .vanillaItemsApplyHardcodedLegacyIcons  = 0,
    .creativeItemRegistryReset              = 0x0FBC3604,
    .creativeItemInitializerCtor            = 0x0FFFC988,
    .creativeItemInitializerAddCreativeServer = 0x1002227C,
    .playerGetSelectedItem                  = 0, // Disabled: avoid calling unverified vslot
    .containerGetItem                       = 0x0F6EFE7C,
    .containerSetItem                       = 0x0F6EFEB8,
    .blockItemConstructor                   = 0x10009DC4,
    .blockItemVtable                        = 0x13099938, // Native Bedrock BlockItem vtable
    .blockItemUseOn                         = 0x1000B300, // BlockItem::_useOn
    .blockSourceSetBlock                    = 0x0FB99330,
    .blockSourceDestroyBlock                = 0x0FB9A468,
    .dimensionGetChunkSource                = 0x102F1BEC,
    .blockTypeRegistryGetInstance           = 0x103E33A8,
    .blockTypeRegistryLookupByName          = 0,
    .blockTypeRegistryLookupByNameImpl      = 0,
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

// Verified Golden Offsets for Minecraft Bedrock 1.26.60.6 (Android ARM64)
static const EngineOffsets kOffsets_1_26_60_6 = {
    .getTechStackForScreen                    = 0,
    .startMenuOnOpen                          = 0,
    .startMenuTick                            = 0,
    .playButton                               = 0,
    .settingsButton                           = 0,
    .realmsButton                             = 0,
    .dressingRoomButton                       = 0,
    .marketplaceButton                        = 0,
    .menuExit                                 = 0,
    .progressBarRender                        = 0,
    .panoramaRender                           = 0,
    .cubemapBackgroundRender                  = 0x09effb18,
    .cubemapFaceSubmit                        = 0,
    .cubemapRendererRender                    = 0,
    .mainMenuCubemapPass                      = 0,
    .cubemapDraw                              = 0,
    .inGamePlayScreenRender                   = 0,
    .inGamePlayScreenRenderLevel              = 0x11b3f100,
    .inGamePlayScreenRenderLevelPrep          = 0,
    .gameRendererPrepareFrame                 = 0,
    .levelRendererPlayerPreRenderUpdate       = 0x11b3f100,
    .oreUiSceneRender                         = 0,
    .uiSceneRender                            = 0,
    .whiskerDisplayRender                     = 0,
    .minecraftGameClientUpdate                = 0,
    .clientInstanceRequestLeaveGame           = 0,
    .clientNetworkHandlerHandle               = 0,
    .loopbackPacketSenderSendToServer         = 0,
    .glClearGotRva                            = 0,
    .uiDrawImage                              = 0,
    .uiFlushImages                            = 0,
    .uiFillRectangle                          = 0,
    .uiDrawRectangle                          = 0,
    .uiDrawText                               = 0,
    .uiFlushText                              = 0,
    .uiDrawNineslice                          = 0,
    .uiGetScreenContext                       = 0,
    .localPlayerMove                          = 0,
    .playerNormalTick                         = 0x101eccd4,
    .levelTick                                = 0,
    .gameModeDestroyBlock                     = 0,
    .gameModeUseItem                          = 0,
    .gameModeUseItemOn                        = 0,
    .gameModeAttack                           = 0,
    .gameModeBuildBlock                       = 0,
    .gameModeStartBuildBlock                  = 0,
    .mouseHandlerAndroidVtable                = 0,
    .clientInstanceVtable                     = 0,
    .minecraftGameVtable                      = 0,
    .blockSourceVtable                        = 0,
    .mouseHandlerAndroidButtonInput           = 0x0bfa0a28,
    .mouseHandlerAndroidAbsoluteMouseInput    = 0x0bfa0a14,
    .mouseHandlerAndroidScrollInput           = 0x0bfa0a3c,
    .mainActivityNativeKeyHandler             = 0,
    .clientInstanceUpdate                     = 0,
    .clientInstanceGetLocalPlayer             = 0x09b7f684,
    .localPlayerApplyTurnDelta                = 0x0fd9ae30,
    .tessellatorBegin                         = 0,
    .blockSourceGetBlock                      = 0x10371548,
    .blockSourceGetBrightness                 = 0x10371620,
    .dimensionGetBlockSource                  = 0,
    .commandAutocomplete                      = 0,
    .commandRequestPacketWrite                = 0,
    .soundEnginePlay                          = 0x0bce01c8,
    .soundEnginePlayUI                        = 0x0bce0350,
    .soundEnginePlayAt                        = 0x0bce07b8,
    .soundEngineStop                          = 0,
    .directoryPackGetAsset                    = 0,
    .zipPackGetAsset                          = 0,
    .directoryPackHasAsset                    = 0,
    .zipPackHasAsset                          = 0,
    .packForEachInAssetSet                    = 0,
    .resourcePackManagerLoad                  = 0x0cf4e7d0,
    .resourcePackManagerGetStack              = 0x088b7404,
    .resourcePackManagerDoStackOp             = 0x0cf535d8,
    .resourcePackManagerComposeFullStack      = 0x0cf52894,
    .resourcePackManager_composeFullStack     = 0x0cf52774,
    .resourcePackManagerSetStack              = 0x0cf51778,
    .resourcePackGetResource                  = 0,
    .textureGroupLoadAsync                    = 0,
    .textureGroupEnableLRU                    = 0,
    .textureHotReloaderRegister               = 0,
    .textureHotReloaderRegisterAtlas          = 0,
    .blockGraphicsGetTextureItem              = 0,
    .cubemapLoadPanoramas                     = 0,
    .resourcePackRepositoryGetResourcePackInPath = 0,
    .repositoryLoadingFindVanillaPacks        = 0,
    .cohtmlLoadURL                            = 0,
    .cohtmlLoadURLImpl                        = 0,
    .cohtmlExecuteScript                      = 0,
    .cohtmlAddInitialScript                   = 0,
    .cohtmlTriggerEvent                       = 0,
    .cohtmlRegisterForEvent                   = 0,
    .cohtmlUnregisterFromEvent                = 0,
    .cohtmlTryDispatchClickEvent              = 0,
    .cohtmlMouseEvent                         = 0,
    .cohtmlTouchEvent                         = 0,
    .oreUiInit                                = 0,
    .screenControllerTick                     = 0x09e9586c,
    .screenControllerHandleEvent              = 0x09e95354,
    .screenControllerBind                     = 0,
    .screenControllerAddStaticScreenVars      = 0,
    .screenControllerSetScreenState           = 0,
    .screenControllerCanExit                  = 0,
    .screenControllerGetScreenModel           = 0,
    .sceneFactoryCreateScreenController       = 0,
    .sceneFactoryCreateScreen                 = 0,
    .buttonComponentHandleEvent               = 0,
    .buttonComponentSetPressed                = 0,
    .buttonComponentGetButtonState            = 0,
    .dataDrivenUILayout                       = 0,
    .dataDrivenUIUIRoot                       = 0,
    .guiDataDisplayClientMessage              = 0,
    .chatScreenControllerHandleEvent          = 0,
    .chatScreenControllerPropertyBind         = 0,
    .commandRegistryRegisterCommand           = 0x08a06ce8,
    .loopbackPacketSenderSend                 = 0,
    .serverNetworkHandlerHandle               = 0,
    .recipesAddShapedRecipe                   = 0,
    .recipesAddShapelessRecipe                = 0,
    .itemRegistryTryRegisterAndInitItem       = 0,
    .itemRegistryParseItemIdentifier          = 0,
    .blockTypeRegistryInit                    = 0x0a2fc65c,
    .blockDefinitionLoaderProcessCustomComponents = 0,
    .actorNormalTick                          = 0,
    .actorBaseTick                            = 0,
    .actorHurt                                = 0,
    .actorDie                                 = 0,
    .actorTeleportTo                          = 0,
    .actorHasComponent                        = 0,
    .actorSetCustomName                       = 0,
    .scriptingQuickJS                         = 0,
    .scriptModuleMinecraftScriptSystem        = 0,
    .itemRegistryRegisterItem                 = 0,
    .itemRegistryGetItem                      = 0,
    .itemRegistryPostInitHook                 = 0,
    .itemConstructor                          = 0,
    .itemSetIconInfo                          = 0,
    .itemSetDescriptionId                     = 0,
    .itemVtable                               = 0,
    .itemStackVtable                          = 0,
    .itemInstanceVtable                       = 0,
    .itemStackCopyCtor                        = 0,
    .vanillaItemsRegisterItems                = 0,
    .vanillaItemsServerInitCreativeCallback   = 0,
    .vanillaItemsApplyHardcodedLegacyIcons    = 0,
    .creativeItemRegistryReset                = 0,
    .creativeItemInitializerCtor              = 0,
    .creativeItemInitializerAddCreativeServer = 0,
    .playerGetSelectedItem                    = 0,
    .containerGetItem                         = 0,
    .containerSetItem                         = 0,
    .blockItemConstructor                     = 0,
    .blockItemVtable                          = 0,
    .blockItemUseOn                           = 0,
    .blockSourceSetBlock                      = 0,
    .blockSourceDestroyBlock                  = 0,
    .dimensionGetChunkSource                  = 0,
    .blockTypeRegistryGetInstance             = 0,
    .blockTypeRegistryLookupByName            = 0,
    .blockTypeRegistryLookupByNameImpl        = 0,
    .blockTypeRegistryGetDirectAccessBlocks   = 0,
    .blockDefinitionGroupInitBlockTypeFromDefinition = 0,
    .blockDefinitionGroupInitBlockFromDefinition = 0,
    .registerItemAndBlockDefinitions          = 0,
    .screenControllerRegisterButtonEventHandler = 0,
    .screenControllerBindString               = 0,
    .screenControllerBindStringForAnyCollection = 0,
    .clientInstanceLocalPlayerSlot            = 32,
    .clientInstanceIsShowingMenuSlot          = 151,
    .levelGetBlockDefinitionGroupSlot         = 28,
    .sceneFactoryCreateUIScene                = 0,
    .sceneFactoryCreateScreenAbstract         = 0,
    .clientInstanceSceneFactorySlot           = 226,
    .clientInstanceSceneStackSlot             = 236,
    .sceneStackPushScreenSlot                 = 26,
    .sceneStackSchedulePopSlot                = 27,
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
    if (version.find("1.26.60") != std::string::npos) {
        sActiveOffsets = kOffsets_1_26_60_6;
    } else {
        sActiveOffsets = kOffsets_1_26_52_3;
    }
    ll::Logger::getInstance().info("LeviLamroid bound verified offsets & dynamic resolver for: ", sDetectedVersion);
}
void autoDetectVersion() {
    // 0. Instant Fast-path: Check liblevilamroid.so's own load path from active launcher profile
    Dl_info selfInfo;
    if (dladdr(reinterpret_cast<void*>(&autoDetectVersion), &selfInfo) && selfInfo.dli_fname) {
        std::string_view p(selfInfo.dli_fname);
        if (p.find("1.26.60") != std::string_view::npos) {
            setTargetVersion("1.26.60.6");
            return;
        } else if (p.find("1.26.45") != std::string_view::npos) {
            setTargetVersion("1.26.52.3");
            return;
        } else if (p.find("1.26.44") != std::string_view::npos) {
            setTargetVersion("1.26.44.3");
            return;
        } else if (p.find("1.26.51") != std::string_view::npos) {
            setTargetVersion("1.26.51.1");
            return;
        } else if (p.find("1.26.52") != std::string_view::npos) {
            setTargetVersion("1.26.52.3");
            return;
        }
    }

    struct BuildIdCtx {
        std::string buildId;
        uintptr_t targetBase{0};
    } ctx;
    ctx.targetBase = ll::memory::getMinecraftBase();

    dl_iterate_phdr([](struct dl_phdr_info* info, size_t, void* data) -> int {
        auto* ctxPtr = static_cast<BuildIdCtx*>(data);
        bool isMc = (ctxPtr->targetBase != 0 && info->dlpi_addr == ctxPtr->targetBase) ||
                    (info->dlpi_name && strstr(info->dlpi_name, "libminecraftpe.so"));
        if (!isMc) {
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
                        ctxPtr->buildId = hex;
                        return 1;
                    }
                    notePtr += sizeof(Elf64_Nhdr) + nameLen + descLen;
                    remaining -= sizeof(Elf64_Nhdr) + nameLen + descLen;
                }
            }
        }
        return 0;
    }, &ctx);

    if (!ctx.buildId.empty()) {
        const auto& buildId = ctx.buildId;
        ll::Logger::getInstance().info("Discovered libminecraftpe.so GNU Build ID: ", buildId);
        if (buildId.starts_with("eb291b88")) {
            setTargetVersion("1.26.60.6");
            return;
        } else if (buildId.starts_with("b480c79a")) {
            setTargetVersion("1.26.44.3");
            return;
        } else if (buildId.starts_with("56de9eed")) {
            setTargetVersion("1.26.52.3");
            return;
        } else if (buildId.starts_with("868e275c") || buildId.starts_with("86895c02")) {
            setTargetVersion("1.26.52.3");
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
                if (strstr(line, "1.26.60")) {
                    fclose(fp);
                    setTargetVersion("1.26.60.6");
                    return;
                } else if (strstr(line, "1.26.44")) {
                    fclose(fp);
                    setTargetVersion("1.26.44.3");
                    return;
                } else if (strstr(line, "1.26.45")) {
                    fclose(fp);
                    setTargetVersion("1.26.52.3");
                    return;
                } else if (strstr(line, "1.26.51")) {
                    fclose(fp);
                    setTargetVersion("1.26.51.1");
                    return;
                } else if (strstr(line, "1.26.52")) {
                    fclose(fp);
                    setTargetVersion("1.26.52.3");
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
        if (cmd.find("1.26.60") != std::string::npos) {
            setTargetVersion("1.26.60.6");
            return;
        } else if (cmd.find("1.26.44") != std::string::npos) {
            setTargetVersion("1.26.44.3");
            return;
        } else if (cmd.find("1.26.45") != std::string::npos) {
            setTargetVersion("1.26.52.3");
            return;
        } else if (cmd.find("1.26.51") != std::string::npos) {
            setTargetVersion("1.26.51.1");
            return;
        } else if (cmd.find("1.26.52") != std::string::npos) {
            setTargetVersion("1.26.52.3");
            return;
        }
    }

    // Default to most widely deployed LeviLauncher release (1.26.45.1)
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
    if (name == "clientInstanceGetLocalPlayer" || name == "ClientInstance::getLocalPlayer") return off.clientInstanceGetLocalPlayer;
    if (name == "clientInstanceUpdate" || name == "ClientInstance::update") return off.clientInstanceUpdate;
    if (name == "localPlayerMove" || name == "LocalPlayer::move" || name == "LocalPlayerMove") return off.localPlayerMove;
    if (name == "playerNormalTick" || name == "Player::normalTick" || name == "PlayerNormalTick") return off.playerNormalTick;
    if (name == "levelTick" || name == "Level::tick" || name == "LevelTick") return off.levelTick;
    if (name == "gameModeDestroyBlock" || name == "GameMode::destroyBlock") return off.gameModeDestroyBlock;
    if (name == "gameModeUseItem" || name == "GameMode::useItem") return off.gameModeUseItem;
    if (name == "gameModeUseItemOn" || name == "GameMode::useItemOn") return off.gameModeUseItemOn;
    if (name == "gameModeAttack" || name == "GameMode::attack") return off.gameModeAttack;
    if (name == "gameModeBuildBlock" || name == "GameMode::buildBlock") return off.gameModeBuildBlock;
    if (name == "gameModeStartBuildBlock" || name == "GameMode::startBuildBlock") return off.gameModeStartBuildBlock;

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
    if (name == "resourcePackManagerDoStackOp" || name == "ResourcePackManager::_doStackOperation") return off.resourcePackManagerDoStackOp;
    if (name == "resourcePackManagerComposeFullStack" || name == "ResourcePackManager::composeFullStack") return off.resourcePackManagerComposeFullStack;
    if (name == "resourcePackManager_composeFullStack" || name == "ResourcePackManager::_composeFullStack") return off.resourcePackManager_composeFullStack;
    if (name == "resourcePackManagerSetStack" || name == "ResourcePackManager::setStack") return off.resourcePackManagerSetStack;
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
    if (name == "dimensionGetBlockSource" || name == "Dimension::getBlockSource") return off.dimensionGetBlockSource;
    if (name == "blockSourceGetBlock" || name == "BlockSource::getBlock") return off.blockSourceGetBlock;

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
