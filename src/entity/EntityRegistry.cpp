#include "levilamroid/api/entity/EntityRegistry.h"
#include "levilamroid/api/Logger.h"
#include <vector>
#include <mutex>
#include <algorithm>
#include <cstdio>

extern "C" LL_API bool ll_command_dispatch_client(const char* command);
namespace levilamroid::hooks {
    bool executeServerCommand(const std::string& rawCmd);
}

namespace levilamroid::entity {

static std::vector<CustomEntityDefinition> sEntities;
static std::mutex sEntityMutex;

struct SpawnListener {
    LLEntitySpawnCallback cb;
    void* udata;
};
static std::vector<SpawnListener> sSpawnListeners;
static std::mutex sSpawnMutex;

struct HurtListener {
    LLEntityHurtCallback cb;
    void* udata;
};
static std::vector<HurtListener> sHurtListeners;
static std::mutex sHurtMutex;

struct DeathListener {
    LLEntityDeathCallback cb;
    void* udata;
};
static std::vector<DeathListener> sDeathListeners;
static std::mutex sDeathMutex;

bool registerEntity(const CustomEntityDefinition& def) {
    std::lock_guard<std::mutex> lock(sEntityMutex);
    for (auto& e : sEntities) {
        if (e.identifier == def.identifier) {
            e = def;
            return true;
        }
    }
    sEntities.push_back(def);

    auto& logger = ll::Logger::getInstance();
    logger.info("§6[LeviLamroid] §aRegistered custom entity definition: §b", def.identifier,
                " §7(RuntimeId: ", def.runtimeId, 
                ", Health: ", def.maxHealth, 
                ", Dim: ", def.width, "x", def.height, ")");
    return true;
}

const CustomEntityDefinition* getEntity(std::string_view identifier) {
    std::lock_guard<std::mutex> lock(sEntityMutex);
    for (const auto& e : sEntities) {
        if (e.identifier == identifier) return &e;
        auto colon = e.identifier.find(':');
        if (colon != std::string_view::npos && e.identifier.substr(colon + 1) == identifier) {
            return &e;
        }
    }
    return nullptr;
}

bool hasEntity(std::string_view identifier) {
    return getEntity(identifier) != nullptr;
}

size_t getEntityCount() {
    std::lock_guard<std::mutex> lock(sEntityMutex);
    return sEntities.size();
}

std::vector<CustomEntityDefinition> getAllEntities() {
    std::lock_guard<std::mutex> lock(sEntityMutex);
    return sEntities;
}

bool registerSpawnListener(LLEntitySpawnCallback callback, void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(sSpawnMutex);
    sSpawnListeners.push_back({callback, udata});
    return true;
}

bool registerHurtListener(LLEntityHurtCallback callback, void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(sHurtMutex);
    sHurtListeners.push_back({callback, udata});
    return true;
}

bool registerDeathListener(LLEntityDeathCallback callback, void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(sDeathMutex);
    sDeathListeners.push_back({callback, udata});
    return true;
}

void dispatchEntitySpawn(void* actor, const char* identifier, float x, float y, float z) {
    std::vector<SpawnListener> copies;
    {
        std::lock_guard<std::mutex> lock(sSpawnMutex);
        copies = sSpawnListeners;
    }
    for (const auto& l : copies) {
        if (l.cb) l.cb(actor, identifier, x, y, z, l.udata);
    }
}

bool dispatchEntityHurt(void* actor, const char* identifier, void* damageSource, float damage) {
    std::vector<HurtListener> copies;
    {
        std::lock_guard<std::mutex> lock(sHurtMutex);
        copies = sHurtListeners;
    }
    for (const auto& l : copies) {
        if (l.cb && l.cb(actor, identifier, damageSource, damage, l.udata)) return true;
    }
    return false;
}

void dispatchEntityDeath(void* actor, const char* identifier, void* damageSource) {
    std::vector<DeathListener> copies;
    {
        std::lock_guard<std::mutex> lock(sDeathMutex);
        copies = sDeathListeners;
    }
    for (const auto& l : copies) {
        if (l.cb) l.cb(actor, identifier, damageSource, l.udata);
    }
}

EntityRegistry& EntityRegistry::getInstance() {
    static EntityRegistry instance;
    return instance;
}

bool EntityRegistry::registerEntity(const CustomEntityDefinition& def) {
    return ::levilamroid::entity::registerEntity(def);
}

const CustomEntityDefinition* EntityRegistry::get(std::string_view identifier) const {
    return ::levilamroid::entity::getEntity(identifier);
}

bool EntityRegistry::has(std::string_view identifier) const {
    return ::levilamroid::entity::hasEntity(identifier);
}

size_t EntityRegistry::getCount() const {
    return ::levilamroid::entity::getEntityCount();
}

std::vector<CustomEntityDefinition> EntityRegistry::getAll() const {
    return ::levilamroid::entity::getAllEntities();
}

} // namespace levilamroid::entity

extern "C" {

LL_API bool ll_entity_register(
    const char* identifier,
    const char* displayName,
    const char* runtimeBaseId,
    float width,
    float height,
    float maxHealth,
    bool isSummonable
) {
    if (!identifier) return false;
    ::levilamroid::entity::CustomEntityDefinition def;
    def.identifier = identifier;
    def.displayName = displayName ? displayName : identifier;
    def.runtimeId = (runtimeBaseId && runtimeBaseId[0] != '\0') ? runtimeBaseId : "minecraft:zombie";
    def.width = (width > 0.0f) ? width : 0.6f;
    def.height = (height > 0.0f) ? height : 1.8f;
    def.maxHealth = (maxHealth > 0.0f) ? maxHealth : 20.0f;
    def.isSummonable = isSummonable;
    return ::levilamroid::entity::registerEntity(def);
}

LL_API bool ll_entity_spawn(const char* identifier, float x, float y, float z) {
    if (!identifier) return false;
    char cmd[256];
    std::snprintf(cmd, sizeof(cmd), "/summon %s %.2f %.2f %.2f", identifier, x, y, z);
    if (!::levilamroid::hooks::executeServerCommand(cmd)) {
        return ll_command_dispatch_client(cmd);
    }
    return true;
}

LL_API bool ll_entity_is_registered(const char* identifier) {
    if (!identifier) return false;
    return ::levilamroid::entity::hasEntity(identifier);
}

LL_API size_t ll_entity_get_count() {
    return ::levilamroid::entity::getEntityCount();
}

LL_API bool ll_entity_register_spawn_listener(::levilamroid::entity::LLEntitySpawnCallback callback, void* udata) {
    return ::levilamroid::entity::registerSpawnListener(callback, udata);
}

LL_API bool ll_entity_register_hurt_listener(::levilamroid::entity::LLEntityHurtCallback callback, void* udata) {
    return ::levilamroid::entity::registerHurtListener(callback, udata);
}

LL_API bool ll_entity_register_death_listener(::levilamroid::entity::LLEntityDeathCallback callback, void* udata) {
    return ::levilamroid::entity::registerDeathListener(callback, udata);
}

}
