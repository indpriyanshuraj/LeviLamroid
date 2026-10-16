#pragma once
#include "ll/api/Macros.h"
#include <cstdint>
#include <string>
#include <string_view>

namespace levilamroid::mcpe {

struct EngineOffsets {
    // UI Screen Tech Stack
    uintptr_t getTechStackForScreen;

    // Start Menu Screen Controller
    uintptr_t startMenuOnOpen;
    uintptr_t startMenuTick;

    // Direct Menu Button Handlers
    uintptr_t playButton;
    uintptr_t settingsButton;
    uintptr_t realmsButton;
    uintptr_t dressingRoomButton;
    uintptr_t marketplaceButton;
    uintptr_t menuExit;

    // RenderDragon & Skybox Passes
    uintptr_t progressBarRender;
    uintptr_t panoramaRender;
    uintptr_t cubemapBackgroundRender;
    uintptr_t cubemapFaceSubmit;
    uintptr_t cubemapRendererRender;
    uintptr_t mainMenuCubemapPass;
    uintptr_t cubemapDraw;

    // GPU / libGLESv2 Hooks
    uintptr_t glClearGotRva;

    // MinecraftUIRenderContext Virtual Method RVAs
    uintptr_t uiDrawImage;
    uintptr_t uiFlushImages;
    uintptr_t uiFillRectangle;
    uintptr_t uiDrawRectangle;
    uintptr_t uiDrawText;
    uintptr_t uiFlushText;
    uintptr_t uiDrawNineslice;
    uintptr_t uiGetScreenContext;

    // Player / GameMode / World RVAs
    uintptr_t localPlayerMove;
    uintptr_t playerNormalTick;
    uintptr_t levelTick;
    uintptr_t gameModeDestroyBlock;
    uintptr_t gameModeUseItem;
    uintptr_t gameModeUseItemOn;
    uintptr_t gameModeAttack;

    // Audio / SoundEngine RVAs (Method B & Sound Physics)
    uintptr_t soundEnginePlay;
    uintptr_t soundEnginePlayUI;
    uintptr_t soundEnginePlayAt;
    uintptr_t soundEngineStop;

    // Resource Pack & Asset Access Strategy RVAs (Audited 1.26.52.3)
    uintptr_t directoryPackGetAsset;
    uintptr_t zipPackGetAsset;
    uintptr_t directoryPackHasAsset;
    uintptr_t zipPackHasAsset;
    uintptr_t packForEachInAssetSet;
    uintptr_t resourcePackManagerLoad;
    uintptr_t resourcePackManagerGetStack;
    uintptr_t resourcePackManagerDoStackOp;
    uintptr_t resourcePackGetResource;
    uintptr_t textureGroupLoadAsync;
    uintptr_t textureGroupEnableLRU;
    uintptr_t textureHotReloaderRegister;
    uintptr_t textureHotReloaderRegisterAtlas;
    uintptr_t blockGraphicsGetTextureItem;
    uintptr_t cubemapLoadPanoramas;
    uintptr_t resourcePackRepositoryGetResourcePackInPath;
    uintptr_t repositoryLoadingFindVanillaPacks;

    // OreUI (Cohtml Engine) RVAs
    uintptr_t cohtmlLoadURL;
    uintptr_t cohtmlLoadURLImpl;
    uintptr_t cohtmlExecuteScript;
    uintptr_t cohtmlAddInitialScript;
    uintptr_t cohtmlTriggerEvent;
    uintptr_t cohtmlRegisterForEvent;
    uintptr_t cohtmlUnregisterFromEvent;
    uintptr_t cohtmlTryDispatchClickEvent;
    uintptr_t cohtmlMouseEvent;
    uintptr_t cohtmlTouchEvent;
    uintptr_t oreUiInit;

    // JSON UI & Screen Controller RVAs
    uintptr_t screenControllerTick;
    uintptr_t screenControllerHandleEvent;
    uintptr_t screenControllerBind;
    uintptr_t screenControllerAddStaticScreenVars;
    uintptr_t screenControllerSetScreenState;
    uintptr_t screenControllerCanExit;
    uintptr_t screenControllerGetScreenModel;
    uintptr_t sceneFactoryCreateScreenController;
    uintptr_t sceneFactoryCreateScreen;
    uintptr_t buttonComponentHandleEvent;
    uintptr_t buttonComponentSetPressed;
    uintptr_t buttonComponentGetButtonState;
    uintptr_t dataDrivenUILayout;
    uintptr_t dataDrivenUIUIRoot;
    uintptr_t guiDataDisplayClientMessage;

    // Chat, Command & Packet System RVAs
    uintptr_t chatScreenControllerHandleEvent;
    uintptr_t chatScreenControllerPropertyBind;
    uintptr_t commandRegistryRegisterCommand;
    uintptr_t loopbackPacketSenderSend;
    uintptr_t serverNetworkHandlerHandle;

    // Behavior, Recipes, Items, Blocks & Scripting RVAs
    uintptr_t recipesAddShapedRecipe;
    uintptr_t recipesAddShapelessRecipe;
    uintptr_t itemRegistryTryRegisterAndInitItem;
    uintptr_t itemRegistryParseItemIdentifier;
    uintptr_t blockTypeRegistryInit;
    uintptr_t blockDefinitionLoaderProcessCustomComponents;
    uintptr_t actorNormalTick;
    uintptr_t actorBaseTick;
    uintptr_t actorHurt;
    uintptr_t actorDie;
    uintptr_t actorTeleportTo;
    uintptr_t actorHasComponent;
    uintptr_t actorSetCustomName;
    uintptr_t scriptingQuickJS;
    uintptr_t scriptModuleMinecraftScriptSystem;

    // VTable Slot Indices
    size_t clientInstanceLocalPlayerSlot; // Slot 32
    size_t clientInstanceIsShowingMenuSlot; // Slot 151 (1.26.5x+) / 150 (1.26.4x)
};

/// Verified ARM64 Offhand Item Slot Accessor Pattern (Sig123)
inline constexpr const char* kSig123Pattern = 
    "? ? ? A9 FD 03 00 91 ? ? ? 91 ? ? ? 94 ? ? ? F9 ? ? ? F9 21 00 80 52 ? ? ? A8 40 00 1F D6 28 1C 00 72";

/// Get the active offsets struct bound to the detected Minecraft version
LL_API const EngineOffsets& getOffsets();

/// Get the detected version string (e.g. "1.26.52.3", "1.26.45.1")
LL_API std::string getDetectedVersion();

/// Manually bind a version profile
LL_API void setTargetVersion(const std::string& version);

/// Automatically probe libminecraftpe.so memory or process info to detect version
LL_API void autoDetectVersion();

/// Lookup RVA or symbol by name across the active version dictionary
LL_API uintptr_t findFunctionRva(std::string_view name);

} // namespace levilamroid::mcpe

extern "C" {
LL_API const char* ll_mcpe_get_detected_version();
}

