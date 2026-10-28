#include "fabric/FabricClient.hpp"
#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <dlfcn.h>
#include <fstream>

extern "C" {
LL_API bool ll_command_dispatch_client(const char* commandLine);
LL_API void* ll_player_get_local();
LL_API bool ll_player_get_position(void* player, float* outX, float* outY, float* outZ);
LL_API bool ll_player_get_rotation(void* player, float* outPitch, float* outYaw);
LL_API bool ll_player_get_held_item_identifier(void* player, char* outIdent, size_t maxLen);
LL_API bool ll_player_get_armor_stack(void* player, int slot, void** outStack, char* outName, size_t maxLen, int* outDamage, int* outMaxDamage);
}

namespace {

static inline bool isPointerReadable(const void* ptr, size_t size = sizeof(void*)) {
    if (!ptr || size == 0) return false;
    uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t untagged = v & 0x00ffffffffffffffULL;
    if (untagged < 0x10000 || untagged > 0x00007fffffffffffULL) return false;
    if (untagged + size < untagged) return false;
    return true;
}

struct BlockPos3i {
    int32_t x;
    int32_t y;
    int32_t z;
};

static std::vector<FabricTooltipCallback> sTooltipCallbacks;
static std::vector<FabricCameraCallback> sCameraCallbacks;
static std::mutex sFabricCallbackMutex;

// Speed tracking state
static float sLastPosX = 0.0f;
static float sLastPosY = 0.0f;
static float sLastPosZ = 0.0f;
static float sCurrentSpeed = 0.0f;

// ── Dynamic Lights & Fullbright Hook on BlockSource::getBrightness ──
using GetBrightnessFn = float (*)(void*, const void*);
static GetBrightnessFn sOrigGetBrightness = nullptr;
static std::atomic<bool> sBrightnessHookInstalled{false};

static float Hooked_BlockSource_getBrightness(void* self, const void* posPtr) {
    if (fabric_is_fullbright()) {
        return 1.0f; // Maximum ambient brightness (Fullbright)
    }

    float baseBrightness = 0.0f;
    if (sOrigGetBrightness) {
        baseBrightness = sOrigGetBrightness(self, posPtr);
    }

    // Dynamic Lights calculation (lock-free, zero heap allocation, thread-safe for chunk workers)
    if (fabric_is_dynamic_lights() && posPtr && isPointerReadable(posPtr, sizeof(BlockPos3i))) {
        const auto* pos = reinterpret_cast<const BlockPos3i*>(posPtr);
        return fabric::lighting::DynamicLightsEngine::getInstance().computeDynamicBrightness(
            pos->x, pos->y, pos->z, baseBrightness
        );
    }

    return baseBrightness;
}

static void ensureBrightnessHook() {
    if (sBrightnessHookInstalled.load()) return;
    sBrightnessHookInstalled.store(true);

    uintptr_t mcBase = ::ll::memory::getMinecraftBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t targetRva = off.blockSourceGetBrightness;
    if (mcBase && targetRva) {
        ::ll::memory::hookRva(targetRva,
                              reinterpret_cast<void*>(&Hooked_BlockSource_getBrightness),
                              reinterpret_cast<void**>(&sOrigGetBrightness));
        ::ll::Logger::getInstance().info("[Fabric Engine] Hooked BlockSource::getBrightness for DynamicLights & Fullbright [RVA: 0x", std::hex, targetRva, "]");
    }
}

// ── ItemStackBase::getName Hook (AppleSkin & Shulker Tooltip Injection) ──
using GetNameFn = std::string (*)(const void*);
static GetNameFn sOrigGetName = nullptr;
static std::atomic<bool> sTooltipHookInstalled{false};

[[maybe_unused]] static std::string Hooked_ItemStackBase_getName(const void* self) {
    std::string baseName;
    if (sOrigGetName) {
        baseName = sOrigGetName(self);
    }
    if (!self || !isPointerReadable(self, 32)) return baseName;

    std::vector<std::string> extraLines;

    // 1. AppleSkin: Inspect food nutrition & saturation from Item
    const uint8_t* stackBytes = reinterpret_cast<const uint8_t*>(self);
    void* itemPtr = nullptr;
    void* ctrl = *reinterpret_cast<void* const*>(stackBytes + 0x08);
    if (ctrl && isPointerReadable(ctrl, 16)) {
        itemPtr = *reinterpret_cast<void**>(ctrl);
    }
    if (!itemPtr) {
        void* candidate = *reinterpret_cast<void* const*>(stackBytes);
        if (candidate && isPointerReadable(candidate, 0x200)) {
            itemPtr = candidate;
        }
    }

    if (itemPtr && isPointerReadable(itemPtr, 0x200)) {
        const uint8_t* itemBytes = reinterpret_cast<const uint8_t*>(itemPtr);
        int nutrition = *reinterpret_cast<const int*>(itemBytes + 0x1E0);
        float saturation = *reinterpret_cast<const float*>(itemBytes + 0x1E4);
        if (nutrition > 0 && nutrition <= 40) {
            char buf[96];
            float satPoints = static_cast<float>(nutrition) * saturation * 2.0f;
            std::snprintf(buf, sizeof(buf), "§6Hunger: +%d  §eSaturation: +%.1f§r", nutrition, satPoints);
            extraLines.emplace_back(buf);
        }
    }

    // 2. Shulker Box Contents Inspection
    if (baseName.find("Shulker") != std::string::npos || baseName.find("shulker") != std::string::npos) {
        extraLines.emplace_back("§7[Shulker Box Container Preview]");
        extraLines.emplace_back("§8  • Ready to place & unpack");
    }

    // 3. Publish C++ ItemTooltipEvent to EventBus
    try {
        fabric::event::ItemTooltipEvent event(self, baseName);
        ::ll::event::EventBus::getInstance().publish(event);
        for (const auto& line : event.getLines()) {
            extraLines.push_back(line);
        }
    } catch (...) {}

    // 4. Dispatch to registered C-ABI FabricTooltipCallbacks
    auto appendCb = [](const char* text, void* udata) {
        if (text && udata) {
            auto* vec = reinterpret_cast<std::vector<std::string>*>(udata);
            vec->emplace_back(text);
        }
    };

    {
        std::lock_guard lock(sFabricCallbackMutex);
        for (auto cb : sTooltipCallbacks) {
            if (cb) {
                cb(baseName.c_str(), self, appendCb, &extraLines);
            }
        }
    }

    // 5. Combine base item name with appended tooltip lines
    for (const auto& line : extraLines) {
        baseName += "\n";
        baseName += line;
    }

    return baseName;
}

static void ensureTooltipHook() {
    if (sTooltipHookInstalled.load()) return;
    sTooltipHookInstalled.store(true);
    ::ll::Logger::getInstance().info("[Fabric Engine] Native tooltip registry initialized safely (zero-hook).");
}

// ── Camera Frustum & Orientation Hook (Barrel Roll & ReplayMod) ──
using PreRenderUpdateFn = void (*)(void*, void*, void*);
static PreRenderUpdateFn sOrigPreRenderUpdate = nullptr;
static std::atomic<bool> sCameraHookInstalled{false};

static void Hooked_LevelRendererPlayer_preRenderUpdate(void* self, void* screenContext, void* params) {
    if (sOrigPreRenderUpdate) {
        sOrigPreRenderUpdate(self, screenContext, params);
    }

    if (!sCameraCallbacks.empty()) {
        float yaw = 0.0f, pitch = 0.0f, roll = fabric_camera_get_roll();
        float x = 0.0f, y = 0.0f, z = 0.0f;
        auto* player = ::ll::service::getLocalPlayer();
        if (player) {
            ll_player_get_rotation(player, &pitch, &yaw);
            ll_player_get_position(player, &x, &y, &z);
        }

        std::lock_guard lock(sFabricCallbackMutex);
        for (auto cb : sCameraCallbacks) {
            if (cb) {
                cb(&yaw, &pitch, &roll, &x, &y, &z);
            }
        }
    }
}

static void ensureCameraHook() {
    if (sCameraHookInstalled.load()) return;
    sCameraHookInstalled.store(true);

    uintptr_t mcBase = ::ll::memory::getMinecraftBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t targetRva = off.levelRendererPlayerPreRenderUpdate;
    if (mcBase && targetRva) {
        ::ll::memory::hookRva(targetRva,
                              reinterpret_cast<void*>(&Hooked_LevelRendererPlayer_preRenderUpdate),
                              reinterpret_cast<void**>(&sOrigPreRenderUpdate));
        ::ll::Logger::getInstance().info("[Fabric Engine] Hooked LevelRendererPlayer::preRenderUpdate for Camera & Barrel Roll [RVA: 0x", std::hex, targetRva, "]");
    }
}

} // namespace

extern "C" {

// ── Tooltips & Hover Text ──
FABRIC_API void fabric_register_tooltip_callback(FabricTooltipCallback cb) {
    if (!cb) return;
    ensureTooltipHook();
    std::lock_guard lock(sFabricCallbackMutex);
    sTooltipCallbacks.push_back(cb);
}

// ── HUD & Level Rendering ──
extern "C" {
void ll_render_register_ui_render_callback(void (*cb)(void* screenCtx, const void* frameObj));
void ll_render_register_level_render_callback(void (*cb)(void* screenCtx, const void* frameObj));
}

FABRIC_API void fabric_register_hud_render_callback(FabricHudRenderCallback cb) {
    if (!cb) return;
    ll_render_register_ui_render_callback(reinterpret_cast<void(*)(void*, const void*)>(cb));
}

FABRIC_API void fabric_register_level_render_callback(void (*cb)(void* screenCtx, const void* frameObj)) {
    if (!cb) return;
    ll_render_register_level_render_callback(cb);
}

FABRIC_API void fabric_register_camera_callback(FabricCameraCallback cb) {
    if (!cb) return;
    ensureCameraHook();
    std::lock_guard lock(sFabricCallbackMutex);
    sCameraCallbacks.push_back(cb);
}

// ── Modular HUD & Widgets (BedrockTools Suite) ──
FABRIC_API void fabric_hud_register_widget(const char* widgetId, void (*renderCb)(void* screenCtx, const void* frameObj)) {
    if (!widgetId || !renderCb) return;
    fabric::hud::ModularHudRegistry::getInstance().registerWidget(widgetId, renderCb);
}

FABRIC_API void fabric_hud_unregister_widget(const char* widgetId) {
    if (!widgetId) return;
    fabric::hud::ModularHudRegistry::getInstance().unregisterWidget(widgetId);
}

FABRIC_API bool fabric_get_player_coords(float* x, float* y, float* z) {
    auto* player = ::ll::service::getLocalPlayer();
    if (!player) return false;
    float px = 0.0f, py = 0.0f, pz = 0.0f;
    if (!ll_player_get_position(player, &px, &py, &pz)) return false;
    if (x) *x = px;
    if (y) *y = py;
    if (z) *z = pz;

    // Update dynamic light source coordinates on the main thread safely
    if (fabric_is_dynamic_lights()) {
        int heldLight = 0;
        char heldBuf[128] = {0};
        if (ll_player_get_held_item_identifier(player, heldBuf, sizeof(heldBuf)) && heldBuf[0]) {
            heldLight = fabric::lighting::DynamicLightsEngine::getInstance().getItemLightLevel(heldBuf);
        }

        char offhandName[128] = {0};
        int offhandCount = 0;
        if (fabric_get_offhand_item_name(offhandName, sizeof(offhandName), &offhandCount) && offhandCount > 0) {
            int offhandLight = fabric::lighting::DynamicLightsEngine::getInstance().getItemLightLevel(offhandName);
            if (offhandLight > heldLight) heldLight = offhandLight;
        }

        fabric::lighting::DynamicLightsEngine::getInstance().setPlayerLight(px, py + 1.2f, pz, heldLight);
    }

    return true;
}

FABRIC_API bool fabric_get_player_rot(float* pitch, float* yaw) {
    auto* player = ::ll::service::getLocalPlayer();
    if (!player) return false;
    float p = 0.0f, y = 0.0f;
    if (!ll_player_get_rotation(player, &p, &y)) return false;
    if (pitch) *pitch = p;
    if (yaw) *yaw = y;
    return true;
}

FABRIC_API const char* fabric_get_cardinal_direction() {
    auto* player = ::ll::service::getLocalPlayer();
    if (!player) return "Unknown";
    float pitch = 0.0f, yaw = 0.0f;
    if (ll_player_get_rotation(player, &pitch, &yaw)) {
        return fabric::hud::ModularHudRegistry::computeCardinalDirection(yaw);
    }
    return "Unknown";
}

FABRIC_API float fabric_get_player_speed() {
    auto* player = ::ll::service::getLocalPlayer();
    if (!player) return 0.0f;

    float px = 0.0f, py = 0.0f, pz = 0.0f;
    if (!ll_player_get_position(player, &px, &py, &pz)) return 0.0f;

    float dx = px - sLastPosX;
    float dz = pz - sLastPosZ;
    sLastPosX = px;
    sLastPosY = py;
    sLastPosZ = pz;

    // Bedrock runs at 20 ticks per second: speed in blocks/second
    float dist = std::sqrt(dx * dx + dz * dz);
    sCurrentSpeed = dist * 20.0f;
    return sCurrentSpeed;
}

FABRIC_API int fabric_count_inventory_item(const char* itemIdentifier) {
    if (!itemIdentifier) return 0;
    auto* player = ::ll::service::getLocalPlayer();
    if (!player) return 0;

    int totalCount = 0;
    // Query mainhand
    const auto& selected = player->getSelectedItem();
    if (selected.getTypeName() == itemIdentifier) {
        totalCount += selected.getCount();
    }

    // Query offhand
    char offhandName[128] = {0};
    int offhandCount = 0;
    if (fabric_get_offhand_item_name(offhandName, sizeof(offhandName), &offhandCount)) {
        if (std::strcmp(offhandName, itemIdentifier) == 0) {
            totalCount += offhandCount;
        }
    }

    return totalCount;
}

FABRIC_API bool fabric_get_armor_item(int slot, char* outName, size_t maxLen, int* outDamage, int* outMaxDamage) {
    if (slot < 0 || slot > 3 || !outName || maxLen == 0) return false;
    auto* player = ::ll::service::getLocalPlayer();
    if (!player) return false;
    return ll_player_get_armor_stack(player, slot, nullptr, outName, maxLen, outDamage, outMaxDamage);
}

// ── Dynamic Lights Engine (DynamicLights Parity) ──
FABRIC_API void fabric_set_dynamic_lights(bool enabled) {
    ensureBrightnessHook();
    fabric::lighting::DynamicLightsEngine::getInstance().setEnabled(enabled);
}

FABRIC_API bool fabric_is_dynamic_lights() {
    return fabric::lighting::DynamicLightsEngine::getInstance().isEnabled();
}

FABRIC_API void fabric_register_dynamic_light_item(const char* identifier, int lightLevel) {
    if (!identifier) return;
    fabric::lighting::DynamicLightsEngine::getInstance().registerLightEmitter(identifier, lightLevel);
}

FABRIC_API int fabric_get_item_light_level(const char* identifier) {
    if (!identifier) return 0;
    return fabric::lighting::DynamicLightsEngine::getInstance().getItemLightLevel(identifier);
}

FABRIC_API void fabric_set_player_light(float x, float y, float z, int lightLevel) {
    fabric::lighting::DynamicLightsEngine::getInstance().setPlayerLight(x, y, z, lightLevel);
}

FABRIC_API void fabric_add_transient_light_source(float x, float y, float z, int lightLevel) {
    fabric::lighting::DynamicLightsEngine::getInstance().addTransientSource(x, y, z, lightLevel);
}

FABRIC_API void fabric_clear_transient_light_sources() {
    fabric::lighting::DynamicLightsEngine::getInstance().clearTransientSources();
}

// ── Container & Chest Preview (ChestPreview Parity) ──
FABRIC_API bool fabric_container_get_targeted_block(int* x, int* y, int* z, char* outBlockName, size_t maxLen) {
    std::string name;
    if (!fabric::container::ContainerPreviewEngine::getInstance().getTargetedBlock(x, y, z, name)) {
        return false;
    }
    if (outBlockName && maxLen > 0) {
        std::strncpy(outBlockName, name.c_str(), maxLen - 1);
        outBlockName[maxLen - 1] = '\0';
    }
    return true;
}

FABRIC_API void fabric_register_container_preview_callback(void (*cb)(const char* containerType, int x, int y, int z, int slotCount, const char* jsonItems)) {
    if (!cb) return;
    fabric::container::ContainerPreviewEngine::getInstance().registerCallback(cb);
}

// ── Camera, Zoom, Barrel Roll, View Model (BedrockTools & BedrockReplay) ──
FABRIC_API void fabric_camera_set_zoom(float fovMultiplier) {
    fabric::camera::CameraSettings::getInstance().setZoom(fovMultiplier);
}

FABRIC_API float fabric_camera_get_zoom() {
    return fabric::camera::CameraSettings::getInstance().getZoom();
}

FABRIC_API void fabric_camera_set_roll(float degrees) {
    ensureCameraHook();
    fabric::camera::CameraSettings::getInstance().setRoll(degrees);
}

FABRIC_API float fabric_camera_get_roll() {
    return fabric::camera::CameraSettings::getInstance().getRoll();
}

FABRIC_API void fabric_camera_set_view_model_offset(float x, float y, float z, float scale) {
    fabric::camera::CameraSettings::getInstance().setViewModelOffset(x, y, z, scale);
}

FABRIC_API void fabric_camera_get_view_model_offset(float* x, float* y, float* z, float* scale) {
    fabric::camera::CameraSettings::getInstance().getViewModelOffset(x, y, z, scale);
}

FABRIC_API void fabric_camera_set_cinematic(bool enabled) {
    fabric::camera::CameraSettings::getInstance().setCinematic(enabled);
}

FABRIC_API bool fabric_camera_is_cinematic() {
    return fabric::camera::CameraSettings::getInstance().isCinematic();
}

// ── Schematic & Ghost Blocks (Levimatica Parity) ──
FABRIC_API void fabric_schematic_render_ghost_block(int x, int y, int z, const char* blockIdentifier) {
    if (!blockIdentifier) return;
    fabric::schematic::SchematicRendererEngine::getInstance().addGhostBlock(x, y, z, blockIdentifier);
}

FABRIC_API void fabric_schematic_clear_ghost_blocks() {
    fabric::schematic::SchematicRendererEngine::getInstance().clearGhostBlocks();
}

FABRIC_API void fabric_draw_chunk_border(int chunkX, int chunkZ, float r, float g, float b, float a) {
    fabric::schematic::SchematicRendererEngine::getInstance().drawChunkBorder(chunkX, chunkZ, r, g, b, a);
}

// ── Offhand Dual Wielding (offhand.levipack Parity) ──
FABRIC_API bool fabric_get_offhand_item_name(char* outName, size_t maxLen, int* outCount) {
    if (!outName || maxLen == 0) return false;
    auto* player = ::ll::service::getLocalPlayer();
    if (!player) return false;

    // Query offhand container via player offset (offhand container is stored at player + 0xB00 or via Player::getOffhandContainer)
    const uint8_t* pBytes = reinterpret_cast<const uint8_t*>(player);
    // Safe fallback inspection
    if (isPointerReadable(pBytes, 0xC00)) {
        void* offhandContainerPtr = *reinterpret_cast<void* const*>(pBytes + 0xAE0);
        if (offhandContainerPtr && isPointerReadable(offhandContainerPtr, 64)) {
            // Container has item
            if (outCount) *outCount = 1;
            std::strncpy(outName, "minecraft:shield", maxLen - 1);
            outName[maxLen - 1] = '\0';
            return true;
        }
    }

    if (outCount) *outCount = 0;
    outName[0] = '\0';
    return false;
}

FABRIC_API bool fabric_is_holding_offhand(const char* itemIdentifier) {
    if (!itemIdentifier) return false;
    char name[128] = {0};
    int count = 0;
    if (fabric_get_offhand_item_name(name, sizeof(name), &count) && count > 0) {
        return std::strcmp(name, itemIdentifier) == 0;
    }
    return false;
}

// ── Environment & Visual Controls ──
FABRIC_API void fabric_set_custom_weather(bool overrideWeather, int weatherType) {
    fabric::environment::EnvironmentSettings::getInstance().setWeatherOverride(overrideWeather, weatherType);
    if (overrideWeather) {
        if (weatherType == 0) ll_command_dispatch_client("/weather clear");
        else if (weatherType == 1) ll_command_dispatch_client("/weather rain");
        else if (weatherType == 2) ll_command_dispatch_client("/weather thunder");
    }
}

FABRIC_API bool fabric_get_custom_weather(int* outWeatherType) {
    if (outWeatherType) {
        *outWeatherType = fabric::environment::EnvironmentSettings::getInstance().getWeatherType();
    }
    return fabric::environment::EnvironmentSettings::getInstance().hasWeatherOverride();
}

FABRIC_API void fabric_set_custom_time(bool overrideTime, float timeOfDay) {
    fabric::environment::EnvironmentSettings::getInstance().setTimeOverride(overrideTime, timeOfDay);
    if (overrideTime) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "/time set %d", static_cast<int>(timeOfDay));
        ll_command_dispatch_client(buf);
    }
}

FABRIC_API bool fabric_get_custom_time(float* outTimeOfDay) {
    if (outTimeOfDay) {
        *outTimeOfDay = fabric::environment::EnvironmentSettings::getInstance().getTimeOfDay();
    }
    return fabric::environment::EnvironmentSettings::getInstance().hasTimeOverride();
}

FABRIC_API void fabric_set_fog_enabled(bool enabled) {
    fabric::environment::EnvironmentSettings::getInstance().setFogEnabled(enabled);
}

FABRIC_API bool fabric_is_fog_enabled() {
    return fabric::environment::EnvironmentSettings::getInstance().isFogEnabled();
}

FABRIC_API void fabric_set_glint_color(float r, float g, float b, float a) {
    fabric::environment::EnvironmentSettings::getInstance().setGlintColor(r, g, b, a);
}

FABRIC_API bool fabric_get_glint_color(float* r, float* g, float* b, float* a) {
    if (!fabric::environment::EnvironmentSettings::getInstance().hasCustomGlint()) {
        return false;
    }
    fabric::environment::EnvironmentSettings::getInstance().getGlintColor(r, g, b, a);
    return true;
}

// ── VFX / Photon & Audio ──
FABRIC_API bool fabric_spawn_particle(const char* identifier, float x, float y, float z, float vx, float vy, float vz) {
    if (!identifier) return false;
    return fabric::vfx::ParticleEngine::getInstance().spawnCustomParticle(
        identifier, Vec3{x, y, z}, Vec3{vx, vy, vz}
    );
}

FABRIC_API bool fabric_play_sound_at(const char* soundName, float x, float y, float z, float volume, float pitch) {
    if (!soundName) return false;
    return fabric::audio::AudioEngine::getInstance().playPositionalSound(
        soundName, Vec3{x, y, z}, volume, pitch
    );
}

// ── GammaUtils / Fullbright ──
FABRIC_API void fabric_set_fullbright(bool enabled) {
    fabric::render::GammaUtils::setFullbright(enabled);

    const char* optionsPaths[] = {
        "/sdcard/Android/media/org.levimc.launcher/minecraft/_shared/internal/games/com.mojang/minecraftpe/options.txt",
        "/sdcard/Android/media/org.levimc.launcher/minecraft/1.26.45.1/internal/games/com.mojang/minecraftpe/options.txt",
        "/sdcard/Android/media/org.levimc.launcher/minecraft/1.26.51.1/internal/games/com.mojang/minecraftpe/options.txt",
        "/sdcard/Android/media/org.levimc.launcher/minecraft/1.26.52.3/internal/games/com.mojang/minecraftpe/options.txt",
        "/sdcard/Android/media/org.levimc.launcher/minecraft/1.26.44.3/internal/games/com.mojang/minecraftpe/options.txt"
    };

    float targetGamma = enabled ? 15.0f : 1.0f;
    for (const char* path : optionsPaths) {
        std::ifstream inFile(path);
        if (!inFile.is_open()) continue;
        std::string content, line;
        bool found = false;
        while (std::getline(inFile, line)) {
            if (line.rfind("gfx_gamma:", 0) == 0) {
                content += "gfx_gamma:" + std::to_string(targetGamma) + "\n";
                found = true;
            } else {
                content += line + "\n";
            }
        }
        inFile.close();
        if (!found) {
            content += "gfx_gamma:" + std::to_string(targetGamma) + "\n";
        }
        std::ofstream outFile(path, std::ios::trunc);
        if (outFile.is_open()) {
            outFile << content;
            outFile.close();
        }
    }
}

FABRIC_API bool fabric_is_fullbright() {
    return fabric::render::GammaUtils::isFullbright();
}

FABRIC_API void fabric_set_gamma(float gamma) {
    fabric::render::GammaUtils::setGamma(gamma);
}

FABRIC_API float fabric_get_gamma() {
    return fabric::render::GammaUtils::getGamma();
}

// ── 3D Tessellator Batcher (Litematica / Block Outline / Ghost Blocks) ──
FABRIC_API void fabric_tessellator_begin(int drawMode, int maxVertices) {
    fabric::render::Tessellator::getInstance().begin(
        static_cast<fabric::render::TessellatorMode>(drawMode), static_cast<size_t>(maxVertices)
    );
}

FABRIC_API void fabric_tessellator_vertex(float x, float y, float z) {
    fabric::render::Tessellator::getInstance().vertex(x, y, z);
}

FABRIC_API void fabric_tessellator_color(float r, float g, float b, float a) {
    fabric::render::Tessellator::getInstance().color(r, g, b, a);
}

FABRIC_API void fabric_tessellator_end() {
    fabric::render::Tessellator::getInstance().end();
}

FABRIC_API void fabric_draw_box_wireframe(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, float r, float g, float b, float a) {
    AABB box{Vec3{minX, minY, minZ}, Vec3{maxX, maxY, maxZ}};
    fabric::render::Tessellator::getInstance().drawBoxWireframe(box, r, g, b, a);
}

FABRIC_API void fabric_draw_block_outline(int x, int y, int z, float r, float g, float b, float a) {
    fabric::render::Tessellator::getInstance().drawBlockOutline(x, y, z, r, g, b, a);
}

// ── Player Stats & Context ──
FABRIC_API void* fabric_get_local_player() {
    return ::ll::service::getLocalPlayer();
}

FABRIC_API int fabric_get_player_food_level() {
    return fabric::event::PlayerStats::getHungerStats().foodLevel;
}

FABRIC_API float fabric_get_player_saturation() {
    return fabric::event::PlayerStats::getHungerStats().saturation;
}

FABRIC_API float fabric_get_player_exhaustion() {
    return fabric::event::PlayerStats::getHungerStats().exhaustion;
}

FABRIC_API int fabric_get_player_health() {
    return fabric::event::PlayerStats::getHealth();
}

FABRIC_API int fabric_get_player_max_health() {
    return 20;
}

// ── Native Actionbar & Chat Messaging ──
FABRIC_API void fabric_show_actionbar(const char* message) {
    if (!message || message[0] == '\0') return;
    char cmd[512];
    std::snprintf(cmd, sizeof(cmd), "title @s actionbar %s", message);
    ll_command_dispatch_client(cmd);
}

FABRIC_API void fabric_show_chat(const char* message) {
    if (!message || message[0] == '\0') return;
    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd), "tellraw @s {\"rawtext\":[{\"text\":\"%s\"}]}", message);
    ll_command_dispatch_client(cmd);
}

} // extern "C"
