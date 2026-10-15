#pragma once
#include <string>
#include <string_view>

namespace Core {

class PathView {
public:
    std::string_view mPath{};

    constexpr PathView() noexcept = default;
    constexpr PathView(std::string_view sv) noexcept : mPath(sv) {}
    constexpr PathView(char const* s) noexcept : mPath(s ? s : "") {}
    PathView(std::string const& s) noexcept : mPath(s) {}

    [[nodiscard]] constexpr std::string_view getView() const noexcept { return mPath; }
    [[nodiscard]] std::string getFullPath() const { return std::string(mPath); }
    [[nodiscard]] constexpr char const* data() const noexcept { return mPath.data(); }
    [[nodiscard]] constexpr size_t size() const noexcept { return mPath.size(); }
    [[nodiscard]] constexpr size_t length() const noexcept { return mPath.size(); }
    [[nodiscard]] constexpr bool empty() const noexcept { return mPath.empty(); }

    [[nodiscard]] constexpr bool operator==(PathView const& rhs) const noexcept {
        return mPath == rhs.mPath;
    }
    [[nodiscard]] constexpr bool operator!=(PathView const& rhs) const noexcept {
        return mPath != rhs.mPath;
    }

    constexpr operator std::string_view() const noexcept { return mPath; }
};

} // namespace Core
