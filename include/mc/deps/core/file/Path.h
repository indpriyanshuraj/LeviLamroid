#pragma once
#include <string>
#include <string_view>
#include "mc/deps/core/file/PathView.h"

namespace Core {

class Path {
public:
    std::string mPath{};

    Path() = default;
    Path(std::string_view sv) : mPath(sv) {}
    Path(char const* s) : mPath(s ? s : "") {}
    Path(std::string s) : mPath(std::move(s)) {}
    Path(PathView pv) : mPath(pv.getView()) {}

    [[nodiscard]] std::string const& getFullPath() const noexcept { return mPath; }
    [[nodiscard]] std::string_view getView() const noexcept { return mPath; }
    [[nodiscard]] char const* c_str() const noexcept { return mPath.c_str(); }
    [[nodiscard]] char const* data() const noexcept { return mPath.data(); }
    [[nodiscard]] size_t size() const noexcept { return mPath.size(); }
    [[nodiscard]] size_t length() const noexcept { return mPath.size(); }
    [[nodiscard]] bool empty() const noexcept { return mPath.empty(); }

    [[nodiscard]] bool operator==(Path const& rhs) const noexcept { return mPath == rhs.mPath; }
    [[nodiscard]] bool operator!=(Path const& rhs) const noexcept { return mPath != rhs.mPath; }
    [[nodiscard]] bool operator==(PathView pv) const noexcept { return mPath == pv.getView(); }
    [[nodiscard]] bool operator!=(PathView pv) const noexcept { return mPath != pv.getView(); }

    operator PathView() const noexcept { return PathView{mPath}; }
    operator std::string_view() const noexcept { return mPath; }
    operator std::string const&() const noexcept { return mPath; }
};

} // namespace Core
