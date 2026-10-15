#pragma once

#include "levilamroid/LeviLamroid.h"
#include <string>

namespace example {

class ExampleMod : public ll::mod::NativeMod {
public:
    ExampleMod();
    ~ExampleMod() override = default;

    static ExampleMod& getInstance();

    /// Called when the shared library is loaded by LeviLauncher / LeviLamroid
    bool load();

    /// Called when the mod is enabled
    bool enable();

    /// Called when the mod is disabled
    bool disable();

private:
    void initConfig();
    void registerHudButton();
    void registerInputListeners();
    void registerEventListeners();

    bool mEnabled{false};
    bool mCustomFeatureActive{false};
};

} // namespace example
