#pragma once
#include "ll/api/Macros.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace levilamroid::geometry {

struct CustomGeometryDefinition {
    std::string identifier;        // e.g. "geometry.cooking_pot", "geometry.cutting_board"
    std::string jsonContent;       // Full raw JSON string or parsed bones
    std::string formatVersion;     // e.g. "1.12.0"
    float visibleBoundsWidth{1.0f};
    float visibleBoundsHeight{1.0f};
    std::vector<std::string> boneNames;
};

LL_API bool registerGeometry(const CustomGeometryDefinition& def);
LL_API bool registerGeometryJson(std::string_view identifier, std::string_view jsonContent);
LL_API const CustomGeometryDefinition* getGeometry(std::string_view identifier);
LL_API bool hasGeometry(std::string_view identifier);
LL_API size_t getGeometryCount();
LL_API std::vector<CustomGeometryDefinition> getAllGeometries();

class LL_API GeometryRegistry {
public:
    static GeometryRegistry& getInstance();

    bool registerGeometry(const CustomGeometryDefinition& def);
    bool registerJson(std::string_view identifier, std::string_view jsonContent);
    const CustomGeometryDefinition* get(std::string_view identifier) const;
    bool has(std::string_view identifier) const;
    size_t getCount() const;
    std::vector<CustomGeometryDefinition> getAll() const;
};

} // namespace levilamroid::geometry

namespace ll::geometry {
    using CustomGeometryDefinition = ::levilamroid::geometry::CustomGeometryDefinition;
    using GeometryRegistry = ::levilamroid::geometry::GeometryRegistry;
    using ::levilamroid::geometry::registerGeometry;
    using ::levilamroid::geometry::registerGeometryJson;
    using ::levilamroid::geometry::getGeometry;
    using ::levilamroid::geometry::hasGeometry;
    using ::levilamroid::geometry::getGeometryCount;
    using ::levilamroid::geometry::getAllGeometries;
}

extern "C" {
LL_API bool ll_geometry_register(const char* identifier, const char* jsonContent);
LL_API bool ll_geometry_has(const char* identifier);
LL_API bool ll_geometry_get(const char* identifier, char* outBuffer, size_t maxLen);
LL_API size_t ll_geometry_get_count();
}
