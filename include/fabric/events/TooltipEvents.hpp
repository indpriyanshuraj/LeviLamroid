#pragma once

#include "ll/api/event/Event.h"
#include <string>
#include <vector>

namespace fabric::event {

class ItemTooltipEvent : public ::ll::event::Event {
private:
    const void* mItemStack;
    std::string mItemIdentifier;
    std::vector<std::string> mLines;
    bool mIsAdvanced;

public:
    ItemTooltipEvent(const void* stack, std::string_view identifier, bool advanced = false)
        : mItemStack(stack), mItemIdentifier(identifier), mIsAdvanced(advanced) {}

    [[nodiscard]] const void* getItemStack() const noexcept { return mItemStack; }
    [[nodiscard]] const std::string& getItemIdentifier() const noexcept { return mItemIdentifier; }
    [[nodiscard]] bool isAdvanced() const noexcept { return mIsAdvanced; }

    [[nodiscard]] const std::vector<std::string>& getLines() const noexcept { return mLines; }
    [[nodiscard]] std::vector<std::string>& getLines() noexcept { return mLines; }

    void appendLine(std::string line) {
        mLines.push_back(std::move(line));
    }
};

} // namespace fabric::event

extern "C" {
typedef void (*FabricTooltipCallback)(const char* itemIdentifier, const void* itemStack, void (*appendFn)(const char* text, void* udata), void* udata);
void fabric_register_tooltip_callback(FabricTooltipCallback cb);
}
