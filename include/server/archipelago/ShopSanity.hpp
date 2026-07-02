#pragma once
#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "sead/prim/seadSafeString.h"

#include "al/Library/Message/MessageHolder.h"

#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/Util/ClothUtil.h"

#include "server/archipelago/ArchipelagoMode.hpp"
#include "server/gamemode/GameModeManager.hpp"

// ===== Shop Items =====
static void buyItemHook(GameDataFile* file, const ShopItem::ItemInfo* itemInfo, bool isPrepoSave) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        archipelago->sendShopCheck(itemInfo);
        archipelago->addItem(itemInfo);
    } else {
        // Send buy item packet here
        file->buyItem(itemInfo, isPrepoSave);
    }
}

static bool isBuyItemHook(GameDataHolderAccessor accessor, ShopItem::ItemInfo* itemInfo) {
    // Add a collected outfits, gifts, stickers based implementation similar to shinechecks
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        return GameModeManager::instance()->getMode<ArchipelagoMode>()->hasItem(itemInfo);
    } else {
        return rs::isBuyItem(accessor, itemInfo);
    }
}

static void wearCapHook(GameDataHolderWriter writer, const char* itemName) {
    if (!GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        GameDataFunction::wearCap(writer, itemName);
    } else {
        return;
    }
}

static void wearCostumeHook(GameDataHolderWriter writer, const char* itemName) {
    if (!GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        GameDataFunction::wearCostume(writer, itemName);
    } else {
        return;
    }
}

// ===== Shop Data Replacement =====
static const char16_t* getShopItemMessage(al::IUseMessageSystem const* messageSystem, char const* fileName, char const* key) {
    GameModeManager* manager = GameModeManager::instance();
    if (manager->isModeAndActive(GameMode::ARCHIPELAGO)) {
        const char16_t* msg = manager->getMode<ArchipelagoMode>()->getShopReplacementText(fileName, key);
        sead::WFixedSafeString<200> confirm;
        confirm = u"";
        confirm.append(msg);
        if (!confirm.isEmpty()) {
            return msg;
        }
    }
    // Default to base game text if no ap text exists
    return al::getSystemMessageString(messageSystem, fileName, key);
}