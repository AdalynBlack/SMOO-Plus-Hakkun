#pragma once
#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "sead/prim/seadSafeString.h"

#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Scene/SceneObjUtil.h"

#include "game/Item/Shine.h"
#include "game/Item/ShineInfo.h"
#include "game/Scene/CapMessageMoonNotifier.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolder.h"
#include "game/System/GameProgressData.h"
#include "game/Util/ItemUtil.h"
#include "game/Util/StageLayoutFunction.h"

#include "custom/rs/util.hpp"

#include "server/archipelago/ArchipelagoMode.hpp"
#include "server/gamemode/GameModeManager.hpp"

// ===== isGotShine Hooks =====
static bool isGrabShine(GameDataHolderAccessor accessor, int hintIdx) {
    ArchipelagoMode* apMode = GameModeManager::instance()->getMode<ArchipelagoMode>();
    GameDataFile::HintInfo* curHintInfo = &accessor.mData->getGameDataFile()->getHintList()[hintIdx];
    if (!curHintInfo->isGrand) {
        return apMode->hasShine(curHintInfo->uniqueId);
    }
    return false;
}

static HkTrampoline<bool, GameDataHolderAccessor, const ShineInfo*> isGrabShineByShineInfoHook =
    hk::hook::trampoline([](GameDataHolderAccessor accessor, const ShineInfo* shineInfo) -> bool {
        if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            int i = 0;
            for (i = 0; i < 0x400; i++) {
                GameDataFile::HintInfo* curHintInfo = &accessor.mData->getGameDataFile()->getHintList()[i];
                if (al::isEqualString(curHintInfo->objId, shineInfo->mObjId) && al::isEqualString(curHintInfo->stageName, shineInfo->mStageName)) {
                    break;
                }
            }
            if (i < 0x400) {
                return isGrabShine(accessor, i);
            } else {
                return false;
            }

        } else {
            return isGrabShineByShineInfoHook.orig(accessor, shineInfo);
        }
    });

static HkTrampoline<bool, GameDataHolderAccessor, int> isGrabShineByHintInfoIdxHook =
    hk::hook::trampoline([](GameDataHolderAccessor accessor, int hintIdx) -> bool {
        if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            return isGrabShine(accessor, hintIdx);
        } else {
            return isGrabShineByHintInfoIdxHook.orig(accessor, hintIdx);
        }
    });

// static HkTrampoline<bool, GameDataHolderAccessor, int, int> isGrabShineByWorldIdHintIdxHook =
//     hk::hook::trampoline([](GameDataHolderAccessor accessor, int worldId, int hintIdx) -> bool {
//         // Examine if not performing check for moon rock scenario causes unintended behavior in game
//         if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
//             return isGrabShine(accessor, hintIdx);
//         } else {
//             return isGrabShineByWorldIdHintIdxHook.orig(accessor, worldId, hintIdx);
//         }
//     });

// might be unneeded
static HkTrampoline<bool, const Shine*> isGotShineRedirectHook = hk::hook::trampoline([](const Shine* curShine) -> bool {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        GameDataHolderAccessor accessor = GameDataHolderAccessor((al::LiveActor*)curShine);
        return isGrabShine(accessor, curShine->mShineIdx);
    } else {
        return isGotShineRedirectHook.orig(curShine);
    }
});

static bool shineListShineCountHook(GameDataHolderAccessor accessor, int worldId, int hintIdx) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        // Gets Shine Uid by index in hint list relative to worldId
        int hintIdxByWorld = -1;
        for (int i = 0; i < 0x400; i++) {
            GameDataFile::HintInfo* curHintInfo = &accessor.mData->getGameDataFile()->getHintList()[i];
            if (curHintInfo->worldId == worldId) {
                hintIdxByWorld += 1;
                if (hintIdxByWorld == hintIdx) {
                    return GameModeManager::instance()->getMode<ArchipelagoMode>()->hasShine(curHintInfo->uniqueId);
                }
            }
        }

        // sead::FixedSafeString<64> errorStr = sead::FixedSafeString<64>();
        // errorStr = "world ";
        // errorStr.append(intToCstr(worldId));
        // errorStr.append(", hint ");
        // errorStr.append(intToCstr(hintIdx));
        // errorStr.append(", unique ");
        // errorStr.append(intToCstr(curHintInfo->uniqueId));
        // Client::addMessage(errorStr.cstr());
        // Client::addMessage("Failed to find shine for list.");
    }

    return GameDataFunction::isGotShine(accessor, worldId, hintIdx);
}

// ===== Unlock Shine Num =====
static int getApUnlockShineNumByWorldId(int worldId) {
    if (worldId < 1 || worldId > 16) {
        worldId = 0;
    }

    return GameModeManager::instance()->getMode<ArchipelagoMode>()->getWorldUnlockCount(worldId);
}

static HkTrampoline<int, GameDataHolder*, bool*, int> getUnlockShineNumHook =
    hk::hook::trampoline([](GameDataHolder* thisPtr, bool* unkBool, int worldId) -> int {
        if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            return getApUnlockShineNumByWorldId(worldId);
        }
        return getUnlockShineNumHook.orig(thisPtr, unkBool, worldId);
    });

// static HkTrampoline<int, bool*, GameDataHolderAccessor> getUnlockShineNumByAccessorHook = hk::hook::trampoline([](bool* unkBool, GameDataHolderAccessor
// accessor) -> int {
//     if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
//         int worldId = accessor.mData->mPlayingFile->getCurrentWorldId();
//         return getApUnlockShineNumByWorldId(worldId);
//     }
//     return getUnlockShineNumByAccessorHook.orig(unkBool, accessor);
// });

// static HkTrampoline<int, GameDataFile*, bool*> getUnlockShineNumByGameDataFileHook = hk::hook::trampoline([](GameDataFile* file, bool* unkBool) -> int {
//     if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
//         int worldId = file->getCurrentWorldId();
//         return getApUnlockShineNumByWorldId(worldId);
//     }
//     return getUnlockShineNumByGameDataFileHook.orig(file, unkBool);
// });
//
// static HkTrampoline<int, bool*, GameDataHolder*, int> getUnlockShineNumByWorldIdHook = hk::hook::trampoline([](bool* unkBool, GameDataHolder* thisPtr, int
// worldId) -> int {
//     if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
//         return getApUnlockShineNumByWorldId(worldId);
//     }
//     return getUnlockShineNumByWorldIdHook.orig(unkBool, thisPtr, worldId);
// });

static bool showHasUnlockShineNumCapMessage(al::IUseSceneObjHolder* sceneObjHolder) {
    GameDataHolderAccessor accessor = GameDataHolderAccessor(sceneObjHolder);
    if (GameDataFunction::getGotShineNum(accessor, -1) >=
        GameModeManager::instance()->getMode<ArchipelagoMode>()->getWorldUnlockCount(GameDataFunction::getCurrentWorldId(accessor))) {
        if (al::isExistSceneObj(sceneObjHolder, 5)) {
            CapMessageMoonNotifier* notifier = (CapMessageMoonNotifier*)al::getSceneObj(sceneObjHolder, 5);
            /*notifier->unlockShineNum =
                Client::getWorldUnlockCount(GameDataFunction::getCurrentWorldId(accessor));*/
            // Client::setMessage(1, "Has Enough Moons for notification.");
            return notifier->tryShowCapMessageMoonNotify();
        }
        return false;
    }
    return false;
}

// ===== Shine Data Replacement =====
static bool isReplaceShineLabel(al::LayoutActor* layout, char const* element, char const* label, char const* param4) {
    if (!GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        return rs::trySetPaneStageMessageIfExist(layout, element, label, param4);
    }

    return false;
}

static void setShineLabel(al::IUseLayout* layout, const char* elementLabel) {
    al::setPaneStringFormat(layout, elementLabel, GameModeManager::instance()->getMode<ArchipelagoMode>()->getShineReplacementText());
}

static int isPowerStarHook(Shine* shine, char* stageName) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        int storedColor = GameModeManager::instance()->getMode<ArchipelagoMode>()->getShineColor(shine);
        if (storedColor - 64 > -1)
            return 99;
    }
    return rs::getStageShineAnimFrame((al::LiveActor*)shine, stageName);
}

static bool isWorldPeachHook(GameDataHolderAccessor accessor) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO))
        return false;

    return GameDataFunction::isWorldPeach(accessor);
}

static void setShineColor(Shine* thisPtr, char* stageName, int color, bool isSetMtpColor) {
    // Get color here using shine unique id
    // Client::setMessage(1, "Set custom shine color");
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        int storedColor = GameModeManager::instance()->getMode<ArchipelagoMode>()->getShineColor(thisPtr);
        if (storedColor - 64 > -1)
            storedColor -= 64;
        rs::setStageShineAnimFrame((al::LiveActor*)thisPtr, stageName, storedColor, isSetMtpColor);
    } else {
        rs::setStageShineAnimFrame((al::LiveActor*)thisPtr, stageName, color, isSetMtpColor);
    }
}

static void setShineModelColor(Shine* thisPtr, char* stageName, int color, bool isSetMtpColor) {
    // Get color here using shine unique id
    // Client::setMessage(1, "Set custom other shine color");
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        int storedColor = GameModeManager::instance()->getMode<ArchipelagoMode>()->getShineColor(thisPtr);
        rs::setStageShineAnimFrame(thisPtr->mModelShine, stageName, storedColor, isSetMtpColor);
    } else {
        rs::setStageShineAnimFrame(thisPtr->mModelShine, stageName, color, isSetMtpColor);
    }
}

// ===== Shine List / Collections List =====
static void setShineCounterAndDenominatorHook(al::LayoutActor* shineList, int numerator, int denominator) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        numerator = GameModeManager::instance()->getMode<ArchipelagoMode>()->getNumGotShines();
        // setShineCounterAndDenominatorHook.orig();
        rs::setCounterAndDenominator(shineList, numerator, denominator);

    } else {
        // setShineCounterAndDenominatorHook.orig();
        rs::setCounterAndDenominator(shineList, numerator, denominator);
    }
}

static int getWorldIdForShineListHook(GameProgressData* gameProgressData, int worldId) {
    if (GameModeManager::instance() && GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        int shineListWorldId = gameProgressData->getWorldIdForShineList(worldId);
        GameModeManager::instance()->getMode<ArchipelagoMode>()->setCurWorldShineList(shineListWorldId);
        return shineListWorldId;

    } else {
        return gameProgressData->getWorldIdForShineList(worldId);
    }
}

static int calcWorldNumForShineListHook(GameProgressData* gpd) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        return 17;
    }
    return gpd->calcWorldNumForShineList();
}