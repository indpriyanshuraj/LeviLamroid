#pragma once
#include "ll/api/command/CommandHandle.h"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace ll::command {

class CommandRegistrar {
private:
    std::unordered_map<std::string, std::shared_ptr<CommandHandle>> mCommands;
    mutable std::mutex mMutex;

    CommandRegistrar() = default;

public:
    static CommandRegistrar& getInstance(bool isClientSide = true) {
        (void)isClientSide;
        static CommandRegistrar instance;
        return instance;
    }

    static CommandRegistrar& getClientInstance() { return getInstance(true); }
    static CommandRegistrar& getServerInstance() { return getInstance(false); }

    CommandHandle& getOrCreateCommand(
        const std::string& name,
        const std::string& description = "",
        CommandPermissionLevel perm = CommandPermissionLevel::Any
    ) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mCommands.find(name);
        if (it != mCommands.end()) {
            return *it->second;
        }
        auto handle = std::make_shared<CommandHandle>(name, description, perm);
        mCommands[name] = handle;
        return *handle;
    }

    std::shared_ptr<CommandHandle> findCommand(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mCommands.find(name);
        if (it != mCommands.end()) {
            return it->second;
        }
        for (const auto& [_, cmd] : mCommands) {
            for (const auto& a : cmd->getAliases()) {
                if (a == name) {
                    return cmd;
                }
            }
        }
        return nullptr;
    }

    bool dispatchCommand(const std::string& line) {
        if (line.empty()) return false;
        std::string clean = (line[0] == '/') ? line.substr(1) : line;
        size_t sp = clean.find(' ');
        std::string cmdName = (sp == std::string::npos) ? clean : clean.substr(0, sp);
        std::vector<std::string> args;
        if (sp != std::string::npos) {
            std::string rest = clean.substr(sp + 1);
            size_t start = 0;
            while (start < rest.size()) {
                while (start < rest.size() && rest[start] == ' ') start++;
                if (start >= rest.size()) break;
                size_t next_sp = rest.find(' ', start);
                if (next_sp == std::string::npos) {
                    args.push_back(rest.substr(start));
                    break;
                } else {
                    args.push_back(rest.substr(start, next_sp - start));
                    start = next_sp + 1;
                }
            }
        }
        auto cmd = findCommand(cmdName);
        if (cmd) {
            cmd->invoke(args);
            return true;
        }
        return false;
    }
};

} // namespace ll::command
