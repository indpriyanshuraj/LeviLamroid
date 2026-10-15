#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <memory>

namespace ll::command {

enum class CommandPermissionLevel : int {
    Any = 0,
    GameDirectors = 1,
    Admin = 2,
    Host = 3,
    Owner = 4,
    Internal = 5
};

class CommandHandle {
public:
    using Callback = std::function<void(const std::vector<std::string>& args)>;

private:
    std::string mName;
    std::string mDescription;
    CommandPermissionLevel mPerm;
    std::vector<std::string> mAliases;
    Callback mCallback;

public:
    CommandHandle(std::string name, std::string desc, CommandPermissionLevel perm = CommandPermissionLevel::Any)
        : mName(std::move(name)), mDescription(std::move(desc)), mPerm(perm) {}

    const std::string& getName() const { return mName; }
    const std::string& getDescription() const { return mDescription; }
    CommandPermissionLevel getPermissionLevel() const { return mPerm; }
    const std::vector<std::string>& getAliases() const { return mAliases; }

    CommandHandle& alias(std::string_view a) {
        mAliases.emplace_back(a);
        return *this;
    }

    CommandHandle& execute(Callback cb) {
        mCallback = std::move(cb);
        return *this;
    }

    template <class Func>
    CommandHandle& overload(Func&& fn) {
        mCallback = [fn = std::forward<Func>(fn)](const std::vector<std::string>& args) {
            fn(args);
        };
        return *this;
    }

    void invoke(const std::vector<std::string>& args) const {
        if (mCallback) {
            mCallback(args);
        }
    }
};

} // namespace ll::command
