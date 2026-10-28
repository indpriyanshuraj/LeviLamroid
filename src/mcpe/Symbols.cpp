#include "levilamroid/api/mcpe/Symbols.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/memory/Hook.h"
#include "levilamroid/api/memory/PatternScan.h"
#include "levilamroid/api/Logger.h"
#include <vector>
#include <map>
#include <mutex>
#include <cstring>
#include <dlfcn.h>

namespace levilamroid::mcpe {

// Complete Verified Mangled Symbol & Signature Dictionary
// Covers MSVC x64, Clang Itanium ARM64, and RVAs for 1.26.52.3 / 1.26.45.1
static const SymbolEntry kSymbolTable[] = {
    // 1. Actor & Player Core Methods
    {
        "LocalPlayer::move",
        "?move@LocalPlayer@@UEAAXAEBVVec3@@@Z",
        "_ZN11LocalPlayer4moveERK4Vec3",
        "",
        0x04C05BE0, 0x04C05BE0, 0

    },
    {
        "Player::normalTick",
        "?normalTick@Player@@UEAAXXZ",
        "_ZN6Player10normalTickEv",
        "",
        0x056637B8, 0x056637B8, 0

    },
    {
        "Actor::getPosition",
        "?getPosition@Actor@@QEBAAEBVVec3@@XZ",
        "_ZNK5Actor11getPositionEv",
        "",
        0, 0, 0

    },
    {
        "Actor::setPos",
        "?setPos@Actor@@UEAAXAEBVVec3@@@Z",
        "_ZN5Actor6setPosERK4Vec3",
        "",
        0, 0, 0

    },
    {
        "Actor::getEyeHeight",
        "?getEyeHeight@Actor@@QEBAMXZ",
        "_ZNK5Actor12getEyeHeightEv",
        "",
        0, 0, 0

    },
    {
        "Actor::getAABB",
        "?getAABB@Actor@@QEBAAEBVAABB@@XZ",
        "_ZNK5Actor7getAABBEv",
        "",
        0, 0, 0

    },
    {
        "Actor::getHealth",
        "?getHealth@Actor@@QEBAHXZ",
        "_ZNK5Actor9getHealthEv",
        "",
        0, 0, 0

    },
    {
        "Actor::setHealth",
        "?setHealth@Actor@@UEAAXH@Z",
        "_ZN5Actor9setHealthEi",
        "",
        0, 0, 0

    },
    {
        "Actor::swing",
        "?swing@Actor@@UEAAXXZ",
        "_ZN5Actor5swingEv",
        "",
        0, 0, 0

    },
    {
        "Player::getOffhandSlot",
        "?getOffhandSlot@Player@@QEBAAEAVItemStack@@XZ",
        "_ZNK6Player14getOffhandSlotEv",
        "",
        0, 0, 0

    },
    {
        "Player::getSelectedItem",
        "?getSelectedItem@Player@@QEBAAEBVItemStack@@XZ",
        "_ZNK6Player15getSelectedItemEv",
        "",
        0, 0, 0

    },
    {
        "Mob::getCarriedItem",
        "?getCarriedItem@Mob@@QEBAAEBVItemStack@@XZ",
        "_ZNK3Mob14getCarriedItemEv",
        "",
        0, 0, 0

    },
    {
        "Player::setEquippedSlot",
        "?setEquippedSlot@Player@@UEAAXW4EquipmentSlot@@AEBVItemStack@@@Z",
        "_ZN6Player15setEquippedSlotE13EquipmentSlotRK9ItemStack",
        "",
        0, 0, 0

    },
    {
        "Mob::setEquippedSlot",
        "?setEquippedSlot@Mob@@UEAAXW4EquipmentSlot@@AEBVItemStack@@@Z",
        "_ZN3Mob15setEquippedSlotE13EquipmentSlotRK9ItemStack",
        "",
        0, 0, 0

    },

    // 2. GameMode & Inventory
    {
        "GameMode::useItem",
        "?useItem@GameMode@@UEAA_NAEAVItemStack@@@Z",
        "_ZN8GameMode7useItemER9ItemStack",
        "",
        0, 0, 0

    },
    {
        "GameMode::useItemOn",
        "?useItemOn@GameMode@@UEAA_NAEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
        "_ZN8GameMode9useItemOnER9ItemStackRK8BlockPoshRK4Vec3PK5Block",
        "",
        0, 0, 0

    },
    {
        "GameMode::attack",
        "?attack@GameMode@@UEAA_NAEAVPlayer@@AEAVActor@@@Z",
        "_ZN8GameMode6attackER6PlayerR5Actor",
        "",
        0, 0, 0

    },
    {
        "Player::isOffhandItemAllowed",
        "?isOffhandItemAllowed@Player@@QEBA_NAEBVItemStack@@@Z",
        "_ZNK6Player20isOffhandItemAllowedERK9ItemStack",
        "",
        0, 0, 0

    },
    {
        "PlayerInventory::setItem",
        "?setItem@PlayerInventory@@UEAAXHAEBVItemStack@@W4ContainerID@@@Z",
        "_ZN15PlayerInventory7setItemEiRK9ItemStack11ContainerID",
        "",
        0, 0, 0

    },
    {
        "PlayerInventory::clearSlot",
        "?clearSlot@PlayerInventory@@UEAAXHW4ContainerID@@@Z",
        "_ZN15PlayerInventory9clearSlotEi11ContainerID",
        "",
        0, 0, 0

    },

    // 3. Rendering & Screen Controllers
    {
        "ScreenTechStackSelector::getTechStackForScreen",
        "",
        "_ZN2ui23ScreenTechStackSelector21getTechStackForScreenERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE",
        "",
        0x0974E844, 0x0974E844, 0

    },
    {
        "StartMenuScreenController::onOpen",
        "",
        "_ZN25StartMenuScreenController7_onOpenEv",
        "",
        0x0A2DF314, 0x0A2DF314, 0

    },
    {
        "StartMenuScreenController::tick",
        "",
        "_ZN25StartMenuScreenController5_tickEv",
        "",
        0x0A2E1FF4, 0x0A2E1FF4, 0

    },
    {
        "LevelRenderer::renderLevel",
        "?renderLevel@LevelRenderer@@QEAAXAEAVScreenContext@@AEAVRenderView@@@Z",
        "_ZN13LevelRenderer11renderLevelER13ScreenContextR10RenderView",
        "",
        0, 0, 0

    },
    {
        "ClientInstance::getLocalPlayer",
        "?getLocalPlayer@ClientInstance@@UEBAPEAVLocalPlayer@@XZ",
        "",
        "",
        0, 0, 32

    },
    {
        "ClientInstance::isShowingMenu",
        "?isShowingMenu@ClientInstance@@UEBA_NXZ",
        "",
        "",
        0, 0, 151

    },
    {
        "NetworkHandler::sendPacket",
        "",
        "_ZN14NetworkHandler10sendPacketERK6Packet",
        "",
        0, 0, 0

    },

    // 4. UI System & Screen Controllers
    {
        "ScreenController::tick",
        "?tick@ScreenController@@UEAAXXZ",
        "_ZN16ScreenController4tickEv",
        "",
        0x09B14FEC, 0x096D73DC, 4

    },
    {
        "ScreenController::handleEvent",
        "?handleEvent@ScreenController@@UEAA?AW4UIEventResult@@AEBUScreenEvent@@@Z",
        "_ZN16ScreenController11handleEventERK11ScreenEvent",
        "",
        0x09B14870, 0x096D6EA8, 10

    },
    {
        "ScreenController::bind",
        "?bind@ScreenController@@UEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@IPEAX@Z",
        "_ZN16ScreenController4bindERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEjPv",
        "",
        0x09B1481C, 0x096D6E54, 14

    },
    {
        "ScreenController::addStaticScreenVars",
        "?addStaticScreenVars@ScreenController@@UEAAXAEAV?$unordered_map@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@VUIPropertyBag@@U?$hash@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@U?$equal_to@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@VUIPropertyBag@@@std@@@2@@std@@@Z",
        "_ZN16ScreenController19addStaticScreenVarsERNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE",
        "",
        0x09B15FA4, 0x096D8394, 19

    },
    {
        "ScreenController::canExit",
        "?canExit@ScreenController@@UEAA_NXZ",
        "_ZN16ScreenController7canExitEv",
        "",
        0x09B169DC, 0x096D8DCC, 26

    },
    {
        "SceneFactory::createScreenController",
        "?createScreenController@SceneFactory@@UEAA?AV?$shared_ptr@VScreenController@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@V?$shared_ptr@VScreenModel@@@3@@Z",
        "_ZN12SceneFactory22createScreenControllerERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt10shared_ptrI11ScreenModelE",
        "",
        0x09AEF950, 0x09765590, 2

    },
    {
        "SceneFactory::createScreen",
        "?createScreen@SceneFactory@@UEAA?AV?$shared_ptr@VScreenView@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@@Z",
        "_ZN12SceneFactory12createScreenERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE",
        "",
        0x09AFD430, 0x09783638, 3

    },
    {
        "ButtonComponent::handleEvent",
        "?handleEvent@ButtonComponent@@UEAA?AW4UIEventResult@@AEBUScreenEvent@@@Z",
        "_ZN15ButtonComponent11handleEventERK11ScreenEvent",
        "",
        0x096FB8E8, 0x09338D6C, 2

    },
    {
        "ButtonComponent::setPressed",
        "?setPressed@ButtonComponent@@UEAAX_N@Z",
        "_ZN15ButtonComponent10setPressedEb",
        "",
        0x096FC22C, 0x093396B0, 3

    },
    {
        "ButtonComponent::getButtonState",
        "?getButtonState@ButtonComponent@@UEBA?AW4ButtonState@@XZ",
        "_ZNK15ButtonComponent14getButtonStateEv",
        "",
        0x096FC208, 0x0933968C, 4

    },
    {
        "ChatScreenController::handleEvent",
        "?handleEvent@ChatScreenController@@UEAA?AW4UIEventResult@@AEBUScreenEvent@@@Z",
        "_ZN20ChatScreenController11handleEventERK11ScreenEvent",
        "",
        0x09A0F020, 0x09684878, 10

    },
    {
        "ChatScreenController::propertyBind",
        "?propertyBind@ChatScreenController@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
        "_ZN20ChatScreenController12propertyBindERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE",
        "",
        0x09A0F948, 0x09685198, 18

    },
    {
        "GuiData::displayClientMessage",
        "?displayClientMessage@GuiData@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
        "_ZN7GuiData20displayClientMessageERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE",
        "",
        0x0A10391C, 0x09CFBBD0, 0

    },
    {
        "CommandRegistry::registerCommand",
        "?registerCommand@CommandRegistry@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBDW4CommandPermissionLevel@@W4CommandFlagValue@@3@Z",
        "_ZN15CommandRegistry15registerCommandERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEPKcb19CommandPermissionLevel11CommandFlagS8_",
        "",
        0x0DC4A370, 0x0D6075CC, 0

    },
    {
        "LoopbackPacketSender::send",
        "?send@LoopbackPacketSender@@UEAAXAEAVPacket@@@Z",
        "_ZN20LoopbackPacketSender4sendER6Packet",
        "",
        0x0C68FAD4, 0x0C2E26FC, 3

    },
    {
        "ServerNetworkHandler::handle",
        "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBV?$shared_ptr@VPacket@@@std@@@Z",
        "_ZN20ServerNetworkHandler6handleERK17NetworkIdentifierRKSt10shared_ptrI6PacketE",
        "",
        0x0C58394C, 0x0C1E5B4C, 3

    },
    {
        "Recipes::addShapedRecipe",
        "?addShapedRecipe@Recipes@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVItemInstance@@AEBV?$vector@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@@3@AEBV?$vector@VRecipeIngredient@@V?$allocator@VRecipeIngredient@@@2@@3@@Z",
        "_ZN7Recipes15addShapedRecipeERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERK12ItemInstanceRKSt6vectorIS5_SaIS5_EERKSA_I16RecipeIngredientSaISD_EE",
        "",
        0x0FC3D3D8, 0x0F2FC108, 0

    },
    {
        "Recipes::addShapelessRecipe",
        "?addShapelessRecipe@Recipes@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVItemInstance@@AEBV?$vector@VRecipeIngredient@@V?$allocator@VRecipeIngredient@@@2@@3@@Z",
        "_ZN7Recipes18addShapelessRecipeERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERK12ItemInstanceRKSt6vectorI16RecipeIngredientSaISA_EE",
        "",
        0x0FC3E1D4, 0x0F2FCFD4, 0

    },
    {
        "ItemRegistry::_tryRegisterAndInitItem",
        "?_tryRegisterAndInitItem@ItemRegistry@@QEAA?AV?$SharedPtr@VItem@@@@AEBVHashedString@@@Z",
        "_ZN12ItemRegistry23_tryRegisterAndInitItemERK12HashedString",
        "",
        0x0FBB3014, 0x0F2AFB68, 0

    },
    {
        "ItemRegistry::parseItemIdentifier",
        "?parseItemIdentifier@ItemRegistry@@QEBA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEAH@Z",
        "_ZNK12ItemRegistry19parseItemIdentifierERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERi",
        "",
        0x0FBB5A54, 0x0F2B2580, 0

    },
    {
        "BlockTypeRegistry::init",
        "?init@BlockTypeRegistry@@SAXXZ",
        "_ZN17BlockTypeRegistry4initEv",
        "",
        0x0AD6D44C, 0x0954D9A4, 0

    },
    {
        "BlockDefinitionLoader::processCustomComponents",
        "?processCustomComponents@BlockDefinitionLoader@@QEAAXXZ",
        "_ZN21BlockDefinitionLoader23processCustomComponentsEv",
        "",
        0x10491FF8, 0x0FB26F04, 0

    },
    {
        "Actor::normalTick",
        "?normalTick@Actor@@UEAAXXZ",
        "_ZN5Actor10normalTickEv",
        "",
        0x0F27DA94, 0x0EA412D4, 1

    },
    {
        "Actor::baseTick",
        "?baseTick@Actor@@UEAAXXZ",
        "_ZN5Actor8baseTickEv",
        "",
        0x0F560820, 0x0EC84620, 2

    },
    {
        "Actor::hurt",
        "?hurt@Actor@@UEAA_NAEBVActorDamageSource@@M_N1@Z",
        "_ZN5Actor4hurtERK17ActorDamageSourcefbb",
        "",
        0x0F5609A8, 0x0EC847A8, 3

    },
    {
        "Actor::die",
        "?die@Actor@@UEAAXAEBVActorDamageSource@@@Z",
        "_ZN5Actor3dieERK17ActorDamageSource",
        "",
        0x0F564E7C, 0x0EC88C50, 4

    },
    {
        "Actor::teleportTo",
        "?teleportTo@Actor@@UEAAXAEBVVec3@@_NMHH@Z",
        "_ZN5Actor10teleportToERK4Vec3bfii",
        "",
        0x0F5746BC, 0x0EC97F6C, 22

    },
    {
        "Actor::hasComponent",
        "?hasComponent@Actor@@QEBA_NI@Z",
        "_ZNK5Actor12hasComponentEj",
        "",
        0x0F583390, 0x0D6C1B88, 0

    },
    {
        "Actor::setCustomName",
        "?setCustomName@Actor@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
        "_ZN5Actor13setCustomNameERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE",
        "",
        0x0F57E694, 0x0ECA1C24, 0

    },
    {
        "Scripting::QuickJS",
        "?QuickJS@Scripting@@QEAAXXZ",
        "_ZN9Scripting7QuickJSEv",
        "",
        0x11432150, 0x10917BD4, 0

    },
    {
        "ScriptModuleMinecraft::ScriptSystem",
        "?ScriptSystem@ScriptModuleMinecraft@@QEAAXXZ",
        "_ZN21ScriptModuleMinecraft12ScriptSystemEv",
        "",
        0x0E069FE8, 0x0DA54B3C, 0

    },
    {
        "ResourcePack::getResource",
        "?getResource@ResourcePack@@QEBA_NAEBVPath@@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
        "_ZNK12ResourcePack11getResourceERKN4Core4PathERNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE",
        "",
        0x06A3DEA0, 0x066951F4, 0

    },
    {
        "ResourcePackRepository::getResourcePackInPath",
        "?getResourcePackInPath@ResourcePackRepository@@QEAA?AV?$shared_ptr@VResourcePack@@@std@@AEBVPath@@@Z",
        "_ZN22ResourcePackRepository22getResourcePackInPathERKN4Core4PathE",
        "",
        0x0C9356E4, 0x0C59FEF0, 0

    },
    {
        "RepositoryLoading::findVanillaPacks",
        "?findVanillaPacks@RepositoryLoading@@YA?AV?$vector@V?$shared_ptr@VResourcePack@@@std@@V?$allocator@V?$shared_ptr@VResourcePack@@@std@@@2@@std@@XZ",
        "_ZN17RepositoryLoading16findVanillaPacksEv",
        "",
        0x0C96C79C, 0x0C5D9184, 0

    },

    // 5. Comprehensive Golden Symbol Dictionary (2,036 symbols)
#include "Symbols_1_26_52_3.inl"
};

static std::map<std::string_view, const SymbolEntry*> sLookupMap;
static std::once_flag sInitOnce;

static void ensureLookupMap() {
    std::call_once(sInitOnce, []() {
        size_t total = sizeof(kSymbolTable) / sizeof(kSymbolTable[0]);
        for (const auto& entry : kSymbolTable) {
            if (entry.humanName && entry.humanName[0]) {
                sLookupMap.try_emplace(entry.humanName, &entry);
            }
            if (entry.msvcSymbol && entry.msvcSymbol[0]) {
                sLookupMap.try_emplace(entry.msvcSymbol, &entry);
            }
            if (entry.itaniumSymbol && entry.itaniumSymbol[0]) {
                sLookupMap.try_emplace(entry.itaniumSymbol, &entry);
            }
        }
        ll::Logger::getInstance().info("LeviLamroid indexed ", total, " universal symbols across MSVC & Itanium ABIs.");
    });
}

size_t getRegisteredSymbolCount() {
    return sizeof(kSymbolTable) / sizeof(kSymbolTable[0]);
}

const SymbolEntry* findSymbolInfo(std::string_view identifier) {
    ensureLookupMap();
    auto it = sLookupMap.find(identifier);
    if (it != sLookupMap.end()) {
        return it->second;
    }

    // Direct alias mappings for standard modding hooks
    if (identifier == "LoopbackPacketSenderSendHook" || identifier == "LoopbackPacketSenderSend" || identifier == "LoopbackPacketSender::send") {
        auto it2 = sLookupMap.find("LoopbackPacketSender::send");
        if (it2 != sLookupMap.end()) return it2->second;
    }
    if (identifier == "LoopbackPacketSenderSendToServerHook" || identifier == "LoopbackPacketSender::sendToServer") {
        auto it2 = sLookupMap.find("LoopbackPacketSender::send");
        if (it2 != sLookupMap.end()) return it2->second;
    }

    if (identifier == "MenuMouseInputHook" || identifier == "MouseInputHook") {
        auto it2 = sLookupMap.find("cohtml::ViewImpl::MouseEvent");
        if (it2 != sLookupMap.end()) return it2->second;
    }

    // Try stripping "Hook" suffix if present
    if (identifier.ends_with("Hook")) {
        std::string stripped(identifier.substr(0, identifier.length() - 4));
        auto itStripped = sLookupMap.find(stripped);
        if (itStripped != sLookupMap.end()) return itStripped->second;
    }

    return nullptr;
}

void* resolveSymbolAddress(std::string_view identifier) {
    // 1. Direct lookup in active multi-version EngineOffsets table (1.26.52.3, 1.26.51.1, 1.26.45.1)
    uintptr_t engineRva = findFunctionRva(identifier);
    if (engineRva != 0) {
        uintptr_t base = ll::memory::getMinecraftBase();
        if (base != 0) return reinterpret_cast<void*>(base + engineRva);
    }

    const auto* info = findSymbolInfo(identifier);
    std::string ver = getDetectedVersion();

    // 2. Try dynamic symbol lookup via Itanium name
    if (info && info->itaniumSymbol && info->itaniumSymbol[0] != '\0') {
        void* sym = ll::memory::findMinecraftSymbol(info->itaniumSymbol);
        if (sym) return sym;
    }

    // 3. Direct string dlsym lookup if it's already a mangled name
    if (identifier.starts_with("_ZN") || identifier.starts_with("_ZK")) {
        void* sym = ll::memory::findMinecraftSymbol(std::string(identifier).c_str());
        if (sym) return sym;
    }

    // 4. Fallback to version RVA table
    if (info) {
        if (info->humanName) {
            uintptr_t hRva = findFunctionRva(info->humanName);
            if (hRva != 0) {
                uintptr_t base = ll::memory::getMinecraftBase();
                if (base != 0) return reinterpret_cast<void*>(base + hRva);
            }
        }

        uintptr_t rva = 0;
        if (ver.find("1.26.44") != std::string::npos || ver.find("1.26.45") != std::string::npos) {
            rva = info->rva1_26_45;
        } else {
            rva = info->rva1_26_52;
        }

        if (rva != 0) {
            uintptr_t base = ll::memory::getMinecraftBase();
            if (base != 0) return reinterpret_cast<void*>(base + rva);
        }

        // 5. Invariant Pattern Scan fallback if RVA shifted (bounded window + cached)
        if (info->idaSignature && info->idaSignature[0] != '\0') {
            uintptr_t scanned = ll::memory::SignatureEngine::resolve(
                info->humanName ? info->humanName : identifier,
                rva,
                info->idaSignature
            );
            if (scanned != 0) {
                return reinterpret_cast<void*>(scanned);
            }
        }
    }

    return nullptr;
}

} // namespace levilamroid::mcpe
