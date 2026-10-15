#pragma once
#include <string>
#include <vector>
#include <functional>
#include <string_view>
#include "mc/enums/Enums.h"

namespace pl::modmenu {

struct ConfigEntry {
    std::string key;
    std::string value;
    std::string type;
};

struct ModuleInfo {
    std::string moduleId;               // 0x00 (24 bytes)
    std::string displayName;            // 0x18 (24 bytes)
    std::string description;            // 0x30 (24 bytes)
    std::string modId;                  // 0x48 (24 bytes)
    bool defaultEnabled{};              // 0x60 (1 byte)
    bool hideInHudEditor{};             // 0x61 (1 byte)
    char padding[6]{};                  // 0x62 - 0x67
    std::vector<ConfigEntry> configs;   // 0x68 (24 bytes)
    std::function<void(std::string_view moduleId, bool enabled)> onToggle; // 0x80 (48 bytes)
    std::function<void(std::string_view moduleId, std::string_view key, std::string_view value)> onConfigChanged; // 0xb0 (48 bytes)
    std::function<void(std::string_view moduleId, std::string_view key, bool value)> onBoolConfigChanged; // 0xe0 (48 bytes)
};
#if defined(__aarch64__) && defined(_LIBCPP_VERSION)
static_assert(sizeof(ModuleInfo) == 272, "ModuleInfo must be 272 bytes on ARM64");
#endif

struct ButtonInfo {
    std::string buttonId;               // 0x00 (24 bytes)
    std::string moduleId;               // 0x18 (24 bytes)
    std::string displayName;            // 0x30 (24 bytes)
    std::string modId;                  // 0x48 (24 bytes)
    std::string label;                  // 0x60 (24 bytes)
    int androidKeyCode{};               // 0x78 (4 bytes)
    ButtonBehavior behavior{};          // 0x7c (4 bytes)
    bool defaultVisible{true};          // 0x80 (1 byte)
    char padding1[3]{};                 // 0x81 - 0x83
    ButtonStylePreset stylePreset{};    // 0x84 (4 bytes)
    uint32_t normalBgColor{};           // 0x88 (4 bytes)
    uint32_t activeBgColor{};           // 0x8c (4 bytes)
    uint32_t borderColor{};             // 0x90 (4 bytes)
    uint32_t textColor{};               // 0x94 (4 bytes)
    uint32_t activeTextColor{};         // 0x98 (4 bytes)
    float widthScale{};                 // 0x9c (4 bytes)
    float heightScale{1.0f};            // 0xa0 (4 bytes)
    ButtonIconFormat iconFormat{};      // 0xa4 (4 bytes)
    bool hideLabelWhenIconPresent{true};// 0xa8 (1 byte)
    char padding2[7]{};                 // 0xa9 - 0xaf
    std::vector<unsigned char> iconData;       // 0xb0 (24 bytes)
    std::vector<unsigned char> activeIconData; // 0xc8 (24 bytes)
    std::function<void(std::string_view buttonId, ButtonEvent event, float value)> onEvent; // 0xe0 (48 bytes)
};
#if defined(__aarch64__) && defined(_LIBCPP_VERSION)
static_assert(sizeof(ButtonInfo) == 272, "ButtonInfo must be 272 bytes on ARM64");
#endif

void registerButton(const ButtonInfo& info);
void unregisterButton(std::string_view buttonId);
void registerModule(const ModuleInfo& info);
void unregisterModule(std::string_view moduleId);
void setConfigSchemaJson(std::string_view modId, std::string_view json);

} // namespace pl::modmenu
