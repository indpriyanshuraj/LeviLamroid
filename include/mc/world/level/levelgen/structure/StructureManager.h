#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include "mc/deps/core/file/Path.h"
#include "mc/world/level/levelgen/structure/StructureTemplate.h"

class StructureManager {
public:
    std::unordered_map<std::string, std::unique_ptr<StructureTemplate>> mTemplates{};

    StructureManager() = default;
    virtual ~StructureManager() = default;

    virtual StructureTemplate* getOrCreate(std::string const& name);

    static bool exportStructure(
        StructureTemplate const& structure,
        Core::Path const& outputPath
    );

    static std::unique_ptr<StructureTemplate> importStructure(
        Core::Path const& inputPath
    );
};

inline bool StructureManager::exportStructure(
    StructureTemplate const& structure,
    Core::Path const& outputPath
) {
    (void)structure;
    (void)outputPath;
    return true;
}

inline std::unique_ptr<StructureTemplate> StructureManager::importStructure(
    Core::Path const& inputPath
) {
    (void)inputPath;
    return nullptr;
}
