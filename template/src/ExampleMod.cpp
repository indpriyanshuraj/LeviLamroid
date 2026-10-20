#include "ExampleMod.h"

#include "levilamroid/LeviLamroid.h"
#include "pl/Input.hpp"
#include "pl/ModMenu.hpp"
#include "nlohmann/json.hpp"

#include <fstream>
#include <filesystem>

namespace example {

ExampleMod::ExampleMod() : ll::mod::NativeMod("ExampleMod") {}

ExampleMod& ExampleMod::getInstance() {
    static ExampleMod instance;
    return instance;
}

bool ExampleMod::load() {
    getLogger().info("=========================================");
    getLogger().info(" ExampleMod loaded on LeviLamroid!");
    getLogger().info(" Canonical Namespace: ll");
    getLogger().info(" Detected Bedrock Version: {}", ll::mcpe::getDetectedVersion());
    getLogger().info("=========================================");

    initConfig();
    return true;
}

bool ExampleMod::enable() {
    if (mEnabled) return true;
    mEnabled = true;

    getLogger().info("Enabling ExampleMod features...");

    // 1. Register LeviLauncher On-Screen Floating HUD Button
    registerHudButton();

    // 2. Register LeviLauncher Mod Menu Module (top drawer)
    try {
        pl::modmenu::ModuleInfo modInfo;
        modInfo.moduleId = "ExampleMod";
        modInfo.displayName = "Example Mod";
        modInfo.description = "Default template mod for LeviLamroid on Android ARM64";
        modInfo.modId = "ExampleMod";
        modInfo.defaultEnabled = true;
        modInfo.onToggle = [this](std::string_view, bool enabled) {
            getLogger().info("ExampleMod toggled via ModMenu: {}", enabled);
            mCustomFeatureActive = enabled;
        };
        pl::modmenu::registerModule(modInfo);
        getLogger().info("Registered LeviLauncher ModMenu module");
    } catch (...) {
        getLogger().warn("Failed to register ModMenu module");
    }

    // 3. Register PreLoader Input Listeners (Keyboard & Touch)
    registerInputListeners();

    // 4. Register EventBus Listeners
    registerEventListeners();

    getLogger().info("ExampleMod successfully enabled!");
    return true;
}

bool ExampleMod::disable() {
    if (!mEnabled) return true;
    mEnabled = false;

    getLogger().info("Disabling ExampleMod...");
    try {
        pl::modmenu::unregisterButton("example_hud_btn");
        pl::modmenu::unregisterModule("ExampleMod");
    } catch (...) {}

    return true;
}

void ExampleMod::initConfig() {
    try {
        auto configDir = getConfigDir();
        std::error_code ec;
        std::filesystem::create_directories(configDir, ec);

        auto configFile = configDir / "config.json";
        if (std::filesystem::exists(configFile, ec)) {
            std::ifstream file(configFile);
            if (file.is_open()) {
                nlohmann::json j;
                file >> j;
                mCustomFeatureActive = j.value("featureActive", true);
                getLogger().info("Loaded config from: {}", configFile.string());
            }
        } else {
            nlohmann::json j;
            j["version"] = "1.0.0";
            j["featureActive"] = true;
            std::ofstream file(configFile);
            if (file.is_open()) {
                file << j.dump(4);
                getLogger().info("Created default config at: {}", configFile.string());
            }
        }
    } catch (const std::exception& e) {
        getLogger().warn("Config initialization exception: {}", e.what());
    }
}

void ExampleMod::registerHudButton() {
    try {
        pl::modmenu::ButtonInfo btn;
        btn.buttonId = "example_hud_btn";
        btn.moduleId = "ExampleMod";
        btn.displayName = "Example Mod HUD";
        btn.modId = "ExampleMod";
        btn.label = "Example";
        btn.androidKeyCode = 0;
        btn.behavior = pl::modmenu::ButtonBehavior::Click;
        btn.defaultVisible = true;
        btn.stylePreset = pl::modmenu::ButtonStylePreset::Keycap;
        btn.normalBgColor = 0xAA2563EB; // Royal Blue ARGB
        btn.activeBgColor = 0xDD38BDF8; // Glow Sky Blue
        btn.borderColor = 0xFF38BDF8;
        btn.textColor = 0xFFFFFFFF;
        btn.activeTextColor = 0xFF000000;
        btn.widthScale = 1.25f;
        btn.heightScale = 1.25f;
        btn.iconFormat = pl::modmenu::ButtonIconFormat::Auto;
        btn.hideLabelWhenIconPresent = false;
        btn.onEvent = [this](std::string_view, pl::modmenu::ButtonEvent event, float) {
            if (event == pl::modmenu::ButtonEvent::Click) {
                mCustomFeatureActive = !mCustomFeatureActive;
                getLogger().info("HUD Button Clicked! Feature active: {}", mCustomFeatureActive);
            }
        };

        pl::modmenu::registerButton(btn);
        getLogger().info("Registered on-screen HUD button: example_hud_btn");
    } catch (...) {
        getLogger().warn("Failed to register on-screen HUD button");
    }
}

void ExampleMod::registerInputListeners() {
    // Intercept physical keyboard hotkeys (e.g. 'O' key toggles feature)
    try {
        pl::input::registerKeyCallback([this](const pl::input::KeyEvent& ev) -> bool {
            // Keycode 43 = 'O' key on standard Android KeyCharacterMap
            if (ev.isKeyDown && (ev.keyCode == 43 || ev.keyCode == 'o' || ev.keyCode == 'O')) {
                mCustomFeatureActive = !mCustomFeatureActive;
                getLogger().info("Physical hotkey pressed! Feature active: {}", mCustomFeatureActive);
                return true; // Consume event
            }
            return false;
        });
        getLogger().info("Registered physical keyboard hotkey listener");
    } catch (...) {
        getLogger().warn("Failed to register key callback");
    }

    // Touch event listener
    try {
        pl::input::registerTouchCallback([](const pl::input::TouchEvent& /*ev*/) -> bool {
            // Return true if you want to consume touch so Minecraft camera doesn't rotate
            return false;
        });
    } catch (...) {}
}

void ExampleMod::registerEventListeners() {
    // 1. Listen for Start Menu Open via ll event bus
    ll::event::EventBus::getInstance().subscribe<ll::event::StartMenuOpenEvent>([](auto&) {
        ll::Logger::getInstance("ExampleMod").info("StartMenu opened event intercepted by ExampleMod!");
    });

    // 2. 3D In-Game Level Render Pass (ESP, Tracers, 3D Markers, Block Geometry)
    // Hooked to InGamePlayScreen::_renderLevel (0x09ACD4C8)
    ll::event::EventBus::getInstance().subscribe<ll::event::RenderLevelEvent>([this](ll::event::RenderLevelEvent& ev) {
        if (!mCustomFeatureActive || !mEspEnabled) return;
        mFrameCounter++;
        // Every 300 frames (~5 seconds at 60 FPS), log render heartbeat
        if (mFrameCounter % 300 == 0) {
            getLogger().info("[ESP 3D] RenderLevel pass active: Frame={}, ScreenContext={:p}, FrameObj={:p}",
                             mFrameCounter, ev.getScreenContext(), ev.getFrameRenderObject());
        }
        // In-world 3D drawing: mods can project entity world coordinates into screen space
        // and draw bounding boxes, wireframes, or distance markers here.
    });

    // 3. 2D In-Game HUD & UI Render Pass (Armor Status, Keystrokes, Coords Overlay, Mini-Map)
    // Hooked to InGamePlayScreen::render (0x09ACC828) & flushImages (0x0B251D70)
    ll::event::EventBus::getInstance().subscribe<ll::event::UIRenderEvent>([this]([[maybe_unused]] ll::event::UIRenderEvent& ev) {
        if (!mCustomFeatureActive || !mHudEnabled) return;
        // In-game 2D HUD overlay drawing: mods render custom status widgets, text overlays,
        // and crosshairs using MinecraftUIRenderContext.
    });

    // 4. Render Frame Preparation Pass (Matrix Transforms, Camera Updates)
    // Hooked to GameRenderer::_prepareFrame (0x0B2EB50C)
    ll::event::EventBus::getInstance().subscribe<ll::event::RenderFramePrepareEvent>([this]([[maybe_unused]] ll::event::RenderFramePrepareEvent& ev) {
        if (!mCustomFeatureActive) return;
        // Matrix calculations, viewport setup, and camera frustum updates
    });

    // 5. GPU Render Clear Event
    ll::event::EventBus::getInstance().subscribe<ll::event::GlClearEvent>([](auto&) {
        // Can be used to hook OpenGL ES 3.0 render passes before UI is drawn
    });
}

} // namespace example

// Export all required loader ABIs (LeviLauncher PreLoader + LeviLamroid + LeviLamina)
LL_REGISTER_MOD(example::ExampleMod, example::ExampleMod::getInstance());
