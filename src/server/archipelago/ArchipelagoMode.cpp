#include "server/archipelago/ArchipelagoMode.hpp"

#include "hk/svc/api.h"  // hk::svc::getSystemTick — Cappy settle-gate wallclock

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
#include "al/Library/Screen/ScreenFunction.h"

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
#include "server/Client.hpp"
#include "server/DeltaTime.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "System/PlayerHitPointData.h"

ArchipelagoMode::ArchipelagoMode(const char* name) : GameModeBase(name) {}

void ArchipelagoMode::init(const GameModeInitInfo& info) {
    // mHeap = sead::ExpHeap::create(60000, "ArchipelagoHeap", sead::HeapMgr::instance()->getCurrentHeap(), 8, sead::Heap::cHeapDirection_Forward, false);
    // // Approx size = 50608
    // // Approx 9392 extra bytes allocated
    // sead::ScopedCurrentHeapSetter heapSetter(mHeap);

    mSceneObjHolder = info.mSceneObjHolder;
    mMode = info.mMode;
    mCurScene = (StageScene*)info.mScene;
    mPuppetHolder = info.mPuppetHolder;
    GameDataHolderAccessor accessor(mCurScene);

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<ArchipelagoInfo>();

    if (curGameInfo)
        Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    else
        Logger::log("No gamemode info found\n");
    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (ArchipelagoInfo*)curGameInfo;
        // mModeTimer = new GameModeTimer(mRoundTimer);
    } else {
        if (curGameInfo)
            delete curGameInfo;  // attempt to destory previous info before creating new one
        mInfo = GameModeManager::instance()->createModeInfo<ArchipelagoInfo>();
        mWorldScenarios.fill(1);

        mWorldPayCounts.fill(-1);

        clearCollectibles();

        shineTextReplacements.fill({0, 0});
        // mShineItemNames.fill(sead::FixedSafeString<40>());
        shineColors.fill(0);

        mShopCapTextReplacements.fill({254, 255, 255, 255});
        mShopClothTextReplacements.fill({254, 255, 255, 255});
        mShopRegionalTextReplacements.fill({254, 255, 255, 255});
        // shopGiftTextReplacements.fill({254, 255, 255, 255});
        mShopMoonTextReplacements = {254, 255, 255, 255};

        mGameNames.fill(sead::FixedSafeString<APNAMESIZE>());
        mSlotNames.fill(sead::FixedSafeString<APNAMESIZE>());
        mItemNames.fill(sead::FixedSafeString<APNAMESIZE>());

        mOverworldStageConnections.fill({255, 255});
        mSubAreaStageConnections.fill({255, 255});

        mLastERStageId = sead::FixedSafeString<128>();
        mLastERStageName = sead::FixedSafeString<128>();
        mLastExitStageId = sead::FixedSafeString<128>();
        mLastExitStageName = sead::FixedSafeString<128>();

        mStoryShineArray.allocBuffer(10, nullptr);  // max of 10 shine actors in buffer to account for story moons and multi moons
    }

    Logger::log("Scene Heap Free Size: %f/%f\n", al::getSceneHeap()->getFreeSize() * 0.001f, al::getSceneHeap()->getSize() * 0.001f);

    Logger::log("Scene Heap Free Size: %f/%f\n", al::getSceneHeap()->getFreeSize() * 0.001f, al::getSceneHeap()->getSize() * 0.001f);

    if (!GameModeManager::instance()->isActive())
        GameModeManager::instance()->toggleActive();

    // Clear main shine array
    clearArrays();

    // Create hint arrow
    mHintArrow = new ArchipelagoHintArrow("CheckHintArrow");
    mHintArrow->init(*info.mActorInitInfo);

    if (!GameDataFunction::isHomeShipStage(accessor.mData)) {
        Client::sendChangeStagePacket(info.mSceneObjHolder);
    }
    int worldId = GameDataFunction::getCurrentWorldId(info.mSceneObjHolder);
    int worldScenario = GameDataFunction::getWorldScenarioNo(info.mSceneObjHolder, worldId);

    if (worldScenario > getScenario(worldId)) {
        setScenario(worldId, worldScenario);
    }
}

void ArchipelagoMode::begin() {
    unpause();
    if (mInfo && mInfo->mIsClientConnected == ArchipelagoState::NOT_CONNECTED) {
        mInfo->isNeedArchipelagoConnect = true;
    }

    mCoinCollectHintTarget = nullptr;
    StageSceneStateModConfig::setCostumeDoorsUnlocked(false);
    PlayerHitPointData* hit = GameDataHolderAccessor(mCurScene)->getGameDataFile()->getPlayerHitPointData();

    GameDataHolderAccessor accessor(mCurScene);
    GameDataHolderWriter writer(mCurScene);

    GameModeBase::begin();

    // mCurScene->stageSceneLayout->end();
}

void ArchipelagoMode::end() {
    pause();

    mCurScene->stageSceneLayout->start();
    if (!GameModeManager::instance()->isPaused()) {
    }

    GameModeBase::end();
}

void ArchipelagoMode::pause() {
    GameModeBase::pause();
}

void ArchipelagoMode::unpause() {
    GameModeBase::unpause();
    mIsActive = true;
}

PlayerActorHakoniwa* ArchipelagoMode::getPlayerActorHakoniwa() {
    PlayerActorBase* playerBase = (PlayerActorBase*)rs::getPlayerActor(mCurScene);
    bool isYukimaru = !playerBase->getPlayerInfo();

    if (isYukimaru)
        return nullptr;

    return (PlayerActorHakoniwa*)playerBase;
}

void ArchipelagoMode::setDying(bool value) {
    mDying = value;
}

void ArchipelagoMode::setApDeath(bool value) {
    mApDeath = value;
}

ChangeStageInfo* ArchipelagoMode::handleER(const ChangeStageInfo* info) {
    if (!mIsEntranceRandomizationEnabled) {
        setRelativeWorldCoinCollect(info->getStageName());
        return nullptr;
    }

    GameDataHolderAccessor accessor(mCurScene);

    sead::FixedSafeString<64> stageId = sead::FixedSafeString<64>();
    stageId = info->getChangeStageId();

    mLastExitStageId = stageId.cstr();
    mLastExitStageName = GameDataFunction::getCurrentStageName(accessor);

    // Gets custom stageIds added by Archipelago based on kingdom to prevent duplicates
    getCustomStageId(accessor, info, &stageId);

    // if (mGoal == GameDataFunction::getWorldIndexMoon() && al::isEqualString(stageId.cstr(), "aaa") &&
    //     !accessor.mData->getGameDataFile()->getGameProgressData()->mIsUnlockWorld[GameDataFunction::getWorldIndexMoon()]) {
    //     Client::addMessage("Unlock Moon Kingdom normally to enter the church.");
    //     ChangeStageInfo toInfo(accessor.mData, "aaa", "MoonWorldHomeStage", false, -1, static_cast<ChangeStageInfo::SubScenarioType>(0));
    //     toInfo.mPlacementString = info->getPlacementString();
    //     return &toInfo;
    // }

    int stageIndex = getIndexStageNameList(info->getStageName());
    int stageIdIndex = getIndexStageIdList(stageId.cstr());
    bool isInSubArea = false;
    isSubArea(accessor, &isInSubArea, stageId);

    sead::FixedSafeString<64> toStageId = sead::FixedSafeString<64>();
    sead::FixedSafeString<64> toStageName = sead::FixedSafeString<64>();

    sead::FixedSafeString<128> foo = sead::FixedSafeString<128>();
    foo = "";
    foo.append(info->getStageName());
    foo.append(", ");
    foo.append(info->getChangeStageId());
    foo.append(", ");
    foo.append(info->getPlacementString());
    // Client::addMessage(foo.cstr());
    if (stageIdIndex < 0) {
        foo = "";
        foo.append("Error, Invalid Stage ID ");
        foo.append(stageId.cstr());
        mLastERStageId = stageId.cstr();
        Client::addMessage(foo.cstr());
        setRelativeWorldCoinCollect(info->getStageName());
        return nullptr;
    }

    // Handle Sub area entrances in sub areas (i.e Shiveria)
    // Might fail on snow sub areas ammend with list of strictly effects maps
    stageConnection currentStageConnection;
    if (isInSubArea) {
        // Access Sub World conenctions
        currentStageConnection = mSubAreaStageConnections[stageIdIndex];
    } else {
        // Access Over World connections
        currentStageConnection = mOverworldStageConnections[stageIdIndex];
    }

    toStageId = stageIdList[currentStageConnection.toStageIdIndex];
    toStageName = stageNameList[currentStageConnection.toStageNameIndex];
    // Corrects custom stageIds added by Archipelago back to the base game stageId
    // Prevents need to change stageIds in game files.
    correctCustomStageId(&toStageId);

    // Add GetSubAreaScenario function for scenario dependent sub areas
    // like top hat tower and wooded boss arena, and deep woods
    int toScenario = isPartOf(toStageName.cstr(), "WorldHomeStage") ? getScenario(toStageName.cstr()) : getSubAreaScenario(toStageName.cstr());

    setRelativeWorldCoinCollect(toStageName.cstr());

    ChangeStageInfo toInfo(accessor.mData, toStageId.cstr(), toStageName.cstr(), false, toScenario, static_cast<ChangeStageInfo::SubScenarioType>(0));
    toInfo.mPlacementString = info->getPlacementString();
    foo = "";
    foo.append(toStageId);
    foo.append(", ");
    foo.append(toStageName);
    foo.append(", ");
    foo.append(toInfo.getPlacementString());
    Client::addMessage(foo.cstr());

    mLastERStageId = toInfo.mChangeStageId.cstr();
    mLastERStageName = toInfo.mChangeStageName.cstr();

    return &toInfo;
    // return nullptr
}

bool ArchipelagoMode::isTargetAlive() {
    if (mCoinCollectHintTarget) {
        return al::isAlive(mCoinCollectHintTarget);
    }

    return false;
}

bool ArchipelagoMode::trySetHintTargetValid() {
    mInfo->mIsHintTargetValid = isTargetAlive();

    return true;
}

void ArchipelagoMode::setScenario(int worldID, int scenario) {
    mWorldScenarios[worldID] = scenario;
}

bool ArchipelagoMode::setScenario(const char* worldName, int scenario) {
    GameDataHolderAccessor accessor(mCurScene);

    int worldID = accessor.mData->mWorldList->tryFindWorldIndexByStageName(worldName);
    if (scenario == -1) {
        // Client::addMessage("ChangeStageInfo failed to init");
    }

    // Exclude revisitable scenarios like festival
    if (!(al::isEqualString(worldName, "CityWorldHomeStage") && scenario == 3)) {
        if (scenario != getScenario(worldID) && scenario <= accessor.mData->mWorldList->getMoonRockScenarioNo(worldID) &&
            !GameDataFunction::isUnlockedWorld(accessor, worldID)) {
            if (getScenario(worldID) < scenario) {
                // Client::addMessage("Scenario Updated");
                setScenario(worldID, scenario);
            }
            return true;
        }
    }
    return false;
}

int ArchipelagoMode::getScenario(const char* worldName) {
    GameDataHolderAccessor accessor(mCurScene);

    int worldID = accessor.mData->mWorldList->tryFindWorldIndexByStageName(worldName);

    /*if (worldScenarios[worldID] < GameDataFunction::getWorldScenarioNo(accessor, worldID))
    {
        setScenario(worldID, GameDataFunction::getWorldScenarioNo(accessor, worldID));
    }*/
    return mWorldScenarios[worldID];
}

int ArchipelagoMode::getScenario(int worldID) {
    return mWorldScenarios[worldID];
}

void ArchipelagoMode::sendCorrectScenario(const ChangeStageInfo* stageInfo) {
    GameDataHolderWriter writer(mCurScene);
    // try changing isReturn (param_4)
    /*if (stageInfo->isReturn)
    {
        Client::addMessage("isReturn: True");

    } else {
        Client::addMessage("isReturn: False");
    }
    sead::FixedSafeString<40> str;
    str = "";
    str.append("subScenario type: ");
    str.append(static_cast<char>(48 + static_cast<unsigned int>(stageInfo->subType)));
    Client::addMessage(str.cstr());*/
    ChangeStageInfo info(writer.mData, stageInfo->mChangeStageId.cstr(), stageInfo->mChangeStageName.cstr(), false,
                         getScenario(stageInfo->mChangeStageName.cstr()), static_cast<ChangeStageInfo::SubScenarioType>(0));
    GameDataFunction::tryChangeNextStage(writer, &info);
}

void ArchipelagoMode::setCheckIndex(int index) {
    mCheckIndex = index;
}

void ArchipelagoMode::setWorldUnlockCount(int worldId, int count) {
    mWorldPayCounts[worldId] = count;
}

int ArchipelagoMode::getWorldUnlockCount(int worldId) {
    return mWorldPayCounts[worldId];
}

void ArchipelagoMode::setGameName(int index, const char* name) {
    mGameNames[index] = mGameNames[index].cEmptyString;
    mGameNames[index].append(name);
}

void ArchipelagoMode::setSlotName(int index, const char* name) {
    mSlotNames[index] = mSlotNames[index].cEmptyString;
    mSlotNames[index].append(name);
}

void ArchipelagoMode::setItemName(int index, const char* name) {
    mItemNames[index] = mItemNames[index].cEmptyString;
    mItemNames[index].append(name);
}

void ArchipelagoMode::setShineTextReplacement(int index, replaceText replace) {
    shineTextReplacements[index] = replace;
}

void ArchipelagoMode::setShineColors(int index, u8 replace) {
    shineColors[index] = replace;
}

void ArchipelagoMode::setCapTextReplacement(int index, shopReplaceText replace) {
    if (index < mShopCapTextReplacements.size())
        mShopCapTextReplacements[index] = replace;
}

void ArchipelagoMode::setClothesTextReplacement(int index, shopReplaceText replace) {
    if (index < mShopClothTextReplacements.size())
        mShopClothTextReplacements[index] = replace;
}

void ArchipelagoMode::setRegionalTextReplacement(int index, shopReplaceText replace) {
    if (index < mShopRegionalTextReplacements.size())
        mShopRegionalTextReplacements[index] = replace;
}

void ArchipelagoMode::setShopMoonTextReplacement(shopReplaceText replace) {
    mShopMoonTextReplacements = replace;
}

void ArchipelagoMode::setOverWorldStageConnection(int index, stageConnection replace) {
    if (index < mOverworldStageConnections.size())
        mOverworldStageConnections[index] = replace;
}

void ArchipelagoMode::setSubAreaStageConnection(int index, stageConnection replace) {
    if (index < mSubAreaStageConnections.size())
        mSubAreaStageConnections[index] = replace;
}

void ArchipelagoMode::addShine(int uid) {
    int shines = mCollectedShines[uid / 8];

    int index = (uid / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (index == uid) {
            shines = shines | i;
            break;
        }
        if (i == 0x100) {
            sead::FixedSafeString<60> str;
            str = "";
            str.append("Shine UID ");
            str.append(intToCstr(uid));
            str.append(" failed to add to shine list at index ");
            str.append(intToCstr(index));
            Client::addMessage(str.cstr());
            break;
        }
        i = i << 1;
        index += 1;
    }

    mCollectedShines[uid / 8] = shines;
}

void ArchipelagoMode::setRecentShineHintIndex(int index) {
    mRecentShineHintIndex = index;
}

bool ArchipelagoMode::hasShine(int uid) {
    int shines = mCollectedShines[uid / 8];

    int index = (uid / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (index == uid) {
            shines = shines & i;
            return (shines == i);
        }
        if (i == 0x100) {
            sead::FixedSafeString<60> str;
            str = "";
            str.append("Shine UID ");
            str.append(intToCstr(uid));
            str.append(" failed to find in shine list at index ");
            str.append(intToCstr(index));
            Client::addMessage(str.cstr());
            break;
        }
        i = i << 1;
        index += 1;
    }
    return false;
}

int ArchipelagoMode::getShineChecks(int index) {
    return mCollectedShines[index];
}

void ArchipelagoMode::setShineChecks(int index, int checks) {
    mCollectedShines[index] = checks;
}

// ===== Talkatoo% mode =====
// Mirrors addShine/hasShine's packed-bitmap layout (1 bit per uid). The uid
// range covered is 0..(148*8 - 1) = 0..1183, which spans the apworld's
// current max uid (1166 — "Lake Gardening: Spiky Passage Seed"). Out-of-range
// uids are dropped silently rather than overflowing the SafeArray.
void ArchipelagoMode::markMoonNamed(int uid) {
    if (uid < 0 || uid / 8 >= static_cast<int>(mNamedShines.size())) {
        sead::FixedSafeString<48> str;
        str = "Talkatoo named uid OOB: ";
        str.append(intToCstr(uid));
        Client::addMessage(str.cstr());
        return;
    }
    mNamedShines[uid / 8] = static_cast<u8>(mNamedShines[uid / 8] | (1 << (uid % 8)));
}

void ArchipelagoMode::clearNamedMoons() {
    mNamedShines.fill(0);
}

bool ArchipelagoMode::isMoonNamed(int uid) const {
    if (uid < 0 || uid / 8 >= static_cast<int>(mNamedShines.size()))
        return false;
    return (mNamedShines[uid / 8] & (1 << (uid % 8))) != 0;
}

bool ArchipelagoMode::chooseTalkatooSpokenUtf8(int /*world_id*/, int index, char* out, u32 out_cap) {
    if (!mTalkatooMode || !out || out_cap < 16)
        return false;

    // STUB rotation. The vanilla picker passes a stable `index` per visit
    // when its pool is non-empty; we cycle through three probe strings so
    // each Talkatoo visit visibly shows a different bubble during bring-up.
    // Replace with: pick `index`-th moon from the per-kingdom
    // named-but-uncollected set populated by markMoonNamed.
    static const char* kProbe[3] = {
        "Power Moon 1",
        "Power Moon 2",
        "Power Moon 3",
    };
    const u32 slot = static_cast<u32>(index % 3 + 3) % 3;
    const char* src = kProbe[slot];

    u32 i = 0;
    while (src[i] != '\0' && i + 1 < out_cap) {
        out[i] = src[i];
        ++i;
    }
    out[i] = '\0';
    return true;
}

void ArchipelagoMode::addOutfit(const ShopItem::ItemInfo* info) {
    int index = getIndexApCostumeList(info->name) + 44 * static_cast<int>(info->type);

    int outfits = mCollectedOutfits[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            outfits = outfits | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mCollectedOutfits[index / 8] = outfits;
}

bool ArchipelagoMode::hasOutfit(const ShopItem::ItemInfo* info) {
    int index = getIndexApCostumeList(info->name) + 44 * static_cast<int>(info->type);
    if (index == -1) {
        // Client::addMessage(info->mName);
        return false;
    }

    u8 outfits = mCollectedOutfits[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            outfits = outfits & i;
            return (outfits == i);
        }
        i = i << 1;
        curIndex += 1;
    }

    return false;
}

int ArchipelagoMode::getOutfitChecks(int index) {
    return static_cast<int>(mCollectedOutfits[index]);
}

void ArchipelagoMode::setOutfitChecks(int index, int checks) {
    u8 u8Checks = static_cast<u8>(checks);
    mCollectedOutfits[index] = u8Checks;
}

void ArchipelagoMode::addSticker(const ShopItem::ItemInfo* info) {
    int index = getIndexStickerList(info->name);

    int stickers = mCollectedStickers[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            stickers = stickers | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mCollectedStickers[index / 8] = stickers;
}

bool ArchipelagoMode::hasSticker(const ShopItem::ItemInfo* info) {
    int index = getIndexStickerList(info->name);
    if (index == -1) {
        // Client::addMessage(info->mName);
        return false;
    }

    u8 stickers = mCollectedStickers[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            stickers = stickers & i;
            return (stickers == i);
        }
        i = i << 1;
        curIndex += 1;
    }

    return false;
}

int ArchipelagoMode::getStickerChecks(int index) {
    return static_cast<int>(mCollectedStickers[index]);
}

void ArchipelagoMode::setStickerChecks(int index, int checks) {
    u8 u8Checks = static_cast<u8>(checks);
    mCollectedStickers[index] = u8Checks;
}

void ArchipelagoMode::addSouvenir(const ShopItem::ItemInfo* info) {
    int index = getIndexSouvenirList(info->name);

    int souvenirs = mCollectedSouvenirs[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            souvenirs = souvenirs | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mCollectedSouvenirs[index / 8] = souvenirs;
}

bool ArchipelagoMode::hasSouvenir(const ShopItem::ItemInfo* info) {
    int index = getIndexSouvenirList(info->name);
    if (index == -1) {
        // Client::addMessage(info->mName);
        return false;
    }

    u8 souvenirs = mCollectedSouvenirs[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            souvenirs = souvenirs & i;
            return (souvenirs == i);
        }
        i = i << 1;
        curIndex += 1;
    }

    return false;
}

int ArchipelagoMode::getSouvenirChecks(int index) {
    return static_cast<int>(mCollectedSouvenirs[index]);
}

void ArchipelagoMode::setSouvenirChecks(int index, int checks) {
    u8 u8Checks = static_cast<u8>(checks);
    mCollectedSouvenirs[index] = u8Checks;
}

bool ArchipelagoMode::hasItem(const ShopItem::ItemInfo* info) {
    switch (static_cast<int>(info->type)) {
    case 1:
        return hasOutfit(info);
    case 0:
        return hasOutfit(info);
    case 3:
        return hasSticker(info);
    case 2:
        return hasSouvenir(info);
    default:
        // Moon and useitem
        return false;
    }
}

void ArchipelagoMode::addScoutedOutfit(int index) {
    int outfits = mScoutedOutfits[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            outfits = outfits | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mScoutedOutfits[index / 8] = outfits;
}

bool ArchipelagoMode::hasScoutedOutfit(int index) {
    u8 outfits = mScoutedOutfits[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            outfits = outfits & i;
            return (outfits == i);
        }
        i = i << 1;
        curIndex += 1;
    }

    return false;
}

void ArchipelagoMode::addScoutedSticker(int index) {
    int stickers = mScoutedStickers[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            stickers = stickers | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mScoutedStickers[index / 8] = stickers;
}

bool ArchipelagoMode::hasScoutedSticker(int index) {
    if (index == -1) {
        // Client::addMessage(info->mName);
        return false;
    }

    u8 stickers = mScoutedStickers[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            stickers = stickers & i;
            return (stickers == i);
        }
        i = i << 1;
        curIndex += 1;
    }

    return false;
}

void ArchipelagoMode::addScoutedSouvenir(int index) {
    int souvenirs = mScoutedSouvenirs[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            souvenirs = souvenirs | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mScoutedSouvenirs[index / 8] = souvenirs;
}

bool ArchipelagoMode::hasScoutedSouvenir(int index) {
    if (index == -1) {
        // Client::addMessage(info->mName);
        return false;
    }

    u8 souvenirs = mScoutedSouvenirs[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            souvenirs = souvenirs & i;
            return (souvenirs == i);
        }
        i = i << 1;
        curIndex += 1;
    }

    return false;
}

void ArchipelagoMode::addScoutedShopMoon(int index) {
    int shopMoons = mScoutedShopMoons[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            shopMoons = shopMoons | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mScoutedShopMoons[index / 8] = shopMoons;
}

bool ArchipelagoMode::hasScoutedShopMoon(int index) {
    if (index == -1) {
        // Client::addMessage(info->mName);
        return false;
    }

    u8 shopMoons = mScoutedShopMoons[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            shopMoons = shopMoons & i;
            return (shopMoons == i);
        }
        i = i << 1;
        curIndex += 1;
    }

    return false;
}

bool ArchipelagoMode::hasScoutedItem(int type, int index) {
    switch (type) {
    case -6:
        return hasScoutedOutfit(index + 44);
    case -5:
        return hasScoutedOutfit(index);
    case -7:
        return hasScoutedSticker(index);
    case -8:
        return hasScoutedSouvenir(index);
    case -10:
        return hasScoutedShopMoon(index);

    default:
        // useitem
        return true;
    }
}

void ArchipelagoMode::addItem(const ShopItem::ItemInfo* info) {
    switch (static_cast<int>(info->type)) {
    case 1:
        addOutfit(info);
        break;
    case 0:
        addOutfit(info);
        break;
    case 3:
        addSticker(info);
        break;
    case 2:
        addSouvenir(info);
        break;
    default:
        // Moon and useitem
        break;
    }
}

void ArchipelagoMode::addScoutedItem(int type, int index) {
    switch (type) {
    case -6:
        addScoutedOutfit(index + 44);
        break;
    case -5:
        addScoutedOutfit(index);
        break;
    case -7:
        addScoutedSticker(index);
        break;
    case -8:
        addScoutedSouvenir(index);
        break;
    default:
        // Moon and useitem
        return;
    }
}

void ArchipelagoMode::addCapture(const char* capture) {
    int index = getIndexCaptureList(capture);

    int checkedCapturesEntry = mCollectedCaptures[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            checkedCapturesEntry = checkedCapturesEntry | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mCollectedCaptures[index / 8] = checkedCapturesEntry;
}

bool ArchipelagoMode::hasCapture(const char* capture) {
    int index = getIndexCaptureList(capture);
    if (index == -1) {
        sead::FixedSafeString<40> str;
        str = "";
        str.append(capture);
        str.append(" not in captures list.");
        Client::addMessage(str.cstr());
        return false;
    }

    u8 mCheckedCaptures = mCollectedCaptures[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            mCheckedCaptures = mCheckedCaptures & i;
            return (mCheckedCaptures == i);
        }
        i = i << 1;
        curIndex += 1;
    }
    return false;
}

int ArchipelagoMode::getCaptureChecks(int index) {
    return static_cast<int>(mCollectedCaptures[index]);
}

void ArchipelagoMode::setCaptureChecks(int index, int checks) {
    u8 u8Checks = static_cast<u8>(checks);
    mCollectedCaptures[index] = u8Checks;
}

void ArchipelagoMode::addCaptureCheck(const char* capture) {
    int index = getIndexCaptureList(capture);

    int mCheckedCapturesEntry = mCheckedCaptures[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            mCheckedCapturesEntry = mCheckedCapturesEntry | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mCheckedCaptures[index / 8] = mCheckedCapturesEntry;
}

bool ArchipelagoMode::hasCaptureCheck(const char* capture) {
    int index = getIndexCaptureList(capture);
    if (index == -1) {
        sead::FixedSafeString<40> str;
        str = "";
        str.append(capture);
        str.append(" not in captures list.");
        Client::addMessage(str.cstr());
        return false;
    }

    u8 mCheckedCapturesEntry = mCheckedCaptures[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            mCheckedCapturesEntry = mCheckedCapturesEntry & i;
            return (mCheckedCapturesEntry == i);
        }
        i = i << 1;
        curIndex += 1;
    }
    return false;
}

void ArchipelagoMode::setIsRecordCapture(bool value) {
    mIsRecordCapture = value;
}

void ArchipelagoMode::addRegionalCoin(const char* placementId) {
    GameDataHolderAccessor accessor(mCurScene);
    int index = getIndexRegionalCoinId(GameDataFunction::getCurrentStageName(accessor), placementId);

    if (index == -1) {
        sead::FixedSafeString<128> errorStr = sead::FixedSafeString<128>();
        errorStr = "";
        errorStr.append("ERROR: Regional Coin ");
        errorStr.append(placementId);
        errorStr.append(" not in placement ids list.");
        Client::addMessage(errorStr.cstr());
        return;
    }

    addRegionalCoin(index);
}

void ArchipelagoMode::addRegionalCoin(int index) {
    // GameDataHolderAccessor accessor(mCurScene);

    // if (index == -1) {
    //     sead::FixedSafeString<128> errorStr = sead::FixedSafeString<128>();
    //     errorStr = "";
    //     errorStr.append("ERROR: Regional Coin ");
    //     errorStr.append(placementId);
    //     errorStr.append(" not in placement ids list.");
    //     Client::addMessage(errorStr.cstr());
    //     return;
    // }

    u8 checkedRegionalsEntry = mCollectedRegionals[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            checkedRegionalsEntry = checkedRegionalsEntry | i;
            break;
        }
        i = i << 1;
        curIndex += 1;
    }

    mCollectedRegionals[index / 8] = checkedRegionalsEntry;
}

bool ArchipelagoMode::hasRegionalCoin(const char* placementId) {
    GameDataHolderAccessor accessor(mCurScene);
    int index = getIndexRegionalCoinId(GameDataFunction::getCurrentStageName(accessor), placementId);

    if (index < 0) {
        sead::FixedSafeString<128> errorStr = sead::FixedSafeString<128>();
        errorStr.append("ERROR: ");
        if (index == -1) {
            errorStr.append(placementId);
            // errorStr.append(" not in placement ids list for ");
            // errorStr.append(GameDataFunction::getCurrentStageName(accessor));
        } else if (index == -2) {
            errorStr.append(GameDataFunction::getCurrentStageName(accessor));
            errorStr.append(" not in stage names.");
        }

        Client::addMessage(errorStr.cstr());
        return false;
    }

    return hasRegionalCoin(index);
}

bool ArchipelagoMode::hasRegionalCoin(int index) {
    if (index < 0 || index > 999) {
        return false;
    }

    u8 checkedRegionalsEntry = mCollectedRegionals[index / 8];
    // if (index == 0) {
    //     Client::addMessage(intToCstr(int(checkedRegionalsEntry & 0b1)));
    //     Client::addMessage(intToCstr(int(checkedRegionalsEntry)));
    //     Client::addMessage(intToCstr(checkedRegionalsEntry));
    // }

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            checkedRegionalsEntry = checkedRegionalsEntry & i;
            return (checkedRegionalsEntry == i);
        }
        i = i << 1;
        curIndex += 1;
    }
    return false;
}

void ArchipelagoMode::sendMoonCheck(int uid) {
    // Talkatoo% block. Moons the player hasn't been told about by Talkatoo
    // don't credit. The cosmetic get-cinematic still plays because we never
    // hit this method until after sendShinePacketHook has fired; the cutscene
    // title pane is repurposed via mIsTalkatooBlockedLabelPending +
    // getShineReplacementText. In our virtualization model Orig is never
    // called in AP mode (see sendShinePacketHook), so the moon remains
    // re-collectible on next stage entry — the player can return for it once
    // Talkatoo names it.
    //
    // TODO(progression): scenario-advancing Multi Moons should be exempt
    // (otherwise the player can soft-lock when the named pool fills up before
    // the next progression moon is named). Wire either a per-uid exempt list
    // from the server (preferred — matches apworld's `progression: true` flag)
    // or the existing rs::isProgressionMoon-style check once available.
    if (mTalkatooMode && !isMoonNamed(uid)) {
        mIsTalkatooBlockedLabelPending = true;
        return;
    }
    Client::sendCheckPacket(uid, CheckType::Moon);
    if (mInfo && mInfo->mIsClientConnected == ArchipelagoState::CLIENT_CONNECTED)
        addShine(uid);
}

void ArchipelagoMode::sendShopCheck(const ShopItem::ItemInfo* itemInfo) {
    int itemType = static_cast<int>(itemInfo->type);

    switch (itemType) {
    case 0:
        Client::sendCheckPacket(getIndexApCostumeList(itemInfo->name), CheckType::Clothes);
        break;
    case 1:
        Client::sendCheckPacket(getIndexApCostumeList(itemInfo->name), CheckType::Cap);
        break;
    case 2:
        Client::sendCheckPacket(getIndexSouvenirList(itemInfo->name), CheckType::Souvenir);
        break;
    case 3:
        Client::sendCheckPacket(getIndexStickerList(itemInfo->name), CheckType::Sticker);
        break;
    }
}

void ArchipelagoMode::sendRegionalCoinCheck(const char* objId, const char* stageName) {
    Client::sendCheckPacket(CheckType::RegionalCoin, objId, stageName);
    if (mInfo && mInfo->mIsClientConnected == ArchipelagoState::CLIENT_CONNECTED)
        addRegionalCoin(objId);
}

void ArchipelagoMode::sendCaptureCheck(const char* hackName) {
    Client::sendCheckPacket(getIndexCaptureList(hackName), CheckType::Capture);
}

const char* ArchipelagoMode::getShineReplacementText() {
    // Talkatoo% one-shot override. Consumed by the next setShineLabel call
    // from the existing isReplaceShineLabel/setShineLabel BL chain (see
    // hooksArchipelago.hpp). The Multi Moon cutscene fires setShineLabel
    // three times; the flag is read+cleared on the first to keep the label
    // stable across the remaining frames of the cutscene.
    if (mIsTalkatooBlockedLabelPending) {
        mIsTalkatooBlockedLabelPending = false;
        return "Blocked by Talkatoo!";
    }

    GameDataHolderAccessor accessor(mCurScene);

    sead::FixedSafeString<128> shineName = sead::FixedSafeString<128>();

    if (isPartOf(GameDataFunction::getCurrentStageName(accessor), "WorldShop")) {
        shineName = mSlotNames[mShopMoonTextReplacements.slotNameIndex].cstr();
        shineName.append(" ");
        shineName.append(mItemNames[mShopMoonTextReplacements.itemNameIndex].cstr());

        return shineName.cstr();
    }

    replaceText curReplaceText;

    if (mRecentShineHintIndex > 99) {
        if (mRecentShineHintIndex == 1091 || (mRecentShineHintIndex > 1122 && mRecentShineHintIndex < 1152)) {
            curReplaceText = shineTextReplacements[99];
        }

        else {
            curReplaceText = shineTextReplacements[98];
        }
    } else {
        curReplaceText = shineTextReplacements[mRecentShineHintIndex];
    }

    // Client::addMessage(intToCstr(mRecentShineHintIndex));

    if (curReplaceText.itemNameIndex == 255) {
        return "Invalid shine item name index";
    } else {
        shineName = mSlotNames[curReplaceText.slotNameIndex];
        shineName.append("'s ");
        shineName.append(mItemNames[curReplaceText.itemNameIndex]);

        return shineName.cstr();
    }
}

int ArchipelagoMode::getShineColor(Shine* curShine) {
    GameDataHolderAccessor accessor(mCurScene);

    GameDataFile::HintInfo* info = &accessor.mData->mPlayingFile->getHintList()[curShine->mShineIdx];

    // Hint arts Uid is 0 on the moon object in the other world.
    // Stage name in the shine info is still the kingdom the hint art comes from.
    int color = -1;
    if (info->uniqueId == 0) {
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "CapWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1086]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SandWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1096]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "LakeWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1094]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "ForestWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1089]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "CityWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1088]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SnowWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1087]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SeaWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1095]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "LavaWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1090]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SkyWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1091]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "MoonWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1165]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "PeachWorldHomeStage") == 0) {
            color = static_cast<int>(shineColors[1152]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "Special1WorldHomeStage") == 0) {
            // Add conditions for other Dark Side hint arts
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "WaterfallWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1132]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "LakeWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1128]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "CloudWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1124]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "ClashWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1126]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "CityWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1130]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "SnowWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1129]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "SeaWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1127]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "LavaWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1123]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "BossRaidWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1125]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "PeachWorldHomeStage") == 0) {
                color = static_cast<int>(shineColors[1131]);
            }
        }
    } else {
        /*sead::FixedSafeString<40> shineData;
        shineData = "";
        shineData.append("Uid: ");
        shineData.append(intToCstr(info->uniqueId));
        shineData.append(" Color: ");
        shineData.append(intToCstr(shineColors[info->uniqueId]));

        Client::addMessage(shineData.cstr());
        shineData = "";
        shineData.append("Uid: ");
        shineData.append(intToCstr(1145));
        shineData.append(" Color: ");
        shineData.append(intToCstr(shineColors[1145]));

        Client::addMessage(shineData.cstr());*/
        color = static_cast<int>(shineColors[info->uniqueId]);
    }

    if (color > -1) {
        if (color - 64 > -1) {
            return color - 64;
        }
        return color;
    }

    return 99;
}

const char16_t* ArchipelagoMode::getShopReplacementText(const char* fileName, const char* key) {
    sead::WFixedSafeString<200> message;
    message = message.cEmptyString;
    bool isExplain = false;
    sead::FixedSafeString<40> convert;
    convert = convert.cEmptyString;
    convert.append(key);
    if (convert.calcLength() != convert.removeSuffix("_Explain")) {
        isExplain = true;
    }
    shopReplaceText curItem = {255, 255, 255, 255};

    CheckType scoutType = CheckType::CapScout;
    int locationId = -1;

    if (strcmp("ItemCap", fileName) == 0) {
        locationId = getIndexApCostumeList(convert.cstr());
        int itemIndex = locationId - 1;
        if (itemIndex > 18) {
            itemIndex -= 19;
            if (itemIndex > 16)
                itemIndex -= 3;
            curItem = mShopCapTextReplacements[itemIndex];
        } else {
            int regionalListIndex = getIndexRegionalItemList(mRelativeWorldCoinCollect, convert.cstr());
            if (regionalListIndex == 0)
                mRegionalOffset = 0;
            curItem = mShopRegionalTextReplacements[regionalListIndex + mRegionalOffset];
            // Client::addMessage(mItemNames[curItem.itemNameIndex].cstr());
        }
        mRegionalOffset += 1;
        scoutType = CheckType::CapScout;
    } else if (strcmp("ItemCloth", fileName) == 0) {
        locationId = getIndexApCostumeList(convert.cstr());
        int itemIndex = locationId - 1;
        if (itemIndex > 18) {
            itemIndex -= 19;
            if (itemIndex > 16)
                itemIndex -= 3;
            curItem = mShopClothTextReplacements[itemIndex];
        } else {
            curItem = mShopRegionalTextReplacements[getIndexRegionalItemList(mRelativeWorldCoinCollect, convert.cstr()) + mRegionalOffset];
        }
        scoutType = CheckType::ClothesScout;
    } else if (strcmp("ItemSticker", fileName) == 0) {
        locationId = getIndexStickerList(convert.cstr());
        curItem = curItem = mShopRegionalTextReplacements[getIndexRegionalItemList(mRelativeWorldCoinCollect, convert.cstr()) + mRegionalOffset];
        scoutType = CheckType::StickerScout;
    } else if (strcmp("ItemGift", fileName) == 0) {
        locationId = getIndexSouvenirList(convert.cstr());
        curItem = curItem = mShopRegionalTextReplacements[getIndexRegionalItemList(mRelativeWorldCoinCollect, convert.cstr()) + mRegionalOffset];
        scoutType = CheckType::SouvenirScout;
    } else if (strcmp("ItemMoon", fileName) == 0) {
        // Find out key for each kingdom as still is unknown
        locationId = getIndexMoonItemList(convert.cstr());
        curItem = mShopMoonTextReplacements;
        scoutType = CheckType::ShopMoonScout;
    } else {
        // Not included items like Life Up Hearts
        return u"";
    }

    if (locationId > -1 && !hasScoutedItem(scoutType, locationId)) {
        addScoutedItem(scoutType, locationId);
        Client::sendCheckPacket(locationId, scoutType);
    }

    if (curItem.gameIndex == 254) {
        // Client::addMessage("No Item Data Received.");
    }

    if (isExplain) {
        message.append(u"Comes from the world of ");
        // if (mGameNames[curItem.gameIndex].isEmpty()) {
        message.append(utf8ToUtf16(mGameNames[curItem.gameIndex].cstr()));
        //} else {
        // message.append(u"Missing Game");
        // }

        message.append(u".\nSeems to belong to ");
        // if (mSlotNames[curItem.slotIndex].isEmpty()) {
        message.append(utf8ToUtf16(mSlotNames[curItem.slotNameIndex].cstr()));
        //} else {
        // message.append(u"Missing Slot Name");
        //}
        message.append(u".\n");
        if (curItem.itemClassification == 0) {
            message.append(u"It looks like junk, but may as well ask...");
        } else if (curItem.itemClassification == 0b0010) {
            message.append(u"It looks useful.");

        } else if (curItem.itemClassification == 254) {
            message.append(u"Error or Not in the Item Pool.");
        } else {
            message.append(u"It looks really important!");
        }
    } else {
        // if (mSlotNames[curItem.slotIndex].isEmpty()) {
        message.append(utf8ToUtf16(mItemNames[curItem.itemNameIndex].cstr()));
        //} else {
        // message.append(u"Missing Item Name");
        //}
    }

    return message.cstr();
}

ChangeStageInfo* ArchipelagoMode::getLastERTransition() {
    if (!mCurScene || mLastERStageId.isEmpty() || mLastERStageName.isEmpty())
        return nullptr;

    ChangeStageInfo info(GameDataHolderAccessor(mCurScene).mData, mLastERStageId.cstr(), mLastERStageName.cstr());
    return &info;
}

void ArchipelagoMode::clearArrays() {
    mStoryShineArray.clear();
}

void ArchipelagoMode::clearCollectibles() {
    mCollectedShines.fill(0);
    mCollectedOutfits.fill(0);
    mCollectedStickers.fill(0);
    mCollectedSouvenirs.fill(0);
    mCollectedCaptures.fill(0);
    mCheckedCaptures.fill(0);
    mCollectedRegionals.fill(0);
    mScoutedOutfits.fill(0);
    mScoutedStickers.fill(0);
    mScoutedSouvenirs.fill(0);
    mScoutedShopMoons.fill(0);
    mScoutedMoonRocks.fill(0);
}

void ArchipelagoMode::clearScenarios() {
    for (int i = 0; i < GameDataFunction::getWorldIndexSpecial2(); i++) {
        setScenario(i, 1);
    }
}

void ArchipelagoMode::sendStage(GameDataHolderWriter writer, const ChangeStageInfo* stageInfo) {
    GameDataHolderAccessor accessor(mCurScene);

    if (!stageInfo->mChangeStageName.isEmpty())
        setScenario(stageInfo->mChangeStageName.cstr(), stageInfo->mScenarioNo);
    // Client::addMessage("onGrandShineStageChange");
    // Client::addMessage(stageInfo->mChangeStageName.cstr());

    if (GameDataFunction::getWorldIndexWaterfall() == GameDataFunction::getCurrentWorldId(accessor) || GameDataFunction::isUnlockedCurrentWorld(accessor)) {
        GameDataFunction::tryChangeNextStage(writer, stageInfo);
    } else {
        int i = 0;
        for (i = GameDataFunction::getWorldIndexSpecial2(); i > 0; i--) {
            if (GameDataFunction::isUnlockedWorld(accessor, i)) {
                break;
            }
        }
        ChangeStageInfo info(accessor.mData, "", GameDataFunction::getMainStageName(accessor, i), false, -1, static_cast<ChangeStageInfo::SubScenarioType>(0));
        GameDataFunction::tryChangeNextStage(writer, &info);
    }
}

void ArchipelagoMode::sendBack() {
    GameDataHolderAccessor accessor(mCurScene);
    GameDataHolderWriter writer(mCurScene);
    int i = 0;
    for (i = GameDataFunction::getWorldIndexSpecial2(); i > 0; i--) {
        if (GameDataFunction::isUnlockedWorld(accessor, i)) {
            break;
        }
    }

    if (i == GameDataFunction::getWorldIndexWaterfall() &&
        accessor.mData->getGameDataFile()->getGameProgressData()->mHomeStatus > GameProgressData::HomeStatus::None) {
        i = GameDataFunction::getWorldIndexHat();
    }

    ChangeStageInfo info(writer.mData, "home", GameDataFunction::getMainStageName(accessor, i), false, getScenario(i),
                         static_cast<ChangeStageInfo::SubScenarioType>(0));
    GameDataFunction::tryChangeNextStage(writer, &info);
}

void ArchipelagoMode::update() {
    PlayerActorHakoniwa* playerHakoniwa = getPlayerActorHakoniwa();
    PlayerActorBase* playerBase = (PlayerActorBase*)playerHakoniwa;
    GameDataHolderWriter writer(mCurScene);
    GameDataHolderAccessor accessor(mCurScene);
    StageScene* stageScene = (StageScene*)mCurScene;
    if (!playerHakoniwa)
        return;

    if (!GameModeManager::instance()->isPaused()) {
        // Check if Odyssey Active and is ER
        if (mIsConnectInit) {
            mIsConnectInit = false;
            clearCollectibles();
            if (!GameDataFunction::isEnableCap(accessor)) {
                GameDataFunction::enableCap(writer);
            }

            GameProgressData* gameProgressData = accessor.mData->getGameDataFile()->getGameProgressData();
            if (gameProgressData->mUnlockWorldNum < 2) {
                gameProgressData->mUnlockWorldNum = 2;
            }

            gameProgressData->mIsUnlockWorld[0] = true;
            gameProgressData->mIsUnlockWorld[1] = true;
            gameProgressData->mIsFirstTimeWorld[0] = false;
            gameProgressData->mIsFirstTimeWorld[1] = false;

            if (!GameDataFunction::isActivateHome(accessor))
                GameDataFunction::activateHome(writer);

            if (!GameDataFunction::isLaunchHome(accessor))
                GameDataFunction::launchHome(writer);

            // Correct impassible scenarios
            if (getScenario(GameDataFunction::getWorldIndexPeach()) < 2)
                setScenario(GameDataFunction::getWorldIndexPeach(), 2);
            if (getScenario(GameDataFunction::getWorldIndexSpecial1()) < 2)
                setScenario(GameDataFunction::getWorldIndexSpecial1(), 2);
            if (getScenario(GameDataFunction::getWorldIndexHat()) < 2) {
                setScenario(GameDataFunction::getWorldIndexHat(), 2);
            }
            if (mIsFirstConnect) {
                mIsFirstConnect = false;
                sendBack();
            }
        }

        if (!mIsEntranceRandomizationEnabled) {
            if (GameDataFunction::getCurrentWorldId(accessor) == GameDataFunction::getWorldIndexForest() &&
                !GameDataFunction::isUnlockedWorld(accessor, GameDataFunction::getWorldIndexForest())) {
                sendBack();
            }

            if (GameDataFunction::getCurrentWorldId(accessor) == GameDataFunction::getWorldIndexSea() &&
                !GameDataFunction::isUnlockedWorld(accessor, GameDataFunction::getWorldIndexSea())) {
                sendBack();
            }
        }

        handleDeathLink(playerBase, playerHakoniwa, writer);

        handleCaptureSanity(playerBase, accessor);

        updateCounter(playerBase, accessor);

        handleSoftLocks(accessor, writer);

        // Cappy speech-bubble queue pump. No-op until the rs:: function
        // pointers are wired (setCappyRsCalls is called from main.cpp once
        // hk::ro::lookupSymbol resolves both) and the scene-settle gates pass.
        // tryPumpCappyMessage();

        mUpdateCounterTimer += 1;
        mSoftlockTimer += 1;

        // Regional Coin Arrow
        trySetHintTargetValid();
        if (mInfo->isNeedArchipelagoConnect) {
            Client::sendArchipelagoConnectPacket();
            mInfo->isNeedArchipelagoConnect = false;
        }
    }

    // D-Pad functions
    if (al::isPadHoldL(-1)) {
        // Purple Coin Search
        if (al::isPadTriggerUp(-1)) {
            getNearestRegional(stageScene, playerBase);
        }

        // Return to Odyssey
        if (al::isPadTriggerLeft(-1)) {
            sendBack();
        }
    }
    // ADD return to last loading zone entrance... maybe

    // Menu Page Turning
    if (mIsInfoMenuOpen) {
        if (al::isPadTriggerRight(-1)) {
            if (mInfoMenuPageNum < mInfoMenuPageMax) {
                mInfoMenuPageNum += 1;
            } else {
                mInfoMenuPageNum = 0;
            }
        }

        if (al::isPadTriggerLeft(-1)) {
            if (mInfoMenuPageNum > 0) {
                mInfoMenuPageNum -= 1;
            } else {
                mInfoMenuPageNum = mInfoMenuPageMax;
            }
        }
    }

    if (al::isPadTriggerDown(-1)) {
        mIsInfoMenuOpen = !mIsInfoMenuOpen;
    }

    // Debug arcipelago buttons
    if (mInfo->mIsDebugMode) {
    }
}

void ArchipelagoMode::debugMenuControls() {
    ImGui::Text("- [AP] L + ↑ | Show Nearest Regional Coin\n");
    ImGui::Text("- [AP] L + ← | Return to the Odyssey\n");

    // if (mIsDebugMode) {
    //     ImGui::Text("- [AP][Debug] \n");
    //     ImGui::Text("- [AP][Debug] \n");
    //     ImGui::Text("- [AP][Debug] \n");
    // } → ← ↓ ↑
}

// Returns if menu was drawn
bool ArchipelagoMode::infoMenu() {
    if (!mIsInfoMenuOpen)
        return false;

    ImGui::Begin("Archipelago", nullptr,
                 ImGuiWindowFlags_NoSavedSettings /*| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse*/ | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar);

    int dispHeight = al::getLayoutDisplayHeight();
    ImGui::SetWindowPos(ImVec2(0, dispHeight / 3.f), ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(al::getLayoutDisplayWidth() / 3.5f, dispHeight - (dispHeight / 4.f)));

    const char* captureText = mCapturesEnabled ? "Enabled" : "Disabled";
    const char* erText = mIsEntranceRandomizationEnabled ? "Enabled" : "Disabled";
    const char* dlText = mDeathLinkEnabled ? "Enabled" : "Disabled";

    switch (mInfoMenuPageNum) {
    case 0:
        ImGui::Text("\n------------------- Controls --------------------\n");
        debugMenuControls();
        break;

    case 1:
        ImGui::Text("\n------------------- Slot Data --------------------\n");
        ImGui::Text("Slot Name: %s\n\n", Client::getArchipelagoSlot());

        ImGui::Text("- Capture-Sanity | %s\n", captureText);
        ImGui::Text("- Entrance Randomizer | %s\n", erText);
        ImGui::Text("\n\n- Death Link | %s\n", dlText);
        break;

    case 2:
        break;

    case 3:
        ImGui::Text("\n------------------- Info --------------------\n");
        ImGui::Text("Current Regional Coin World ID: %d", mRelativeWorldCoinCollect);
        ImGui::Text("\nCurrent last stage ID: \n%s", mLastERStageId.cstr());
        ImGui::Text("\nCurrent last stage name: \n%s", mLastERStageName.cstr());
        break;
    }

    ImGui::Text("\n\n\n\n\n\n\n\n\n\npage %d/%d", mInfoMenuPageNum, mInfoMenuPageMax);

    ImGui::End();
    return true;
}

// ===== Cappy Messenger =====
//
// In-game speech-bubble notification system. Three pieces:
//   - enqueueCappyMessage : append a UTF-8 string to a small FIFO.
//   - tryPumpCappyMessage : called once per frame from update(); drains the
//     FIFO via rs::tryShowCapMessagePriorityLow + a "Nintendo bubble busy"
//     poll on rs::isActiveCapMessage.
//   - lookupCappyMessageSubstitution : consulted by the hooked al::*Message
//     accessors when CapMessageLayout::exeDelay asks for kArchipelagoCappyLabel.
//
// The buffer lifetime contract: once tryShow succeeds, our UTF-16 buffer
// must stay valid + unchanged until isActiveCapMessage returns false (SMO
// is reading from the buffer for the duration the balloon is on screen).

namespace {

// Minimal UTF-8 -> UTF-16 transcoder. Stops at NUL or when `out` is full,
// always NUL-terminates the output, and accepts up to 3-byte UTF-8 sequences
// (covers the BMP, which is all MessageFont38 can render anyway). Returns
// the number of char16_t words written excluding the trailing NUL.
u32 cappyUtf8ToUtf16(const char* src, char16_t* out, u32 out_cap) {
    if (out_cap == 0 || out == nullptr || src == nullptr)
        return 0;
    u32 i = 0;
    u32 o = 0;
    while (src[i] != '\0' && o + 1 < out_cap) {
        const unsigned char b0 = static_cast<unsigned char>(src[i]);
        if (b0 < 0x80) {
            out[o++] = static_cast<char16_t>(b0);
            ++i;
        } else if ((b0 & 0xE0) == 0xC0 && src[i + 1] != '\0') {
            const unsigned char b1 = static_cast<unsigned char>(src[i + 1]);
            out[o++] = static_cast<char16_t>(((b0 & 0x1F) << 6) | (b1 & 0x3F));
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0 && src[i + 1] != '\0' && src[i + 2] != '\0') {
            const unsigned char b1 = static_cast<unsigned char>(src[i + 1]);
            const unsigned char b2 = static_cast<unsigned char>(src[i + 2]);
            out[o++] = static_cast<char16_t>(((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F));
            i += 3;
        } else {
            // Malformed lead byte — skip and continue rather than UB.
            ++i;
        }
    }
    out[o] = 0;
    return o;
}

// Switch system tick is 19.2 MHz (19200 ticks per ms). hk::svc::getSystemTick
// wraps the SVC; same primitive ApState::nowMs uses in smo_archipelago.
s64 cappyNowMs() {
    return static_cast<s64>(hk::svc::getSystemTick() / 19200ULL);
}

}  // namespace

void ArchipelagoMode::enqueueCappyMessage(const char* utf8_text) {
    if (!utf8_text || utf8_text[0] == '\0')
        return;
    if (mCappyLiveCount >= kCappyQueueCap) {
        // Drop newest. Matches smo_archipelago CappyMessenger behavior — the
        // dropped item is recent (likely a stale notification from a bulk
        // replay) and queued items are older and more representative of what
        // the player has been waiting on.
        Logger::log("[cappy] queue full (cap=%u) — dropping '%s'\n", static_cast<unsigned>(kCappyQueueCap), utf8_text);
        return;
    }
    CappyEntry& e = mCappyQueue[mCappyTail];
    // strncpy with explicit NUL termination — strlcpy isn't available.
    u32 i = 0;
    while (utf8_text[i] != '\0' && i + 1 < kCappyTextCap) {
        e.text[i] = utf8_text[i];
        ++i;
    }
    e.text[i] = '\0';
    e.live = true;
    mCappyTail = (mCappyTail + 1) % kCappyQueueCap;
    ++mCappyLiveCount;
}

// Should no longer be needed. Now calls functions directly via rs
// Class-static rs:: entry-point cache definitions. See header comment.
ArchipelagoMode::TryShowCapMessagePriorityLowFn ArchipelagoMode::sTryShowCapMessage = nullptr;
ArchipelagoMode::IsActiveCapMessageFn ArchipelagoMode::sIsActiveCapMessage = nullptr;

void ArchipelagoMode::setCappyRsCalls(TryShowCapMessagePriorityLowFn tryShow, IsActiveCapMessageFn isActive) {
    sTryShowCapMessage = tryShow;
    sIsActiveCapMessage = isActive;
}

void ArchipelagoMode::tryPumpCappyMessage() {
    const al::IUseSceneObjHolder* scene = mCurScene;

    // Scene-stability bookkeeping. Reset BOTH counters whenever
    // mSceneObjHolder changes; bump frames each tick the scene is stable.
    if (scene != mCappyLastScene) {
        mCappyLastScene = scene;
        mCappySettleFrames = 0;
        mCappySceneChangeMs = (scene != nullptr) ? cappyNowMs() : 0;
        if (mCappyBufferInUse) {
            // Force-release: SMO can't be reading the buffer through a torn-
            // down scene. The next bubble re-fills the same memory.
            mCappyBufferInUse = false;
        }
    } else if (scene != nullptr) {
        ++mCappySettleFrames;
    }

    if (mCappyLiveCount == 0)
        return;
    if (!scene)
        return;

    // Dual settle gate: both halves must pass. See header for the rationale
    // (frame-only fails on Ryujinx during save load; ms-only fails on real
    // Switch when scene resolves before any frame runs).
    {
        const s64 elapsedMs = mCappySceneChangeMs == 0 ? 0 : cappyNowMs() - mCappySceneChangeMs;
        if (mCappySettleFrames < kCappySettleFrames)
            return;
        if (elapsedMs < kCappySettleMs)
            return;
    }

    // If our buffer is still live, wait for Nintendo's bubble pipeline to
    // finish reading it before releasing.
    if (mCappyBufferInUse) {
        if (rs::isActiveCapMessage(scene))
            return;
        mCappyBufferInUse = false;
    }

    // Pre-flight: don't try to dispatch while a non-AP Cappy bubble is on
    // screen — rs::tryShowCapMessagePriorityLow would either refuse or queue
    // us indefinitely. Bump a retry counter; if we get stuck, drop the head
    // entry rather than blocking the FIFO forever.
    if (rs::isActiveCapMessage(scene)) {
        ++mCappyRetryFrames;
        if (mCappyRetryFrames >= kCappyMaxRetryFrames) {
            Logger::log("[cappy] dropping head after %u frames (text='%s')\n", static_cast<unsigned>(mCappyRetryFrames), mCappyQueue[mCappyHead].text);
            mCappyQueue[mCappyHead].live = false;
            mCappyHead = (mCappyHead + 1) % kCappyQueueCap;
            --mCappyLiveCount;
            mCappyRetryFrames = 0;
        }
        return;
    }

    // Prepare the substitution buffer.
    CappyEntry& e = mCappyQueue[mCappyHead];
    const u32 written = cappyUtf8ToUtf16(e.text, mCappyBuffer, kCappyBufferWords);
    if (written == 0 && e.text[0] != '\0') {
        Logger::log("[cappy] utf8->utf16 produced empty buffer for '%s' — dropping head\n", e.text);
        mCappyQueue[mCappyHead].live = false;
        mCappyHead = (mCappyHead + 1) % kCappyQueueCap;
        --mCappyLiveCount;
        mCappyRetryFrames = 0;
        return;
    }
    mCappyBufferInUse = true;

    // CAVEAT (verified by smo_archipelago disassembly of rs::tryShowCapMessage
    // PriorityLow at 0x23a910 + CapMessageShowInfo ctor at 0x23a540): the
    // function's 3rd arg lands in mWaitTime and its 4th in mDelayTime — wait
    // FIRST, delay SECOND. Opposite of the natural reading order.
    const bool ok = rs::tryShowCapMessagePriorityLow(scene, kArchipelagoCappyLabel,
                                                     /*waitTime=*/kCappyWaitTicks,
                                                     /*delayTime=*/0);
    if (!ok) {
        mCappyBufferInUse = false;
        ++mCappyRetryFrames;
        if (mCappyRetryFrames >= kCappyMaxRetryFrames) {
            Logger::log("[cappy] dropping head after %u tryShow refusals (text='%s')\n", static_cast<unsigned>(mCappyRetryFrames),
                        mCappyQueue[mCappyHead].text);
            mCappyQueue[mCappyHead].live = false;
            mCappyHead = (mCappyHead + 1) % kCappyQueueCap;
            --mCappyLiveCount;
            mCappyRetryFrames = 0;
        }
        return;
    }

    // Dispatched. Advance head; mCappyBufferInUse stays true until the
    // isActive poll above flips false (SMO keeps reading the buffer for the
    // duration of the on-screen balloon).
    mCappyQueue[mCappyHead].live = false;
    mCappyHead = (mCappyHead + 1) % kCappyQueueCap;
    --mCappyLiveCount;
    mCappyRetryFrames = 0;
}

const char16_t* ArchipelagoMode::lookupCappyMessageSubstitution(const char* label) const {
    if (!label)
        return nullptr;
    // Cheap-first: vast majority of MSBT lookups are not for our label.
    // kArchipelagoCappyLabel starts with 'A'.
    if (label[0] != 'A')
        return nullptr;
    if (strcmp(label, kArchipelagoCappyLabel) != 0)
        return nullptr;
    if (!mCappyBufferInUse)
        return nullptr;
    return mCappyBuffer;
}