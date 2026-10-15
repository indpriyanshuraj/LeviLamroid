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
    .localPlayerMove          = 0x04C05BE0,
    .playerNormalTick         = 0x056637B8,
    .levelTick                = 0x05465C54,
    .gameModeDestroyBlock     = 0x04C0A7EC,
    .gameModeUseItem          = 0x04C0B120,
    .gameModeUseItemOn        = 0x04C0B530,
    .gameModeAttack           = 0x04C0BE40,
    .soundEnginePlay          = 0x0B79F7F4,
    .soundEnginePlayUI        = 0x0B79F978,
    .soundEnginePlayAt        = 0x0B79FDE0,
    .soundEngineStop          = 0x0B79FF1C,

    // Resource Pack & Asset Access Strategy RVAs (Audited 1.26.52.3)
    .directoryPackGetAsset    = 0x111EE8A8,
    .zipPackGetAsset          = 0x11157F00,
    .directoryPackHasAsset    = 0x111F888C,
    .zipPackHasAsset          = 0x11158654,
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
    .commandRegistryRegisterCommand      = 0x0DC4A4BC,
    .loopbackPacketSenderSend            = 0x0C68FAD4,
    .serverNetworkHandlerHandle          = 0x0C58394C,

    // Behavior, Recipes, Items, Blocks & Scripting RVAs
    .recipesAddShapedRecipe              = 0x0FC3D3D8,
    .recipesAddShapelessRecipe            = 0x0FC3E1D4,
    .itemRegistryTryRegisterAndInitItem  = 0x0FBB3014,
    .itemRegistryParseItemIdentifier     = 0x0FBB5A54,
    .blockTypeRegistryInit               = 0x0AD6D44C,
    .blockDefinitionLoaderProcessCustomComponents = 0x10491FF8,
    .actorNormalTick                     = 0x0F27DA94,
    .actorBaseTick                       = 0x0F560820,
    .actorHurt                           = 0x0F5609A8,
    .actorDie                            = 0x0F564E7C,
    .actorTeleportTo                     = 0x0F5746BC,
    .actorHasComponent                   = 0x0F583390,
    .actorSetCustomName                  = 0x0F57E694,
    .scriptingQuickJS                    = 0x11432150,
    .scriptModuleMinecraftScriptSystem   = 0x0E069FE8,

    // VTable Slots
    .clientInstanceLocalPlayerSlot = 32,
    .clientInstanceIsShowingMenuSlot = 151
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
    .panoramaRender           = 0x0967FA98,
    .cubemapBackgroundRender  = 0x097C99A4,
    .cubemapFaceSubmit        = 0, // In-world skybox sharing; do not suppress
    .cubemapRendererRender    = 0,
    .mainMenuCubemapPass      = 0,
    .cubemapDraw              = 0,
    .glClearGotRva            = 0x124C9688,
    .uiDrawImage              = 0x0AD845F0,
    .uiFlushImages            = 0x0AD859C4,
    .uiFillRectangle          = 0x0AD85F80,
    .uiDrawRectangle          = 0x0AD87350,
    .uiDrawText               = 0x0AD84430,
    .uiFlushText              = 0x0AD84530,
    .uiDrawNineslice          = 0x0AD855E0,
    .uiGetScreenContext       = 0x0AD87E3C,
    .localPlayerMove          = 0x04C05BE0,
    .playerNormalTick         = 0x056637B8,
    .levelTick                = 0x05465C54,
    .gameModeDestroyBlock     = 0x04C0A7EC,
    .gameModeUseItem          = 0x04C0B120,
    .gameModeUseItemOn        = 0x04C0B530,
    .gameModeAttack           = 0x04C0BE40,
    .soundEnginePlay          = 0x0B2848E0,
    .soundEnginePlayUI        = 0x0B284A5C,
    .soundEnginePlayAt        = 0x0B284EC4,
    .soundEngineStop          = 0x0B285000,
    .directoryPackGetAsset    = 0x1042CB64,
    .zipPackGetAsset          = 0x1042D28C,
    .directoryPackHasAsset    = 0x104417EC,
    .zipPackHasAsset          = 0x10432C5C,
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
    .screenControllerTick                = 0x096D73DC,
    .screenControllerHandleEvent         = 0x096D6EA8,
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
    .loopbackPacketSenderSend            = 0x0C2E26FC,
    .serverNetworkHandlerHandle          = 0x0C1E5B4C,

    // Behavior, Recipes, Items, Blocks & Scripting RVAs
    .recipesAddShapedRecipe              = 0x0F2FC108,
    .recipesAddShapelessRecipe            = 0x0F2FCFD4,
    .itemRegistryTryRegisterAndInitItem  = 0x0F2AFB68,
    .itemRegistryParseItemIdentifier     = 0x0F2B2580,
    .blockTypeRegistryInit               = 0x0954D9A4,
    .blockDefinitionLoaderProcessCustomComponents = 0x0FB26F04,
    .actorNormalTick                     = 0x0EA412D4,
    .actorBaseTick                       = 0x0EC84620,
    .actorHurt                           = 0x0EC847A8,
    .actorDie                            = 0x0EC88C50,
    .actorTeleportTo                     = 0x0EC97F6C,
    .actorHasComponent                   = 0x0D6C1B88,
    .actorSetCustomName                  = 0x0ECA1C24,
    .scriptingQuickJS                    = 0x10917BD4,
    .scriptModuleMinecraftScriptSystem   = 0x0DA54B3C,

    // VTable Slots
    .clientInstanceLocalPlayerSlot = 32,
    .clientInstanceIsShowingMenuSlot = 150 // Slot 150 in 1.26.4x
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
    if (version.find("1.26.45") != std::string::npos) {
        sActiveOffsets = kOffsets_1_26_45_1;
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
        if (buildId.starts_with("56de9eed")) {
            setTargetVersion("1.26.52.3");
            return;
        } else if (buildId.starts_with("868e275c")) {
            setTargetVersion("1.26.45.1");
            return;
        }
    }

    // 2. Check memory maps for profile path indications
    FILE* fp = fopen("/proc/self/maps", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "libminecraftpe.so")) {
                if (strstr(line, "1.26.45")) {
                    fclose(fp);
                    setTargetVersion("1.26.45.1");
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
        if (cmd.find("1.26.45") != std::string::npos) {
            setTargetVersion("1.26.45.1");
            return;
        }
    }

    // Default to primary active version
    setTargetVersion("1.26.52.3");
}

uintptr_t findFunctionRva(std::string_view name) {
    const auto& off = getOffsets();
    if (name == "getTechStackForScreen") return off.getTechStackForScreen;
    if (name == "startMenuOnOpen") return off.startMenuOnOpen;
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
    if (name == "localPlayerMove") return off.localPlayerMove;
    if (name == "playerNormalTick") return off.playerNormalTick;
    if (name == "directoryPackGetAsset") return off.directoryPackGetAsset;
    if (name == "zipPackGetAsset") return off.zipPackGetAsset;
    if (name == "directoryPackHasAsset") return off.directoryPackHasAsset;
    if (name == "zipPackHasAsset") return off.zipPackHasAsset;
    if (name == "packForEachInAssetSet") return off.packForEachInAssetSet;
    if (name == "resourcePackManagerLoad") return off.resourcePackManagerLoad;
    if (name == "resourcePackManagerGetStack") return off.resourcePackManagerGetStack;
    if (name == "resourcePackManagerDoStackOp") return off.resourcePackManagerDoStackOp;
    if (name == "resourcePackGetResource") return off.resourcePackGetResource;
    if (name == "textureGroupLoadAsync") return off.textureGroupLoadAsync;
    if (name == "textureGroupEnableLRU") return off.textureGroupEnableLRU;
    if (name == "textureHotReloaderRegister") return off.textureHotReloaderRegister;
    if (name == "textureHotReloaderRegisterAtlas") return off.textureHotReloaderRegisterAtlas;
    if (name == "blockGraphicsGetTextureItem") return off.blockGraphicsGetTextureItem;
    if (name == "cubemapLoadPanoramas") return off.cubemapLoadPanoramas;
    if (name == "resourcePackRepositoryGetResourcePackInPath") return off.resourcePackRepositoryGetResourcePackInPath;
    if (name == "repositoryLoadingFindVanillaPacks") return off.repositoryLoadingFindVanillaPacks;

    // OreUI
    if (name == "cohtmlLoadURL") return off.cohtmlLoadURL;
    if (name == "cohtmlLoadURLImpl") return off.cohtmlLoadURLImpl;
    if (name == "cohtmlExecuteScript") return off.cohtmlExecuteScript;
    if (name == "cohtmlAddInitialScript") return off.cohtmlAddInitialScript;
    if (name == "cohtmlTriggerEvent") return off.cohtmlTriggerEvent;
    if (name == "cohtmlRegisterForEvent") return off.cohtmlRegisterForEvent;
    if (name == "cohtmlUnregisterFromEvent") return off.cohtmlUnregisterFromEvent;
    if (name == "cohtmlTryDispatchClickEvent") return off.cohtmlTryDispatchClickEvent;
    if (name == "cohtmlMouseEvent") return off.cohtmlMouseEvent;
    if (name == "cohtmlTouchEvent") return off.cohtmlTouchEvent;
    if (name == "oreUiInit") return off.oreUiInit;

    // JSON UI & Screen Controller
    if (name == "screenControllerTick") return off.screenControllerTick;
    if (name == "screenControllerHandleEvent") return off.screenControllerHandleEvent;
    if (name == "screenControllerBind") return off.screenControllerBind;
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
    if (name == "guiDataDisplayClientMessage") return off.guiDataDisplayClientMessage;

    // Chat, Command & Packet System
    if (name == "chatScreenControllerHandleEvent") return off.chatScreenControllerHandleEvent;
    if (name == "chatScreenControllerPropertyBind") return off.chatScreenControllerPropertyBind;
    if (name == "commandRegistryRegisterCommand") return off.commandRegistryRegisterCommand;
    if (name == "loopbackPacketSenderSend") return off.loopbackPacketSenderSend;
    if (name == "serverNetworkHandlerHandle") return off.serverNetworkHandlerHandle;

    // Behavior, Recipes, Items, Blocks & Scripting
    if (name == "recipesAddShapedRecipe") return off.recipesAddShapedRecipe;
    if (name == "recipesAddShapelessRecipe") return off.recipesAddShapelessRecipe;
    if (name == "itemRegistryTryRegisterAndInitItem") return off.itemRegistryTryRegisterAndInitItem;
    if (name == "itemRegistryParseItemIdentifier") return off.itemRegistryParseItemIdentifier;
    if (name == "blockTypeRegistryInit") return off.blockTypeRegistryInit;
    if (name == "blockDefinitionLoaderProcessCustomComponents") return off.blockDefinitionLoaderProcessCustomComponents;
    if (name == "actorNormalTick") return off.actorNormalTick;
    if (name == "actorBaseTick") return off.actorBaseTick;
    if (name == "actorHurt") return off.actorHurt;
    if (name == "actorDie") return off.actorDie;
    if (name == "actorTeleportTo") return off.actorTeleportTo;
    if (name == "actorHasComponent") return off.actorHasComponent;
    if (name == "actorSetCustomName") return off.actorSetCustomName;
    if (name == "scriptingQuickJS") return off.scriptingQuickJS;
    if (name == "scriptModuleMinecraftScriptSystem") return off.scriptModuleMinecraftScriptSystem;
    return 0;
}

} // namespace levilamroid::mcpe
