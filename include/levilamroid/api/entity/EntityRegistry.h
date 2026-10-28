#pragma once
#include "ll/api/Macros.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>

namespace levilamroid::entity {

struct CustomEntityDefinition {
    std::string identifier;        // e.g. "mymod:custom_mob"
    std::string runtimeId;         // Base vanilla entity ID fallback (e.g. "minecraft:zombie")
    std::string displayName;       // Localized display name
    std::string spawnEggItem;      // Associated spawn egg identifier (e.g. "mymod:custom_mob_spawn_egg")
    std::string geometry;          // Geometry identifier (e.g. "geometry.custom_mob")
    std::string texture;           // Texture path/alias
    float width{0.6f};
    float height{1.8f};
    float maxHealth{20.0f};
    bool isSummonable{true};
    bool isSpawnable{true};
};

typedef void (*LLEntitySpawnCallback)(void* actor, const char* identifier, float x, float y, float z, void* udata);
typedef bool (*LLEntityHurtCallback)(void* actor, const char* identifier, void* damageSource, float damage, void* udata);
typedef void (*LLEntityDeathCallback)(void* actor, const char* identifier, void* damageSource, void* udata);

LL_API bool registerEntity(const CustomEntityDefinition& def);
LL_API const CustomEntityDefinition* getEntity(std::string_view identifier);
LL_API bool hasEntity(std::string_view identifier);
LL_API size_t getEntityCount();
LL_API std::vector<CustomEntityDefinition> getAllEntities();

LL_API bool registerSpawnListener(LLEntitySpawnCallback callback, void* udata);
LL_API bool registerHurtListener(LLEntityHurtCallback callback, void* udata);
LL_API bool registerDeathListener(LLEntityDeathCallback callback, void* udata);

LL_API void dispatchEntitySpawn(void* actor, const char* identifier, float x, float y, float z);
LL_API bool dispatchEntityHurt(void* actor, const char* identifier, void* damageSource, float damage);
LL_API void dispatchEntityDeath(void* actor, const char* identifier, void* damageSource);

class LL_API EntityRegistry {
public:
    static EntityRegistry& getInstance();

    bool registerEntity(const CustomEntityDefinition& def);
    const CustomEntityDefinition* get(std::string_view identifier) const;
    bool has(std::string_view identifier) const;
    size_t getCount() const;
    std::vector<CustomEntityDefinition> getAll() const;
};

} // namespace levilamroid::entity

namespace ll::entity {
    using CustomEntityDefinition = ::levilamroid::entity::CustomEntityDefinition;
    using EntityRegistry = ::levilamroid::entity::EntityRegistry;
    using ::levilamroid::entity::registerEntity;
    using ::levilamroid::entity::getEntity;
    using ::levilamroid::entity::hasEntity;
    using ::levilamroid::entity::getEntityCount;
    using ::levilamroid::entity::getAllEntities;
    using ::levilamroid::entity::registerSpawnListener;
    using ::levilamroid::entity::registerHurtListener;
    using ::levilamroid::entity::registerDeathListener;
}

extern "C" {

LL_API bool ll_entity_register(
    const char* identifier,
    const char* displayName,
    const char* runtimeBaseId,
    float width,
    float height,
    float maxHealth,
    bool isSummonable
);

LL_API bool ll_entity_spawn(const char* identifier, float x, float y, float z);
LL_API bool ll_entity_is_registered(const char* identifier);
LL_API size_t ll_entity_get_count();

LL_API bool ll_entity_register_spawn_listener(::levilamroid::entity::LLEntitySpawnCallback callback, void* udata);
LL_API bool ll_entity_register_hurt_listener(::levilamroid::entity::LLEntityHurtCallback callback, void* udata);
LL_API bool ll_entity_register_death_listener(::levilamroid::entity::LLEntityDeathCallback callback, void* udata);

}
