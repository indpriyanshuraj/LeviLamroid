#pragma once

#include <cstdint>
#include <string>
#include <functional>

enum class CommandPermissionLevel : int32_t {
    Any = 0,
    GameDirectors = 1,
    Admin = 2,
    Host = 3,
    Owner = 4,
    Internal = 5
};

enum class CommandFlag : int32_t {
    None = 0,
    Cheat = 1,
    Hidden = 2,
    Async = 4,
    Disabled = 8
};

class CommandRegistry {
public:
    // Native Bedrock registration
    static void registerCommand(
        void* self,
        const std::string& name,
        const char* description,
        CommandPermissionLevel level = CommandPermissionLevel::Any,
        CommandFlag flag1 = CommandFlag::None,
        CommandFlag flag2 = CommandFlag::None
    );
};
