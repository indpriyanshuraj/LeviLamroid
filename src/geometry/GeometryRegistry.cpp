#include "levilamroid/api/geometry/GeometryRegistry.h"
#include "levilamroid/api/Logger.h"
#include <vector>
#include <mutex>
#include <cstring>
#include <algorithm>

namespace levilamroid::geometry {

static std::vector<CustomGeometryDefinition> sGeometries;
static std::mutex sGeometryMutex;

bool registerGeometry(const CustomGeometryDefinition& def) {
    std::lock_guard<std::mutex> lock(sGeometryMutex);
    for (auto& g : sGeometries) {
        if (g.identifier == def.identifier) {
            g = def;
            return true;
        }
    }
    sGeometries.push_back(def);

    auto& logger = ll::Logger::getInstance();
    logger.info("§6[LeviLamroid] §aRegistered custom geometry definition: §b", def.identifier,
                " §7(Format: ", def.formatVersion, ", Size: ", def.jsonContent.size(), " bytes)");
    return true;
}

bool registerGeometryJson(std::string_view identifier, std::string_view jsonContent) {
    CustomGeometryDefinition def;
    def.identifier = identifier;
    def.jsonContent = jsonContent;
    return registerGeometry(def);
}

const CustomGeometryDefinition* getGeometry(std::string_view identifier) {
    std::lock_guard<std::mutex> lock(sGeometryMutex);
    for (const auto& g : sGeometries) {
        if (g.identifier == identifier) return &g;
        auto dot = g.identifier.rfind('.');
        if (dot != std::string_view::npos && g.identifier.substr(dot + 1) == identifier) {
            return &g;
        }
        auto qdot = identifier.rfind('.');
        if (qdot != std::string_view::npos && identifier.substr(qdot + 1) == g.identifier) {
            return &g;
        }
    }
    return nullptr;
}

bool hasGeometry(std::string_view identifier) {
    return getGeometry(identifier) != nullptr;
}

size_t getGeometryCount() {
    std::lock_guard<std::mutex> lock(sGeometryMutex);
    return sGeometries.size();
}

std::vector<CustomGeometryDefinition> getAllGeometries() {
    std::lock_guard<std::mutex> lock(sGeometryMutex);
    return sGeometries;
}

GeometryRegistry& GeometryRegistry::getInstance() {
    static GeometryRegistry instance;
    return instance;
}

bool GeometryRegistry::registerGeometry(const CustomGeometryDefinition& def) {
    return ::levilamroid::geometry::registerGeometry(def);
}

bool GeometryRegistry::registerJson(std::string_view identifier, std::string_view jsonContent) {
    return ::levilamroid::geometry::registerGeometryJson(identifier, jsonContent);
}

const CustomGeometryDefinition* GeometryRegistry::get(std::string_view identifier) const {
    return ::levilamroid::geometry::getGeometry(identifier);
}

bool GeometryRegistry::has(std::string_view identifier) const {
    return ::levilamroid::geometry::hasGeometry(identifier);
}

size_t GeometryRegistry::getCount() const {
    return ::levilamroid::geometry::getGeometryCount();
}

std::vector<CustomGeometryDefinition> GeometryRegistry::getAll() const {
    return ::levilamroid::geometry::getAllGeometries();
}

} // namespace levilamroid::geometry

extern "C" {

LL_API bool ll_geometry_register(const char* identifier, const char* jsonContent) {
    if (!identifier || !jsonContent) return false;
    return ::levilamroid::geometry::registerGeometryJson(identifier, jsonContent);
}

LL_API bool ll_geometry_has(const char* identifier) {
    if (!identifier) return false;
    return ::levilamroid::geometry::hasGeometry(identifier);
}

LL_API bool ll_geometry_get(const char* identifier, char* outBuffer, size_t maxLen) {
    if (!identifier || !outBuffer || maxLen == 0) return false;
    const auto* g = ::levilamroid::geometry::getGeometry(identifier);
    if (!g) return false;
    std::strncpy(outBuffer, g->jsonContent.c_str(), maxLen - 1);
    outBuffer[maxLen - 1] = '\0';
    return true;
}

LL_API size_t ll_geometry_get_count() {
    return ::levilamroid::geometry::getGeometryCount();
}

}
