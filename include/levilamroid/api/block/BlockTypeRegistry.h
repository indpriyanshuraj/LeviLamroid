#pragma once
#include "ll/api/Macros.h"
#include "levilamroid/api/block/Block.h"
#include "levilamroid/api/block/BlockLegacy.h"
#include "levilamroid/api/block/BlockGroup.h"
#include "levilamroid/api/block/CustomBlockDefinition.h"
#include "levilamroid/api/math/HashedString.h"
#include <string>
#include <string_view>
#include <vector>

namespace levilamroid::block {

LL_API bool registerCustomBlock(const CustomBlockDefinition& def);
LL_API std::vector<CustomBlockDefinition> getRegisteredCustomBlocks();
LL_API const CustomBlockDefinition* getCustomBlock(std::string_view identifier);
LL_API bool isCustomBlock(std::string_view identifier);
LL_API size_t getCustomBlockCount();
LL_API void* getCustomBlockPermutation(std::string_view identifier);
LL_API void* getCustomBlockLegacy(std::string_view identifier);
LL_API void trackPlacedCustomBlock(int x, int y, int z, const std::string& identifier);
LL_API bool getTrackedCustomBlock(int x, int y, int z, std::string* outIdentifier);
LL_API bool untrackPlacedCustomBlock(int x, int y, int z, std::string* outIdentifier = nullptr);
LL_API bool isPlaceableItem(const std::string& id);
LL_API std::string getFunctionalBlockForCustomItem(const std::string& id);
LL_API const char* getPlacementSoundForBlock(const std::string& id);

class LL_API BlockRegistry {
public:
    static BlockRegistry& getInstance();

    bool registerBlock(const CustomBlockDefinition& def);
    const CustomBlockDefinition* getBlock(std::string_view identifier) const;
    std::vector<CustomBlockDefinition> getAllBlocks() const;
    bool hasBlock(std::string_view identifier) const;
    size_t getCount() const;
    void* getPermutation(std::string_view identifier) const;
    void* getLegacy(std::string_view identifier) const;
};

} // namespace levilamroid::block

namespace ll::block {
    using ::levilamroid::block::registerCustomBlock;
    using ::levilamroid::block::getRegisteredCustomBlocks;
    using ::levilamroid::block::getCustomBlock;
    using ::levilamroid::block::isCustomBlock;
    using ::levilamroid::block::getCustomBlockCount;
    using ::levilamroid::block::getCustomBlockPermutation;
    using ::levilamroid::block::getCustomBlockLegacy;
    using ::levilamroid::block::BlockRegistry;
}

extern "C" {

LL_API bool ll_block_register(
    const char* identifier,
    const char* displayName,
    const char* textureName,
    int category,
    const char* creativeGroup,
    float destroyTime,
    float explosionResistance,
    uint8_t lightEmission
);

LL_API bool ll_block_register_with_id(
    const char* identifier,
    short id,
    const char* displayName,
    const char* textureName,
    int category,
    const char* creativeGroup,
    float destroyTime,
    float explosionResistance,
    uint8_t lightEmission
);

LL_API bool ll_block_register_with_base(
    const char* identifier,
    const char* displayName,
    const char* textureName,
    const char* baseBlock,
    int category,
    const char* creativeGroup,
    float destroyTime,
    float explosionResistance,
    uint8_t lightEmission
);

LL_API bool ll_block_is_registered(const char* identifier);
LL_API size_t ll_block_get_count();
LL_API void* ll_block_get_permutation(const char* identifier);
LL_API void* ll_block_get_legacy(const char* identifier);
LL_API const char* ll_block_get_name(void* block);
LL_API uint32_t ll_block_get_runtime_id(void* block);
LL_API void ll_block_track_placed(int x, int y, int z, const char* identifier);
LL_API bool ll_block_get_tracked(int x, int y, int z, char* outIdentifier, size_t maxLen);
LL_API bool ll_block_untrack(int x, int y, int z);

// Custom Block Event Listeners
typedef bool (*LLBlockInteractCallback)(void* player, int x, int y, int z, uint8_t face, const char* blockIdent, const char* heldItemIdent, void* udata);
typedef bool (*LLBlockDestroyCallback)(void* player, int x, int y, int z, const char* blockIdent, const char* heldItemIdent, void* udata);
typedef bool (*LLBlockPlaceCallback)(void* player, int x, int y, int z, uint8_t face, bool isTouch, const char* blockIdent, void* udata);

LL_API bool ll_block_register_interact_listener(LLBlockInteractCallback callback, void* udata);
LL_API bool ll_block_register_destroy_listener(LLBlockDestroyCallback callback, void* udata);
LL_API bool ll_block_register_place_listener(LLBlockPlaceCallback callback, void* udata);

// Fake Block / Ghost Block Management APIs (Levimatica / Schematic Preview)
LL_API bool ll_fake_block_set(int x, int y, int z, const char* blockIdent, bool clientOnly);
LL_API bool ll_fake_block_clear(int x, int y, int z);
LL_API void ll_fake_block_clear_all();

// BlockSource & Dimension APIs
LL_API void* ll_block_source_get_block(void* blockSource, int x, int y, int z);
LL_API bool ll_block_source_set_block(void* blockSource, int x, int y, int z, void* blockPermutation, int updateFlags);
LL_API bool ll_block_source_set_ghost_block(void* blockSource, int x, int y, int z, void* blockPermutation);
LL_API bool ll_block_source_destroy_block(void* blockSource, int x, int y, int z, bool dropResources);
LL_API int ll_block_source_get_brightness(void* blockSource, int x, int y, int z);
LL_API void* ll_dimension_get_blocksource(void* dimension);
LL_API int ll_dimension_get_id(void* dimension);
LL_API void* ll_dimension_get_chunksource(void* dimension);
LL_API void ll_dimension_convert_coordinates(int fromDim, int toDim, float inX, float inZ, float* outX, float* outZ);

// Player & Entity Query APIs (LeviMap / Levimatica integration)
LL_API bool ll_player_get_position(void* player, float* outX, float* outY, float* outZ);
LL_API bool ll_player_get_rotation(void* player, float* outPitch, float* outYaw);
LL_API int ll_player_get_dimension_id(void* player);

// Chunk Analysis APIs (Slime chunks, Chunk coordinates)
LL_API bool ll_chunk_is_slime_chunk(int chunkX, int chunkZ, int64_t worldSeed);
LL_API void ll_chunk_get_coordinates(float playerX, float playerZ, int* outChunkX, int* outChunkZ, int* outRelX, int* outRelZ);

// Network Packet Interception & Replay Recording APIs (BedrockReplay integration)
typedef bool (*LLPacketListenerCallback)(int packetId, void* packet, bool isClientBound, void* udata);
LL_API bool ll_network_register_packet_listener(LLPacketListenerCallback callback, void* udata);
LL_API bool ll_network_unregister_packet_listener(LLPacketListenerCallback callback);
// Engine Service Singletons
LL_API void* ll_service_get_local_player();
LL_API void* ll_service_get_client_instance();
LL_API void* ll_service_get_level();
LL_API void* ll_service_get_block_source();

// Actor & Entity Query APIs
LL_API bool ll_actor_get_position(void* actor, float* outX, float* outY, float* outZ);
LL_API bool ll_actor_get_rotation(void* actor, float* outPitch, float* outYaw);
LL_API bool ll_actor_is_player(void* actor);
LL_API bool ll_actor_is_alive(void* actor);
LL_API bool ll_actor_hurt(void* actor, int damage);

}

