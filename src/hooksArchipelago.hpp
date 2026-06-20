#pragma once

#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "sead/prim/seadSafeString.h"

#include "al/Library/Base/StringUtil.h"
#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/LiveActor/ActorAreaFunction.h"
#include "al/Library/Message/MessageHolder.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Scene/Scene.h"
#include "al/Library/Scene/SceneObjUtil.h"

#include "game/Actors/GrowFlowerPot.h"
#include "game/Demo/DemoStateHackFirst.h"
#include "game/Layout/TalkMessage.h"
#include "game/Scene/CapMessageMoonNotifier.h"
#include "game/Sequence/ChangeStageInfo.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolder.h"
#include "game/System/GameDataUtil.h"
#include "game/System/GameProgressData.h"
#include "game/Util/ClothUtil.h"
#include "game/Util/ItemUtil.h"
#include "game/Util/StageLayoutFunction.h"

#include "rs/util.hpp"
#include "server/archipelago/ArchipelagoMode.hpp"
#include "server/Client.hpp"
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
        GameDataHolderAccessor accessor = GameDataHolderAccessor(curShine);
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
        Client::addMessage("Failed to find shine for list.");
    }

    return GameDataFunction::isGotShine(accessor, worldId, hintIdx);
}

// ===== Unlock Shine Num =====
int getApUnlockShineNumByWorldId(int worldId) {
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

// ===== Shop Items =====
static void buyItemHook(GameDataFile* file, const ShopItem::ItemInfo* itemInfo, bool isPrepoSave) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        GameModeManager::instance()->getMode<ArchipelagoMode>()->sendShopCheck(itemInfo);
        GameModeManager::instance()->getMode<ArchipelagoMode>()->addItem(itemInfo);
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

// ===== Stage Changing =====
static void onGrandShineStageChange(GameDataHolderWriter writer, ChangeStageInfo const* stageInfo) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        GameModeManager::instance()->getMode<ArchipelagoMode>()->setScenario(stageInfo->mChangeStageName.cstr(), stageInfo->mScenarioNo);

        // GameModeManager::instance()->getMode<ArchipelagoMode>()->sendStage(writer, stageInfo);
    } else {
        GameDataFunction::tryChangeNextStage(writer, stageInfo);
    }
}

static void changeNextStage(GameDataFile* file, const ChangeStageInfo* stageInfo, int param2) {
    if (!GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        file->changeNextStage(stageInfo, param2);
    } else {
        ArchipelagoMode* apMode = GameModeManager::instance()->getMode<ArchipelagoMode>();
        ChangeStageInfo* erInfo = nullptr;
        if (file->isUseMissRestartInfo()) {
            erInfo = apMode->getLastERTransition();
        } else {
            erInfo = apMode->handleER(stageInfo);
        }
        // Client::setMessage(1, stageInfo->mChangeStageId.cstr());
        //  Add Wooded shop moon warp

        if (!(al::isEqualString(stageInfo->mChangeStageId.cstr(), "obj846") || al::isEqualString(stageInfo->mChangeStageId.cstr(), "obj1084"))) {
            if (isPartOf(stageInfo->mChangeStageName.cstr(), "WorldHomeStage")) {
                if (apMode->setScenario(stageInfo->mChangeStageName.cstr(), stageInfo->mScenarioNo)) {
                    // Client::setMessage(2, "attempting send to correct scenario");
                    apMode->sendCorrectScenario(stageInfo);

                } else {
                    // Client::setMessage(2, "setScenario false");
                    if (erInfo) {
                        file->changeNextStage(erInfo, param2);
                    } else {
                        file->changeNextStage(stageInfo, param2);
                    }
                }
            } else {
                // Non world transitions
                // Client::setMessage(2, "non world transition");
                if (erInfo) {
                    file->changeNextStage(erInfo, param2);
                } else {
                    file->changeNextStage(stageInfo, param2);
                }
            }
        } else {
            // Catch cap and cascade shop moons
            // Client::setMessage(2, "Shop moon stageID caught");
            file->changeNextStage(stageInfo, param2);
        }
    }
}

// static bool tryFindLinkDestStageInfoOverride(GameDataHolder* holder, const char** destStageName, const char** destLabel, const char* srcStageName,
//                                              const char* srcLabel) {
//     sead::S return holder->tryFindLinkDestStageInfo(destStageName, destLabel, srcStageName, srcLabel);
// }

// includes paintings
// static HkTrampoline<void, GameDataFile*, const ChangeStageInfo*, int> changeNextStageHook =
//     hk::hook::trampoline([](GameDataFile* file, const ChangeStageInfo* stageInfo, int param2) -> void {
//         if (!GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
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

// ===== Hack Data Replacement =====
// isExistInHackDictionary for capture tracking
static void onAddHack(GameDataHolderWriter writer, const char* hackName) {
    GameModeManager* manager = GameModeManager::instance();
    if (manager->isModeAndActive(GameMode::ARCHIPELAGO) && manager->getMode<ArchipelagoMode>()->getCapturesFlag()) {
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

// ===== QOL Changes =====
bool growOnPlant(GrowFlowerPot* thisPtr) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO))
        // rs::addGrowFlowerGrowLevel(thisPtr, thisPtr->mPlacementId, 255);
        thisPtr->tryMaxGrowLevel();
    return al::isActionEnd(thisPtr);
}

// ===== Demo Hooks =====
// _ZN16HakoniwaSequence15exeBootLoadDataEv = 0x50F29C - 0x50F304
void onNewGameDemoStart(char* name, bool unkBool) {
    ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
    archipelago->setConnectInitFlag(true);
    archipelago->setFirstConnectFlag(true);
    archipelago->clearCollectibles();
    archipelago->clearScenarios();
    al::createSceneHeap(name, unkBool);
    return;
}

// First time entering lost in demo from cloud
static void onUnlockLost(GameDataHolderWriter writer, int worldIndex) {
    // Send Beat Bowser in Cloud location
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        Client::sendCheckPacket(2500, CheckType::Moon);
    }

    GameDataFunction::unlockWorld(writer, worldIndex);

    return;
}

// On credits scene initialization
static void onCreditsStart(al::Scene* thisPtr, const al::SceneInitInfo info) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        Client::sendCheckPacket(2499, CheckType::Moon);
    }

    thisPtr->initDrawSystemInfo(info);
    return;
}

//
bool skipHackCutscene(DemoStateHackFirst* thisPtr, IUsePlayerHack** param_1, al::SensorMsg* param_2, al::HitSensor* param_3, al::HitSensor* param_4) {
    return GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO);
}

int calcWorldNumForShineListHook(GameProgressData* gpd) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
        return 17;
    }
    return gpd->calcWorldNumForShineList();
}

static void updateListHook(GameProgressData* gameProgressData) {
    gameProgressData->updateList();
    if (GameModeManager::instance() && GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        for (int i = 0; i < 17; i++) {
            gameProgressData->mWorldIdForShineList[i] = i;
        }

        for (int i = 0; i < 17; i++) {
            gameProgressData->mWorldIdForWorldMap[i] = i;
        }

        gameProgressData->mUnlockWorldStatusFirstBranch = GameProgressData::FirstBranch::Lake;
        gameProgressData->mUnlockWorldStatusSecondBranch = GameProgressData::SecondBranch::Snow;

        if (gameProgressData->mUnlockWorldNum == 4) {
            gameProgressData->mIsUnlockWorld[GameDataFunction::getWorldIndexForest()] = true;
            gameProgressData->mIsUnlockWorld[GameDataFunction::getWorldIndexLake()] = false;
        }

        if (gameProgressData->mUnlockWorldNum == 9) {
            gameProgressData->mIsUnlockWorld[GameDataFunction::getWorldIndexSea()] = true;
            gameProgressData->mIsUnlockWorld[GameDataFunction::getWorldIndexSnow()] = false;
        }

        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexWaterfall()] = GameDataFunction::getWorldIndexSky();
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexSand()] = GameDataFunction::getWorldIndexCity();
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexForest()] = GameDataFunction::getWorldIndexLava();
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexLake()] = GameDataFunction::getWorldIndexSand();
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexCity()] = GameDataFunction::getWorldIndexForest();
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexSea()] = GameDataFunction::getWorldIndexLake();
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexSnow()] = GameDataFunction::getWorldIndexWaterfall();
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexLava()] = GameDataFunction::getWorldIndexPeach();
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexSky()] = GameDataFunction::getWorldIndexSea();
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexPeach()] = GameDataFunction::getWorldIndexSnow();

        // 0 = ??? assume Bowser
        // 1 =

        // for (int i = 0; i < 17; i++) {
        //     sead::FixedSafeString<128> paintingId = sead::FixedSafeString<128>();
        //     paintingId = "World Id: ";
        //     paintingId.append(intToCstr(i));
        //     paintingId.append(" Painting Id: ");
        //     paintingId.append(intToCstr(gameProgressData->mWorldIdForWorldWarpHole[i]));
        //     Client::addMessage(paintingId.cstr());
        // }
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexWaterfall()] = 1;
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexSand()] = 1;
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexForest()] = 1;
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexLake()] = 1;
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexCity()] = 1;
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexSea()] = 1;
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexSnow()] = 1;
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexLava()] = 1;
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexSky()] = 1;
        // gameProgressData->mWorldIdForWorldWarpHole[GameDataFunction::getWorldIndexPeach()] = 1;
    }
}

// static HkTrampoline<bool, GameDataHolderAccessor, int> isUnlockedWorldHook = hk::hook::trampoline([](GameDataHolderAccessor accessor, int worldId) -> bool {
//     if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
//         if (worldId == (int)GameDataFunction::getWorldIndexHat() || worldId == (int)GameDataFunction::getWorldIndexWaterfall()) {
//             return true;
//         }
//         int curWorldId = GameDataFunction::getCurrentWorldId(accessor);
//         if (curWorldId >= 0 && worldId == curWorldId) {
//             return true;
//         }
//     }
//     return isUnlockedWorldHook.orig(accessor, worldId);
// });

static bool isUnlockWorldForHomeHook(GameDataHolderAccessor accessor, int worldId) {
    if (GameModeManager::instance() && GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        return true;
    }

    return GameDataFunction::isUnlockedWorld(accessor, worldId);
}

static bool isExistHomeHook(GameDataHolderAccessor accessor) {
    if (GameModeManager::instance() && GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        return true;
    }

    return GameDataFunction::isExistHome(accessor);
}

// static int exeDemoWorldSelectTalkMessageHook(TalkMessage* worldSelection) {
//     if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
//         GameDataHolderAccessor accessor(((al::LayoutActor*)worldSelection)->getSceneObjHolder());
//         int worldId = GameDataFunction::getCurrentWorldId(accessor);
//         if (worldId == GameDataFunction::getWorldIndexSand()) {
//             worldSelection->mCommonSelectParts->exeDecide();
//             return GameDataFunction::getWorldIndexLake();
//         }
//     }

//     return worldSelection->getSelectedChoiceIndex();
// }

// static void addPayShineHook(GameDataHolderWriter writer, int count) {
//     GameDataFunction::addPayShine(writer, count);
//     if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
//         int worldId = GameDataFunction::getCurrentWorldId(GameDataHolderAccessor(writer.mData));
//         if (worldId == GameDataFunction::getWorldIndexSand()) {
//             if (GameDataFunction::getPayShineNum(GameDataHolderAccessor(writer.mData)) >=
//                 GameModeManager::instance()->getMode<ArchipelagoMode>()->getWorldUnlockCount(worldId)) {
//                 GameProgressData* gameProgressData = writer.mData->getGameDataFile()->getGameProgressData();
//                 gameProgressData->mIsUnlockWorld[GameDataFunction::getWorldIndexForest()] = true;
//                 gameProgressData->mUnlockWorldNum += 1;  // = 4
//             }
//         }
//     }
// }

// =============================================================================
// Talkatoo% mode + Cappy Messenger hooks
// =============================================================================
//
// These hooks are inert until the corresponding ArchipelagoMode flags are
// flipped on:
//   - Talkatoo speech substitution:  ArchipelagoMode::setTalkatooMode(true)
//   - Cappy speech-bubble dispatch:  ArchipelagoMode::setCappyRsCalls(...)
//                                    + ArchipelagoMode::enqueueCappyMessage(...)
//
// All trampolines pass through to Orig when not in ARCHIPELAGO mode, so they
// are also safe to leave installed during multiplayer or freeze-tag modes.
//
// Symbol provenance is on each block. The mangled strings here mirror the
// entries appended to syms/main.sym in the same commit.

// ----- Talkatoo speech substitution -----
//
// Trampoline on GameDataFunction::tryFindShineMessage(const al::LiveActor*,
// const al::IUseMessageSystem*, s32 world_id, s32 index). Talkatoo's
// Poetter::exeWait picks an index from rs::calcShineIndexTableNameAvailable
// and calls this to resolve it to a char16_t* for the speech bubble. We let
// vanilla run, then if (a) Talkatoo% mode is on AND (b) the caller is a
// Poetter (vptr range-check against _ZTV7Poetter), substitute our buffer.
//
// Why vtable filter, not per-callsite hook: tryFindShineMessage is also
// called from cutscene cards, the pause-menu Power Moon list, and Achievement
// reveal popups. Substituting at those would visibly corrupt non-Talkatoo
// flows. The vtable check costs one load + one compare per call.

namespace TalkatooHook {

// Address of _ZTV7Poetter resolved at install time. 0 = symbol lookup failed
// (degraded mode — substitute hook returns vanilla for every caller).
static uintptr_t g_poetterVtableAddr = 0;

// Vtable span: _ZTV7Poetter primary table + immediately-following Poetter-only
// aux symbols (_ZTT7Poetter, _ZTC7Poetter, _ZTI7Poetter). Range-check window
// is widened from the primary table's ~0x1f8 bytes to 0x400 to also catch
// vptrs that briefly point into a construction-vtable during ctor.
constexpr uintptr_t kPoetterVtableSpan = 0x400;

// UTF-16 buffer rotation. The hook returns a char16_t* that SMO stores at
// Poetter+0x130 and reads via an EventFlow for the duration of the speech
// bubble (~3-5 s). Four slots make overlapping bubbles + re-entrant calls
// safe under the single-Talkatoo-per-scene invariant.
static constexpr size_t kUtfBufCount = 4;
static constexpr size_t kUtfBufWords = 200;
static char16_t g_utfBuffers[kUtfBufCount][kUtfBufWords] = {};
static size_t g_utfBufCursor = 0;

// Widen UTF-8-ASCII (after the chooseTalkatooSpokenUtf8 stub) into the next
// rotation slot. Returns the buffer pointer; never returns null (worst case:
// an empty buffer).
static const char16_t* asciiToUtf16BufStatic(const char* src) {
    const size_t slot = (g_utfBufCursor++) % kUtfBufCount;
    char16_t* dst = g_utfBuffers[slot];
    size_t o = 0;
    if (src) {
        while (src[o] != '\0' && o + 1 < kUtfBufWords) {
            dst[o] = static_cast<char16_t>(static_cast<unsigned char>(src[o]));
            ++o;
        }
    }
    dst[o] = 0;
    return dst;
}

// Range-check the actor's vptr (offset 0) against the Poetter vtable window.
// Treats g_poetterVtableAddr == 0 as "not a Poetter" so an install-time
// lookup failure degrades to vanilla speech instead of crashing.
static bool actorIsPoetter(const void* actor) {
    if (!actor || g_poetterVtableAddr == 0)
        return false;
    const uintptr_t vptr = *reinterpret_cast<const uintptr_t*>(actor);
    return vptr >= g_poetterVtableAddr && vptr < g_poetterVtableAddr + kPoetterVtableSpan;
}

}  // namespace TalkatooHook

static HkTrampoline<const char16_t*, const al::LiveActor*, const al::IUseMessageSystem*, int, int> tryFindShineMessageHook =
    hk::hook::trampoline([](const al::LiveActor* actor, const al::IUseMessageSystem* sys, int worldId, int index) -> const char16_t* {
        const char16_t* vanilla = tryFindShineMessageHook.orig(actor, sys, worldId, index);

        if (!GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            return vanilla;
        }
        if (!TalkatooHook::actorIsPoetter(actor)) {
            return vanilla;
        }

        ArchipelagoMode* apMode = GameModeManager::instance()->getMode<ArchipelagoMode>();
        if (!apMode->getTalkatooMode()) {
            return vanilla;
        }

        char ascii[64];
        if (!apMode->chooseTalkatooSpokenUtf8(worldId, index, ascii, sizeof(ascii))) {
            return vanilla;
        }
        return TalkatooHook::asciiToUtf16BufStatic(ascii);
    });

// ----- Talkatoo% picker non-exhaustion -----
//
// Force-false on GameDataFile::isOpenShineName under talkatoo_mode so
// rs::calcShineIndexTableNameAvailable's pool stays at full capacity (it
// counts "indices where this getter returns false"). Without this, every
// vanilla Talkatoo visit shrinks the pool by one and after enough visits
// Poetter shows the terminal "No more hints" line and tryFindShineMessage
// is never called again. See the smo_archipelago equivalent for full
// rationale (worktree's switch-mod/src/hooks/TalkatooMenuMarkHook.cpp).
//
// Pre-approved tradeoff: under talkatoo_mode the pause-menu Power Moon list
// shows NO moons as "named" (vanilla, AP, and Hint-Toad reveals all render
// unmarked). Achievement-hint reveals also re-show next session. Both
// regressions are accepted to keep the picker non-exhausting.

static HkTrampoline<bool, const GameDataFile*, int, int> isOpenShineNameHook =
    hk::hook::trampoline([](const GameDataFile* self, int worldId, int index) -> bool {
        if (!GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            return isOpenShineNameHook.orig(self, worldId, index);
        }
        if (!GameModeManager::instance()->getMode<ArchipelagoMode>()->getTalkatooMode()) {
            return isOpenShineNameHook.orig(self, worldId, index);
        }
        // Talkatoo% mode ON: force false so the picker pool stays full.
        return false;
    });

// Pass-through trampoline on GameDataFile::tryUnlockShineName. Kept hooked
// for observability only — log the first hit per session so we can confirm
// non-Talkatoo callers (Achievement reveal, Hint-Toad) exist in this build
// of SMO. Vanilla logic runs unchanged.
static HkTrampoline<bool, GameDataFile*, int, int> tryUnlockShineNameHook = hk::hook::trampoline([](GameDataFile* self, int worldId, int index) -> bool {
    static bool s_loggedFirst = false;
    if (!s_loggedFirst) {
        s_loggedFirst = true;
        sead::FixedSafeString<80> str;
        str = "[talkatoo] first tryUnlockShineName world=";
        str.append(intToCstr(worldId));
        str.append(" idx=");
        str.append(intToCstr(index));
        Client::addMessage(str.cstr());
    }
    return tryUnlockShineNameHook.orig(self, worldId, index);
});

// ----- Cappy Messenger: text-system intercept -----
//
// Four trampolines on al's per-mstxt-file message accessors. When
// CapMessageLayout::exeDelay (called from rs::tryShowCapMessagePriorityLow
// downstream) asks for ArchipelagoMode::kArchipelagoCappyLabel and a Cappy
// buffer is currently live, return our UTF-16 buffer and synthesize the
// "label exists" probe. All four are hooked because exeDelay dispatches
// through either the System or Stage variant based on
// CapMessageShowInfo::isStageMessage; rs::tryShowCapMessagePriorityLow uses
// the System path but defensive hooking of both costs little and protects
// against future code that uses the Stage path.

static HkTrampoline<bool, const al::IUseMessageSystem*, const char*, const char*> isExistLabelInSystemMessageHook =
    hk::hook::trampoline([](const al::IUseMessageSystem* sys, const char* mstxt, const char* label) -> bool {
        if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            if (GameModeManager::instance()->getMode<ArchipelagoMode>()->lookupCappyMessageSubstitution(label) != nullptr) {
                return true;
            }
        }
        return isExistLabelInSystemMessageHook.orig(sys, mstxt, label);
    });

static HkTrampoline<const char16_t*, const al::IUseMessageSystem*, const char*, const char*> getSystemMessageStringTrampolineHook =
    hk::hook::trampoline([](const al::IUseMessageSystem* sys, const char* mstxt, const char* label) -> const char16_t* {
        if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            const char16_t* sub = GameModeManager::instance()->getMode<ArchipelagoMode>()->lookupCappyMessageSubstitution(label);
            if (sub)
                return sub;
        }
        return getSystemMessageStringTrampolineHook.orig(sys, mstxt, label);
    });

static HkTrampoline<bool, const al::IUseMessageSystem*, const char*, const char*> isExistLabelInStageMessageHook =
    hk::hook::trampoline([](const al::IUseMessageSystem* sys, const char* mstxt, const char* label) -> bool {
        if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            if (GameModeManager::instance()->getMode<ArchipelagoMode>()->lookupCappyMessageSubstitution(label) != nullptr) {
                return true;
            }
        }
        return isExistLabelInStageMessageHook.orig(sys, mstxt, label);
    });

static HkTrampoline<const char16_t*, const al::IUseMessageSystem*, const char*, const char*> getStageMessageStringHook =
    hk::hook::trampoline([](const al::IUseMessageSystem* sys, const char* mstxt, const char* label) -> const char16_t* {
        if (GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO)) {
            const char16_t* sub = GameModeManager::instance()->getMode<ArchipelagoMode>()->lookupCappyMessageSubstitution(label);
            if (sub)
                return sub;
        }
        return getStageMessageStringHook.orig(sys, mstxt, label);
    });