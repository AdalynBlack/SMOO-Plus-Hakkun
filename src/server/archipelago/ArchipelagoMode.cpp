#include "server/archipelago/ArchipelagoMode.hpp"

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
#include "game/System/WorldList.h"
#include "game/Util/ActorDimensionKeeper.h"
#include "game/Util/ObjUtil.h"
#include "game/Util/PlayerUtil.h"

#include "basis/seadNew.h"
#include "imgui.h"
#include "logger.hpp"
#include "puppets/PuppetInfo.h"
#include "rs/util.hpp"
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

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<ArchipelagoInfo>();

    if (curGameInfo)
        Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    else
        Logger::log("No gamemode info found\n");
    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (ArchipelagoInfo*)curGameInfo;
        // mModeTimer = new GameModeTimer(mInfo->mRoundTimer);
    } else {
        if (curGameInfo)
            delete curGameInfo;  // attempt to destory previous info before creating new one
        mInfo = GameModeManager::instance()->createModeInfo<ArchipelagoInfo>();
        // mModeTimer = new GameModeTimer();
    }

    Logger::log("Scene Heap Free Size: %f/%f\n", al::getSceneHeap()->getFreeSize() * 0.001f, al::getSceneHeap()->getSize() * 0.001f);

    Logger::log("Scene Heap Free Size: %f/%f\n", al::getSceneHeap()->getFreeSize() * 0.001f, al::getSceneHeap()->getSize() * 0.001f);

    // Create hint arrow
    mHintArrow = new ArchipelagoHintArrow("CheckHintArrow");
    mHintArrow->init(*info.mActorInitInfo);
}

void ArchipelagoMode::begin() {
    unpause();

    PlayerHitPointData* hit = GameDataHolderAccessor(mCurScene)->getGameDataFile()->getPlayerHitPointData();
    // hit->mCurrentHealth = 3;

    GameModeBase::begin();

    mCurScene->stageSceneLayout->end();
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

void ArchipelagoMode::setScenario(int worldID, int scenario) {
    mWorldScenarios[worldID] = scenario;
}

bool ArchipelagoMode::setScenario(const char* worldName, int scenario) {
    GameDataHolderAccessor accessor(mCurScene);

    int worldID = accessor.mData->mWorldList->tryFindWorldIndexByStageName(worldName);
    if (scenario == -1) {
        // setMessage(3, "ChangeStageInfo failed to init");
    }

    // Exclude revisitable scenarios like festival
    if (!(al::isEqualString(worldName, "CityWorldHomeStage") && scenario == 3)) {
        if (scenario != getScenario(worldID) && scenario <= accessor.mData->mWorldList->getMoonRockScenarioNo(worldID) &&
            !GameDataFunction::isUnlockedWorld(accessor, worldID)) {
            if (getScenario(worldID) < scenario) {
                // setMessage(1, "Scenario Updated");
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
        setMessage(1, "isReturn: True");

    } else {
        setMessage(1, "isReturn: False");
    }
    sead::FixedSafeString<40> str;
    str = "";
    str.append("subScenario type: ");
    str.append(static_cast<char>(48 + static_cast<unsigned int>(stageInfo->subType)));
    setMessage(2, str.cstr());*/
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

void ArchipelagoMode::setGameName(int index, const char16_t* name) {
    mGameNames[index] = mGameNames[index].cEmptyString;
    mGameNames[index].append(name);
}

void ArchipelagoMode::setSlotName(int index, const char16_t* name) {
    mSlotNames[index] = mSlotNames[index].cEmptyString;
    mSlotNames[index].append(name);
}

void ArchipelagoMode::setItemName(int index, const char16_t* name) {
    mItemNames[index] = mItemNames[index].cEmptyString;
    mItemNames[index].append(name);
}

void ArchipelagoMode::setShineItemName(int index, const char* name) {
    mShineItemNames[index] = mShineItemNames[index].cEmptyString;
    mShineItemNames[index].append(name);
}

void ArchipelagoMode::setShineTextReplacement(int index, shineReplaceText replace) {
    shineTextReplacements[index] = replace;
}

void ArchipelagoMode::setShineColors(int index, u8 replace) {
    shineColors[index] = replace;
}

void ArchipelagoMode::setCapTextReplacement(int index, shopReplaceText replace) {
    shopCapTextReplacements[index] = replace;
}

void ArchipelagoMode::setClothesTextReplacement(int index, shopReplaceText replace) {
    shopClothTextReplacements[index] = replace;
}

void ArchipelagoMode::setSouvenirTextReplacement(int index, shopReplaceText replace) {
    shopGiftTextReplacements[index] = replace;
}

void ArchipelagoMode::setStickerTextReplacement(int index, shopReplaceText replace) {
    shopStickerTextReplacements[index] = replace;
}

void ArchipelagoMode::setShopMoonTextReplacement(int index, shopReplaceText replace) {
    shopMoonTextReplacements[index] = replace;
}

void ArchipelagoMode::addShine(int uid) {
    int shines = collectedShines[uid / 32];

    int index = (uid / 32) * 32;
    int i = 1;
    while (i <= 0x80000000) {
        if (index == uid) {
            shines = shines | i;
            break;
        }
        if (i == 0x80000000) {
            sead::FixedSafeString<60> str;
            str = "";
            str.append("Shine UID ");
            str.append(intToCstr(uid));
            str.append(" failed to add to shine list at index ");
            str.append(intToCstr(index));
            setMessage(2, str.cstr());
            break;
        }
        i = i << 1;
        index += 1;
    }

    collectedShines[uid / 32] = shines;
}

void ArchipelagoMode::setRecentShine(Shine* curShine) {
    mRecentShine = curShine;
}

bool ArchipelagoMode::hasShine(int uid) {
    int shines = collectedShines[uid / 32];

    int index = (uid / 32) * 32;
    int i = 1;
    while (i <= 0x80000000) {
        if (index == uid) {
            shines = shines & i;
            return (shines == i);
        }
        if (i == 0x80000000) {
            sead::FixedSafeString<60> str;
            str = "";
            str.append("Shine UID ");
            str.append(intToCstr(uid));
            str.append(" failed to find in shine list at index ");
            str.append(intToCstr(index));
            setMessage(3, str.cstr());
            break;
        }
        i = i << 1;
        index += 1;
    }
    return false;
}

int ArchipelagoMode::getShineChecks(int index) {
    return collectedShines[index];
}

void ArchipelagoMode::setShineChecks(int index, int checks) {
    collectedShines[index] = checks;
}

void ArchipelagoMode::addOutfit(const ShopItem::ItemInfo* info) {
    int index = getIndexApCostumeList(info->name) + 44 * static_cast<int>(info->type);

    int outfits = collectedOutfits[index / 8];

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

    collectedOutfits[index / 8] = outfits;
}

bool ArchipelagoMode::hasOutfit(const ShopItem::ItemInfo* info) {
    int index = getIndexApCostumeList(info->name) + 44 * static_cast<int>(info->type);
    if (index == -1) {
        // setMessage(2, info->mName);
        return false;
    }

    u8 outfits = collectedOutfits[index / 8];

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
    return static_cast<int>(collectedOutfits[index]);
}

void ArchipelagoMode::setOutfitChecks(int index, int checks) {
    u8 u8Checks = static_cast<u8>(checks);
    collectedOutfits[index] = u8Checks;
}

void ArchipelagoMode::addSticker(const ShopItem::ItemInfo* info) {
    int index = getIndexStickerList(info->name);

    int stickers = collectedStickers[index / 8];

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

    collectedStickers[index / 8] = stickers;
}

bool ArchipelagoMode::hasSticker(const ShopItem::ItemInfo* info) {
    int index = getIndexStickerList(info->name);
    if (index == -1) {
        // setMessage(2, info->mName);
        return false;
    }

    u8 stickers = collectedStickers[index / 8];

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
    return static_cast<int>(collectedStickers[index]);
}

void ArchipelagoMode::setStickerChecks(int index, int checks) {
    u8 u8Checks = static_cast<u8>(checks);
    collectedStickers[index] = u8Checks;
}

void ArchipelagoMode::addSouvenir(const ShopItem::ItemInfo* info) {
    int index = getIndexSouvenirList(info->name);

    int souvenirs = collectedSouvenirs[index / 8];

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

    collectedSouvenirs[index / 8] = souvenirs;
}

bool ArchipelagoMode::hasSouvenir(const ShopItem::ItemInfo* info) {
    int index = getIndexSouvenirList(info->name);
    if (index == -1) {
        // setMessage(2, info->mName);
        return false;
    }

    u8 souvenirs = collectedSouvenirs[index / 8];

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
    return static_cast<int>(collectedSouvenirs[index]);
}

void ArchipelagoMode::setSouvenirChecks(int index, int checks) {
    u8 u8Checks = static_cast<u8>(checks);
    collectedSouvenirs[index] = u8Checks;
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

void ArchipelagoMode::addCapture(const char* capture) {
    int index = getIndexCaptureList(capture);

    int checkedCapturesEntry = collectedCaptures[index / 8];

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

    collectedCaptures[index / 8] = checkedCapturesEntry;
}

bool ArchipelagoMode::hasCapture(const char* capture) {
    int index = getIndexCaptureList(capture);
    if (index == -1) {
        sead::FixedSafeString<40> str;
        str = "";
        str.append(capture);
        str.append(" not in captures list.");
        setMessage(1, str.cstr());
        return false;
    }

    u8 checkedCaptures = collectedCaptures[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            checkedCaptures = checkedCaptures & i;
            return (checkedCaptures == i);
        }
        i = i << 1;
        curIndex += 1;
    }
    return false;
}

int ArchipelagoMode::getCaptureChecks(int index) {
    return static_cast<int>(collectedCaptures[index]);
}

void ArchipelagoMode::setCaptureChecks(int index, int checks) {
    u8 u8Checks = static_cast<u8>(checks);
    collectedCaptures[index] = u8Checks;
}

void ArchipelagoMode::addCaptureCheck(const char* capture) {
    int index = getIndexCaptureList(capture);

    int checkedCapturesEntry = checkedCaptures[index / 8];

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

    checkedCaptures[index / 8] = checkedCapturesEntry;
}

bool ArchipelagoMode::hasCaptureCheck(const char* capture) {
    int index = getIndexCaptureList(capture);
    if (index == -1) {
        sead::FixedSafeString<40> str;
        str = "";
        str.append(capture);
        str.append(" not in captures list.");
        setMessage(1, str.cstr());
        return false;
    }

    u8 checkedCapturesEntry = checkedCaptures[index / 8];

    int curIndex = (index / 8) * 8;
    int i = 1;
    while (i < 0x100) {
        if (curIndex == index) {
            checkedCapturesEntry = checkedCapturesEntry & i;
            return (checkedCapturesEntry == i);
        }
        i = i << 1;
        curIndex += 1;
    }
    return false;
}

void ArchipelagoMode::setIsRecordCapture(bool value) {
    mIsRecordCapture = value;
}

void ArchipelagoMode::setMessage(int num, const char* msg) {
    switch (num) {
    case 1:
        apChatLine1 = msg;
        break;
    case 2:
        apChatLine2 = msg;
        break;
    case 3:
        apChatLine3 = msg;
        break;
    }
}

void ArchipelagoMode::sendMoonCheck(int uid) {
    Client::sendCheckPacket(uid, CheckType::Moon);
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
}

void ArchipelagoMode::sendCaptureCheck(const char* hackName) {
    Client::sendCheckPacket(getIndexCaptureList(hackName), CheckType::Capture);
}

const char* ArchipelagoMode::getShineReplacementText() {
    GameDataHolderAccessor accessor(mCurScene);

    Shine* curShine = mRecentShine;

    GameDataFile::HintInfo* info = &accessor.mData->getGameDataFile()->getHintList()[curShine->mShineIdx];

    shineReplaceText curReplaceText;

    if (info->uniqueId == 0) {
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "CapWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SandWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "LakeWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "ForestWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "CityWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SnowWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SeaWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "LavaWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SkyWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[99];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "MoonWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "PeachWorldHomeStage") == 0) {
            curReplaceText = shineTextReplacements[98];
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "Special1WorldHomeStage") == 0) {
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "WaterfallWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "LakeWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "CloudWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "ClashWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "CityWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "SnowWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "SeaWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "LavaWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "BossRaidWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "PeachWorldHomeStage") == 0) {
                curReplaceText = shineTextReplacements[99];
            }
        }
    } else {
        curReplaceText = shineTextReplacements[info->hintIdx];
    }

    // setMessage(1, intToCstr(info->hintIdx));

    if (curReplaceText.shineItemNameIndex == 255) {
        setMessage(2, "Invalid shine item name index");
        return mRecentShine->curShineInfo->mLabel.cstr();
    } else {
        return mShineItemNames[curReplaceText.shineItemNameIndex].cstr();
    }
}

int ArchipelagoMode::getShineColor(Shine* curShine) {
    GameDataHolderAccessor accessor(mCurScene);

    GameDataFile::HintInfo* info = &accessor.mData->mPlayingFile->getHintList()[curShine->mShineIdx];

    // Hint arts Uid is 0 on the moon object in the other world.
    // Stage name in the shine info is still the kingdom the hint art comes from.
    if (info->uniqueId == 0) {
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "CapWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1086]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SandWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1096]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "LakeWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1094]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "ForestWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1089]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "CityWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1088]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SnowWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1087]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SeaWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1095]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "LavaWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1090]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "SkyWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1091]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "MoonWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1165]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "PeachWorldHomeStage") == 0) {
            return static_cast<int>(shineColors[1152]);
        }
        if (strcmp(curShine->curShineInfo->mStageName.cstr(), "Special1WorldHomeStage") == 0) {
            // Add conditions for other Dark Side hint arts
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "WaterfallWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1132]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "LakeWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1128]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "CloudWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1124]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "ClashWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1126]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "CityWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1130]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "SnowWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1129]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "SeaWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1127]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "LavaWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1123]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "BossRaidWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1125]);
            }
            if (strcmp(GameDataFunction::tryGetCurrentMainStageName(accessor), "PeachWorldHomeStage") == 0) {
                return static_cast<int>(shineColors[1131]);
            }
        }
    } else {
        /*sead::FixedSafeString<40> shineData;
        shineData = "";
        shineData.append("Uid: ");
        shineData.append(intToCstr(info->uniqueId));
        shineData.append(" Color: ");
        shineData.append(intToCstr(shineColors[info->uniqueId]));

        setMessage(1, shineData.cstr());
        shineData = "";
        shineData.append("Uid: ");
        shineData.append(intToCstr(1145));
        shineData.append(" Color: ");
        shineData.append(intToCstr(shineColors[1145]));

        setMessage(2, shineData.cstr());*/
        return static_cast<int>(shineColors[info->uniqueId]);
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

    if (strcmp("ItemCap", fileName) == 0) {
        curItem = shopCapTextReplacements[getIndexApCostumeList(convert.cstr()) - 1];
    } else if (strcmp("ItemCloth", fileName) == 0) {
        curItem = shopClothTextReplacements[getIndexApCostumeList(convert.cstr()) - 1];
    } else if (strcmp("ItemSticker", fileName) == 0) {
        curItem = shopStickerTextReplacements[getIndexStickerList(convert.cstr())];
    } else if (strcmp("ItemGift", fileName) == 0) {
        curItem = shopGiftTextReplacements[getIndexSouvenirList(convert.cstr())];
    } else if (strcmp("ItemMoon", fileName) == 0) {
        // Find out key for each kingdom as still is unknown
        curItem = shopMoonTextReplacements[getIndexMoonItemList(convert.cstr())];
    } else {
        // Not included items like Life Up Hearts
        return u"";
    }

    if (curItem.gameIndex == 254) {
        // setMessage(1, "No Item Data Received.");
    }

    if (isExplain) {
        message.append(u"Comes from the world of ");
        // if (mGameNames[curItem.gameIndex].isEmpty()) {
        message.append(mGameNames[curItem.gameIndex].cstr());
        //} else {
        // message.append(u"Missing Game");
        // }

        message.append(u".\nSeems to belong to ");
        // if (mSlotNames[curItem.slotIndex].isEmpty()) {
        message.append(mSlotNames[curItem.slotIndex].cstr());
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
        message.append(mItemNames[curItem.apItemNameIndex].cstr());
        //} else {
        // message.append(u"Missing Item Name");
        //}
    }

    return message.cstr();
}

// void ArchipelagoMode::updateChatMessages(ArchipelagoChatMessage* packet) {
//
//     apChatLine1 = packet->message1;
//     apChatLine2 = packet->message2;
//     apChatLine3 = packet->message3;
// }

void ArchipelagoMode::sendStage(GameDataHolderWriter writer, const ChangeStageInfo* stageInfo) {
    GameDataHolderAccessor accessor(mCurScene);

    setScenario(stageInfo->mChangeStageName.cstr(), stageInfo->mScenarioNo);
    // setMessage(1, "onGrandShineStageChange");
    // setMessage(2, stageInfo->mChangeStageName.cstr());

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

void ArchipelagoMode::update() {
    PlayerActorHakoniwa* playerHakoniwa = getPlayerActorHakoniwa();
    PlayerActorBase* playerBase = (PlayerActorBase*)playerHakoniwa;
    GameDataHolderWriter writer(mCurScene);
    GameDataHolderAccessor accessor(mCurScene);
    StageScene* stageScene = (StageScene*)mCurScene;
    if (!playerBase)
        return;

    if (!GameModeManager::instance()->isPaused()) {
        // Death Link handling
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

        // Capture Sanity Enforcement
        if (mCapturesEnabled) {
            al::LiveActor* curHack = playerBase->getPlayerHackKeeper()->mHackModel;
            const char* hackName = playerBase->getPlayerHackKeeper()->getCurrentHackName();
            if (hackName != nullptr && !hasCapture(hackName) && mIsRecordCapture) {
                if (!(al::isEqualString(hackName, "ElectricWire") && getScenario(0) < 2 && GameDataFunction::getCurrentWorldId(accessor) == 0)) {
                    // Client::setMessage(1, hackNamehackName);
                    if (!playerBase->getPlayerHackKeeper()->isActiveHackStartDemo()) {
                        bool tryEscape = false;
                        int nonKillCaptures[7] = {10, 13, 24, 25, 28, 29, 37};
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

        // Moon Shard Updater
        // Prevents softlock when moon is received mid shard moon
        if (!(al::isEqualString(GameDataFunction::tryGetCurrentMainStageName(accessor), "CapWorldHomeStage") && getScenario(0) < 2) &&
            rs::isExistShineChipWatcher(playerBase) && rs::getShineChipCount(playerBase) > 0) {
            Client::startShineChipCount();
        }

        if (mUpdateCounterTimer >= 1800) {
            Client::startShineCount();
            mUpdateCounterTimer = 0;
        }

        // softlock prevention
        if (mSoftlockTimer >= 60) {
            // Check and prevent crashed home softlock
            if (GameDataFunction::isBossAttackedHome(accessor)) {
                // Client::setMessage(1, GameDataFunction::getCurrentStageName(accessor));
                if (strcmp(GameDataFunction::getCurrentStageName(accessor), "BossRaidWorldHomeStage") == 0) {
                    GameDataFunction::repairHomeByCrashedBoss(writer);
                    GameDataFunction::crashHome(writer);
                    // isGotShine crashes game here for some reason
                    /*int ruinedCount = 0;
                    if (GameDataFunction::isGotShine(accessor, GameDataFunction::getWorldIndexBoss(),
                                                     0)) {
                        ruinedCount += 3;
                    }

                    for (int i = 1; i < 9; i++) {
                        if (GameDataFunction::isGotShine(accessor, GameDataFunction::getWorldIndexBoss(),
                                                         i)) {
                            ruinedCount++;
                        }
                    }
                    if (ruinedCount < Client::getRaidCount()) {
                        GameDataFunction::repairHome(accessor);
                    } else {
                        GameDataFunction::bossAttackHome(accessor);
                    }*/
                } else {
                    GameDataFunction::repairHome(writer);
                }
            }

            // Edge case where game repairs odyssey in ruined but doesn't unlock bowser kingdom
            if (GameDataFunction::isRepairHomeByCrashedBoss(accessor)) {
                GameDataFunction::unlockWorld(writer, GameDataFunction::getWorldIndexSky());
            }

            // Check for lost kingdom softlock state
            if (GameDataFunction::isCrashHome(accessor)) {
                if (strcmp(GameDataFunction::getCurrentStageName(accessor), "ClashWorldHomeStage") == 0) {
                    int lostCount = 0;
                    for (int i = 1; i < 25; i++) {
                        if (GameDataFunction::isGotShine(accessor, GameDataFunction::getWorldIndexClash(), i))
                            lostCount++;
                    }
                    if (lostCount < getWorldUnlockCount(GameDataFunction::getWorldIndexClash())) {
                        GameDataFunction::repairHome(writer);
                        GameDataFunction::unlockWorld(writer, GameDataFunction::getWorldIndexClash());
                    } else {
                        GameDataFunction::crashHome(writer);
                    }
                } else {
                    GameDataFunction::repairHome(writer);
                }
            }

            mSoftlockTimer = 0;
        }

        mUpdateCounterTimer += 0;
        mSoftlockTimer += 0;
    }

    // D-Pad functions
    if (al::isPadHoldR(-1)) {
        // Purple Coin Search
        if (al::isPadTriggerRight(-1)) {
            if (al::isExistSceneObj(stageScene, 7)) {
                CoinCollectHolder* coinCollectHolder = (CoinCollectHolder*)al::getSceneObj(stageScene, 7);
                if (coinCollectHolder) {
                    CoinCollect* coinCollect = coinCollectHolder->tryFindAliveCoinCollect(al::getTrans(playerBase), true);
                    if (coinCollect) {
                        mHintArrow->setTarget(al::getTransPtr(coinCollect));
                        mCoinCollectHintTarget = coinCollect;

                    } else {
                        CoinCollect2D* coinCollect2D = coinCollectHolder->tryFindAliveCoinCollect2D(al::getTrans(playerBase), true);

                        if (coinCollect2D) {
                            mHintArrow->setTarget(al::getTransPtr(coinCollect2D));
                            mCoinCollectHintTarget = coinCollect2D;
                        }
                    }
                }
            }
        }

        // Return to Odyssey
        if (al::isPadTriggerLeft(-1)) {
        }
    }

    // Debug arcipelago buttons
    if (mInfo->mIsDebugMode) {
    }
}

void ArchipelagoMode::debugMenuControls() {
    // ImGui::Text("- L + ← | Enable/disable Freeze Tag [FT]\n");
    // ImGui::Text("- [FT] ↑ | Switch between runners and chasers\n");
    // ImGui::Text("- [FT] L + ↓ | Reset score\n");

    // if (mInfo->mIsDebugMode) {
    //     ImGui::Text("- [FT][Debug] A + → | Increment score\n");
    //     ImGui::Text("- [FT][Debug] A + ← | Set time to 01:05\n");
    //     ImGui::Text("- [FT][Debug] B + → | Wipeout\n");
    // }
}