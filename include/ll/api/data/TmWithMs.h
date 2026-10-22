#pragma once

#include <ctime>

#if __has_include("fmt/base.h")
#include "fmt/base.h"
#else
#include "fmt/format.h"
#endif
#include "ll/api/base/StdInt.h"

#include "fmt/chrono.h"

namespace ll::data {
struct TmWithMs : public std::tm {
    ushort ms;
};
} // namespace ll::data

template <typename Char>
struct fmt::formatter<ll::data::TmWithMs, Char> : formatter<std::tm, Char> {
private:
    detail::arg_ref<Char>        precisionRef;
    fmt::format_specs<Char>      specs;

public:
    constexpr formatter() = default;

    template <typename FormatContext>
    auto format(ll::data::TmWithMs const& val, FormatContext& ctx) const -> decltype(ctx.out()) {
        formatter<std::tm, Char>::format(val, ctx);
        auto out = ctx.out();
        if (specs.precision > 0) {
            fmt::format_to(out, "{0:0>{1}}", val.ms, specs.precision);
        }
        return out;
    }
    constexpr auto parse(basic_format_parse_context<Char>& ctx) -> decltype(ctx.begin()) {
        return formatter<std::tm, Char>::parse(ctx);
    }
};
