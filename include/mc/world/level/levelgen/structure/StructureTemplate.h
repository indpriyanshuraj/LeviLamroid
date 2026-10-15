#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/levelgen/structure/BoundingBox.h"
#include "mc/world/level/levelgen/structure/LegacyStructureSettings.h"
#include "mc/world/level/levelgen/structure/StructureBlockPalette.h"

class BlockSource;

class StructureTemplateData {
public:
    struct SizeWrapper {
        BlockPos mSize{0, 0, 0};
        [[nodiscard]] BlockPos const& get() const noexcept { return mSize; }
        [[nodiscard]] BlockPos& get() noexcept { return mSize; }
    };

    struct IndicesWrapper {
        std::vector<int32_t> mIndices{};
        [[nodiscard]] std::vector<int32_t> const& get() const noexcept { return mIndices; }
        [[nodiscard]] std::vector<int32_t>& get() noexcept { return mIndices; }
    };

    struct OptionalIndicesWrapper {
        std::optional<std::vector<int32_t>> mIndices{};
        [[nodiscard]] std::optional<std::vector<int32_t>> const& get() const noexcept { return mIndices; }
        [[nodiscard]] std::optional<std::vector<int32_t>>& get() noexcept { return mIndices; }
    };

    SizeWrapper                                            mSize{};
    IndicesWrapper                                         mBlockIndices{};
    OptionalIndicesWrapper                                 mExtraBlockIndices{};
    std::unordered_map<std::string, StructureBlockPalette> mPalettes{};

    static std::string const& DEFAULT_PALETTE_NAME() {
        static std::string const sDefault = "default";
        return sDefault;
    }

    [[nodiscard]] StructureBlockPalette const* getPalette(std::string const& name) const noexcept {
        auto it = mPalettes.find(name);
        if (it != mPalettes.end()) return &it->second;
        return nullptr;
    }

    [[nodiscard]] StructureBlockPalette* getPalette(std::string const& name) noexcept {
        auto it = mPalettes.find(name);
        if (it != mPalettes.end()) return &it->second;
        return nullptr;
    }
};

class StructureTemplate {
public:
    struct TemplateDataWrapper {
        StructureTemplateData mData{};
        [[nodiscard]] StructureTemplateData const& get() const noexcept { return mData; }
        [[nodiscard]] StructureTemplateData& get() noexcept { return mData; }
    };

    struct RegistryWrapper {
        void* mPtr{nullptr};
        [[nodiscard]] void* get() const noexcept { return mPtr; }
    };

    std::string         mName{};
    TemplateDataWrapper mStructureTemplateData{};
    RegistryWrapper     mUnknownBlockRegistry{};

    StructureTemplate() = default;
    explicit StructureTemplate(std::string_view name, void* unknownRegistry = nullptr)
        : mName(name), mUnknownBlockRegistry{unknownRegistry} {}

    virtual ~StructureTemplate() = default;

    virtual bool load(CompoundTag const& tag) { (void)tag; return true; }
    virtual bool save(CompoundTag& tag) const { (void)tag; return true; }

    virtual void fillFromWorld(
        BlockSource& region,
        BlockPos const& pos,
        LegacyStructureSettings const& settings
    ) {
        (void)region;
        (void)pos;
        (void)settings;
    }

    virtual void placeInWorld(
        BlockSource& region,
        BlockPos const& pos,
        LegacyStructureSettings const& settings
    ) {
        (void)region;
        (void)pos;
        (void)settings;
    }

    [[nodiscard]] BlockPos getSize() const noexcept {
        return mStructureTemplateData.mData.mSize.get();
    }

    static std::unique_ptr<StructureTemplate> create(
        std::string_view name,
        BlockSource& region,
        BoundingBox const& boundingBox,
        bool includeEntities,
        bool ignoreBlocks
    );
};

inline std::unique_ptr<StructureTemplate> StructureTemplate::create(
    std::string_view name,
    BlockSource& region,
    BoundingBox const& boundingBox,
    bool includeEntities,
    bool ignoreBlocks
) {
    (void)region;
    (void)includeEntities;
    (void)ignoreBlocks;
    auto templ = std::make_unique<StructureTemplate>(name);
    templ->mStructureTemplateData.mData.mSize.mSize = boundingBox.getSpan();
    return templ;
}
