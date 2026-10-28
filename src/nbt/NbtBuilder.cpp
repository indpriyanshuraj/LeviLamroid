#include "levilamroid/api/nbt/NbtBuilder.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <atomic>

namespace levilamroid::nbt {

// ── Binary VarInt & Primitive Encoding Utilities ──────────────────────────

void writeVarInt(std::string& buf, uint32_t val) {
    while (val >= 0x80) {
        buf.push_back(static_cast<char>((val & 0x7F) | 0x80));
        val >>= 7;
    }
    buf.push_back(static_cast<char>(val & 0x7F));
}

void writeSignedVarInt(std::string& buf, int32_t val) {
    uint32_t zigzag = (static_cast<uint32_t>(val) << 1) ^ static_cast<uint32_t>(val >> 31);
    writeVarInt(buf, zigzag);
}

void writeSignedVarLong(std::string& buf, int64_t val) {
    uint64_t zigzag = (static_cast<uint64_t>(val) << 1) ^ static_cast<uint64_t>(val >> 63);
    while (zigzag >= 0x80) {
        buf.push_back(static_cast<char>((zigzag & 0x7F) | 0x80));
        zigzag >>= 7;
    }
    buf.push_back(static_cast<char>(zigzag & 0x7F));
}

void writeString(std::string& buf, std::string_view str) {
    writeVarInt(buf, static_cast<uint32_t>(str.size()));
    buf.append(str);
}

void writeU8(std::string& buf, uint8_t val) {
    buf.push_back(static_cast<char>(val));
}

void writeU16LE(std::string& buf, uint16_t val) {
    buf.push_back(static_cast<char>(val & 0xFF));
    buf.push_back(static_cast<char>((val >> 8) & 0xFF));
}

void writeU32LE(std::string& buf, uint32_t val) {
    buf.push_back(static_cast<char>(val & 0xFF));
    buf.push_back(static_cast<char>((val >> 8) & 0xFF));
    buf.push_back(static_cast<char>((val >> 16) & 0xFF));
    buf.push_back(static_cast<char>((val >> 24) & 0xFF));
}

void writeU64LE(std::string& buf, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<char>((val >> (i * 8)) & 0xFF));
    }
}

void writeFloatLE(std::string& buf, float val) {
    uint32_t u;
    std::memcpy(&u, &val, sizeof(float));
    writeU32LE(buf, u);
}

void writeDoubleLE(std::string& buf, double val) {
    uint64_t u;
    std::memcpy(&u, &val, sizeof(double));
    writeU64LE(buf, u);
}

uint32_t readVarInt(const uint8_t*& ptr, const uint8_t* end) {
    uint32_t val = 0;
    int shift = 0;
    while (ptr < end) {
        uint8_t b = *ptr++;
        val |= static_cast<uint32_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) break;
        shift += 7;
        if (shift >= 35) break;
    }
    return val;
}

int32_t readSignedVarInt(const uint8_t*& ptr, const uint8_t* end) {
    uint32_t raw = readVarInt(ptr, end);
    return static_cast<int32_t>((raw >> 1) ^ (-(raw & 1)));
}

std::string_view readString(const uint8_t*& ptr, const uint8_t* end) {
    uint32_t len = readVarInt(ptr, end);
    if (ptr + len > end) len = static_cast<uint32_t>(end - ptr);
    std::string_view s(reinterpret_cast<const char*>(ptr), len);
    ptr += len;
    return s;
}

// ── Network NBT Serialization ─────────────────────────────────────────────

static void serializePayloadNetwork(std::string& buf, const Tag* tag) {
    if (!tag) return;
    switch (tag->getId()) {
        case Tag::Type::Byte: {
            auto t = static_cast<const ByteTag*>(tag);
            writeU8(buf, static_cast<uint8_t>(t->data));
            break;
        }
        case Tag::Type::Short: {
            auto t = static_cast<const ShortTag*>(tag);
            writeU16LE(buf, static_cast<uint16_t>(t->mVal));
            break;
        }
        case Tag::Type::Int: {
            auto t = static_cast<const IntTag*>(tag);
            writeSignedVarInt(buf, t->data);
            break;
        }
        case Tag::Type::Int64: {
            auto t = static_cast<const Int64Tag*>(tag);
            writeSignedVarLong(buf, t->mVal);
            break;
        }
        case Tag::Type::Float: {
            auto t = static_cast<const FloatTag*>(tag);
            writeFloatLE(buf, t->mVal);
            break;
        }
        case Tag::Type::Double: {
            auto t = static_cast<const DoubleTag*>(tag);
            writeDoubleLE(buf, t->mVal);
            break;
        }
        case Tag::Type::ByteArray: {
            auto t = static_cast<const ByteArrayTag*>(tag);
            writeSignedVarInt(buf, static_cast<int32_t>(t->mVal.size()));
            buf.append(reinterpret_cast<const char*>(t->mVal.data()), t->mVal.size());
            break;
        }
        case Tag::Type::String: {
            auto t = static_cast<const StringTag*>(tag);
            writeString(buf, t->mVal);
            break;
        }
        case Tag::Type::List: {
            auto t = static_cast<const ListTag*>(tag);
            uint8_t elemType = static_cast<uint8_t>(Tag::Type::End);
            if (!t->mList.empty()) {
                elemType = static_cast<uint8_t>(t->mList[0].getId());
            }
            writeU8(buf, elemType);
            writeSignedVarInt(buf, static_cast<int32_t>(t->mList.size()));
            for (const auto& item : t->mList) {
                if (item.mTag) {
                    serializePayloadNetwork(buf, item.mTag.get());
                }
            }
            break;
        }
        case Tag::Type::Compound: {
            auto t = static_cast<const CompoundTag*>(tag);
            for (const auto& [k, v] : t->mTags) {
                if (v.mTag && v.mTag->getId() != Tag::Type::End) {
                    serializeTagNetwork(buf, v.mTag.get(), k);
                }
            }
            writeU8(buf, 0x00); // TAG_End
            break;
        }
        case Tag::Type::IntArray: {
            auto t = static_cast<const IntArrayTag*>(tag);
            writeSignedVarInt(buf, static_cast<int32_t>(t->mVal.size()));
            for (int32_t v : t->mVal) {
                writeSignedVarInt(buf, v);
            }
            break;
        }
        default:
            break;
    }
}

void serializeTagNetwork(std::string& buf, const Tag* tag, std::string_view tagName) {
    if (!tag) return;
    writeU8(buf, static_cast<uint8_t>(tag->getId()));
    writeString(buf, tagName);
    serializePayloadNetwork(buf, tag);
}

std::string toNetworkNbt(const CompoundTag& tag) {
    std::string buf;
    // Root Compound: Tag Type 10, empty name, payload, TAG_End
    writeU8(buf, static_cast<uint8_t>(Tag::Type::Compound));
    writeString(buf, "");
    for (const auto& [k, v] : tag.mTags) {
        if (v.mTag && v.mTag->getId() != Tag::Type::End) {
            serializeTagNetwork(buf, v.mTag.get(), k);
        }
    }
    writeU8(buf, 0x00); // TAG_End
    return buf;
}

// ── Little-Endian (Standard Disk) NBT Serialization ───────────────────────

static void serializePayloadLE(std::string& buf, const Tag* tag) {
    if (!tag) return;
    switch (tag->getId()) {
        case Tag::Type::Byte: {
            auto t = static_cast<const ByteTag*>(tag);
            writeU8(buf, static_cast<uint8_t>(t->data));
            break;
        }
        case Tag::Type::Short: {
            auto t = static_cast<const ShortTag*>(tag);
            writeU16LE(buf, static_cast<uint16_t>(t->mVal));
            break;
        }
        case Tag::Type::Int: {
            auto t = static_cast<const IntTag*>(tag);
            writeU32LE(buf, static_cast<uint32_t>(t->data));
            break;
        }
        case Tag::Type::Int64: {
            auto t = static_cast<const Int64Tag*>(tag);
            writeU64LE(buf, static_cast<uint64_t>(t->mVal));
            break;
        }
        case Tag::Type::Float: {
            auto t = static_cast<const FloatTag*>(tag);
            writeFloatLE(buf, t->mVal);
            break;
        }
        case Tag::Type::Double: {
            auto t = static_cast<const DoubleTag*>(tag);
            writeDoubleLE(buf, t->mVal);
            break;
        }
        case Tag::Type::ByteArray: {
            auto t = static_cast<const ByteArrayTag*>(tag);
            writeU32LE(buf, static_cast<uint32_t>(t->mVal.size()));
            buf.append(reinterpret_cast<const char*>(t->mVal.data()), t->mVal.size());
            break;
        }
        case Tag::Type::String: {
            auto t = static_cast<const StringTag*>(tag);
            writeU16LE(buf, static_cast<uint16_t>(t->mVal.size()));
            buf.append(t->mVal);
            break;
        }
        case Tag::Type::List: {
            auto t = static_cast<const ListTag*>(tag);
            uint8_t elemType = static_cast<uint8_t>(Tag::Type::End);
            if (!t->mList.empty()) {
                elemType = static_cast<uint8_t>(t->mList[0].getId());
            }
            writeU8(buf, elemType);
            writeU32LE(buf, static_cast<uint32_t>(t->mList.size()));
            for (const auto& item : t->mList) {
                if (item.mTag) {
                    serializePayloadLE(buf, item.mTag.get());
                }
            }
            break;
        }
        case Tag::Type::Compound: {
            auto t = static_cast<const CompoundTag*>(tag);
            for (const auto& [k, v] : t->mTags) {
                if (v.mTag && v.mTag->getId() != Tag::Type::End) {
                    serializeTagLE(buf, v.mTag.get(), k);
                }
            }
            writeU8(buf, 0x00); // TAG_End
            break;
        }
        case Tag::Type::IntArray: {
            auto t = static_cast<const IntArrayTag*>(tag);
            writeU32LE(buf, static_cast<uint32_t>(t->mVal.size()));
            for (int32_t v : t->mVal) {
                writeU32LE(buf, static_cast<uint32_t>(v));
            }
            break;
        }
        default:
            break;
    }
}

void serializeTagLE(std::string& buf, const Tag* tag, std::string_view tagName) {
    if (!tag) return;
    writeU8(buf, static_cast<uint8_t>(tag->getId()));
    writeU16LE(buf, static_cast<uint16_t>(tagName.size()));
    buf.append(tagName);
    serializePayloadLE(buf, tag);
}

std::string toBinaryNbtLE(const CompoundTag& tag) {
    std::string buf;
    writeU8(buf, static_cast<uint8_t>(Tag::Type::Compound));
    writeU16LE(buf, 0); // empty root name
    for (const auto& [k, v] : tag.mTags) {
        if (v.mTag && v.mTag->getId() != Tag::Type::End) {
            serializeTagLE(buf, v.mTag.get(), k);
        }
    }
    writeU8(buf, 0x00); // TAG_End
    return buf;
}

// ── Network NBT Deserializer ──────────────────────────────────────────────

static std::unique_ptr<Tag> parseNetworkPayload(Tag::Type type, const uint8_t*& ptr, const uint8_t* end) {
    switch (type) {
        case Tag::Type::Byte: {
            if (ptr >= end) return nullptr;
            int8_t b = static_cast<int8_t>(*ptr++);
            return std::make_unique<ByteTag>(b);
        }
        case Tag::Type::Short: {
            if (ptr + 2 > end) return nullptr;
            int16_t s = static_cast<int16_t>(ptr[0] | (ptr[1] << 8));
            ptr += 2;
            return std::make_unique<ShortTag>(s);
        }
        case Tag::Type::Int: {
            int32_t val = readSignedVarInt(ptr, end);
            return std::make_unique<IntTag>(val);
        }
        case Tag::Type::Int64: {
            uint64_t val = 0;
            int shift = 0;
            while (ptr < end) {
                uint8_t b = *ptr++;
                val |= static_cast<uint64_t>(b & 0x7F) << shift;
                if ((b & 0x80) == 0) break;
                shift += 7;
                if (shift >= 70) break;
            }
            int64_t zigzag = static_cast<int64_t>((val >> 1) ^ (-(val & 1)));
            return std::make_unique<Int64Tag>(zigzag);
        }
        case Tag::Type::Float: {
            if (ptr + 4 > end) return nullptr;
            uint32_t u = static_cast<uint32_t>(ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24));
            ptr += 4;
            float f;
            std::memcpy(&f, &u, 4);
            return std::make_unique<FloatTag>(f);
        }
        case Tag::Type::Double: {
            if (ptr + 8 > end) return nullptr;
            uint64_t low = static_cast<uint32_t>(ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24));
            uint64_t high = static_cast<uint32_t>(ptr[4] | (ptr[5] << 8) | (ptr[6] << 16) | (ptr[7] << 24));
            ptr += 8;
            uint64_t u = low | (high << 32);
            double d;
            std::memcpy(&d, &u, 8);
            return std::make_unique<DoubleTag>(d);
        }
        case Tag::Type::ByteArray: {
            int32_t count = readSignedVarInt(ptr, end);
            if (count < 0 || ptr + count > end) return nullptr;
            std::vector<int8_t> arr(count);
            for (int32_t i = 0; i < count; ++i) {
                arr[i] = static_cast<int8_t>(*ptr++);
            }
            return std::make_unique<ByteArrayTag>(std::move(arr));
        }
        case Tag::Type::String: {
            std::string_view s = readString(ptr, end);
            return std::make_unique<StringTag>(std::string(s));
        }
        case Tag::Type::List: {
            if (ptr >= end) return nullptr;
            uint8_t elemType = *ptr++;
            int32_t count = readSignedVarInt(ptr, end);
            auto list = std::make_unique<ListTag>();
            if (count <= 0 || elemType == 0) return list;
            for (int32_t i = 0; i < count && ptr < end; ++i) {
                auto elem = parseNetworkPayload(static_cast<Tag::Type>(elemType), ptr, end);
                if (elem) list->add(std::move(elem));
            }
            return list;
        }
        case Tag::Type::Compound: {
            auto comp = std::make_unique<CompoundTag>();
            while (ptr < end) {
                uint8_t t = *ptr++;
                if (t == 0) break; // TAG_End
                std::string_view name = readString(ptr, end);
                auto val = parseNetworkPayload(static_cast<Tag::Type>(t), ptr, end);
                if (val) comp->put(std::string(name), std::move(val));
            }
            return comp;
        }
        case Tag::Type::IntArray: {
            int32_t count = readSignedVarInt(ptr, end);
            if (count < 0) return nullptr;
            std::vector<int32_t> arr;
            arr.reserve(count);
            for (int32_t i = 0; i < count && ptr < end; ++i) {
                arr.push_back(readSignedVarInt(ptr, end));
            }
            return std::make_unique<IntArrayTag>(std::move(arr));
        }
        default:
            return nullptr;
    }
}

std::unique_ptr<CompoundTag> fromNetworkNbt(std::string_view bytes) {
    if (bytes.size() < 2) return nullptr;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(bytes.data());
    const uint8_t* end = ptr + bytes.size();

    uint8_t rootType = *ptr++;
    if (rootType != static_cast<uint8_t>(Tag::Type::Compound)) return nullptr;

    (void)readString(ptr, end); // skip root name

    auto root = std::make_unique<CompoundTag>();
    while (ptr < end) {
        uint8_t t = *ptr++;
        if (t == 0) break; // TAG_End
        std::string_view name = readString(ptr, end);
        auto val = parseNetworkPayload(static_cast<Tag::Type>(t), ptr, end);
        if (val) root->put(std::string(name), std::move(val));
    }
    return root;
}

// ── Geyser-Matched Schema Generators ──────────────────────────────────────

std::unique_ptr<CompoundTag> NbtBuilder::buildItemComponentNbt(const item::CustomItemDefinition& def) {
    auto root = std::make_unique<CompoundTag>();
    root->putString("name", def.identifier);
    root->putInt("id", def.id);

    auto components = std::make_unique<CompoundTag>();
    auto itemProps = std::make_unique<CompoundTag>();

    // 1. Icon (if texture icon name is specified and not a 3D block)
    // Matches Geyser CustomItemRegistryPopulator.java:468
    if (!def.iconName.empty() && !def.isBlock) {
        auto iconComp = std::make_unique<CompoundTag>();
        auto texturesComp = std::make_unique<CompoundTag>();
        texturesComp->putString("default", def.iconName);
        iconComp->putCompound("textures", std::move(texturesComp));

        auto iconCompDirect = std::make_unique<CompoundTag>();
        auto texturesCompDirect = std::make_unique<CompoundTag>();
        texturesCompDirect->putString("default", def.iconName);
        iconCompDirect->putCompound("textures", std::move(texturesCompDirect));

        itemProps->putCompound("minecraft:icon", std::move(iconComp));
        components->putCompound("minecraft:icon", std::move(iconCompDirect));
    }

    // 2. Creative Category & Group (Geyser CustomItemRegistryPopulator.java:475)
    if (def.category != static_cast<int>(item::CreativeItemCategory::None)) {
        itemProps->putInt("creative_category", def.category);
        if (!def.creativeGroup.empty()) {
            itemProps->putString("creative_group", def.creativeGroup);
        }
    }

    // 3. Offhand & Hand Equipped (Geyser CustomItemRegistryPopulator.java:494)
    itemProps->putByte("allow_off_hand", def.allowOffhand ? 1 : 0);
    itemProps->putByte("hand_equipped", (!def.isBlock && def.isHandEquipped) ? 1 : 0);

    // 4. Max Stack Size (Geyser CustomItemRegistryPopulator.java:504-508)
    uint8_t stackSize = def.maxStackSize;
    if (stackSize == 0) stackSize = 64;
    itemProps->putInt("max_stack_size", stackSize);
    {
        auto stackComp = std::make_unique<CompoundTag>();
        stackComp->putByte("value", static_cast<int8_t>(stackSize));
        components->putCompound("minecraft:max_stack_size", std::move(stackComp));
    }

    // 5. Display Name (Geyser CustomItemRegistryPopulator.java:483)
    {
        std::string nameVal;
        if (!def.displayName.empty()) {
            nameVal = def.displayName;
        } else {
            auto colon = def.identifier.find(':');
            std::string s = (colon != std::string::npos) ? def.identifier.substr(colon + 1) : def.identifier;
            bool cap = true;
            for (char& c : s) {
                if (c == '_') {
                    c = ' ';
                    cap = true;
                } else if (cap && std::isalpha(static_cast<unsigned char>(c))) {
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    cap = false;
                }
            }
            nameVal = s;
        }
        auto nameComp = std::make_unique<CompoundTag>();
        nameComp->putString("value", nameVal);
        components->putCompound("minecraft:display_name", std::move(nameComp));
    }

    // 6. Attack Damage (minecraft:damage) (Geyser CustomItemRegistryPopulator.java:271)
    if (def.attackDamage > 0.0f) {
        int damageInt = static_cast<int>(def.attackDamage);
        itemProps->putInt("damage", damageInt);
        auto dmgComp = std::make_unique<CompoundTag>();
        dmgComp->putByte("value", static_cast<int8_t>(damageInt));
        components->putCompound("minecraft:damage", std::move(dmgComp));
    }

    // 7. Durability (minecraft:durability) (Geyser CustomItemRegistryPopulator.java:513)
    if (def.maxDamage > 0) {
        auto durComp = std::make_unique<CompoundTag>();
        auto chanceComp = std::make_unique<CompoundTag>();
        chanceComp->putInt("min", 1);
        chanceComp->putInt("max", 1);
        durComp->putCompound("damage_chance", std::move(chanceComp));
        durComp->putInt("max_durability", def.maxDamage);
        components->putCompound("minecraft:durability", std::move(durComp));
    }

    // 8. Food (minecraft:food) (Geyser CustomItemRegistryPopulator.java:665)
    if (!def.isBlock && (def.isFood || def.nutrition > 0)) {
        auto foodComp = std::make_unique<CompoundTag>();
        foodComp->putInt("nutrition", def.nutrition);
        foodComp->putFloat("saturation_modifier", def.saturationModifier);
        foodComp->putByte("can_always_eat", def.canAlwaysEat ? 1 : 0);
        foodComp->putCompound("using_converts_to", std::make_unique<CompoundTag>());
        components->putCompound("minecraft:food", std::move(foodComp));
    }

    // 9. Use Animation (Geyser CustomItemRegistryPopulator.java:650-660)
    std::string animStr = "none";
    int animId = def.isBlock ? 0 : def.useAnimation;
    if (animId == 0 && !def.isBlock && (def.isFood || def.nutrition > 0)) animId = 1;

    switch (animId) {
        case 1: animStr = "eat"; break;
        case 2: animStr = "drink"; break;
        case 3: animStr = "block"; break;
        case 4: animStr = "bow"; break;
        case 5: animStr = "camera"; break;
        case 6: animStr = "spear"; break;
        case 9: animStr = "crossbow"; break;
        case 10: animStr = "spyglass"; break;
        case 11: animStr = "goat_horn"; break;
        case 12: animStr = "brush"; break;
        default: animStr = (!def.isBlock && (def.isFood || def.nutrition > 0)) ? "eat" : "none"; break;
    }
    if (animStr != "none") {
        itemProps->putInt("use_animation", animId);
        auto animComp = std::make_unique<CompoundTag>();
        animComp->putString("value", animStr);
        components->putCompound("minecraft:use_animation", std::move(animComp));
    }

    // 10. Use Modifiers & Duration (Geyser CustomItemRegistryPopulator.java:404, 731-737)
    // Per Geyser rule: only apply use_duration/use_modifiers to consumables, chargeables, or explicit use animations.
    // Weapons/swords (category == 3) must never have use_modifiers unless explicitly food or chargeable.
    bool isConsumable = !def.isBlock && (def.isFood || def.nutrition > 0);
    bool isExplicitUsable = !def.isBlock && (def.useAnimation > 0);
    bool isWeapon = (def.category == 3 && def.attackDamage > 0.0f);
    if (!def.isBlock && !isWeapon && (isConsumable || isExplicitUsable || def.useDuration > 0)) {
        float durationSec = (def.useDuration > 0) ? (static_cast<float>(def.useDuration) / 20.0f) : 1.6f;
        itemProps->putInt("use_duration", static_cast<int>(durationSec * 20.0f));
        auto modComp = std::make_unique<CompoundTag>();
        modComp->putFloat("movement_modifier", 0.2f);
        modComp->putFloat("use_duration", durationSec);
        components->putCompound("minecraft:use_modifiers", std::move(modComp));
    }

    // 11. Block Placer (minecraft:block_placer) (Geyser CustomItemRegistryPopulator.java:616)
    if (def.isBlock || !def.baseBlock.empty()) {
        auto placerComp = std::make_unique<CompoundTag>();
        std::string targetBlock = def.isBlock ? def.identifier : (!def.baseBlock.empty() ? def.baseBlock : def.identifier);
        placerComp->putString("block", targetBlock);
        placerComp->putByte("canUseBlockAsIcon", def.isBlock ? 1 : (def.iconName.empty() ? 1 : 0));
        placerComp->put("use_on", std::make_unique<ListTag>());
        components->putCompound("minecraft:block_placer", std::move(placerComp));
    }

    // 12. Tool Digger (minecraft:digger) (Geyser CustomItemRegistryPopulator.java:528-546)
    if (def.miningSpeed > 1.0f) {
        itemProps->putFloat("mining_speed", def.miningSpeed);
        auto diggerComp = std::make_unique<CompoundTag>();
        diggerComp->putByte("use_efficiency", 0);
        auto destroySpeeds = std::make_unique<ListTag>();
        auto speedEntry = std::make_unique<CompoundTag>();
        auto blockCompound = std::make_unique<CompoundTag>();
        blockCompound->putString("name", "");
        blockCompound->putCompound("states", std::make_unique<CompoundTag>());
        blockCompound->putString("tags", "1");
        speedEntry->putCompound("block", std::move(blockCompound));
        speedEntry->putInt("speed", static_cast<int>(def.miningSpeed));
        destroySpeeds->add(std::move(speedEntry));
        diggerComp->put("destroy_speeds", std::move(destroySpeeds));
        components->putCompound("minecraft:digger", std::move(diggerComp));
    }

    // 13. Cooldown (minecraft:cooldown) (Geyser CustomItemRegistryPopulator.java:696-702)
    if (def.cooldownDuration > 0.0f) {
        auto cdComp = std::make_unique<CompoundTag>();
        std::string cat = !def.cooldownCategory.empty() ? def.cooldownCategory : def.identifier;
        cdComp->putString("category", cat);
        cdComp->putFloat("duration", def.cooldownDuration);
        components->putCompound("minecraft:cooldown", std::move(cdComp));
    }

    // 14. Item Tags (Geyser CustomItemRegistryPopulator.java:486, 775)
    // Over Bedrock network, item tags are sent inside components as a List of strings under "item_tags"
    auto itemTagsList = std::make_unique<ListTag>();
    itemTagsList->add(std::make_unique<StringTag>("geyser:is_custom"));
    itemTagsList->add(std::make_unique<StringTag>("levilamroid:is_custom"));
    if (def.attackDamage > 0.0f) {
        itemTagsList->add(std::make_unique<StringTag>("minecraft:is_sword"));
    }
    if (!def.isBlock && (def.isFood || def.nutrition > 0)) {
        itemTagsList->add(std::make_unique<StringTag>("minecraft:is_food"));
    }
    components->put("item_tags", std::move(itemTagsList));

    components->putCompound("item_properties", std::move(itemProps));
    root->putCompound("components", std::move(components));
    return root;
}

std::unique_ptr<CompoundTag> NbtBuilder::buildBlockPropertyNbt(const block::CustomBlockDefinition& def) {
    auto root = std::make_unique<CompoundTag>();
    root->putInt("molangVersion", 1);

    // Menu category (Geyser CustomBlockRegistryPopulator.java:377-381)
    auto menuCategory = std::make_unique<CompoundTag>();
    std::string catName = "construction";
    switch (static_cast<item::CreativeItemCategory>(def.category)) {
        case item::CreativeItemCategory::Construction: catName = "construction"; break;
        case item::CreativeItemCategory::Nature: catName = "nature"; break;
        case item::CreativeItemCategory::Equipment: catName = "equipment"; break;
        case item::CreativeItemCategory::Items: catName = "items"; break;
        default: catName = "construction"; break;
    }
    menuCategory->putString("category", catName);
    menuCategory->putString("group", !def.creativeGroup.empty() ? def.creativeGroup : "itemGroup.name.stone");
    menuCategory->putByte("is_hidden_in_commands", 0);
    root->putCompound("menu_category", std::move(menuCategory));

    // Vanilla block data with sequential block_id starting at 10000 (Geyser CustomBlockRegistryPopulator.java:88, 386)
    static std::atomic<int> sBlockIdAllocator{10000};
    int allocatedId = (def.blockId != 0) ? def.blockId : sBlockIdAllocator.fetch_add(1);
    auto vanillaData = std::make_unique<CompoundTag>();
    vanillaData->putInt("block_id", allocatedId);
    root->putCompound("vanilla_block_data", std::move(vanillaData));

    // Permutations and properties lists
    root->put("permutations", std::make_unique<ListTag>());
    root->put("properties", std::make_unique<ListTag>());

    // Components (Geyser CustomBlockRegistryPopulator.java:399-534)
    auto components = std::make_unique<CompoundTag>();

    // 1. Display Name
    {
        std::string bName = !def.displayName.empty() ? def.displayName : "";
        if (bName.empty()) {
            auto colon = def.identifier.find(':');
            std::string s = (colon != std::string::npos) ? def.identifier.substr(colon + 1) : def.identifier;
            bool cap = true;
            for (char& c : s) {
                if (c == '_') {
                    c = ' ';
                    cap = true;
                } else if (cap && std::isalpha(static_cast<unsigned char>(c))) {
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    cap = false;
                }
            }
            bName = s;
        }
        auto nameComp = std::make_unique<CompoundTag>();
        nameComp->putString("value", bName);
        components->putCompound("minecraft:display_name", std::move(nameComp));
    }

    // 2. Geometry
    std::string geomId = !def.geometry.empty() ? def.geometry : "minecraft:geometry.full_block";
    auto geomComp = std::make_unique<CompoundTag>();
    geomComp->putString("identifier", geomId);
    components->putCompound("minecraft:geometry", std::move(geomComp));

    // 3. Material Instances (Geyser CustomBlockRegistryPopulator.java:434-468)
    std::string tex = !def.textureName.empty() ? def.textureName : def.identifier;
    auto matInstances = std::make_unique<CompoundTag>();
    matInstances->putCompound("mappings", std::make_unique<CompoundTag>());
    auto materials = std::make_unique<CompoundTag>();
    auto starMat = std::make_unique<CompoundTag>();
    starMat->putString("texture", tex);

    std::string renderMethod = "opaque";
    std::string lowId = def.identifier;
    std::transform(lowId.begin(), lowId.end(), lowId.begin(), [](unsigned char c){ return std::tolower(c); });
    if (lowId.find("glass") != std::string::npos || lowId.find("ice") != std::string::npos ||
        lowId.find("trans") != std::string::npos) {
        renderMethod = "blend";
    } else if (lowId.find("crop") != std::string::npos || lowId.find("plant") != std::string::npos ||
               lowId.find("flower") != std::string::npos || lowId.find("wild") != std::string::npos ||
               lowId.find("shrub") != std::string::npos || lowId.find("sapling") != std::string::npos ||
               lowId.find("net") != std::string::npos || lowId.find("chain") != std::string::npos ||
               lowId.find("leaves") != std::string::npos || !def.isSolid) {
        renderMethod = "alpha_test";
    }
    starMat->putString("render_method", renderMethod);
    starMat->putFloat("ambient_occlusion", (renderMethod == "opaque") ? 1.0f : 0.0f); // Float required since Bedrock 1.26.20+
    starMat->putByte("isotropic", 0);
    starMat->putByte("packed_bools", 1);
    materials->putCompound("*", std::move(starMat));
    matInstances->putCompound("materials", std::move(materials));
    components->putCompound("minecraft:material_instances", std::move(matInstances));

    // 4. Destructible by mining ("value" float in seconds, Geyser line 479)
    auto miningComp = std::make_unique<CompoundTag>();
    miningComp->putFloat("value", def.destroyTime > 0.0f ? def.destroyTime : 1.5f);
    components->putCompound("minecraft:destructible_by_mining", std::move(miningComp));

    // 5. Destructible by explosion
    auto explComp = std::make_unique<CompoundTag>();
    explComp->putFloat("value", def.explosionResistance > 0.0f ? def.explosionResistance : 6.0f);
    components->putCompound("minecraft:destructible_by_explosion", std::move(explComp));

    // 6. Friction ("value" float, Geyser line 486)
    auto frictionComp = std::make_unique<CompoundTag>();
    frictionComp->putFloat("value", def.friction > 0.0f ? def.friction : 0.6f);
    components->putCompound("minecraft:friction", std::move(frictionComp));

    // 7. Light emission ("emission" byte, Geyser line 493)
    if (def.lightEmission > 0) {
        auto lightComp = std::make_unique<CompoundTag>();
        lightComp->putByte("emission", static_cast<int8_t>(def.lightEmission));
        components->putCompound("minecraft:light_emission", std::move(lightComp));
    }

    // 8. Selection Box (Geyser CustomBlockRegistryPopulator.java:577-580)
    auto selBox = std::make_unique<CompoundTag>();
    selBox->putByte("enabled", 1);
    auto selOrigin = std::make_unique<ListTag>();
    selOrigin->add(std::make_unique<FloatTag>(-8.0f));
    selOrigin->add(std::make_unique<FloatTag>(0.0f));
    selOrigin->add(std::make_unique<FloatTag>(-8.0f));
    selBox->put("origin", std::move(selOrigin));
    auto selSize = std::make_unique<ListTag>();
    selSize->add(std::make_unique<FloatTag>(16.0f));
    selSize->add(std::make_unique<FloatTag>(16.0f));
    selSize->add(std::make_unique<FloatTag>(16.0f));
    selBox->put("size", std::move(selSize));
    components->putCompound("minecraft:selection_box", std::move(selBox));

    // 9. Collision Box (Geyser CustomBlockRegistryPopulator.java:542-569)
    if (def.isSolid) {
        auto colBox = std::make_unique<CompoundTag>();
        colBox->putByte("enabled", 1);
        auto boxesList = std::make_unique<ListTag>();
        auto boxEntry = std::make_unique<CompoundTag>();
        boxEntry->putFloat("minX", 0.0f);
        boxEntry->putFloat("minY", 0.0f);
        boxEntry->putFloat("minZ", 0.0f);
        boxEntry->putFloat("maxX", 16.0f);
        boxEntry->putFloat("maxY", 16.0f);
        boxEntry->putFloat("maxZ", 16.0f);
        boxesList->add(std::move(boxEntry));
        colBox->put("boxes", std::move(boxesList));
        components->putCompound("minecraft:collision_box", std::move(colBox));
    }

    // 10. Tags (Geyser CustomBlockRegistryPopulator.java:530)
    // Block tags are sent as components with empty compound values
    components->putCompound("tag:geyser:is_custom", std::make_unique<CompoundTag>());
    components->putCompound("tag:levilamroid:is_custom", std::make_unique<CompoundTag>());

    root->putCompound("components", std::move(components));
    return root;
}

std::unique_ptr<CompoundTag> NbtBuilder::buildEntityDefinitionNbt(const entity::CustomEntityDefinition& def, int runtimeId) {
    auto tag = std::make_unique<CompoundTag>();
    tag->putByte("hasSpawnEgg", !def.spawnEggItem.empty() ? 1 : 0);
    tag->putString("id", def.identifier);
    tag->putByte("summonable", def.isSummonable ? 1 : 0);
    tag->putString("bid", !def.runtimeId.empty() ? def.runtimeId : "");
    tag->putInt("rid", runtimeId);
    tag->putByte("experimental", 0);
    return tag;
}

void NbtBuilder::writeItemComponentEntry(std::string& stream, const item::CustomItemDefinition& def) {
    // 1. identifier: string (VarInt len + utf8)
    writeString(stream, def.identifier);
    // 2. runtimeId: int16_t Little Endian
    writeU16LE(stream, static_cast<uint16_t>(def.id));
    // 3. isComponentBased: bool (1 byte: 1)
    writeU8(stream, 1);
    // 4. itemVersion: VarInt (2 = DataDriven)
    writeVarInt(stream, 2);
    // 5. components: Network NBT CompoundTag
    auto nbt = buildItemComponentNbt(def);
    if (nbt) {
        std::string nbtBytes = toNetworkNbt(*nbt);
        stream.append(nbtBytes);
    } else {
        CompoundTag emptyTag;
        std::string nbtBytes = toNetworkNbt(emptyTag);
        stream.append(nbtBytes);
    }
}

std::string NbtBuilder::serializeItemComponentPacketPayload(const std::vector<item::CustomItemDefinition>& items) {
    std::string stream;
    writeVarInt(stream, static_cast<uint32_t>(items.size()));
    for (const auto& item : items) {
        writeItemComponentEntry(stream, item);
    }
    return stream;
}

} // namespace levilamroid::nbt
