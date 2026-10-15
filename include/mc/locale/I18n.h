#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <memory>

class Localization {
public:
    virtual ~Localization() = default;
};

class I18n {
public:
    struct LanguageWrapper {
        std::shared_ptr<Localization> mLoc{};
        [[nodiscard]] Localization* get() const noexcept { return mLoc.get(); }
        explicit operator bool() const noexcept { return mLoc != nullptr; }
    };

    static std::string get(std::string_view key, std::vector<std::string> const& params = {}) { (void)params; return std::string(key); }
    static std::string get(std::string const& key) { return key; }
    static std::string get(char const* key) { return key ? std::string(key) : std::string(); }
    static std::string get(
        std::string const& key,
        std::vector<std::string> const& params,
        Localization* locale
    ) { (void)params; (void)locale; return key; }

    [[nodiscard]] LanguageWrapper getCurrentLanguage() const { return {}; }
    [[nodiscard]] Localization* getLocaleFor(std::string const& code) const { (void)code; return nullptr; }
};

inline I18n& getI18n() {
    static I18n sI18n;
    return sI18n;
}
