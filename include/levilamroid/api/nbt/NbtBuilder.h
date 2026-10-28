#pragma once
#include "ll/api/Macros.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/block/CustomBlockDefinition.h"
#include "levilamroid/api/entity/EntityRegistry.h"
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <cstdint>

namespace levilamroid::nbt {

// ── Binary VarInt & Primitive Encoding Utilities ──────────────────────────

LL_API void writeVarInt(std::string& buf, uint32_t val);
LL_API void writeSignedVarInt(std::string& buf, int32_t val);
LL_API void writeSignedVarLong(std::string& buf, int64_t val);
LL_API void writeString(std::string& buf, std::string_view str);
LL_API void writeU8(std::string& buf, uint8_t val);
LL_API void writeU16LE(std::string& buf, uint16_t val);
LL_API void writeU32LE(std::string& buf, uint32_t val);
LL_API void writeU64LE(std::string& buf, uint64_t val);
LL_API void writeFloatLE(std::string& buf, float val);
LL_API void writeDoubleLE(std::string& buf, double val);

LL_API uint32_t readVarInt(const uint8_t*& ptr, const uint8_t* end);
LL_API int32_t readSignedVarInt(const uint8_t*& ptr, const uint8_t* end);
LL_API std::string_view readString(const uint8_t*& ptr, const uint8_t* end);

// ── Tag Serialization (Bedrock Network NBT vs Little-Endian NBT) ──────────

LL_API void serializeTagNetwork(std::string& buf, const Tag* tag, std::string_view tagName = "");
LL_API std::string toNetworkNbt(const CompoundTag& tag);

LL_API void serializeTagLE(std::string& buf, const Tag* tag, std::string_view tagName = "");
LL_API std::string toBinaryNbtLE(const CompoundTag& tag);

LL_API std::unique_ptr<CompoundTag> fromNetworkNbt(std::string_view bytes);

// ── Geyser-Matched Schema Generators (Bedrock 1.26.44.3 - 1.26.52.3) ───────

class LL_API NbtBuilder {
public:
    // Builds the authentic data-driven NBT compound for ItemComponentPacket (Packet ID 162)
    static std::unique_ptr<CompoundTag> buildItemComponentNbt(const item::CustomItemDefinition& def);

    // Builds the authentic data-driven block NBT compound for StartGamePacket (Packet ID 11) blockProperties
    static std::unique_ptr<CompoundTag> buildBlockPropertyNbt(const block::CustomBlockDefinition& def);

    // Builds the entity definition NBT compound for AvailableActorIdentifiersPacket (Packet ID 119) idlist
    static std::unique_ptr<CompoundTag> buildEntityDefinitionNbt(const entity::CustomEntityDefinition& def, int runtimeId);

    // Serializes a complete ItemComponentPacket item definition entry:
    // [identifier (string)] [runtimeId (short LE)] [isComponentBased (bool)] [version (VarInt)] [components (Network NBT)]
    static void writeItemComponentEntry(std::string& stream, const item::CustomItemDefinition& def);

    // Serializes a full ItemComponentPacket payload given a list of item definitions
    static std::string serializeItemComponentPacketPayload(const std::vector<item::CustomItemDefinition>& items);
};

} // namespace levilamroid::nbt

namespace ll::nbt {
    using namespace ::levilamroid::nbt;
}
