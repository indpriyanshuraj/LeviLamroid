#pragma once
#include "ll/api/command/CommandHandle.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>

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

    const std::unordered_map<std::string, std::shared_ptr<CommandHandle>>& getCommands() const {
        return mCommands;
    }

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

    std::vector<std::string> getCompletions(const std::string& input) const {
        std::lock_guard<std::mutex> lock(mMutex);
        std::vector<std::string> results;

        if (input.empty() || input == "/") {
            for (const auto& [name, _] : mCommands) {
                results.push_back("/" + name);
            }
            return results;
        }

        // If typing the root command name, e.g. "/l" or "/cos"
        if (input.find(' ') == std::string::npos) {
            std::string prefix = (input[0] == '/') ? input.substr(1) : input;
            for (const auto& [name, handle] : mCommands) {
                if (name.rfind(prefix, 0) == 0) {
                    results.push_back("/" + name);
                }
                for (const auto& a : handle->getAliases()) {
                    if (a.rfind(prefix, 0) == 0) {
                        results.push_back("/" + a);
                    }
                }
            }
            return results;
        }

        // If typing arguments / subcommands, e.g. "/levi " or "/levi par"
        std::string clean = (input[0] == '/') ? input.substr(1) : input;
        size_t firstSpace = clean.find(' ');
        std::string cmdName = clean.substr(0, firstSpace);

        auto cmd = findCommand(cmdName);
        if (!cmd) return results;

        std::string subPrefix = clean.substr(firstSpace + 1);
        // Find if there's a second space (sub-subcommand)
        size_t secondSpace = subPrefix.find(' ');
        if (secondSpace == std::string::npos) {
            for (const auto& sub : cmd->getSubcommands()) {
                if (sub.rfind(subPrefix, 0) == 0) {
                    results.push_back(sub);
                }
            }
        }
        return results;
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
