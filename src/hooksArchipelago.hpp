#pragma once

#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "sead/prim/seadSafeString.h"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/LiveActor/ActorAreaFunction.h"
#include "al/Library/Message/MessageHolder.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Scene/Scene.h"
#include "al/Library/Scene/SceneObjUtil.h"

#include "game/Actors/GrowFlowerPot.h"
#include "game/Demo/DemoStateHackFirst.h"
#include "game/Item/Shine.h"
#include "game/Layout/ShopLayoutInfo.h"
#include "game/Scene/CapMessageMoonNotifier.h"
#include "game/Sequence/ChangeStageInfo.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolder.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/GameDataHolderWriter.h"
#include "game/Util/ItemUtil.h"
#include "game/Util/StageLayoutFunction.h"

#include "server/archipelago/ArchipelagoMode.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"

// ===== isGotShine Hooks =====
static bool isGrabShine(GameDataHolderAccessor accessor, int hintIdx) {
    ArchipelagoMode* apMode = GameModeManager::instance()->getMode<ArchipelagoMode>();
    GameDataFile::HintInfo* curHintInfo = &accessor.mData->getGameDataFile()->getHintList()[hintIdx];
    if (!curHintInfo->isGrand) {
        if (curHintInfo->uniqueId == 205 && apMode->getScenario(1) > 1 || curHintInfo->uniqueId == 129) {
            return true;
        }
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

            return isGrabShine(accessor, i);
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

static HkTrampoline<bool, GameDataHolderAccessor, int, int> isGrabShineByWorldIdHintIdxHook =
    hk::hook::trampoline([](GameDataHolderAccessor accessor, int worldId, int hintIdx) -> bool {
        // Examine if not performing check for moon rock scenario causes unintended behavior in game
        if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            return isGrabShine(accessor, hintIdx);
        } else {
            return isGrabShineByWorldIdHintIdxHook.orig(accessor, worldId, hintIdx);
        }
    });

// might be unneeded
static HkTrampoline<bool, const Shine*> isGotShineRedirectHook = hk::hook::trampoline([](const Shine* curShine) -> bool {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        GameDataHolderAccessor accessor = GameDataHolderAccessor(curShine);
        return isGrabShine(accessor, curShine->mShineIdx);
    } else {
        return isGotShineRedirectHook.orig(curShine);
    }
});

// ===== Unlock Shine Num =====
int getApUnlockShineNumByWorldId(int worldId) {
    if (worldId < 1 || worldId > 16) {
        worldId = 0;
    }

    return GameModeManager::instance()->getMode<ArchipelagoMode>()->getWorldUnlockCount(worldId);
}

static HkTrampoline<int, GameDataHolder*, bool*, int> getUnlockShineNumHook =
    hk::hook::trampoline([](GameDataHolder* thisPtr, bool* unkBool, int worldId) -> int {
        if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
            return getApUnlockShineNumByWorldId(worldId);
        }
        return getUnlockShineNumHook.orig(thisPtr, unkBool, worldId);
    });

// static HkTrampoline<int, bool*, GameDataHolderAccessor> getUnlockShineNumByAccessorHook = hk::hook::trampoline([](bool* unkBool, GameDataHolderAccessor
// accessor) -> int {
//     if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
//         int worldId = accessor.mData->mPlayingFile->getCurrentWorldId();
//         return getApUnlockShineNumByWorldId(worldId);
//     }
//     return getUnlockShineNumByAccessorHook.orig(unkBool, accessor);
// });

// static HkTrampoline<int, GameDataFile*, bool*> getUnlockShineNumByGameDataFileHook = hk::hook::trampoline([](GameDataFile* file, bool* unkBool) -> int {
//     if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
//         int worldId = file->getCurrentWorldId();
//         return getApUnlockShineNumByWorldId(worldId);
//     }
//     return getUnlockShineNumByGameDataFileHook.orig(file, unkBool);
// });
//
// static HkTrampoline<int, bool*, GameDataHolder*, int> getUnlockShineNumByWorldIdHook = hk::hook::trampoline([](bool* unkBool, GameDataHolder* thisPtr, int
// worldId) -> int {
//     if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
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

// ===== Shop Items =====
static HkTrampoline<void, GameDataFile*, ShopItem::ItemInfo*, bool> buyItemHook =
    hk::hook::trampoline([](GameDataFile* file, ShopItem::ItemInfo* itemInfo, bool unkBool) -> void {
        if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
            GameModeManager::instance()->getMode<ArchipelagoMode>()->sendShopCheck(itemInfo);
        } else {
            // Send buy item packet here
            buyItemHook.orig(file, itemInfo, unkBool);
        }
    });

// ===== Stage Changing =====
static void onGrandShineStageChange(GameDataHolderWriter holder, ChangeStageInfo const* stageInfo) {
    GameModeManager::instance()->getMode<ArchipelagoMode>()->sendStage(holder, stageInfo);
}

static void changeNextStage(GameDataFile* file, const ChangeStageInfo* stageInfo, int param2) {
    if (!GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        file->changeNextStage(stageInfo, param2);
    } else {
        // Client::setMessage(1, stageInfo->mChangeStageId.cstr());
        //  Add Wooded shop moon warp

        if (!(al::isEqualString(stageInfo->mChangeStageId.cstr(), "obj846") || al::isEqualString(stageInfo->mChangeStageId.cstr(), "obj1084"))) {
            if (isPartOf(stageInfo->mChangeStageName.cstr(), "WorldHomeStage")) {
                if (GameModeManager::instance()->getMode<ArchipelagoMode>()->setScenario(stageInfo->mChangeStageName.cstr(), stageInfo->mScenarioNo)) {
                    // Client::setMessage(2, "attempting send to correct scenario");
                    GameModeManager::instance()->getMode<ArchipelagoMode>()->sendCorrectScenario(stageInfo);

                } else {
                    // Client::setMessage(2, "setScenario false");
                    file->changeNextStage(stageInfo, param2);
                }
            } else {
                // Non world transitions
                // Client::setMessage(2, "non world transition");
                file->changeNextStage(stageInfo, param2);
            }
        } else {
            // Catch cap and cascade shop moons
            // Client::setMessage(2, "Shop moon stageID caught");
            file->changeNextStage(stageInfo, param2);
        }
    }
}

// includes paintings
// static HkTrampoline<void, GameDataFile*, const ChangeStageInfo*, int> changeNextStageHook =
//     hk::hook::trampoline([](GameDataFile* file, const ChangeStageInfo* stageInfo, int param2) -> void {
//         if (!GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
//             changeNextStageHook.orig(file, stageInfo, param2);
//         } else {
//             // Client::setMessage(1, stageInfo->mChangeStageId.cstr());
//             //  Add Wooded shop moon warp

//             if (!(al::isEqualString(stageInfo->mChangeStageId.cstr(), "obj846") || al::isEqualString(stageInfo->mChangeStageId.cstr(), "obj1084"))) {
//                 if (isPartOf(stageInfo->mChangeStageName.cstr(), "WorldHomeStage")) {
//                     if (Client::setScenario(stageInfo->mChangeStageName.cstr(), stageInfo->mScenarioNo)) {
//                         // Client::setMessage(2, "attempting send to correct scenario");
//                         GameModeManager::instance()->getMode<ArchipelagoMode>()->sendCorrectScenario(stageInfo);

//                     } else {
//                         // Client::setMessage(2, "setScenario false");
//                         file->changeNextStage(stageInfo, param2);
//                     }
//                 } else {
//                     // Non world transitions
//                     // Client::setMessage(2, "non world transition");
//                     file->changeNextStage(stageInfo, param2);
//                 }
//             } else {
//                 // Catch cap and cascade shop moons
//                 // Client::setMessage(2, "Shop moon stageID caught");
//                 file->changeNextStage(stageInfo, param2);
//             }
//         }
//     });

// ===== Shine Data Replacement =====
static bool isReplaceShineLabel(al::LayoutActor* layout, char const* element, char const* label, char const* param4) {
    if (!GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        return rs::trySetPaneStageMessageIfExist(layout, element, label, param4);
    }

    return false;
}

static void setShineLabel(al::IUseLayout* layout, const char* elementLabel) {
    al::setPaneStringFormat(layout, elementLabel, GameModeManager::instance()->getMode<ArchipelagoMode>()->getShineReplacementText());
}

static void setShineColor(Shine* thisPtr, char* stageName, int color, bool isSetMtpColor) {
    // Get color here using shine unique id
    // Client::setMessage(1, "Set custom shine color");
    int storedColor = GameModeManager::instance()->getMode<ArchipelagoMode>()->getShineColor(thisPtr);
    rs::setStageShineAnimFrame((al::LiveActor*)thisPtr, stageName, storedColor, isSetMtpColor);
}

static void setShineModelColor(Shine* thisPtr, char* stageName, int color, bool isSetMtpColor) {
    // Get color here using shine unique id
    // Client::setMessage(1, "Set custom other shine color");
    int storedColor = GameModeManager::instance()->getMode<ArchipelagoMode>()->getShineColor(thisPtr);
    rs::setStageShineAnimFrame(thisPtr->mModelShine, stageName, storedColor, isSetMtpColor);
}

// ===== Shop Data Replacement =====
static const char16_t* getShopItemMessage(al::IUseMessageSystem const* messageSystem, char const* fileName, char const* key) {
    GameModeManager* manager = GameModeManager::instance();
    if (manager->isMode(GameMode::ARCHIPELAGO)) {
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

static bool isBuyItems(ShopItem::ItemInfo* itemInfo) {
    // Add a collected outfits, gifts, stickers based implementation similar to shinechecks
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        return false;
    } else {
        return Client::sInstance->getHolder()->getGameDataFile()->isBuyItem(itemInfo);
    }
}

// ===== Hack Data Replacement =====
// isExistInHackDictionary for capture tracking
static void onAddHack(GameDataHolderWriter writer, const char* hackName) {
    GameModeManager* manager = GameModeManager::instance();
    if (manager->isMode(GameMode::ARCHIPELAGO) && manager->getMode<ArchipelagoMode>()->getCapturesFlag()) {
        // Client::setMessage(2, hackName);
        manager->getMode<ArchipelagoMode>()->sendCaptureCheck(hackName);
        manager->getMode<ArchipelagoMode>()->setIsRecordCapture(true);
    } else {
        GameDataFunction::addHackDictionary(writer, hackName);
    }
}

static void canEndHack(al::LiveActor* actor) {
    if (actor != nullptr) {
        ((PlayerHackKeeper*)actor)->endHackStartDemo(actor);
    }
}

// ===== QOL Changes =====
bool growOnPlant(GrowFlowerPot* thisPtr) {
    rs::setGrowFlowerTime(thisPtr, thisPtr->mPlacementId, 3600000);
    return al::isActionEnd(thisPtr);
}

// ===== Demo Hooks =====
// _ZN16HakoniwaSequence15exeBootLoadDataEv = 0x50F29C - 0x50F304
// void onNewGameDemoStart(char* name, bool unkBool) {
//    for (int i = 0; i < 18; i++) {
//        Client::setScenario(i, 1);
//    }
//
//    for (int i = 0; i < 25; i++) {
//        Client::setShineChecks(i, 0);
//    }
//
//    for (int i = 0; i < 12; i++) {
//        Client::setOutfitChecks(i, 0);
//    }
//
//    for (int i = 0; i < 4; i++) {
//        Client::setStickerChecks(i, 0);
//    }
//
//    for (int i = 0; i < 5; i++) {
//        Client::setSouvenirChecks(i, 0);
//    }
//
//    for (int i = 0; i < 8; i++) {
//        Client::setCaptureChecks(i, 0);
//    }
//
//    Client::setCheckIndex(-1);
//
//    // al::initActorWithArchiveName(thisPtr, info, str, name);
//    al::createSceneHeap(name, unkBool);
//    return;
//}

// First time entering lost in demo from cloud
static void onUnlockLost(GameDataHolderWriter writer, int worldIndex) {
    // Send Beat Bowser in Cloud location
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        Client::sendCheckPacket(2500, CheckType::Moon);
    }

    GameDataFunction::unlockWorld(writer, worldIndex);

    return;
}

static void onCreditsStart(al::Scene* thisPtr, const al::SceneInitInfo info) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        Client::sendCheckPacket(2499, CheckType::Moon);
    }

    thisPtr->initDrawSystemInfo(info);
    return;
}

bool skipHackCutscene(DemoStateHackFirst* thisPtr, IUsePlayerHack** param_1, al::SensorMsg* param_2, al::HitSensor* param_3, al::HitSensor* param_4) {
    return false;
}
