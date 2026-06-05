#include "sead/heap/seadHeap.h"
#include "sead/prim/seadSafeString.h"

#include "al/Library/Base/StringUtil.h"
#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Scene/SceneObjUtil.h"

#include "game/Item/CoinCollectHolder.h"
#include "game/Item/ShineInfo.h"
#include "game/Player/HackCap.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Scene/StageScene.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/GameDataHolderWriter.h"
#include "game/System/GameProgressData.h"
#include "game/System/WorldList.h"
#include "game/Util/ActorDimensionKeeper.h"
#include "game/Util/ObjUtil.h"
#include "game/Util/PlayerUtil.h"

#include "basis/seadNew.h"
#include "helpers.hpp"
#include "imgui.h"
#include "logger.hpp"
#include "puppets/PuppetInfo.h"
#include "rs/util.hpp"
#include "Scene/StageSceneStateModConfig.hpp"
#include "server/archipelago/ArchipelagoConfigMenu.hpp"
#include "server/archipelago/ArchipelagoHelpers.hpp"
#include "server/archipelago/ArchipelagoMode.hpp"
#include "server/Client.hpp"
#include "server/DeltaTime.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "System/PlayerHitPointData.h"

void ArchipelagoMode::isSubArea(GameDataHolderAccessor accessor, bool* isInSubArea, sead::FixedSafeString<32> stageId) {
    *isInSubArea = !GameDataFunction::isMainStage(accessor);

    // Handle Sub Area -> Sub Area connections
    if (*isInSubArea) {
        const char* subAreaExclusions[] = {"SnowWorldTown", "Underground000", "Special2WorldLavaStage", "Revenge"};
        for (int i = 0; i < sizeof(subAreaExclusions) / sizeof(subAreaExclusions[0]); i++) {
            if (isPartOf(GameDataFunction::getCurrentStageName(accessor), subAreaExclusions[i])) {
                // Stage Ids that lead into the sub area from an overworld
                // might need to add darker side transitions and dark side transitions
                // possibly re add Under01
                const char* excludedStageIds[] = {"SnowUG",       "CP_Entrance", "MoonGoal",         "MofumofuA", "BossMagmaA",
                                                  "BossKnuckleA", "BossForestA", "GiantWanderBossA", "BossRaidA"};
                *isInSubArea = false;
                for (int j = 0; j < sizeof(excludedStageIds) / sizeof(excludedStageIds[0]); j++) {
                    *isInSubArea = isPartOf(stageId.cstr(), excludedStageIds[j]);
                    if (*isInSubArea) {
                        break;
                    }
                }
                break;
            }
        }
    }
}

void ArchipelagoMode::getCustomStageId(GameDataHolderAccessor accessor, const ChangeStageInfo* info, sead::FixedSafeString<64>* stageId) {
    if (isPartOf(info->getStageName(), "LavaBonus") || isPartOf(GameDataFunction::getCurrentStageName(accessor), "LavaBonus")) {
        *stageId = "town_lava";
    }

    if (isPartOf(info->getStageName(), "LavaWorldShopStage") || isPartOf(GameDataFunction::getCurrentStageName(accessor), "LavaWorldShopStage")) {
        *stageId = "shop_lava";
    }

    if (isPartOf(info->getStageName(), "SphinxEx")) {
        *stageId = "run00";
    }

    if (isPartOf(info->getStageName(), "Revenge") || isPartOf(GameDataFunction::getCurrentStageName(accessor), "Picture")) {
        *stageId = "PictureBoss";
        if (isPartOf(GameDataFunction::getCurrentStageName(accessor), "Knuckle"))
            stageId->append("Knuckle");

        if (isPartOf(GameDataFunction::getCurrentStageName(accessor), "Mofumofu"))
            *stageId = "PictureMofumofu";

        if (isPartOf(GameDataFunction::getCurrentStageName(accessor), "Forest"))
            stageId->append("Forest");

        if (isPartOf(GameDataFunction::getCurrentStageName(accessor), "Wander"))
            *stageId = "PictureGiantWanderBoss";

        if (isPartOf(GameDataFunction::getCurrentStageName(accessor), "Raid"))
            stageId->append("Raid");

        if (isPartOf(GameDataFunction::getCurrentStageName(accessor), "Magma"))
            stageId->append("Magma");
    }
}

void ArchipelagoMode::correctCustomStageId(sead::FixedSafeString<64>* toStageId) {
    if (al::isEqualString(toStageId->cstr(), "town_lava")) {
        *toStageId = "town";
    }

    if (al::isEqualString(toStageId->cstr(), "shop_lava")) {
        *toStageId = "shop";
    }
}

// Doesn't handle achieve
int ArchipelagoMode::getNumGotShines() {
    GameDataHolderAccessor accessor(mCurScene);
    int totalShines = GameDataFunction::getWorldTotalShineNum(accessor, mCurWorldShineList);
    int curShines = 0;
    for (int k = 0; k < 0x400; k++) {
        GameDataFile::HintInfo curHintInfo = accessor.mData->getGameDataFile()->getHintList()[k];
        if (curHintInfo.worldId == mCurWorldShineList && curHintInfo.isGet) {
            curShines += 1;
        }
    }

    return curShines;
}

int ArchipelagoMode::getNumCoinCollect() {
    GameDataHolderAccessor accessor(mCurScene);
    GameDataFile* gameDataFile = accessor.mData->getGameDataFile();
    return gameDataFile->getCoinCollectGotNum(mRelativeWorldCoinCollect) - gameDataFile->getUseCoinCollectNum(mRelativeWorldCoinCollect);
}

void ArchipelagoMode::setRelativeWorldCoinCollect(const char* stageName) {
    GameDataHolderAccessor accessor(mCurScene);
    sead::FixedSafeString<64> toStageCoinCollect = sead::FixedSafeString<64>();
    if (isPartOf(stageName, "WorldShop")) {
        toStageCoinCollect = stageName;
        toStageCoinCollect.replaceString("WorldShopStage", "");
        toStageCoinCollect.replaceString("WorldShop01Stage", "");
    } else {
        toStageCoinCollect = getWorldStageNameByRegionalCoinStageList(stageName);
        toStageCoinCollect.replaceString("WorldHomeStage", "");
    }
    mRelativeWorldCoinCollect = GameDataFunction::findWorldIdByDevelopName(accessor, toStageCoinCollect.cstr());
}

// Death Link handling
void ArchipelagoMode::handleDeathLink(PlayerActorBase* playerBase, PlayerActorHakoniwa* playerHakoniwa, GameDataHolderWriter writer) {
    if (!PlayerFunction::isPlayerDeadStatus(playerBase) && mApDeath) {
        GameDataFunction::killPlayer(writer);
        playerBase->startDemoPuppetable();
        al::setVelocityZero(playerBase);
        rs::faceToCamera(playerBase);
        playerHakoniwa->mAnimator->endSubAnim();
        playerHakoniwa->mAnimator->startAnimDead();
        mApDeath = false;
    }

    if (PlayerFunction::isPlayerDeadStatus(playerBase) && !mDying) {
        Client::sendDeathlinkPacket();
        mDying = true;
    }

    if (!PlayerFunction::isPlayerDeadStatus(playerBase) && mDying) {
        mDying = false;
    }
}

// Capture Sanity Enforcement
void ArchipelagoMode::handleCaptureSanity(PlayerActorBase* playerBase, GameDataHolderAccessor accessor) {
    if (mCapturesEnabled) {
        al::LiveActor* curHack = playerBase->getPlayerHackKeeper()->mHackModel;
        const char* hackName = playerBase->getPlayerHackKeeper()->getCurrentHackName();
        if (hackName != nullptr && !hasCapture(hackName) && mIsRecordCapture) {
            if (!(al::isEqualString(hackName, "ElectricWire") && getScenario(0) < 2 && GameDataFunction::getCurrentWorldId(accessor) == 0)) {
                // Client::Client::addMessage(hackNamehackName);
                if (!playerBase->getPlayerHackKeeper()->isActiveHackStartDemo()) {
                    bool tryEscape = false;
                    // 29 (pole) removed
                    int nonKillCaptures[7] = {10, 13, 24, 25, 28, 37};
                    for (int i = 0; i < 7; i++) {
                        tryEscape = al::isEqualString(captureListNames[nonKillCaptures[i]], hackName);
                        if (tryEscape) {
                            break;
                        }
                    }
                    if (tryEscape) {
                        playerBase->getPlayerHackKeeper()->tryEscapeHack();
                    } else {
                        playerBase->getPlayerHackKeeper()->forceKillHack();
                    }
                    mIsRecordCapture = false;
                }
            }
        }
    }
}

void ArchipelagoMode::getNearestRegional(StageScene* stageScene, PlayerActorBase* playerBase) {
    if (al::isExistSceneObj(stageScene, 7)) {
        CoinCollectHolder* coinCollectHolder = (CoinCollectHolder*)al::getSceneObj(stageScene, 7);
        if (coinCollectHolder) {
            CoinCollect* coinCollect = coinCollectHolder->tryFindAliveCoinCollect(al::getTrans(playerBase), true);
            if (coinCollect) {
                mHintArrow->setTarget(al::getTransPtr(coinCollect));
                mCoinCollectHintTarget = coinCollect;
                coinCollect->appearHelpAmiiboEffect();
            } else {
                CoinCollect2D* coinCollect2D = coinCollectHolder->tryFindAliveCoinCollect2D(al::getTrans(playerBase), true);

                if (coinCollect2D) {
                    mHintArrow->setTarget(al::getTransPtr(coinCollect2D));
                    mCoinCollectHintTarget = coinCollect2D;
                    coinCollect2D->appearHintEffect();
                }
            }
        }
    }
}

void ArchipelagoMode::handleSoftLocks(GameDataHolderAccessor accessor, GameDataHolderWriter writer) {
    // softlock prevention
    GameProgressData* gameProgressData = accessor.mData->getGameDataFile()->getGameProgressData();
    // Doesn't fix soft lock...
    // if (gameProgressData->mWaterfallWorldProgress != GameProgressData::WaterfallWorldProgress::TalkedCapNearHome) {
    //     gameProgressData->mWaterfallWorldProgress = GameProgressData::WaterfallWorldProgress::TalkedCapNearHome;
    // }

    for (int i = 0; i < mStoryShineArray.size(); i++) {
        if (mStoryShineArray[i] && hasShine(mStoryShineArray[i]->mShineIdx)) {
            mStoryShineArray[i]->onSwitchGet();
            // find different form of assignment
            // mStoryShineArray[i] = nullptr;
        }
    }

    if (accessor.mData->getGameDataFile()->isUseMissRestartInfo()) {
        accessor.mData->getGameDataFile()->setIsUseMissRestartInfo(false);
    }

    // Lost and Ruined Odyssey softlock handling
    if (gameProgressData->mHomeStatus == GameProgressData::HomeStatus::CrashedHome ||
        gameProgressData->mHomeStatus == GameProgressData::HomeStatus::BossAttackedHome) {
        gameProgressData->mHomeStatus = GameProgressData::HomeStatus::LaunchedHome;
    }

    if (mSoftlockTimer >= 60) {
        // Check and prevent crashed home softlock no longer needed
        // if (GameDataFunction::isBossAttackedHome(accessor) && GameDataFunction::isUnlockedWorld(accessor, GameDataFunction::getWorldIndexBoss())) {
        //     // Client::Client::addMessage(GameDataFunction::getCurrentStageName(accessor));
        //     if (strcmp(GameDataFunction::getCurrentStageName(accessor), "BossRaidWorldHomeStage") == 0) {
        //         GameDataFunction::repairHomeByCrashedBoss(writer);
        //         GameDataFunction::crashHome(writer);
        //         // isGotShine crashes game here for some reason
        //         /*int ruinedCount = 0;
        //         if (GameDataFunction::isGotShine(accessor, GameDataFunction::getWorldIndexBoss(),
        //                                             0)) {
        //             ruinedCount += 3;
        //         }

        //         for (int i = 1; i < 9; i++) {
        //             if (GameDataFunction::isGotShine(accessor, GameDataFunction::getWorldIndexBoss(),
        //                                                 i)) {
        //                 ruinedCount++;
        //             }
        //         }
        //         if (ruinedCount < Client::getRaidCount()) {
        //             GameDataFunction::repairHome(accessor);
        //         } else {
        //             GameDataFunction::bossAttackHome(accessor);
        //         }*/
        //     } else {
        //         GameDataFunction::repairHome(writer);
        //     }
        // }

        // Edge case where game repairs odyssey in ruined but doesn't unlock bowser kingdom
        // if (GameDataFunction::isRepairHomeByCrashedBoss(accessor)) {
        //     GameDataFunction::unlockWorld(writer, GameDataFunction::getWorldIndexSky());
        // }

        // Check for Cloud to prevent early Odyssey in ER may be uneeded
        // if (GameDataFunction::isUnlockedWorld(accessor, GameDataFunction::getWorldIndexCloud())) {
        //     // Check for lost kingdom softlock state
        //     if (GameDataFunction::isCrashHome(accessor)) {
        //         if (strcmp(GameDataFunction::getCurrentStageName(accessor), "ClashWorldHomeStage") == 0) {
        //             int lostCount = 0;
        //             for (int i = 1; i < 25; i++) {
        //                 if (GameDataFunction::isGotShine(accessor, GameDataFunction::getWorldIndexClash(), i))
        //                     lostCount++;
        //             }
        //             if (lostCount < getWorldUnlockCount(GameDataFunction::getWorldIndexClash())) {
        //                 GameDataFunction::repairHome(writer);
        //                 GameDataFunction::unlockWorld(writer, GameDataFunction::getWorldIndexClash());
        //             } else {
        //                 GameDataFunction::crashHome(writer);
        //             }
        //         } else {
        //             GameDataFunction::repairHome(writer);
        //         }
        //     }
        // }

        mSoftlockTimer = 0;
    }
}

void ArchipelagoMode::updateCounter(PlayerActorBase* playerBase, GameDataHolderAccessor accessor) {
    // Moon Shard Updater
    // Prevents softlock when moon is received mid shard moon
    if (!(al::isEqualString(GameDataFunction::tryGetCurrentMainStageName(accessor), "CapWorldHomeStage") && getScenario(0) < 2) &&
        rs::isExistShineChipWatcher(playerBase) && rs::getShineChipCount(playerBase) > 0) {
        Client::startShineChipCount();
    }

    if (mUpdateCounterTimer >= 1800) {
        if (mIsNeedUpdateCounter) {
            Client::startShineCount();
            mIsNeedUpdateCounter = false;
        }
        mUpdateCounterTimer = 0;
    }
}

int ArchipelagoMode::getRelativeWorldCoinCollectCheckGotNum(GameDataHolderAccessor accessor) {
    sead::FixedSafeString<64> currentWorldStageName = sead::FixedSafeString<64>();
    currentWorldStageName = GameDataFunction::getWorldDevelopName(accessor, mRelativeWorldCoinCollect);
    currentWorldStageName.append("WorldHomeStage");

    int indexCurrentHomeStage = getIndexRegionalCoinStageList(currentWorldStageName.cstr());

    int regionalCoinCheckGotNum = 0;

    int index = 0;
    for (int k = 0; k < indexCurrentHomeStage; k++) {
        index += regionalCoinListLengths[k];
    }

    for (size_t i = indexCurrentHomeStage; i < sizeof(regionalCoinStages) / sizeof(regionalCoinStages[0]); i++) {
        if (i != indexCurrentHomeStage && isPartOf(regionalCoinStages[i], "WorldHomeStage"))
            break;

        for (size_t j = 0; j < regionalCoinListLengths[i]; j++) {
            if (hasRegionalCoin(index + j)) {
                regionalCoinCheckGotNum += 1;
            }
        }
        index += regionalCoinListLengths[i];
    }

    return regionalCoinCheckGotNum - 1;
}

void ArchipelagoMode::calculateShineScenarios() {
    for (int i = 0; i < 14; i++) {
        if (hasShine(shineScenarios[i].shineUid)) {
            setScenario(shineScenarios[i].worldId, shineScenarios[i].scenario);
        }
    }
}

int ArchipelagoMode::getSubAreaScenario(const char* toStageName) {
    int toScenario = -1;
    if (al::isEqualString(toStageName, "ForestWorldBossStage"))
        toScenario = getScenario(GameDataFunction::getWorldIndexForest()) > 2 ? 2 : 1;

    if (al::isEqualString(toStageName, "ForestWorldWoodsStage"))
        toScenario = getScenario(GameDataFunction::getWorldIndexForest()) > 1 ? 2 : 1;

    if (al::isEqualString(toStageName, "CapWorldTowerStage"))
        toScenario = 2;

    return toScenario;
}
