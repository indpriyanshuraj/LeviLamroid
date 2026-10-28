#pragma once

// ============================================================================
// LeviLamroid Fabric Client API for Minecraft Bedrock Edition (Android ARM64)
//
// Provides high-level Fabric-style client modding capabilities:
// - AppleSkin: Player hunger, saturation, exhaustion stats & item hover tooltips
// - Dynamic Lights: Real-time handheld & entity lighting with distance attenuation
// - Modular HUD: Actionbar, coordinates, arrow/totem counters, armor durability
// - Chest & Container Preview: Aim-based container inspection & Shulker Box NBT tooltips
// - Litematica: 3D Tessellator depth-tested wireframes, ghost blocks & chunk boundaries
// - Offhand Dual Wielding: Offhand slot queries, interaction & rendering
// - Camera & View Model: OptiFine FOV zoom, Elytra barrel rolls & first-person offsets
// - Environment: Client-side weather, time-of-day, fog controls & custom glint colors
// - VFX / Photon & Voice Chat: Custom particles & positional 3D audio playback
// ============================================================================

#include "fabric/events/HudEvents.hpp"
#include "fabric/events/TooltipEvents.hpp"
#include "fabric/events/CameraEvents.hpp"
#include "fabric/render/Tessellator.hpp"
#include "fabric/render/GammaUtils.hpp"
#include "fabric/lighting/DynamicLights.hpp"
#include "fabric/hud/ModularHud.hpp"
#include "fabric/container/ContainerPreview.hpp"
#include "fabric/camera/CameraUtils.hpp"
#include "fabric/schematic/SchematicRenderer.hpp"
#include "fabric/environment/EnvironmentUtils.hpp"
#include "fabric/vfx/ParticleEngine.hpp"
#include "fabric/audio/AudioEngine.hpp"
#include "fabric/ClientSignatures.hpp"

// Bedrock Engine Base APIs
#include "ll/api/service/Bedrock.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/render/RenderEvents.h"
#include "ll/api/event/player/PlayerEvents.h"
#include "ll/api/resource/ResourceManager.h"
#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/block/BlockTypeRegistry.h"

#ifndef FABRIC_API
#if defined(__GNUC__) || defined(__clang__)
#define FABRIC_API __attribute__((visibility("default")))
#else
#define FABRIC_API
#endif
#endif

extern "C" {

// ── Tooltips & Hover Text (AppleSkin / Shulker Tooltip / Chest Preview) ──
FABRIC_API void fabric_register_tooltip_callback(FabricTooltipCallback cb);

// ── Rendering & HUD Overlays (AppleSkin HUD / Armor HUD / Health Bar) ──
FABRIC_API void fabric_register_hud_render_callback(FabricHudRenderCallback cb);
FABRIC_API void fabric_register_level_render_callback(void (*cb)(void* screenCtx, const void* frameObj));
FABRIC_API void fabric_register_camera_callback(FabricCameraCallback cb);

// ── Modular HUD & Widgets (BedrockTools Suite) ──
FABRIC_API void fabric_hud_register_widget(const char* widgetId, void (*renderCb)(void* screenCtx, const void* frameObj));
FABRIC_API void fabric_hud_unregister_widget(const char* widgetId);
FABRIC_API bool fabric_get_player_coords(float* x, float* y, float* z);
FABRIC_API bool fabric_get_player_rot(float* pitch, float* yaw);
FABRIC_API const char* fabric_get_cardinal_direction();
FABRIC_API float fabric_get_player_speed();
FABRIC_API int fabric_count_inventory_item(const char* itemIdentifier);
FABRIC_API bool fabric_get_armor_item(int slot, char* outName, size_t maxLen, int* outDamage, int* outMaxDamage);

// ── Dynamic Lights Engine (DynamicLights Parity) ──
FABRIC_API void fabric_set_dynamic_lights(bool enabled);
FABRIC_API bool fabric_is_dynamic_lights();
FABRIC_API void fabric_register_dynamic_light_item(const char* identifier, int lightLevel);
FABRIC_API int fabric_get_item_light_level(const char* identifier);
FABRIC_API void fabric_set_player_light(float x, float y, float z, int lightLevel);
FABRIC_API void fabric_add_transient_light_source(float x, float y, float z, int lightLevel);
FABRIC_API void fabric_clear_transient_light_sources();

// ── Container & Chest Preview (ChestPreview Parity) ──
FABRIC_API bool fabric_container_get_targeted_block(int* x, int* y, int* z, char* outBlockName, size_t maxLen);
FABRIC_API void fabric_register_container_preview_callback(void (*cb)(const char* containerType, int x, int y, int z, int slotCount, const char* jsonItems));

// ── Camera, Zoom, Barrel Roll, View Model (BedrockTools & BedrockReplay) ──
FABRIC_API void fabric_camera_set_zoom(float fovMultiplier);
FABRIC_API float fabric_camera_get_zoom();
FABRIC_API void fabric_camera_set_roll(float degrees);
FABRIC_API float fabric_camera_get_roll();
FABRIC_API void fabric_camera_set_view_model_offset(float x, float y, float z, float scale);
FABRIC_API void fabric_camera_get_view_model_offset(float* x, float* y, float* z, float* scale);
FABRIC_API void fabric_camera_set_cinematic(bool enabled);
FABRIC_API bool fabric_camera_is_cinematic();

// ── Schematic & Ghost Blocks (Levimatica Parity) ──
FABRIC_API void fabric_schematic_render_ghost_block(int x, int y, int z, const char* blockIdentifier);
FABRIC_API void fabric_schematic_clear_ghost_blocks();
FABRIC_API void fabric_draw_chunk_border(int chunkX, int chunkZ, float r, float g, float b, float a);

// ── Offhand Dual Wielding (offhand.levipack Parity) ──
FABRIC_API bool fabric_get_offhand_item_name(char* outName, size_t maxLen, int* outCount);
FABRIC_API bool fabric_is_holding_offhand(const char* itemIdentifier);

// ── Environment & Visual Controls (Weather / Time / Fog / Glint) ──
FABRIC_API void fabric_set_custom_weather(bool overrideWeather, int weatherType);
FABRIC_API bool fabric_get_custom_weather(int* outWeatherType);
FABRIC_API void fabric_set_custom_time(bool overrideTime, float timeOfDay);
FABRIC_API bool fabric_get_custom_time(float* outTimeOfDay);
FABRIC_API void fabric_set_fog_enabled(bool enabled);
FABRIC_API bool fabric_is_fog_enabled();
FABRIC_API void fabric_set_glint_color(float r, float g, float b, float a);
FABRIC_API bool fabric_get_glint_color(float* r, float* g, float* b, float* a);

// ── VFX / Photon & 3D Positional Audio (Simple Voice Chat) ──
FABRIC_API bool fabric_spawn_particle(const char* identifier, float x, float y, float z, float vx, float vy, float vz);
FABRIC_API bool fabric_play_sound_at(const char* soundName, float x, float y, float z, float volume, float pitch);

// ── GammaUtils / Fullbright (Sodium / OptiFine Light Level Overrides) ──
FABRIC_API void fabric_set_fullbright(bool enabled);
FABRIC_API bool fabric_is_fullbright();
FABRIC_API void fabric_set_gamma(float gamma);
FABRIC_API float fabric_get_gamma();

// ── 3D Tessellator (Litematica / Block Outline / Ghost Blocks / Spatial GUI) ──
FABRIC_API void fabric_tessellator_begin(int drawMode, int maxVertices);
FABRIC_API void fabric_tessellator_vertex(float x, float y, float z);
FABRIC_API void fabric_tessellator_color(float r, float g, float b, float a);
FABRIC_API void fabric_tessellator_end();
FABRIC_API void fabric_draw_box_wireframe(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, float r, float g, float b, float a);
FABRIC_API void fabric_draw_block_outline(int x, int y, int z, float r, float g, float b, float a);

// ── Player Stats & Context (AppleSkin Stats / Health Display) ──
FABRIC_API void* fabric_get_local_player();
FABRIC_API int fabric_get_player_food_level();
FABRIC_API float fabric_get_player_saturation();
FABRIC_API float fabric_get_player_exhaustion();
FABRIC_API int fabric_get_player_health();
FABRIC_API int fabric_get_player_max_health();

// ── Native Actionbar & Chat Messaging ──
FABRIC_API void fabric_show_actionbar(const char* message);
FABRIC_API void fabric_show_chat(const char* message);

} // extern "C"
