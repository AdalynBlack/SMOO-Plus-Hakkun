#pragma once
#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "sead/prim/seadSafeString.h"

#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"

#include "server/archipelago/ArchipelagoMode.hpp"
#include "server/gamemode/GameModeManager.hpp"

// ===== Regional Coins =====
static bool isGotCoinCollectHook(GameDataFile* file, al::PlacementId const* placementId) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        al::StringTmp<128> placeIdString;
        placementId->makeString(&placeIdString);
        return archipelago->hasRegionalCoin(placeIdString.cstr());
    } else {
        return file->isGotCoinCollect(placementId);
    }
}

// Gets the relative world Id for CoinCollect archive name and Picture Font for ER
static int getCurrentWorldIdForCoinCollectHook(GameDataHolderAccessor accessor) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO) &&
        GameModeManager::instance()->getMode<ArchipelagoMode>()->getRelativeWorldCoinCollect() > -1) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        return archipelago->getRelativeWorldCoinCollect();
    }
    return GameDataFunction::getCurrentWorldId(accessor);
}

static int getCoinCollectCheckGotNumHook(GameDataHolderAccessor accessor) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO) &&
        GameModeManager::instance()->getMode<ArchipelagoMode>()->getRelativeWorldCoinCollect() > -1) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        return archipelago->getRelativeWorldCoinCollectCheckGotNum(accessor);
    }
    return GameDataFunction::getCoinCollectGotNum(accessor);
}

static int getCoinCollectNumMaxHook(GameDataHolderAccessor accessor) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO) &&
        GameModeManager::instance()->getMode<ArchipelagoMode>()->getRelativeWorldCoinCollect() > -1) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        return accessor.mData->getCoinCollectNumMax(archipelago->getRelativeWorldCoinCollect());
    }
    return GameDataFunction::getCoinCollectNumMax(accessor);
}

static HkTrampoline<int, GameDataHolderAccessor> getCoinCollectNumHook = hk::hook::trampoline([](GameDataHolderAccessor accessor) -> int {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO) &&
        GameModeManager::instance()->getMode<ArchipelagoMode>()->getRelativeWorldCoinCollect() > -1) {
        return GameModeManager::instance()->getMode<ArchipelagoMode>()->getNumCoinCollect();
    }

    return getCoinCollectNumHook.orig(accessor);
});

static void useCoinCollectHook(GameDataHolderWriter writer, int amount) {
    if (!GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        GameDataFunction::useCoinCollect(writer, amount);
    } else {
        return;
    }
}