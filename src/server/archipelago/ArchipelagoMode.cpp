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
#include "server/archipelago/ArchipelagoHintArrow.h"
#include "server/Client.hpp"
#include "server/DeltaTime.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "System/PlayerHitPointData.h"

ArchipelagoMode::ArchipelagoMode(const char* name) : GameModeBase(name) {}

void ArchipelagoMode::init(const GameModeInitInfo& info) {
    mHeap = sead::ExpHeap::create(60000, "ArchipelagoHeap", sead::HeapMgr::instance()->getCurrentHeap(), 8, sead::Heap::cHeapDirection_Forward, false);
    // Approx size = 50608
    // Approx 9392 extra bytes allocated
    sead::ScopedCurrentHeapSetter heapSetter(mHeap);

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

void ArchipelagoMode::receiveDeath(Deathlink* packet) {
    mApDeath = true;
    mDying = true;
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

void ArchipelagoMode::updateSentShines(ShineChecks* packet) {
    addShine(packet->shineUid0);
    addShine(packet->shineUid1);
    addShine(packet->shineUid2);
    addShine(packet->shineUid3);
    addShine(packet->shineUid4);
    addShine(packet->shineUid5);
    addShine(packet->shineUid6);
    addShine(packet->shineUid7);
    addShine(packet->shineUid8);
    addShine(packet->shineUid9);
    addShine(packet->shineUid10);
    addShine(packet->shineUid11);
    addShine(packet->shineUid12);
    addShine(packet->shineUid13);
    addShine(packet->shineUid14);
    addShine(packet->shineUid15);
    addShine(packet->shineUid16);
    addShine(packet->shineUid17);
    addShine(packet->shineUid18);
    addShine(packet->shineUid19);
    addShine(packet->shineUid20);
    addShine(packet->shineUid21);
    addShine(packet->shineUid22);
    addShine(packet->shineUid23);
    addShine(packet->shineUid24);
    addShine(packet->shineUid25);
    addShine(packet->shineUid26);
    addShine(packet->shineUid27);
    addShine(packet->shineUid28);
    addShine(packet->shineUid29);
    addShine(packet->shineUid30);
    addShine(packet->shineUid31);
    addShine(packet->shineUid32);
    addShine(packet->shineUid33);
    addShine(packet->shineUid34);
    addShine(packet->shineUid35);
    addShine(packet->shineUid36);
    addShine(packet->shineUid37);
    addShine(packet->shineUid38);
    addShine(packet->shineUid39);
    addShine(packet->shineUid40);
    addShine(packet->shineUid41);
    addShine(packet->shineUid42);
    addShine(packet->shineUid43);
    addShine(packet->shineUid44);
    addShine(packet->shineUid45);
    addShine(packet->shineUid46);
    addShine(packet->shineUid47);
    addShine(packet->shineUid48);
    addShine(packet->shineUid49);
    addShine(packet->shineUid50);
    addShine(packet->shineUid51);
    addShine(packet->shineUid52);
    addShine(packet->shineUid53);
    addShine(packet->shineUid54);
    addShine(packet->shineUid55);
    addShine(packet->shineUid56);
    addShine(packet->shineUid57);
    addShine(packet->shineUid58);
    addShine(packet->shineUid59);
    addShine(packet->shineUid60);
    addShine(packet->shineUid61);
    addShine(packet->shineUid62);
    addShine(packet->shineUid63);
    addShine(packet->shineUid64);
    addShine(packet->shineUid65);
    addShine(packet->shineUid66);
    addShine(packet->shineUid67);
    addShine(packet->shineUid68);
    addShine(packet->shineUid69);
    addShine(packet->shineUid70);
    addShine(packet->shineUid71);
    addShine(packet->shineUid72);
    addShine(packet->shineUid73);
    addShine(packet->shineUid74);
    addShine(packet->shineUid75);
    addShine(packet->shineUid76);
    addShine(packet->shineUid77);
    addShine(packet->shineUid78);
    addShine(packet->shineUid79);
    addShine(packet->shineUid80);
    addShine(packet->shineUid81);
    addShine(packet->shineUid82);
    addShine(packet->shineUid83);
    addShine(packet->shineUid84);
    addShine(packet->shineUid85);
    addShine(packet->shineUid86);
    addShine(packet->shineUid87);
    addShine(packet->shineUid88);
    addShine(packet->shineUid89);
    addShine(packet->shineUid90);
    addShine(packet->shineUid91);
    addShine(packet->shineUid92);
    addShine(packet->shineUid93);
    addShine(packet->shineUid94);
    addShine(packet->shineUid95);
    addShine(packet->shineUid96);
    addShine(packet->shineUid97);
    addShine(packet->shineUid98);
    addShine(packet->shineUid99);
}

int ArchipelagoMode::getWorldUnlockCount(int worldId) {
    return mWorldPayCounts[worldId];
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

void ArchipelagoMode::addApInfo(ApInfo* packet) {
    int type = static_cast<int>(packet->infoType);

    if (type < 3) {
        sead::WFixedSafeString<40> info1;
        sead::WFixedSafeString<40> info2;
        sead::WFixedSafeString<40> info3;
        info1 = u"";
        info2 = u"";
        info3 = u"";

        for (int i = 0; i < 40; i++) {
            if (packet->info1[i] == '\0') {
                break;
            }
            info1.append(static_cast<char16>(packet->info1[i]));
        }

        for (int i = 0; i < 40; i++) {
            if (packet->info2[i] == '\0') {
                break;
            }
            info2.append(static_cast<char16>(packet->info2[i]));
        }

        for (int i = 0; i < 40; i++) {
            if (packet->info3[i] == '\0') {
                break;
            }
            info3.append(static_cast<char16>(packet->info3[i]));
        }

        // setMessage(2, "AP Info Entered");

        if (type == 0) {
            // setMessage(1, "Game Info Entered");
            // if (!info1.isEmpty())
            //{
            /*if (packet->index1 == 0)
            {
                setMessage(1, packet->info1);
            }*/
            apGameNames[packet->index1] = apGameNames[packet->index1].cEmptyString;
            apGameNames[packet->index1].append(info1.cstr());
            numApGames++;
            //}
            // if (!info2.isEmpty())
            //{
            apGameNames[packet->index2] = apGameNames[packet->index2].cEmptyString;
            apGameNames[packet->index2].append(info2.cstr());
            numApGames++;
            //}
            // if (!info3.isEmpty())
            //{
            apGameNames[packet->index3] = apGameNames[packet->index3].cEmptyString;
            apGameNames[packet->index3].append(info3.cstr());
            numApGames++;
            //}
        }

        if (type == 1) {
            // if (!info1.isEmpty())
            //{
            apSlotNames[packet->index1] = apSlotNames[packet->index1].cEmptyString;
            apSlotNames[packet->index1].append(info1.cstr());
            numApSlots++;
            //}
            // if (!info2.isEmpty())
            //{
            apSlotNames[packet->index2] = apSlotNames[packet->index2].cEmptyString;
            apSlotNames[packet->index2].append(info2.cstr());
            numApSlots++;
            //}
            // if (!info3.isEmpty())
            //{
            apSlotNames[packet->index3] = apSlotNames[packet->index3].cEmptyString;
            apSlotNames[packet->index3].append(info3.cstr());
            numApSlots++;
            //}
        }

        if (type == 2) {
            // if (!info1.isEmpty())
            //{
            apItemNames[packet->index1] = apItemNames[packet->index1].cEmptyString;
            apItemNames[packet->index1].append(info1.cstr());
            numApItems++;
            //}
            // if (!info2.isEmpty())
            //{
            apItemNames[packet->index2] = apItemNames[packet->index2].cEmptyString;
            apItemNames[packet->index2].append(info2.cstr());
            numApItems++;
            //}
            // if (!info3.isEmpty())
            //{
            apItemNames[packet->index3] = apItemNames[packet->index3].cEmptyString;
            apItemNames[packet->index3].append(info3.cstr());
            numApItems++;
            //}
        }
    } else {
        if (type == 3) {
            shineItemNames[packet->index1] = shineItemNames[packet->index1].cEmptyString;
            shineItemNames[packet->index1].append(packet->info1);

            if (packet->index1 < 99) {
                shineItemNames[packet->index2] = shineItemNames[packet->index2].cEmptyString;
                shineItemNames[packet->index2].append(packet->info2);

                shineItemNames[packet->index3] = shineItemNames[packet->index3].cEmptyString;
                shineItemNames[packet->index3].append(packet->info3);
            }
        }
    }
}

void ArchipelagoMode::updateShineReplace(ShineReplacePacket* packet) {
    shineTextReplacements[0] = {packet->itemType0, packet->itemNameIndex0};
    shineTextReplacements[1] = {packet->itemType1, packet->itemNameIndex1};
    shineTextReplacements[2] = {packet->itemType2, packet->itemNameIndex2};
    shineTextReplacements[3] = {packet->itemType3, packet->itemNameIndex3};
    shineTextReplacements[4] = {packet->itemType4, packet->itemNameIndex4};
    shineTextReplacements[5] = {packet->itemType5, packet->itemNameIndex5};
    shineTextReplacements[6] = {packet->itemType6, packet->itemNameIndex6};
    shineTextReplacements[7] = {packet->itemType7, packet->itemNameIndex7};
    shineTextReplacements[8] = {packet->itemType8, packet->itemNameIndex8};
    shineTextReplacements[9] = {packet->itemType9, packet->itemNameIndex9};
    shineTextReplacements[10] = {packet->itemType10, packet->itemNameIndex10};
    shineTextReplacements[11] = {packet->itemType11, packet->itemNameIndex11};
    shineTextReplacements[12] = {packet->itemType12, packet->itemNameIndex12};
    shineTextReplacements[13] = {packet->itemType13, packet->itemNameIndex13};
    shineTextReplacements[14] = {packet->itemType14, packet->itemNameIndex14};
    shineTextReplacements[15] = {packet->itemType15, packet->itemNameIndex15};
    shineTextReplacements[16] = {packet->itemType16, packet->itemNameIndex16};
    shineTextReplacements[17] = {packet->itemType17, packet->itemNameIndex17};
    shineTextReplacements[18] = {packet->itemType18, packet->itemNameIndex18};
    shineTextReplacements[19] = {packet->itemType19, packet->itemNameIndex19};
    shineTextReplacements[20] = {packet->itemType20, packet->itemNameIndex20};
    shineTextReplacements[21] = {packet->itemType21, packet->itemNameIndex21};
    shineTextReplacements[22] = {packet->itemType22, packet->itemNameIndex22};
    shineTextReplacements[23] = {packet->itemType23, packet->itemNameIndex23};
    shineTextReplacements[24] = {packet->itemType24, packet->itemNameIndex24};
    shineTextReplacements[25] = {packet->itemType25, packet->itemNameIndex25};
    shineTextReplacements[26] = {packet->itemType26, packet->itemNameIndex26};
    shineTextReplacements[27] = {packet->itemType27, packet->itemNameIndex27};
    shineTextReplacements[28] = {packet->itemType28, packet->itemNameIndex28};
    shineTextReplacements[29] = {packet->itemType29, packet->itemNameIndex29};
    shineTextReplacements[30] = {packet->itemType30, packet->itemNameIndex30};
    shineTextReplacements[31] = {packet->itemType31, packet->itemNameIndex31};
    shineTextReplacements[32] = {packet->itemType32, packet->itemNameIndex32};
    shineTextReplacements[33] = {packet->itemType33, packet->itemNameIndex33};
    shineTextReplacements[34] = {packet->itemType34, packet->itemNameIndex34};
    shineTextReplacements[35] = {packet->itemType35, packet->itemNameIndex35};
    shineTextReplacements[36] = {packet->itemType36, packet->itemNameIndex36};
    shineTextReplacements[37] = {packet->itemType37, packet->itemNameIndex37};
    shineTextReplacements[38] = {packet->itemType38, packet->itemNameIndex38};
    shineTextReplacements[39] = {packet->itemType39, packet->itemNameIndex39};
    shineTextReplacements[40] = {packet->itemType40, packet->itemNameIndex40};
    shineTextReplacements[41] = {packet->itemType41, packet->itemNameIndex41};
    shineTextReplacements[42] = {packet->itemType42, packet->itemNameIndex42};
    shineTextReplacements[43] = {packet->itemType43, packet->itemNameIndex43};
    shineTextReplacements[44] = {packet->itemType44, packet->itemNameIndex44};
    shineTextReplacements[45] = {packet->itemType45, packet->itemNameIndex45};
    shineTextReplacements[46] = {packet->itemType46, packet->itemNameIndex46};
    shineTextReplacements[47] = {packet->itemType47, packet->itemNameIndex47};
    shineTextReplacements[48] = {packet->itemType48, packet->itemNameIndex48};
    shineTextReplacements[49] = {packet->itemType49, packet->itemNameIndex49};
    shineTextReplacements[50] = {packet->itemType50, packet->itemNameIndex50};
    shineTextReplacements[51] = {packet->itemType51, packet->itemNameIndex51};
    shineTextReplacements[52] = {packet->itemType52, packet->itemNameIndex52};
    shineTextReplacements[53] = {packet->itemType53, packet->itemNameIndex53};
    shineTextReplacements[54] = {packet->itemType54, packet->itemNameIndex54};
    shineTextReplacements[55] = {packet->itemType55, packet->itemNameIndex55};
    shineTextReplacements[56] = {packet->itemType56, packet->itemNameIndex56};
    shineTextReplacements[57] = {packet->itemType57, packet->itemNameIndex57};
    shineTextReplacements[58] = {packet->itemType58, packet->itemNameIndex58};
    shineTextReplacements[59] = {packet->itemType59, packet->itemNameIndex59};
    shineTextReplacements[60] = {packet->itemType60, packet->itemNameIndex60};
    shineTextReplacements[61] = {packet->itemType61, packet->itemNameIndex61};
    shineTextReplacements[62] = {packet->itemType62, packet->itemNameIndex62};
    shineTextReplacements[63] = {packet->itemType63, packet->itemNameIndex63};
    shineTextReplacements[64] = {packet->itemType64, packet->itemNameIndex64};
    shineTextReplacements[65] = {packet->itemType65, packet->itemNameIndex65};
    shineTextReplacements[66] = {packet->itemType66, packet->itemNameIndex66};
    shineTextReplacements[67] = {packet->itemType67, packet->itemNameIndex67};
    shineTextReplacements[68] = {packet->itemType68, packet->itemNameIndex68};
    shineTextReplacements[69] = {packet->itemType69, packet->itemNameIndex69};
    shineTextReplacements[70] = {packet->itemType70, packet->itemNameIndex70};
    shineTextReplacements[71] = {packet->itemType71, packet->itemNameIndex71};
    shineTextReplacements[72] = {packet->itemType72, packet->itemNameIndex72};
    shineTextReplacements[73] = {packet->itemType73, packet->itemNameIndex73};
    shineTextReplacements[74] = {packet->itemType74, packet->itemNameIndex74};
    shineTextReplacements[75] = {packet->itemType75, packet->itemNameIndex75};
    shineTextReplacements[76] = {packet->itemType76, packet->itemNameIndex76};
    shineTextReplacements[77] = {packet->itemType77, packet->itemNameIndex77};
    shineTextReplacements[78] = {packet->itemType78, packet->itemNameIndex78};
    shineTextReplacements[79] = {packet->itemType79, packet->itemNameIndex79};
    shineTextReplacements[80] = {packet->itemType80, packet->itemNameIndex80};
    shineTextReplacements[81] = {packet->itemType81, packet->itemNameIndex81};
    shineTextReplacements[82] = {packet->itemType82, packet->itemNameIndex82};
    shineTextReplacements[83] = {packet->itemType83, packet->itemNameIndex83};
    shineTextReplacements[84] = {packet->itemType84, packet->itemNameIndex84};
    shineTextReplacements[85] = {packet->itemType85, packet->itemNameIndex85};
    shineTextReplacements[86] = {packet->itemType86, packet->itemNameIndex86};
    shineTextReplacements[87] = {packet->itemType87, packet->itemNameIndex87};
    shineTextReplacements[88] = {packet->itemType88, packet->itemNameIndex88};
    shineTextReplacements[89] = {packet->itemType89, packet->itemNameIndex89};
    shineTextReplacements[90] = {packet->itemType90, packet->itemNameIndex90};
    shineTextReplacements[91] = {packet->itemType91, packet->itemNameIndex91};
    shineTextReplacements[92] = {packet->itemType92, packet->itemNameIndex92};
    shineTextReplacements[93] = {packet->itemType93, packet->itemNameIndex93};
    shineTextReplacements[94] = {packet->itemType94, packet->itemNameIndex94};
    shineTextReplacements[95] = {packet->itemType95, packet->itemNameIndex95};
    shineTextReplacements[96] = {packet->itemType96, packet->itemNameIndex96};
    shineTextReplacements[97] = {packet->itemType97, packet->itemNameIndex97};
    shineTextReplacements[98] = {packet->itemType98, packet->itemNameIndex98};
    shineTextReplacements[99] = {packet->itemType99, packet->itemNameIndex99};
}

void ArchipelagoMode::updateShineColor(ShineColor* packet) {
    // setMessage(1, "Entering udpateShineColor");
    shineColors[static_cast<int>(packet->shineUid0)] = packet->color0;
    shineColors[static_cast<int>(packet->shineUid1)] = packet->color1;
    shineColors[static_cast<int>(packet->shineUid2)] = packet->color2;
    shineColors[static_cast<int>(packet->shineUid3)] = packet->color3;
    shineColors[static_cast<int>(packet->shineUid4)] = packet->color4;
    shineColors[static_cast<int>(packet->shineUid5)] = packet->color5;
    shineColors[static_cast<int>(packet->shineUid6)] = packet->color6;
    shineColors[static_cast<int>(packet->shineUid7)] = packet->color7;
    shineColors[static_cast<int>(packet->shineUid8)] = packet->color8;
    shineColors[static_cast<int>(packet->shineUid9)] = packet->color9;
    shineColors[static_cast<int>(packet->shineUid10)] = packet->color10;
    shineColors[static_cast<int>(packet->shineUid11)] = packet->color11;
    shineColors[static_cast<int>(packet->shineUid12)] = packet->color12;
    shineColors[static_cast<int>(packet->shineUid13)] = packet->color13;
    shineColors[static_cast<int>(packet->shineUid14)] = packet->color14;
    shineColors[static_cast<int>(packet->shineUid15)] = packet->color15;
    shineColors[static_cast<int>(packet->shineUid16)] = packet->color16;
    shineColors[static_cast<int>(packet->shineUid17)] = packet->color17;
    shineColors[static_cast<int>(packet->shineUid18)] = packet->color18;
    shineColors[static_cast<int>(packet->shineUid19)] = packet->color19;
    shineColors[static_cast<int>(packet->shineUid20)] = packet->color20;
    shineColors[static_cast<int>(packet->shineUid21)] = packet->color21;
    shineColors[static_cast<int>(packet->shineUid22)] = packet->color22;
    shineColors[static_cast<int>(packet->shineUid23)] = packet->color23;
    shineColors[static_cast<int>(packet->shineUid24)] = packet->color24;
    shineColors[static_cast<int>(packet->shineUid25)] = packet->color25;
    shineColors[static_cast<int>(packet->shineUid26)] = packet->color26;
    shineColors[static_cast<int>(packet->shineUid27)] = packet->color27;
    shineColors[static_cast<int>(packet->shineUid28)] = packet->color28;
    shineColors[static_cast<int>(packet->shineUid29)] = packet->color29;
    shineColors[static_cast<int>(packet->shineUid30)] = packet->color30;
    shineColors[static_cast<int>(packet->shineUid31)] = packet->color31;
    shineColors[static_cast<int>(packet->shineUid32)] = packet->color32;
    shineColors[static_cast<int>(packet->shineUid33)] = packet->color33;
    shineColors[static_cast<int>(packet->shineUid34)] = packet->color34;
    shineColors[static_cast<int>(packet->shineUid35)] = packet->color35;
    shineColors[static_cast<int>(packet->shineUid36)] = packet->color36;
    shineColors[static_cast<int>(packet->shineUid37)] = packet->color37;
    shineColors[static_cast<int>(packet->shineUid38)] = packet->color38;
    shineColors[static_cast<int>(packet->shineUid39)] = packet->color39;
    shineColors[static_cast<int>(packet->shineUid40)] = packet->color40;
    shineColors[static_cast<int>(packet->shineUid41)] = packet->color41;
    shineColors[static_cast<int>(packet->shineUid42)] = packet->color42;
    shineColors[static_cast<int>(packet->shineUid43)] = packet->color43;
    shineColors[static_cast<int>(packet->shineUid44)] = packet->color44;
    shineColors[static_cast<int>(packet->shineUid45)] = packet->color45;
    shineColors[static_cast<int>(packet->shineUid46)] = packet->color46;
    shineColors[static_cast<int>(packet->shineUid47)] = packet->color47;
    shineColors[static_cast<int>(packet->shineUid48)] = packet->color48;
    shineColors[static_cast<int>(packet->shineUid49)] = packet->color49;
    shineColors[static_cast<int>(packet->shineUid50)] = packet->color50;
    // shineColors[static_cast<int>(packet->shineUid51)] = packet->color51;
    // shineColors[static_cast<int>(packet->shineUid52)] = packet->color52;
    // shineColors[static_cast<int>(packet->shineUid53)] = packet->color53;
    // shineColors[static_cast<int>(packet->shineUid54)] = packet->color54;
    // shineColors[static_cast<int>(packet->shineUid55)] = packet->color55;
    // shineColors[static_cast<int>(packet->shineUid56)] = packet->color56;
    // shineColors[static_cast<int>(packet->shineUid57)] = packet->color57;
    // shineColors[static_cast<int>(packet->shineUid58)] = packet->color58;
    // shineColors[static_cast<int>(packet->shineUid59)] = packet->color59;
    // shineColors[static_cast<int>(packet->shineUid60)] = packet->color60;
    // shineColors[static_cast<int>(packet->shineUid61)] = packet->color61;
    // shineColors[static_cast<int>(packet->shineUid62)] = packet->color62;
    // shineColors[static_cast<int>(packet->shineUid63)] = packet->color63;
    // shineColors[static_cast<int>(packet->shineUid64)] = packet->color64;
    // shineColors[static_cast<int>(packet->shineUid65)] = packet->color65;
    // shineColors[static_cast<int>(packet->shineUid66)] = packet->color66;
    // shineColors[static_cast<int>(packet->shineUid67)] = packet->color67;
    // shineColors[static_cast<int>(packet->shineUid68)] = packet->color68;
    // shineColors[static_cast<int>(packet->shineUid69)] = packet->color69;
    // shineColors[static_cast<int>(packet->shineUid70)] = packet->color70;
    // shineColors[static_cast<int>(packet->shineUid71)] = packet->color71;
    // shineColors[static_cast<int>(packet->shineUid72)] = packet->color72;
    // shineColors[static_cast<int>(packet->shineUid73)] = packet->color73;
    // shineColors[static_cast<int>(packet->shineUid74)] = packet->color74;
    // shineColors[static_cast<int>(packet->shineUid75)] = packet->color75;
    // shineColors[static_cast<int>(packet->shineUid76)] = packet->color76;
    // shineColors[static_cast<int>(packet->shineUid77)] = packet->color77;
    // shineColors[static_cast<int>(packet->shineUid78)] = packet->color78;
    // shineColors[static_cast<int>(packet->shineUid79)] = packet->color79;
    // shineColors[static_cast<int>(packet->shineUid80)] = packet->color80;
    // shineColors[static_cast<int>(packet->shineUid81)] = packet->color81;
    // shineColors[static_cast<int>(packet->shineUid82)] = packet->color82;
    // shineColors[static_cast<int>(packet->shineUid83)] = packet->color83;
}

void ArchipelagoMode::updateShopReplace(ShopReplacePacket* packet) {
    int type = static_cast<int>(packet->infoType);
    // Cap
    if (type == 0) {
        shopCapTextReplacements[0] = {packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, packet->itemClassification0};
        shopCapTextReplacements[1] = {packet->gameIndex1, packet->playerIndex1, packet->itemIndex1, packet->itemClassification1};
        shopCapTextReplacements[2] = {packet->gameIndex2, packet->playerIndex2, packet->itemIndex2, packet->itemClassification2};
        shopCapTextReplacements[3] = {packet->gameIndex3, packet->playerIndex3, packet->itemIndex3, packet->itemClassification3};
        shopCapTextReplacements[4] = {packet->gameIndex4, packet->playerIndex4, packet->itemIndex4, packet->itemClassification4};
        shopCapTextReplacements[5] = {packet->gameIndex5, packet->playerIndex5, packet->itemIndex5, packet->itemClassification5};
        shopCapTextReplacements[6] = {packet->gameIndex6, packet->playerIndex6, packet->itemIndex6, packet->itemClassification6};
        shopCapTextReplacements[7] = {packet->gameIndex7, packet->playerIndex7, packet->itemIndex7, packet->itemClassification7};
        shopCapTextReplacements[8] = {packet->gameIndex8, packet->playerIndex8, packet->itemIndex8, packet->itemClassification8};
        shopCapTextReplacements[9] = {packet->gameIndex9, packet->playerIndex9, packet->itemIndex9, packet->itemClassification9};
        shopCapTextReplacements[10] = {packet->gameIndex10, packet->playerIndex10, packet->itemIndex10, packet->itemClassification10};
        shopCapTextReplacements[11] = {packet->gameIndex11, packet->playerIndex11, packet->itemIndex11, packet->itemClassification11};
        shopCapTextReplacements[12] = {packet->gameIndex12, packet->playerIndex12, packet->itemIndex12, packet->itemClassification12};
        shopCapTextReplacements[13] = {packet->gameIndex13, packet->playerIndex13, packet->itemIndex13, packet->itemClassification13};
        shopCapTextReplacements[14] = {packet->gameIndex14, packet->playerIndex14, packet->itemIndex14, packet->itemClassification14};
        shopCapTextReplacements[15] = {packet->gameIndex15, packet->playerIndex15, packet->itemIndex15, packet->itemClassification15};
        shopCapTextReplacements[16] = {packet->gameIndex16, packet->playerIndex16, packet->itemIndex16, packet->itemClassification16};
        shopCapTextReplacements[17] = {packet->gameIndex17, packet->playerIndex17, packet->itemIndex17, packet->itemClassification17};
        shopCapTextReplacements[18] = {packet->gameIndex18, packet->playerIndex18, packet->itemIndex18, packet->itemClassification18};
        shopCapTextReplacements[19] = {packet->gameIndex19, packet->playerIndex19, packet->itemIndex19, packet->itemClassification19};
        shopCapTextReplacements[20] = {packet->gameIndex20, packet->playerIndex20, packet->itemIndex20, packet->itemClassification20};
        shopCapTextReplacements[21] = {packet->gameIndex21, packet->playerIndex21, packet->itemIndex21, packet->itemClassification21};
        shopCapTextReplacements[22] = {packet->gameIndex22, packet->playerIndex22, packet->itemIndex22, packet->itemClassification22};
        shopCapTextReplacements[23] = {packet->gameIndex23, packet->playerIndex23, packet->itemIndex23, packet->itemClassification23};
        shopCapTextReplacements[24] = {packet->gameIndex24, packet->playerIndex24, packet->itemIndex24, packet->itemClassification24};
        shopCapTextReplacements[25] = {packet->gameIndex25, packet->playerIndex25, packet->itemIndex25, packet->itemClassification25};
        shopCapTextReplacements[26] = {packet->gameIndex26, packet->playerIndex26, packet->itemIndex26, packet->itemClassification26};
        shopCapTextReplacements[27] = {packet->gameIndex27, packet->playerIndex27, packet->itemIndex27, packet->itemClassification27};
        shopCapTextReplacements[28] = {packet->gameIndex28, packet->playerIndex28, packet->itemIndex28, packet->itemClassification28};
        shopCapTextReplacements[29] = {packet->gameIndex29, packet->playerIndex29, packet->itemIndex29, packet->itemClassification29};
        shopCapTextReplacements[30] = {packet->gameIndex30, packet->playerIndex30, packet->itemIndex30, packet->itemClassification30};
        shopCapTextReplacements[31] = {packet->gameIndex31, packet->playerIndex31, packet->itemIndex31, packet->itemClassification31};
        shopCapTextReplacements[32] = {packet->gameIndex32, packet->playerIndex32, packet->itemIndex32, packet->itemClassification32};
        shopCapTextReplacements[33] = {packet->gameIndex33, packet->playerIndex33, packet->itemIndex33, packet->itemClassification33};
        shopCapTextReplacements[34] = {packet->gameIndex34, packet->playerIndex34, packet->itemIndex34, packet->itemClassification34};
        shopCapTextReplacements[35] = {packet->gameIndex35, packet->playerIndex35, packet->itemIndex35, packet->itemClassification35};
        shopCapTextReplacements[36] = {packet->gameIndex36, packet->playerIndex36, packet->itemIndex36, packet->itemClassification36};
        shopCapTextReplacements[37] = {packet->gameIndex37, packet->playerIndex37, packet->itemIndex37, packet->itemClassification37};
        shopCapTextReplacements[38] = {packet->gameIndex38, packet->playerIndex38, packet->itemIndex38, packet->itemClassification38};
        shopCapTextReplacements[39] = {packet->gameIndex39, packet->playerIndex39, packet->itemIndex39, packet->itemClassification39};
        shopCapTextReplacements[40] = {packet->gameIndex40, packet->playerIndex40, packet->itemIndex40, packet->itemClassification40};
        shopCapTextReplacements[41] = {packet->gameIndex41, packet->playerIndex41, packet->itemIndex41, packet->itemClassification41};
        shopCapTextReplacements[42] = {packet->gameIndex42, packet->playerIndex42, packet->itemIndex42, packet->itemClassification42};
        shopCapTextReplacements[43] = {packet->gameIndex43, packet->playerIndex43, packet->itemIndex43, packet->itemClassification43};
    }
    // Cloth
    if (type == 1) {
        shopClothTextReplacements[0] = {packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, packet->itemClassification0};
        shopClothTextReplacements[1] = {packet->gameIndex1, packet->playerIndex1, packet->itemIndex1, packet->itemClassification1};
        shopClothTextReplacements[2] = {packet->gameIndex2, packet->playerIndex2, packet->itemIndex2, packet->itemClassification2};
        shopClothTextReplacements[3] = {packet->gameIndex3, packet->playerIndex3, packet->itemIndex3, packet->itemClassification3};
        shopClothTextReplacements[4] = {packet->gameIndex4, packet->playerIndex4, packet->itemIndex4, packet->itemClassification4};
        shopClothTextReplacements[5] = {packet->gameIndex5, packet->playerIndex5, packet->itemIndex5, packet->itemClassification5};
        shopClothTextReplacements[6] = {packet->gameIndex6, packet->playerIndex6, packet->itemIndex6, packet->itemClassification6};
        shopClothTextReplacements[7] = {packet->gameIndex7, packet->playerIndex7, packet->itemIndex7, packet->itemClassification7};
        shopClothTextReplacements[8] = {packet->gameIndex8, packet->playerIndex8, packet->itemIndex8, packet->itemClassification8};
        shopClothTextReplacements[9] = {packet->gameIndex9, packet->playerIndex9, packet->itemIndex9, packet->itemClassification9};
        shopClothTextReplacements[10] = {packet->gameIndex10, packet->playerIndex10, packet->itemIndex10, packet->itemClassification10};
        shopClothTextReplacements[11] = {packet->gameIndex11, packet->playerIndex11, packet->itemIndex11, packet->itemClassification11};
        shopClothTextReplacements[12] = {packet->gameIndex12, packet->playerIndex12, packet->itemIndex12, packet->itemClassification12};
        shopClothTextReplacements[13] = {packet->gameIndex13, packet->playerIndex13, packet->itemIndex13, packet->itemClassification13};
        shopClothTextReplacements[14] = {packet->gameIndex14, packet->playerIndex14, packet->itemIndex14, packet->itemClassification14};
        shopClothTextReplacements[15] = {packet->gameIndex15, packet->playerIndex15, packet->itemIndex15, packet->itemClassification15};
        shopClothTextReplacements[16] = {packet->gameIndex16, packet->playerIndex16, packet->itemIndex16, packet->itemClassification16};
        shopClothTextReplacements[17] = {packet->gameIndex17, packet->playerIndex17, packet->itemIndex17, packet->itemClassification17};
        shopClothTextReplacements[18] = {packet->gameIndex18, packet->playerIndex18, packet->itemIndex18, packet->itemClassification18};
        shopClothTextReplacements[19] = {packet->gameIndex19, packet->playerIndex19, packet->itemIndex19, packet->itemClassification19};
        shopClothTextReplacements[20] = {packet->gameIndex20, packet->playerIndex20, packet->itemIndex20, packet->itemClassification20};
        shopClothTextReplacements[21] = {packet->gameIndex21, packet->playerIndex21, packet->itemIndex21, packet->itemClassification21};
        shopClothTextReplacements[22] = {packet->gameIndex22, packet->playerIndex22, packet->itemIndex22, packet->itemClassification22};
        shopClothTextReplacements[23] = {packet->gameIndex23, packet->playerIndex23, packet->itemIndex23, packet->itemClassification23};
        shopClothTextReplacements[24] = {packet->gameIndex24, packet->playerIndex24, packet->itemIndex24, packet->itemClassification24};
        shopClothTextReplacements[25] = {packet->gameIndex25, packet->playerIndex25, packet->itemIndex25, packet->itemClassification25};
        shopClothTextReplacements[26] = {packet->gameIndex26, packet->playerIndex26, packet->itemIndex26, packet->itemClassification26};
        shopClothTextReplacements[27] = {packet->gameIndex27, packet->playerIndex27, packet->itemIndex27, packet->itemClassification27};
        shopClothTextReplacements[28] = {packet->gameIndex28, packet->playerIndex28, packet->itemIndex28, packet->itemClassification28};
        shopClothTextReplacements[29] = {packet->gameIndex29, packet->playerIndex29, packet->itemIndex29, packet->itemClassification29};
        shopClothTextReplacements[30] = {packet->gameIndex30, packet->playerIndex30, packet->itemIndex30, packet->itemClassification30};
        shopClothTextReplacements[31] = {packet->gameIndex31, packet->playerIndex31, packet->itemIndex31, packet->itemClassification31};
        shopClothTextReplacements[32] = {packet->gameIndex32, packet->playerIndex32, packet->itemIndex32, packet->itemClassification32};
        shopClothTextReplacements[33] = {packet->gameIndex33, packet->playerIndex33, packet->itemIndex33, packet->itemClassification33};
        shopClothTextReplacements[34] = {packet->gameIndex34, packet->playerIndex34, packet->itemIndex34, packet->itemClassification34};
        shopClothTextReplacements[35] = {packet->gameIndex35, packet->playerIndex35, packet->itemIndex35, packet->itemClassification35};
        shopClothTextReplacements[36] = {packet->gameIndex36, packet->playerIndex36, packet->itemIndex36, packet->itemClassification36};
        shopClothTextReplacements[37] = {packet->gameIndex37, packet->playerIndex37, packet->itemIndex37, packet->itemClassification37};
        shopClothTextReplacements[38] = {packet->gameIndex38, packet->playerIndex38, packet->itemIndex38, packet->itemClassification38};
        shopClothTextReplacements[39] = {packet->gameIndex39, packet->playerIndex39, packet->itemIndex39, packet->itemClassification39};
        shopClothTextReplacements[40] = {packet->gameIndex40, packet->playerIndex40, packet->itemIndex40, packet->itemClassification40};
        shopClothTextReplacements[41] = {packet->gameIndex41, packet->playerIndex41, packet->itemIndex41, packet->itemClassification41};
        shopClothTextReplacements[42] = {packet->gameIndex42, packet->playerIndex42, packet->itemIndex42, packet->itemClassification42};
        shopClothTextReplacements[43] = {packet->gameIndex43, packet->playerIndex43, packet->itemIndex43, packet->itemClassification43};
    }
    // Sticker
    if (type == 2) {
        shopStickerTextReplacements[0] = {packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, packet->itemClassification0};
        shopStickerTextReplacements[1] = {packet->gameIndex1, packet->playerIndex1, packet->itemIndex1, packet->itemClassification1};
        shopStickerTextReplacements[2] = {packet->gameIndex2, packet->playerIndex2, packet->itemIndex2, packet->itemClassification2};
        shopStickerTextReplacements[3] = {packet->gameIndex3, packet->playerIndex3, packet->itemIndex3, packet->itemClassification3};
        shopStickerTextReplacements[4] = {packet->gameIndex4, packet->playerIndex4, packet->itemIndex4, packet->itemClassification4};
        shopStickerTextReplacements[5] = {packet->gameIndex5, packet->playerIndex5, packet->itemIndex5, packet->itemClassification5};
        shopStickerTextReplacements[6] = {packet->gameIndex6, packet->playerIndex6, packet->itemIndex6, packet->itemClassification6};
        shopStickerTextReplacements[7] = {packet->gameIndex7, packet->playerIndex7, packet->itemIndex7, packet->itemClassification7};
        shopStickerTextReplacements[8] = {packet->gameIndex8, packet->playerIndex8, packet->itemIndex8, packet->itemClassification8};
        shopStickerTextReplacements[9] = {packet->gameIndex9, packet->playerIndex9, packet->itemIndex9, packet->itemClassification9};
        shopStickerTextReplacements[10] = {packet->gameIndex10, packet->playerIndex10, packet->itemIndex10, packet->itemClassification10};
        shopStickerTextReplacements[11] = {packet->gameIndex11, packet->playerIndex11, packet->itemIndex11, packet->itemClassification11};
        shopStickerTextReplacements[12] = {packet->gameIndex12, packet->playerIndex12, packet->itemIndex12, packet->itemClassification12};
        shopStickerTextReplacements[13] = {packet->gameIndex13, packet->playerIndex13, packet->itemIndex13, packet->itemClassification13};
        shopStickerTextReplacements[14] = {packet->gameIndex14, packet->playerIndex14, packet->itemIndex14, packet->itemClassification14};
        shopStickerTextReplacements[15] = {packet->gameIndex15, packet->playerIndex15, packet->itemIndex15, packet->itemClassification15};
        shopStickerTextReplacements[16] = {packet->gameIndex16, packet->playerIndex16, packet->itemIndex16, packet->itemClassification16};
    }
    // Gift
    if (type == 3) {
        shopGiftTextReplacements[0] = {packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, packet->itemClassification0};
        shopGiftTextReplacements[1] = {packet->gameIndex1, packet->playerIndex1, packet->itemIndex1, packet->itemClassification1};
        shopGiftTextReplacements[2] = {packet->gameIndex2, packet->playerIndex2, packet->itemIndex2, packet->itemClassification2};
        shopGiftTextReplacements[3] = {packet->gameIndex3, packet->playerIndex3, packet->itemIndex3, packet->itemClassification3};
        shopGiftTextReplacements[4] = {packet->gameIndex4, packet->playerIndex4, packet->itemIndex4, packet->itemClassification4};
        shopGiftTextReplacements[5] = {packet->gameIndex5, packet->playerIndex5, packet->itemIndex5, packet->itemClassification5};
        shopGiftTextReplacements[6] = {packet->gameIndex6, packet->playerIndex6, packet->itemIndex6, packet->itemClassification6};
        shopGiftTextReplacements[7] = {packet->gameIndex7, packet->playerIndex7, packet->itemIndex7, packet->itemClassification7};
        shopGiftTextReplacements[8] = {packet->gameIndex8, packet->playerIndex8, packet->itemIndex8, packet->itemClassification8};
        shopGiftTextReplacements[9] = {packet->gameIndex9, packet->playerIndex9, packet->itemIndex9, packet->itemClassification9};
        shopGiftTextReplacements[10] = {packet->gameIndex10, packet->playerIndex10, packet->itemIndex10, packet->itemClassification10};
        shopGiftTextReplacements[11] = {packet->gameIndex11, packet->playerIndex11, packet->itemIndex11, packet->itemClassification11};
        shopGiftTextReplacements[12] = {packet->gameIndex12, packet->playerIndex12, packet->itemIndex12, packet->itemClassification12};
        shopGiftTextReplacements[13] = {packet->gameIndex13, packet->playerIndex13, packet->itemIndex13, packet->itemClassification13};
        shopGiftTextReplacements[14] = {packet->gameIndex14, packet->playerIndex14, packet->itemIndex14, packet->itemClassification14};
        shopGiftTextReplacements[15] = {packet->gameIndex15, packet->playerIndex15, packet->itemIndex15, packet->itemClassification15};
        shopGiftTextReplacements[16] = {packet->gameIndex16, packet->playerIndex16, packet->itemIndex16, packet->itemClassification16};
        shopGiftTextReplacements[17] = {packet->gameIndex17, packet->playerIndex17, packet->itemIndex17, packet->itemClassification17};
        shopGiftTextReplacements[18] = {packet->gameIndex18, packet->playerIndex18, packet->itemIndex18, packet->itemClassification18};
        shopGiftTextReplacements[19] = {packet->gameIndex19, packet->playerIndex19, packet->itemIndex19, packet->itemClassification19};
        shopGiftTextReplacements[20] = {packet->gameIndex20, packet->playerIndex20, packet->itemIndex20, packet->itemClassification20};
        shopGiftTextReplacements[21] = {packet->gameIndex21, packet->playerIndex21, packet->itemIndex21, packet->itemClassification21};
        shopGiftTextReplacements[22] = {packet->gameIndex22, packet->playerIndex22, packet->itemIndex22, packet->itemClassification22};
        shopGiftTextReplacements[23] = {packet->gameIndex23, packet->playerIndex23, packet->itemIndex23, packet->itemClassification23};
        shopGiftTextReplacements[24] = {packet->gameIndex24, packet->playerIndex24, packet->itemIndex24, packet->itemClassification24};
        shopGiftTextReplacements[25] = {packet->gameIndex25, packet->playerIndex25, packet->itemIndex25, packet->itemClassification25};
    }
    // Moon
    if (type == 4) {
        shopMoonTextReplacements[0] = {packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, packet->itemClassification0};
        shopMoonTextReplacements[1] = {packet->gameIndex1, packet->playerIndex1, packet->itemIndex1, packet->itemClassification1};
        shopMoonTextReplacements[2] = {packet->gameIndex2, packet->playerIndex2, packet->itemIndex2, packet->itemClassification2};
        shopMoonTextReplacements[3] = {packet->gameIndex3, packet->playerIndex3, packet->itemIndex3, packet->itemClassification3};
        shopMoonTextReplacements[4] = {packet->gameIndex4, packet->playerIndex4, packet->itemIndex4, packet->itemClassification4};
        shopMoonTextReplacements[5] = {packet->gameIndex5, packet->playerIndex5, packet->itemIndex5, packet->itemClassification5};
        shopMoonTextReplacements[6] = {packet->gameIndex6, packet->playerIndex6, packet->itemIndex6, packet->itemClassification6};
        shopMoonTextReplacements[7] = {packet->gameIndex7, packet->playerIndex7, packet->itemIndex7, packet->itemClassification7};
        shopMoonTextReplacements[8] = {packet->gameIndex8, packet->playerIndex8, packet->itemIndex8, packet->itemClassification8};
        shopMoonTextReplacements[9] = {packet->gameIndex9, packet->playerIndex9, packet->itemIndex9, packet->itemClassification9};
        shopMoonTextReplacements[10] = {packet->gameIndex10, packet->playerIndex10, packet->itemIndex10, packet->itemClassification10};
        shopMoonTextReplacements[11] = {packet->gameIndex11, packet->playerIndex11, packet->itemIndex11, packet->itemClassification11};
        shopMoonTextReplacements[12] = {packet->gameIndex12, packet->playerIndex12, packet->itemIndex12, packet->itemClassification12};
    }
}

const char* ArchipelagoMode::getShineReplacementText() {
    GameDataHolderAccessor accessor(mCurScene);

    Shine* curShine = mRecentShine;

    GameDataFile::HintInfo* info = &accessor.mData->mPlayingFile->getHintList()[curShine->mShineIdx];

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
        return shineItemNames[curReplaceText.shineItemNameIndex].cstr();
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
        // if (apGameNames[curItem.gameIndex].isEmpty()) {
        message.append(apGameNames[curItem.gameIndex].cstr());
        //} else {
        // message.append(u"Missing Game");
        // }

        message.append(u".\nSeems to belong to ");
        // if (apSlotNames[curItem.slotIndex].isEmpty()) {
        message.append(apSlotNames[curItem.slotIndex].cstr());
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
        // if (apSlotNames[curItem.slotIndex].isEmpty()) {
        message.append(apItemNames[curItem.apItemNameIndex].cstr());
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

void ArchipelagoMode::updateSlotData(SlotData* packet) {
    mWorldPayCounts[1] = packet->cascade;
    mWorldPayCounts[2] = packet->sand;
    mWorldPayCounts[3] = packet->wooded;
    mWorldPayCounts[4] = packet->lake;
    mWorldPayCounts[6] = packet->lost;
    mWorldPayCounts[7] = packet->metro;
    mWorldPayCounts[8] = packet->seaside;
    mWorldPayCounts[9] = packet->snow;
    mWorldPayCounts[10] = packet->luncheon;
    mWorldPayCounts[11] = packet->ruined;
    mWorldPayCounts[12] = packet->bowser;
    mWorldPayCounts[15] = packet->dark;
    mWorldPayCounts[16] = packet->darker;
    mRegionalsEnabled = packet->regionals;
    mCapturesEnabled = packet->captures;

    numApGames = 0;
    numApSlots = 0;
    numApItems = 0;
}

void ArchipelagoMode::updateWorlds(UnlockWorld* packet) {
    GameDataHolderWriter writer(mCurScene);
    GameDataFunction::unlockWorld(writer, packet->worldID);
}

void ArchipelagoMode::sendCheckPacket(int locationId, CheckType itemType) {
    Client::sendCheckPacket(locationId, itemType);
}

void ArchipelagoMode::sendCheckPacket(CheckType itemType, const char* objId, const char* stageName) {
    Client::sendCheckPacket(itemType, objId, stageName);
}

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
            sendDeathlinkPacket();
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
        if (!(al::isEqualString(GameDataFunction::tryGetCurrentMainStageName(accessor), "CapWorldHomeStage") && Client::getScenario(0) < 2) &&
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
    ImGui::Text("- L + ← | Enable/disable Freeze Tag [FT]\n");
    ImGui::Text("- [FT] ↑ | Switch between runners and chasers\n");
    ImGui::Text("- [FT] L + ↓ | Reset score\n");

    if (mInfo->mIsDebugMode) {
        ImGui::Text("- [FT][Debug] A + → | Increment score\n");
        ImGui::Text("- [FT][Debug] A + ← | Set time to 01:05\n");
        ImGui::Text("- [FT][Debug] B + → | Wipeout\n");
    }
}