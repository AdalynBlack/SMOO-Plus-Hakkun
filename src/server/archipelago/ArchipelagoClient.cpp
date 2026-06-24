#include "hk/types.h"

#include "nn/os.h"
#include "nn/socket.h"

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"

#include "game/Item/ShineInfo.h"
#include "game/Player/HackCap.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Sequence/ChangeStageInfo.h"
#include "game/System/CustomGameDataFunction.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/SaveDataAccessFunction.h"
#include "game/Util/ActorDimensionKeeper.h"

#include <cmath>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

#include "heap/seadHeapMgr.h"
#include "helpers.hpp"
#include "Library/Base/StringUtil.h"
#include "Library/LiveActor/LiveActor.h"
#include "logger.hpp"
#include "packets/MessagePacket.h"
#include "packets/Packet.h"
#include "packets/PlayerDC.h"
#include "server/archipelago/ArchipelagoHelpers.hpp"
#include "server/archipelago/ArchipelagoMode.hpp"
#include "server/Client.hpp"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "server/snh/SardineMode.hpp"
#include "server/SocketClient.hpp"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"
#include "System/WorldList.h"
#include "thread/seadMessageQueue.h"
#include "types.h"
#include "Util/AchievementUtil.h"

// ===== Setters / Getters =====
/**
 * @brief sets server IP to supplied string, used specifically for loading IP from the save file.
 *
 * @param ip
 */
void Client::setApClientIP(const char* ip) {
    if (sInstance) {
        sInstance->mApClientIP = ip;
    }
}

/**
 * @brief
 *
 * @return const char*
 */
const char* Client::getApClientIP() {
    if (sInstance) {
        return sInstance->mApClientIP.cstr();
    }
    return nullptr;
}

/**
 * @brief sets Archipelago Host Name to supplied string, used specifically for loading from the save file.
 *
 * @param ip
 */
void Client::setArchipelagoHost(const char* host) {
    if (sInstance) {
        sInstance->mArchipelagoHost = host;
    }
}

/**
 * @brief
 *
 * @return const char*
 */
const char* Client::getArchipelagoHost() {
    if (sInstance) {
        return sInstance->mArchipelagoHost.cstr();
    }
    return nullptr;
}

/**
 * @brief sets Archipelago Port to supplied ushort, used specifically for loading from the save file.
 *
 * @param port
 */
void Client::setArchipelagoPort(ushort port) {
    if (sInstance) {
        sInstance->mArchipelagoPort = port;
    }
}

/**
 * @brief
 *
 * @return ushort
 */
ushort Client::getArchipelagoPort() {
    if (sInstance) {
        return sInstance->mArchipelagoPort;
    }
    return 0;
}

/**
 * @brief sets Archipelago slot to supplied string, used specifically for loading from the save file.
 *
 * @param slot
 */
void Client::setArchipelagoSlot(const char* slot) {
    if (sInstance) {
        sInstance->mArchipelagoSlot = slot;
    }
}

/**
 * @brief
 *
 * @return const char*
 */
const char* Client::getArchipelagoSlot() {
    if (sInstance) {
        return sInstance->mArchipelagoSlot.cstr();
    }
    return nullptr;
}

/**
 * @brief sets Archipelago password to supplied string, used specifically for loading from the save file.
 *
 * @param ip
 */
void Client::setArchipelagoPassword(const char* password) {
    if (sInstance) {
        sInstance->mArchipelagoPassword = password;
    }
}

/**
 * @brief
 *
 * @return const char*
 */
const char* Client::getArchipelagoPassword() {
    if (sInstance) {
        return sInstance->mArchipelagoPassword.cstr();
    }
    return nullptr;
}

// ===== Packet Senders =====

void Client::sendArchipelagoConnectPacket() {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    ArchipelagoConnect* packet = new ArchipelagoConnect();
    packet->mUserID = sInstance->mUserID;

    strcpy(packet->hostName, sInstance->mArchipelagoHost.cstr());
    packet->port = sInstance->mArchipelagoPort;
    strcpy(packet->slotName, sInstance->mArchipelagoSlot.cstr());
    strcpy(packet->password, sInstance->mArchipelagoPassword.cstr());

    sInstance->mSocket->queuePacket(packet);
}

/**
 * @brief
 *
 * @param itemName
 */
void Client::sendDeathlinkPacket() {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    Deathlink* packet = new Deathlink();
    packet->mUserID = sInstance->mUserID;

    sInstance->mSocket->queuePacket(packet);
}

void Client::sendChangeStagePacket(GameDataHolderAccessor accessor) {
    if (!sInstance) {
        Logger::log("Client Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    ChangeStagePacket* packet = new ChangeStagePacket();
    int worldId = accessor->getWorldList()->tryFindWorldIndexByStageName(GameDataFunction::getCurrentStageName(accessor));
    strcpy(packet->changeStage, GameDataFunction::getMainStageName(accessor, worldId));

    sInstance->mSocket->queuePacket(packet);
}

void Client::sendCheckPacket(int locationId, int itemType) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    Check* packet = new Check();
    packet->locationId = locationId;
    packet->itemType = itemType;

    sInstance->mSocket->queuePacket(packet);
}

void Client::sendCheckPacket(int itemType, const char* objId, const char* stageName) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    Check* packet = new Check();
    packet->itemType = itemType;
    strcpy(packet->objId, objId);
    strcpy(packet->stage, stageName);

    sInstance->mSocket->queuePacket(packet);
}

// ===== Packet Handlers =====
void Client::receiveCheck(Check* packet) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
    int itemType = packet->itemType;
    archipelago->setCappySlotName(packet->senderName);
    cappyMessage gotItemMessage = {255, 255, 255, false};

    // struct ShopItem::ShopAmiiboInfo amiiboData = {1, 1};
    struct ShopItem::ItemInfo info = {};
    info.index = 1;
    info.type = static_cast<ShopItem::ItemType>(itemType);
    // sead::Buffer<ShopItem::ShopAmiiboInfo> amiiboBuffer({1,1});
    // info.amiiboInfoList = amiiboBuffer;
    info.isAOC = true;

    struct ShopItem::ItemInfo* infoPtr;
    GameDataHolderWriter writer(sInstance->mCurStageScene);
    bool updateIndex = false;
    sead::FixedSafeString<40> indexMessage;
    indexMessage = "";
    indexMessage.append("Received item index ");
    indexMessage.append(intToCstr(packet->index));
    // setMessage(1, indexMessage.cstr());
    indexMessage = "";
    indexMessage.append("Current item index ");
    indexMessage.append(intToCstr(GameModeManager::instance()->getMode<ArchipelagoMode>()->getCheckIndex()));
    // setMessage(2, indexMessage.cstr());
    // Error Handling
    sead::FixedSafeString<128> recCheck = sead::FixedSafeString<128>();

    switch (itemType) {
    case CheckType::Coins:
        // setMessage(3, "Coins Received");
        if (archipelago->getCheckIndex() < packet->index) {
            GameDataFunction::addCoin(writer, packet->amount);
            updateIndex = true;
        }
        break;

    case CheckType::Moon:
        if (collectedShineCount < curCollectedShines.size() - 1) {
            curCollectedShines[collectedShineCount] = packet->locationId;
            collectedShineCount++;
            archipelago->setIsNeedUpdateCounter(true);
        }
        gotItemMessage.itemType = packet->amount > 15 ? CappyMessageTypes::CappyMultiMoon : CappyMessageTypes::CappyMoon;
        gotItemMessage.itemIndex = packet->amount % 17;
        break;

    case CheckType::Clothes:
        strcpy(info.name, costumeNamesByCheckId[packet->locationId]);
        info.type = static_cast<ShopItem::ItemType>(itemType);
        infoPtr = &info;
        writer.mData->mPlayingFile->buyItem(infoPtr, false);
        if (archipelago->getCheckIndex() < packet->index) {
            GameDataFunction::wearCostume(writer, info.name);
            updateIndex = true;
        }
        archipelago->setCappyItemName(packet->objId);
        gotItemMessage.itemType = CappyMessageTypes::CappyClothes;
        gotItemMessage.itemIndex = packet->locationId;
        break;

    case CheckType::Cap:
        strcpy(info.name, costumeNamesByCheckId[packet->locationId]);
        info.type = static_cast<ShopItem::ItemType>(itemType);
        infoPtr = &info;
        writer.mData->mPlayingFile->buyItem(infoPtr, false);
        if (archipelago->getCheckIndex() < packet->index) {
            GameDataFunction::wearCap(writer, info.name);
            updateIndex = true;
        }
        archipelago->setCappyItemName(packet->objId);
        gotItemMessage.itemType = CappyMessageTypes::CappyCap;
        gotItemMessage.itemIndex = packet->locationId;
        break;

    case CheckType::Souvenir:
        strcpy(info.name, souvenirNames[packet->locationId]);
        info.type = static_cast<ShopItem::ItemType>(itemType);
        infoPtr = &info;
        writer.mData->mPlayingFile->buyItem(infoPtr, false);
        archipelago->setCappyItemName(packet->objId);
        gotItemMessage.itemType = CappyMessageTypes::CappySouvenir;
        gotItemMessage.itemIndex = packet->locationId;
        break;

    case CheckType::Sticker:
        strcpy(info.name, stickerNames[packet->locationId]);
        info.type = static_cast<ShopItem::ItemType>(itemType);
        infoPtr = &info;
        writer.mData->mPlayingFile->buyItem(infoPtr, false);
        archipelago->setCappyItemName(packet->objId);
        gotItemMessage.itemType = CappyMessageTypes::CappySticker;
        gotItemMessage.itemIndex = packet->locationId;
        break;

    case CheckType::RegionalCoin:
        if (!sInstance->mCurStageScene && sInstance->mPendingCoinCollectCount < sMaxPendingCoinCollects) {
            PendingCoinCollect& pending = sInstance->mPendingCoinCollects[sInstance->mPendingCoinCollectCount++];
            strcpy(pending.placeID, packet->objId);
            pending.worldID = packet->amount;
            strcpy(pending.stage, packet->stage);
        } else {
            const al::PlacementId placementId(packet->objId, nullptr, nullptr);
            writer.mData->getGameDataFile()->customAddCoinCollect(&placementId, packet->amount, packet->stage);
            sead::FixedSafeString<128> recCoin = sead::FixedSafeString<128>();
            recCoin = "Received Coin at ";
            recCoin.append(packet->objId);
            recCoin.append(", ");
            recCoin.append(intToCstr(packet->amount));
            recCoin.append(", ");
            recCoin.append(packet->stage);
            // addMessage(recCoin.cstr());
        }
        gotItemMessage.itemType = CappyMessageTypes::CappyRegionalCoin;
        gotItemMessage.itemIndex = packet->amount;
        break;

    case CheckType::Capture:
        archipelago->addCapture(captureListNames[packet->locationId]);
        GameDataFunction::addHackDictionary(writer, captureListNames[packet->locationId]);
        recCheck = "Received Capture ";
        recCheck.append(captureListNames[packet->locationId]);
        // addMessage(recCheck.cstr());
        archipelago->setCappyItemName(packet->objId);
        gotItemMessage.itemType = CappyMessageTypes::CappyCapture;
        gotItemMessage.itemIndex = packet->locationId;
        break;

    default:
        recCheck = "Received Invalid Check Type ";
        recCheck.append(intToCstr(itemType));
        addMessage(recCheck.cstr());
        break;
    }

    if (updateIndex) {
        archipelago->setCheckIndex(packet->index);
    }

    archipelago->enqueueCappyMessage(gotItemMessage);
}

void Client::receiveDeath(Deathlink* packet) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        ArchipelagoMode* ap = GameModeManager::instance()->getMode<ArchipelagoMode>();
        ap->setApDeath(true);
        ap->setDying(true);
    }
}

void Client::updateSlotData(SlotData* packet) {
    ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
    ArchipelagoInfo* archipelagoInfo = GameModeManager::instance()->getInfo<ArchipelagoInfo>();
    if (archipelagoInfo) {
        archipelagoInfo->mIsClientConnected = ArchipelagoState::CLIENT_CONNECTED;
        archipelago->setConnectInitFlag(true);
        archipelago->setCappySlotName(getArchipelagoSlot());
        archipelago->enqueueCappyMessage({255, CappyMessageTypes::CappyConnect, 0, false});
    } else
        return;
    archipelago->setWorldUnlockCount(1, packet->cascade);
    archipelago->setWorldUnlockCount(2, packet->sand);
    archipelago->setWorldUnlockCount(3, packet->wooded);
    archipelago->setWorldUnlockCount(4, packet->lake);
    archipelago->setWorldUnlockCount(6, packet->lost);
    archipelago->setWorldUnlockCount(7, packet->metro);
    archipelago->setWorldUnlockCount(8, packet->seaside);
    archipelago->setWorldUnlockCount(9, packet->snow);
    archipelago->setWorldUnlockCount(10, packet->luncheon);
    archipelago->setWorldUnlockCount(11, packet->ruined);
    archipelago->setWorldUnlockCount(12, packet->bowser);
    archipelago->setWorldUnlockCount(15, packet->dark);
    archipelago->setWorldUnlockCount(16, packet->darker);
    archipelago->setGoal(packet->goal);
    archipelago->setDeathLinkFlag(packet->deathLink);
    archipelago->setCapturesFlag(packet->captures);
    archipelago->setERFlag(packet->entranceRandomizer);
}

void Client::updateSentChecks(SentChecks* packet) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        if (packet->checkType == CheckType::Moon) {
            archipelago->addShine(packet->shineUid0);
            archipelago->addShine(packet->shineUid1);
            archipelago->addShine(packet->shineUid2);
            archipelago->addShine(packet->shineUid3);
            archipelago->addShine(packet->shineUid4);
            archipelago->addShine(packet->shineUid5);
            archipelago->addShine(packet->shineUid6);
            archipelago->addShine(packet->shineUid7);
            archipelago->addShine(packet->shineUid8);
            archipelago->addShine(packet->shineUid9);
            archipelago->addShine(packet->shineUid10);
            archipelago->addShine(packet->shineUid11);
            archipelago->addShine(packet->shineUid12);
            archipelago->addShine(packet->shineUid13);
            archipelago->addShine(packet->shineUid14);
            archipelago->addShine(packet->shineUid15);
            archipelago->addShine(packet->shineUid16);
            archipelago->addShine(packet->shineUid17);
            archipelago->addShine(packet->shineUid18);
            archipelago->addShine(packet->shineUid19);
            archipelago->addShine(packet->shineUid20);
            archipelago->addShine(packet->shineUid21);
            archipelago->addShine(packet->shineUid22);
            archipelago->addShine(packet->shineUid23);
            archipelago->addShine(packet->shineUid24);
            archipelago->addShine(packet->shineUid25);
            archipelago->addShine(packet->shineUid26);
            archipelago->addShine(packet->shineUid27);
            archipelago->addShine(packet->shineUid28);
            archipelago->addShine(packet->shineUid29);
            archipelago->addShine(packet->shineUid30);
            archipelago->addShine(packet->shineUid31);
            archipelago->addShine(packet->shineUid32);
            archipelago->addShine(packet->shineUid33);
            archipelago->addShine(packet->shineUid34);
            archipelago->addShine(packet->shineUid35);
            archipelago->addShine(packet->shineUid36);
            archipelago->addShine(packet->shineUid37);
            archipelago->addShine(packet->shineUid38);
            archipelago->addShine(packet->shineUid39);
            archipelago->addShine(packet->shineUid40);
            archipelago->addShine(packet->shineUid41);
            archipelago->addShine(packet->shineUid42);
            archipelago->addShine(packet->shineUid43);
            archipelago->addShine(packet->shineUid44);
            archipelago->addShine(packet->shineUid45);
            archipelago->addShine(packet->shineUid46);
            archipelago->addShine(packet->shineUid47);
            archipelago->addShine(packet->shineUid48);
            archipelago->addShine(packet->shineUid49);
            archipelago->addShine(packet->shineUid50);
            archipelago->addShine(packet->shineUid51);
            archipelago->addShine(packet->shineUid52);
            archipelago->addShine(packet->shineUid53);
            archipelago->addShine(packet->shineUid54);
            archipelago->addShine(packet->shineUid55);
            archipelago->addShine(packet->shineUid56);
            archipelago->addShine(packet->shineUid57);
            archipelago->addShine(packet->shineUid58);
            archipelago->addShine(packet->shineUid59);
            archipelago->addShine(packet->shineUid60);
            archipelago->addShine(packet->shineUid61);
            archipelago->addShine(packet->shineUid62);
            archipelago->addShine(packet->shineUid63);
            archipelago->addShine(packet->shineUid64);
            archipelago->addShine(packet->shineUid65);
            archipelago->addShine(packet->shineUid66);
            archipelago->addShine(packet->shineUid67);
            archipelago->addShine(packet->shineUid68);
            archipelago->addShine(packet->shineUid69);
            archipelago->addShine(packet->shineUid70);
            archipelago->addShine(packet->shineUid71);
            archipelago->addShine(packet->shineUid72);
            archipelago->addShine(packet->shineUid73);
            archipelago->addShine(packet->shineUid74);
            archipelago->addShine(packet->shineUid75);
            archipelago->addShine(packet->shineUid76);
            archipelago->addShine(packet->shineUid77);
            archipelago->addShine(packet->shineUid78);
            archipelago->addShine(packet->shineUid79);
            archipelago->addShine(packet->shineUid80);
            archipelago->addShine(packet->shineUid81);
            archipelago->addShine(packet->shineUid82);
            archipelago->addShine(packet->shineUid83);
            archipelago->addShine(packet->shineUid84);
            archipelago->addShine(packet->shineUid85);
            archipelago->addShine(packet->shineUid86);
            archipelago->addShine(packet->shineUid87);
            archipelago->addShine(packet->shineUid88);
            archipelago->addShine(packet->shineUid89);
            archipelago->addShine(packet->shineUid90);
            archipelago->addShine(packet->shineUid91);
            archipelago->addShine(packet->shineUid92);
            archipelago->addShine(packet->shineUid93);
            archipelago->addShine(packet->shineUid94);
            archipelago->addShine(packet->shineUid95);
            archipelago->addShine(packet->shineUid96);
            archipelago->addShine(packet->shineUid97);
            archipelago->addShine(packet->shineUid98);
            archipelago->addShine(packet->shineUid99);
            archipelago->calculateShineScenarios();
        }
        if (packet->checkType == CheckType::RegionalCoin) {
            archipelago->addRegionalCoin(packet->shineUid0);
            archipelago->addRegionalCoin(packet->shineUid1);
            archipelago->addRegionalCoin(packet->shineUid2);
            archipelago->addRegionalCoin(packet->shineUid3);
            archipelago->addRegionalCoin(packet->shineUid4);
            archipelago->addRegionalCoin(packet->shineUid5);
            archipelago->addRegionalCoin(packet->shineUid6);
            archipelago->addRegionalCoin(packet->shineUid7);
            archipelago->addRegionalCoin(packet->shineUid8);
            archipelago->addRegionalCoin(packet->shineUid9);
            archipelago->addRegionalCoin(packet->shineUid10);
            archipelago->addRegionalCoin(packet->shineUid11);
            archipelago->addRegionalCoin(packet->shineUid12);
            archipelago->addRegionalCoin(packet->shineUid13);
            archipelago->addRegionalCoin(packet->shineUid14);
            archipelago->addRegionalCoin(packet->shineUid15);
            archipelago->addRegionalCoin(packet->shineUid16);
            archipelago->addRegionalCoin(packet->shineUid17);
            archipelago->addRegionalCoin(packet->shineUid18);
            archipelago->addRegionalCoin(packet->shineUid19);
            archipelago->addRegionalCoin(packet->shineUid20);
            archipelago->addRegionalCoin(packet->shineUid21);
            archipelago->addRegionalCoin(packet->shineUid22);
            archipelago->addRegionalCoin(packet->shineUid23);
            archipelago->addRegionalCoin(packet->shineUid24);
            archipelago->addRegionalCoin(packet->shineUid25);
            archipelago->addRegionalCoin(packet->shineUid26);
            archipelago->addRegionalCoin(packet->shineUid27);
            archipelago->addRegionalCoin(packet->shineUid28);
            archipelago->addRegionalCoin(packet->shineUid29);
            archipelago->addRegionalCoin(packet->shineUid30);
            archipelago->addRegionalCoin(packet->shineUid31);
            archipelago->addRegionalCoin(packet->shineUid32);
            archipelago->addRegionalCoin(packet->shineUid33);
            archipelago->addRegionalCoin(packet->shineUid34);
            archipelago->addRegionalCoin(packet->shineUid35);
            archipelago->addRegionalCoin(packet->shineUid36);
            archipelago->addRegionalCoin(packet->shineUid37);
            archipelago->addRegionalCoin(packet->shineUid38);
            archipelago->addRegionalCoin(packet->shineUid39);
            archipelago->addRegionalCoin(packet->shineUid40);
            archipelago->addRegionalCoin(packet->shineUid41);
            archipelago->addRegionalCoin(packet->shineUid42);
            archipelago->addRegionalCoin(packet->shineUid43);
            archipelago->addRegionalCoin(packet->shineUid44);
            archipelago->addRegionalCoin(packet->shineUid45);
            archipelago->addRegionalCoin(packet->shineUid46);
            archipelago->addRegionalCoin(packet->shineUid47);
            archipelago->addRegionalCoin(packet->shineUid48);
            archipelago->addRegionalCoin(packet->shineUid49);
            archipelago->addRegionalCoin(packet->shineUid50);
            archipelago->addRegionalCoin(packet->shineUid51);
            archipelago->addRegionalCoin(packet->shineUid52);
            archipelago->addRegionalCoin(packet->shineUid53);
            archipelago->addRegionalCoin(packet->shineUid54);
            archipelago->addRegionalCoin(packet->shineUid55);
            archipelago->addRegionalCoin(packet->shineUid56);
            archipelago->addRegionalCoin(packet->shineUid57);
            archipelago->addRegionalCoin(packet->shineUid58);
            archipelago->addRegionalCoin(packet->shineUid59);
            archipelago->addRegionalCoin(packet->shineUid60);
            archipelago->addRegionalCoin(packet->shineUid61);
            archipelago->addRegionalCoin(packet->shineUid62);
            archipelago->addRegionalCoin(packet->shineUid63);
            archipelago->addRegionalCoin(packet->shineUid64);
            archipelago->addRegionalCoin(packet->shineUid65);
            archipelago->addRegionalCoin(packet->shineUid66);
            archipelago->addRegionalCoin(packet->shineUid67);
            archipelago->addRegionalCoin(packet->shineUid68);
            archipelago->addRegionalCoin(packet->shineUid69);
            archipelago->addRegionalCoin(packet->shineUid70);
            archipelago->addRegionalCoin(packet->shineUid71);
            archipelago->addRegionalCoin(packet->shineUid72);
            archipelago->addRegionalCoin(packet->shineUid73);
            archipelago->addRegionalCoin(packet->shineUid74);
            archipelago->addRegionalCoin(packet->shineUid75);
            archipelago->addRegionalCoin(packet->shineUid76);
            archipelago->addRegionalCoin(packet->shineUid77);
            archipelago->addRegionalCoin(packet->shineUid78);
            archipelago->addRegionalCoin(packet->shineUid79);
            archipelago->addRegionalCoin(packet->shineUid80);
            archipelago->addRegionalCoin(packet->shineUid81);
            archipelago->addRegionalCoin(packet->shineUid82);
            archipelago->addRegionalCoin(packet->shineUid83);
            archipelago->addRegionalCoin(packet->shineUid84);
            archipelago->addRegionalCoin(packet->shineUid85);
            archipelago->addRegionalCoin(packet->shineUid86);
            archipelago->addRegionalCoin(packet->shineUid87);
            archipelago->addRegionalCoin(packet->shineUid88);
            archipelago->addRegionalCoin(packet->shineUid89);
            archipelago->addRegionalCoin(packet->shineUid90);
            archipelago->addRegionalCoin(packet->shineUid91);
            archipelago->addRegionalCoin(packet->shineUid92);
            archipelago->addRegionalCoin(packet->shineUid93);
            archipelago->addRegionalCoin(packet->shineUid94);
            archipelago->addRegionalCoin(packet->shineUid95);
            archipelago->addRegionalCoin(packet->shineUid96);
            archipelago->addRegionalCoin(packet->shineUid97);
            archipelago->addRegionalCoin(packet->shineUid98);
            archipelago->addRegionalCoin(packet->shineUid99);
        }
    }
}

void Client::addApInfo(ApInfo* packet) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        int type = static_cast<int>(packet->infoType);

        if (type < 3) {
            sead::FixedSafeString<APNAMESIZE> info1;
            sead::FixedSafeString<APNAMESIZE> info2;
            sead::FixedSafeString<APNAMESIZE> info3;
            info1 = packet->info1;
            info2 = packet->info2;
            info3 = packet->info3;

            if (type == 0) {
                archipelago->setGameName(packet->index1, info1.cstr());
                archipelago->setGameName(packet->index2, info2.cstr());
                archipelago->setGameName(packet->index3, info3.cstr());
            }

            if (type == 1) {
                archipelago->setSlotName(packet->index1, info1.cstr());
                archipelago->setSlotName(packet->index2, info2.cstr());
                archipelago->setSlotName(packet->index3, info3.cstr());
            }

            if (type == 2) {
                archipelago->setItemName(packet->index1, info1.cstr());
                archipelago->setItemName(packet->index2, info2.cstr());
                archipelago->setItemName(packet->index3, info3.cstr());
            }
        }
        // } else {
        //     if (type == 3) {
        //         archipelago->setShineItemName(packet->index1, packet->info1);

        //         if (packet->index1 < 99) {
        //             archipelago->setShineItemName(packet->index2, packet->info2);
        //             archipelago->setShineItemName(packet->index3, packet->info3);
        //         }
        //     }
        // }
    }
}

void Client::updateShineReplace(ShineReplacePacket* packet) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        archipelago->setShineTextReplacement(0, {packet->itemType0, packet->itemNameIndex0});
        archipelago->setShineTextReplacement(1, {packet->itemType1, packet->itemNameIndex1});
        archipelago->setShineTextReplacement(2, {packet->itemType2, packet->itemNameIndex2});
        archipelago->setShineTextReplacement(3, {packet->itemType3, packet->itemNameIndex3});
        archipelago->setShineTextReplacement(4, {packet->itemType4, packet->itemNameIndex4});
        archipelago->setShineTextReplacement(5, {packet->itemType5, packet->itemNameIndex5});
        archipelago->setShineTextReplacement(6, {packet->itemType6, packet->itemNameIndex6});
        archipelago->setShineTextReplacement(7, {packet->itemType7, packet->itemNameIndex7});
        archipelago->setShineTextReplacement(8, {packet->itemType8, packet->itemNameIndex8});
        archipelago->setShineTextReplacement(9, {packet->itemType9, packet->itemNameIndex9});
        archipelago->setShineTextReplacement(10, {packet->itemType10, packet->itemNameIndex10});
        archipelago->setShineTextReplacement(11, {packet->itemType11, packet->itemNameIndex11});
        archipelago->setShineTextReplacement(12, {packet->itemType12, packet->itemNameIndex12});
        archipelago->setShineTextReplacement(13, {packet->itemType13, packet->itemNameIndex13});
        archipelago->setShineTextReplacement(14, {packet->itemType14, packet->itemNameIndex14});
        archipelago->setShineTextReplacement(15, {packet->itemType15, packet->itemNameIndex15});
        archipelago->setShineTextReplacement(16, {packet->itemType16, packet->itemNameIndex16});
        archipelago->setShineTextReplacement(17, {packet->itemType17, packet->itemNameIndex17});
        archipelago->setShineTextReplacement(18, {packet->itemType18, packet->itemNameIndex18});
        archipelago->setShineTextReplacement(19, {packet->itemType19, packet->itemNameIndex19});
        archipelago->setShineTextReplacement(20, {packet->itemType20, packet->itemNameIndex20});
        archipelago->setShineTextReplacement(21, {packet->itemType21, packet->itemNameIndex21});
        archipelago->setShineTextReplacement(22, {packet->itemType22, packet->itemNameIndex22});
        archipelago->setShineTextReplacement(23, {packet->itemType23, packet->itemNameIndex23});
        archipelago->setShineTextReplacement(24, {packet->itemType24, packet->itemNameIndex24});
        archipelago->setShineTextReplacement(25, {packet->itemType25, packet->itemNameIndex25});
        archipelago->setShineTextReplacement(26, {packet->itemType26, packet->itemNameIndex26});
        archipelago->setShineTextReplacement(27, {packet->itemType27, packet->itemNameIndex27});
        archipelago->setShineTextReplacement(28, {packet->itemType28, packet->itemNameIndex28});
        archipelago->setShineTextReplacement(29, {packet->itemType29, packet->itemNameIndex29});
        archipelago->setShineTextReplacement(30, {packet->itemType30, packet->itemNameIndex30});
        archipelago->setShineTextReplacement(31, {packet->itemType31, packet->itemNameIndex31});
        archipelago->setShineTextReplacement(32, {packet->itemType32, packet->itemNameIndex32});
        archipelago->setShineTextReplacement(33, {packet->itemType33, packet->itemNameIndex33});
        archipelago->setShineTextReplacement(34, {packet->itemType34, packet->itemNameIndex34});
        archipelago->setShineTextReplacement(35, {packet->itemType35, packet->itemNameIndex35});
        archipelago->setShineTextReplacement(36, {packet->itemType36, packet->itemNameIndex36});
        archipelago->setShineTextReplacement(37, {packet->itemType37, packet->itemNameIndex37});
        archipelago->setShineTextReplacement(38, {packet->itemType38, packet->itemNameIndex38});
        archipelago->setShineTextReplacement(39, {packet->itemType39, packet->itemNameIndex39});
        archipelago->setShineTextReplacement(40, {packet->itemType40, packet->itemNameIndex40});
        archipelago->setShineTextReplacement(41, {packet->itemType41, packet->itemNameIndex41});
        archipelago->setShineTextReplacement(42, {packet->itemType42, packet->itemNameIndex42});
        archipelago->setShineTextReplacement(43, {packet->itemType43, packet->itemNameIndex43});
        archipelago->setShineTextReplacement(44, {packet->itemType44, packet->itemNameIndex44});
        archipelago->setShineTextReplacement(45, {packet->itemType45, packet->itemNameIndex45});
        archipelago->setShineTextReplacement(46, {packet->itemType46, packet->itemNameIndex46});
        archipelago->setShineTextReplacement(47, {packet->itemType47, packet->itemNameIndex47});
        archipelago->setShineTextReplacement(48, {packet->itemType48, packet->itemNameIndex48});
        archipelago->setShineTextReplacement(49, {packet->itemType49, packet->itemNameIndex49});
        archipelago->setShineTextReplacement(50, {packet->itemType50, packet->itemNameIndex50});
        archipelago->setShineTextReplacement(51, {packet->itemType51, packet->itemNameIndex51});
        archipelago->setShineTextReplacement(52, {packet->itemType52, packet->itemNameIndex52});
        archipelago->setShineTextReplacement(53, {packet->itemType53, packet->itemNameIndex53});
        archipelago->setShineTextReplacement(54, {packet->itemType54, packet->itemNameIndex54});
        archipelago->setShineTextReplacement(55, {packet->itemType55, packet->itemNameIndex55});
        archipelago->setShineTextReplacement(56, {packet->itemType56, packet->itemNameIndex56});
        archipelago->setShineTextReplacement(57, {packet->itemType57, packet->itemNameIndex57});
        archipelago->setShineTextReplacement(58, {packet->itemType58, packet->itemNameIndex58});
        archipelago->setShineTextReplacement(59, {packet->itemType59, packet->itemNameIndex59});
        archipelago->setShineTextReplacement(60, {packet->itemType60, packet->itemNameIndex60});
        archipelago->setShineTextReplacement(61, {packet->itemType61, packet->itemNameIndex61});
        archipelago->setShineTextReplacement(62, {packet->itemType62, packet->itemNameIndex62});
        archipelago->setShineTextReplacement(63, {packet->itemType63, packet->itemNameIndex63});
        archipelago->setShineTextReplacement(64, {packet->itemType64, packet->itemNameIndex64});
        archipelago->setShineTextReplacement(65, {packet->itemType65, packet->itemNameIndex65});
        archipelago->setShineTextReplacement(66, {packet->itemType66, packet->itemNameIndex66});
        archipelago->setShineTextReplacement(67, {packet->itemType67, packet->itemNameIndex67});
        archipelago->setShineTextReplacement(68, {packet->itemType68, packet->itemNameIndex68});
        archipelago->setShineTextReplacement(69, {packet->itemType69, packet->itemNameIndex69});
        archipelago->setShineTextReplacement(70, {packet->itemType70, packet->itemNameIndex70});
        archipelago->setShineTextReplacement(71, {packet->itemType71, packet->itemNameIndex71});
        archipelago->setShineTextReplacement(72, {packet->itemType72, packet->itemNameIndex72});
        archipelago->setShineTextReplacement(73, {packet->itemType73, packet->itemNameIndex73});
        archipelago->setShineTextReplacement(74, {packet->itemType74, packet->itemNameIndex74});
        archipelago->setShineTextReplacement(75, {packet->itemType75, packet->itemNameIndex75});
        archipelago->setShineTextReplacement(76, {packet->itemType76, packet->itemNameIndex76});
        archipelago->setShineTextReplacement(77, {packet->itemType77, packet->itemNameIndex77});
        archipelago->setShineTextReplacement(78, {packet->itemType78, packet->itemNameIndex78});
        archipelago->setShineTextReplacement(79, {packet->itemType79, packet->itemNameIndex79});
        archipelago->setShineTextReplacement(80, {packet->itemType80, packet->itemNameIndex80});
        archipelago->setShineTextReplacement(81, {packet->itemType81, packet->itemNameIndex81});
        archipelago->setShineTextReplacement(82, {packet->itemType82, packet->itemNameIndex82});
        archipelago->setShineTextReplacement(83, {packet->itemType83, packet->itemNameIndex83});
        archipelago->setShineTextReplacement(84, {packet->itemType84, packet->itemNameIndex84});
        archipelago->setShineTextReplacement(85, {packet->itemType85, packet->itemNameIndex85});
        archipelago->setShineTextReplacement(86, {packet->itemType86, packet->itemNameIndex86});
        archipelago->setShineTextReplacement(87, {packet->itemType87, packet->itemNameIndex87});
        archipelago->setShineTextReplacement(88, {packet->itemType88, packet->itemNameIndex88});
        archipelago->setShineTextReplacement(89, {packet->itemType89, packet->itemNameIndex89});
        archipelago->setShineTextReplacement(90, {packet->itemType90, packet->itemNameIndex90});
        archipelago->setShineTextReplacement(91, {packet->itemType91, packet->itemNameIndex91});
        archipelago->setShineTextReplacement(92, {packet->itemType92, packet->itemNameIndex92});
        archipelago->setShineTextReplacement(93, {packet->itemType93, packet->itemNameIndex93});
        archipelago->setShineTextReplacement(94, {packet->itemType94, packet->itemNameIndex94});
        archipelago->setShineTextReplacement(95, {packet->itemType95, packet->itemNameIndex95});
        archipelago->setShineTextReplacement(96, {packet->itemType96, packet->itemNameIndex96});
        archipelago->setShineTextReplacement(97, {packet->itemType97, packet->itemNameIndex97});
        archipelago->setShineTextReplacement(98, {packet->itemType98, packet->itemNameIndex98});
        archipelago->setShineTextReplacement(99, {packet->itemType99, packet->itemNameIndex99});
    }
}

void Client::updateShineColor(ShineColor* packet) {
    // setMessage(1, "Entering udpateShineColor");
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        archipelago->setShineColors(static_cast<int>(packet->shineUid0), packet->color0);
        archipelago->setShineColors(static_cast<int>(packet->shineUid1), packet->color1);
        archipelago->setShineColors(static_cast<int>(packet->shineUid2), packet->color2);
        archipelago->setShineColors(static_cast<int>(packet->shineUid3), packet->color3);
        archipelago->setShineColors(static_cast<int>(packet->shineUid4), packet->color4);
        archipelago->setShineColors(static_cast<int>(packet->shineUid5), packet->color5);
        archipelago->setShineColors(static_cast<int>(packet->shineUid6), packet->color6);
        archipelago->setShineColors(static_cast<int>(packet->shineUid7), packet->color7);
        archipelago->setShineColors(static_cast<int>(packet->shineUid8), packet->color8);
        archipelago->setShineColors(static_cast<int>(packet->shineUid9), packet->color9);
        archipelago->setShineColors(static_cast<int>(packet->shineUid10), packet->color10);
        archipelago->setShineColors(static_cast<int>(packet->shineUid11), packet->color11);
        archipelago->setShineColors(static_cast<int>(packet->shineUid12), packet->color12);
        archipelago->setShineColors(static_cast<int>(packet->shineUid13), packet->color13);
        archipelago->setShineColors(static_cast<int>(packet->shineUid14), packet->color14);
        archipelago->setShineColors(static_cast<int>(packet->shineUid15), packet->color15);
        archipelago->setShineColors(static_cast<int>(packet->shineUid16), packet->color16);
        archipelago->setShineColors(static_cast<int>(packet->shineUid17), packet->color17);
        archipelago->setShineColors(static_cast<int>(packet->shineUid18), packet->color18);
        archipelago->setShineColors(static_cast<int>(packet->shineUid19), packet->color19);
        archipelago->setShineColors(static_cast<int>(packet->shineUid20), packet->color20);
        archipelago->setShineColors(static_cast<int>(packet->shineUid21), packet->color21);
        archipelago->setShineColors(static_cast<int>(packet->shineUid22), packet->color22);
        archipelago->setShineColors(static_cast<int>(packet->shineUid23), packet->color23);
        archipelago->setShineColors(static_cast<int>(packet->shineUid24), packet->color24);
        archipelago->setShineColors(static_cast<int>(packet->shineUid25), packet->color25);
        archipelago->setShineColors(static_cast<int>(packet->shineUid26), packet->color26);
        archipelago->setShineColors(static_cast<int>(packet->shineUid27), packet->color27);
        archipelago->setShineColors(static_cast<int>(packet->shineUid28), packet->color28);
        archipelago->setShineColors(static_cast<int>(packet->shineUid29), packet->color29);
        archipelago->setShineColors(static_cast<int>(packet->shineUid30), packet->color30);
        archipelago->setShineColors(static_cast<int>(packet->shineUid31), packet->color31);
        archipelago->setShineColors(static_cast<int>(packet->shineUid32), packet->color32);
        archipelago->setShineColors(static_cast<int>(packet->shineUid33), packet->color33);
        archipelago->setShineColors(static_cast<int>(packet->shineUid34), packet->color34);
        archipelago->setShineColors(static_cast<int>(packet->shineUid35), packet->color35);
        archipelago->setShineColors(static_cast<int>(packet->shineUid36), packet->color36);
        archipelago->setShineColors(static_cast<int>(packet->shineUid37), packet->color37);
        archipelago->setShineColors(static_cast<int>(packet->shineUid38), packet->color38);
        archipelago->setShineColors(static_cast<int>(packet->shineUid39), packet->color39);
        archipelago->setShineColors(static_cast<int>(packet->shineUid40), packet->color40);
        archipelago->setShineColors(static_cast<int>(packet->shineUid41), packet->color41);
        archipelago->setShineColors(static_cast<int>(packet->shineUid42), packet->color42);
        archipelago->setShineColors(static_cast<int>(packet->shineUid43), packet->color43);
        archipelago->setShineColors(static_cast<int>(packet->shineUid44), packet->color44);
        archipelago->setShineColors(static_cast<int>(packet->shineUid45), packet->color45);
        archipelago->setShineColors(static_cast<int>(packet->shineUid46), packet->color46);
        archipelago->setShineColors(static_cast<int>(packet->shineUid47), packet->color47);
        archipelago->setShineColors(static_cast<int>(packet->shineUid48), packet->color48);
        archipelago->setShineColors(static_cast<int>(packet->shineUid49), packet->color49);
        archipelago->setShineColors(static_cast<int>(packet->shineUid50), packet->color50);
    }
}

void Client::updateShopReplace(ShopReplacePacket* packet) {
    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO)) {
        ArchipelagoMode* archipelago = GameModeManager::instance()->getMode<ArchipelagoMode>();
        int type = static_cast<int>(packet->infoType);
        // Cap
        if (type == 0) {
            archipelago->setCapTextReplacement(0, {packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, packet->itemClassification0});
            archipelago->setCapTextReplacement(1, {packet->gameIndex1, packet->playerIndex1, packet->itemIndex1, packet->itemClassification1});
            archipelago->setCapTextReplacement(2, {packet->gameIndex2, packet->playerIndex2, packet->itemIndex2, packet->itemClassification2});
            archipelago->setCapTextReplacement(3, {packet->gameIndex3, packet->playerIndex3, packet->itemIndex3, packet->itemClassification3});
            archipelago->setCapTextReplacement(4, {packet->gameIndex4, packet->playerIndex4, packet->itemIndex4, packet->itemClassification4});
            archipelago->setCapTextReplacement(5, {packet->gameIndex5, packet->playerIndex5, packet->itemIndex5, packet->itemClassification5});
            archipelago->setCapTextReplacement(6, {packet->gameIndex6, packet->playerIndex6, packet->itemIndex6, packet->itemClassification6});
            archipelago->setCapTextReplacement(7, {packet->gameIndex7, packet->playerIndex7, packet->itemIndex7, packet->itemClassification7});
            archipelago->setCapTextReplacement(8, {packet->gameIndex8, packet->playerIndex8, packet->itemIndex8, packet->itemClassification8});
            archipelago->setCapTextReplacement(9, {packet->gameIndex9, packet->playerIndex9, packet->itemIndex9, packet->itemClassification9});
            archipelago->setCapTextReplacement(10, {packet->gameIndex10, packet->playerIndex10, packet->itemIndex10, packet->itemClassification10});
            archipelago->setCapTextReplacement(11, {packet->gameIndex11, packet->playerIndex11, packet->itemIndex11, packet->itemClassification11});
            archipelago->setCapTextReplacement(12, {packet->gameIndex12, packet->playerIndex12, packet->itemIndex12, packet->itemClassification12});
            archipelago->setCapTextReplacement(13, {packet->gameIndex13, packet->playerIndex13, packet->itemIndex13, packet->itemClassification13});
            archipelago->setCapTextReplacement(14, {packet->gameIndex14, packet->playerIndex14, packet->itemIndex14, packet->itemClassification14});
            archipelago->setCapTextReplacement(15, {packet->gameIndex15, packet->playerIndex15, packet->itemIndex15, packet->itemClassification15});
            archipelago->setCapTextReplacement(16, {packet->gameIndex16, packet->playerIndex16, packet->itemIndex16, packet->itemClassification16});
        }
        // Cloth
        if (type == 1) {
            archipelago->setClothesTextReplacement(0, {packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, packet->itemClassification0});
            archipelago->setClothesTextReplacement(1, {packet->gameIndex1, packet->playerIndex1, packet->itemIndex1, packet->itemClassification1});
            archipelago->setClothesTextReplacement(2, {packet->gameIndex2, packet->playerIndex2, packet->itemIndex2, packet->itemClassification2});
            archipelago->setClothesTextReplacement(3, {packet->gameIndex3, packet->playerIndex3, packet->itemIndex3, packet->itemClassification3});
            archipelago->setClothesTextReplacement(4, {packet->gameIndex4, packet->playerIndex4, packet->itemIndex4, packet->itemClassification4});
            archipelago->setClothesTextReplacement(5, {packet->gameIndex5, packet->playerIndex5, packet->itemIndex5, packet->itemClassification5});
            archipelago->setClothesTextReplacement(6, {packet->gameIndex6, packet->playerIndex6, packet->itemIndex6, packet->itemClassification6});
            archipelago->setClothesTextReplacement(7, {packet->gameIndex7, packet->playerIndex7, packet->itemIndex7, packet->itemClassification7});
            archipelago->setClothesTextReplacement(8, {packet->gameIndex8, packet->playerIndex8, packet->itemIndex8, packet->itemClassification8});
            archipelago->setClothesTextReplacement(9, {packet->gameIndex9, packet->playerIndex9, packet->itemIndex9, packet->itemClassification9});
            archipelago->setClothesTextReplacement(10, {packet->gameIndex10, packet->playerIndex10, packet->itemIndex10, packet->itemClassification10});
            archipelago->setClothesTextReplacement(11, {packet->gameIndex11, packet->playerIndex11, packet->itemIndex11, packet->itemClassification11});
            archipelago->setClothesTextReplacement(12, {packet->gameIndex12, packet->playerIndex12, packet->itemIndex12, packet->itemClassification12});
            archipelago->setClothesTextReplacement(13, {packet->gameIndex13, packet->playerIndex13, packet->itemIndex13, packet->itemClassification13});
            archipelago->setClothesTextReplacement(14, {packet->gameIndex14, packet->playerIndex14, packet->itemIndex14, packet->itemClassification14});
            archipelago->setClothesTextReplacement(15, {packet->gameIndex15, packet->playerIndex15, packet->itemIndex15, packet->itemClassification15});
            archipelago->setClothesTextReplacement(16, {packet->gameIndex16, packet->playerIndex16, packet->itemIndex16, packet->itemClassification16});
            archipelago->setClothesTextReplacement(17, {packet->gameIndex17, packet->playerIndex17, packet->itemIndex17, packet->itemClassification17});
            archipelago->setClothesTextReplacement(18, {packet->gameIndex18, packet->playerIndex18, packet->itemIndex18, packet->itemClassification18});
            archipelago->setClothesTextReplacement(19, {packet->gameIndex19, packet->playerIndex19, packet->itemIndex19, packet->itemClassification19});
        }
        // Sticker
        if (type == 2) {
            archipelago->setRegionalTextReplacement(0, {packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, packet->itemClassification0});
            archipelago->setRegionalTextReplacement(1, {packet->gameIndex1, packet->playerIndex1, packet->itemIndex1, packet->itemClassification1});
            archipelago->setRegionalTextReplacement(2, {packet->gameIndex2, packet->playerIndex2, packet->itemIndex2, packet->itemClassification2});
            archipelago->setRegionalTextReplacement(3, {packet->gameIndex3, packet->playerIndex3, packet->itemIndex3, packet->itemClassification3});
            archipelago->setRegionalTextReplacement(4, {packet->gameIndex4, packet->playerIndex4, packet->itemIndex4, packet->itemClassification4});
            archipelago->setRegionalTextReplacement(5, {packet->gameIndex5, packet->playerIndex5, packet->itemIndex5, packet->itemClassification5});
            archipelago->setRegionalTextReplacement(6, {packet->gameIndex6, packet->playerIndex6, packet->itemIndex6, packet->itemClassification6});
            archipelago->setRegionalTextReplacement(7, {packet->gameIndex7, packet->playerIndex7, packet->itemIndex7, packet->itemClassification7});
            archipelago->setRegionalTextReplacement(8, {packet->gameIndex8, packet->playerIndex8, packet->itemIndex8, packet->itemClassification8});
        }
        // Shop Moon
        if (type == 3) {
            archipelago->setShopMoonTextReplacement({packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, packet->itemClassification0});
        }
        if (type == 4) {
            archipelago->enqueueCappyMessage({packet->gameIndex0, packet->playerIndex0, packet->itemIndex0, static_cast<bool>(packet->itemClassification0)});
        }

        // Over world
        if (type == 5) {
            sInstance->sendMessage("ER Overworld");
            archipelago->setOverWorldStageConnection(packet->gameIndex0, {packet->playerIndex0, packet->itemIndex0});
            archipelago->setOverWorldStageConnection(packet->itemClassification0, {packet->gameIndex1, packet->playerIndex1});
            archipelago->setOverWorldStageConnection(packet->itemIndex1, {packet->itemClassification1, packet->gameIndex2});
            archipelago->setOverWorldStageConnection(packet->playerIndex2, {packet->itemIndex2, packet->itemClassification2});
            archipelago->setOverWorldStageConnection(packet->gameIndex3, {packet->playerIndex3, packet->itemIndex3});
            archipelago->setOverWorldStageConnection(packet->itemClassification3, {packet->gameIndex4, packet->playerIndex4});
            archipelago->setOverWorldStageConnection(packet->itemIndex4, {packet->itemClassification4, packet->gameIndex5});
            archipelago->setOverWorldStageConnection(packet->playerIndex5, {packet->itemIndex5, packet->itemClassification5});
            archipelago->setOverWorldStageConnection(packet->gameIndex6, {packet->playerIndex6, packet->itemIndex6});
            archipelago->setOverWorldStageConnection(packet->itemClassification6, {packet->gameIndex7, packet->playerIndex7});
            archipelago->setOverWorldStageConnection(packet->itemIndex7, {packet->itemClassification7, packet->gameIndex8});
            archipelago->setOverWorldStageConnection(packet->playerIndex8, {packet->itemIndex8, packet->itemClassification8});
            archipelago->setOverWorldStageConnection(packet->gameIndex9, {packet->playerIndex9, packet->itemIndex9});
            archipelago->setOverWorldStageConnection(packet->itemClassification9, {packet->gameIndex10, packet->playerIndex10});
            archipelago->setOverWorldStageConnection(packet->itemIndex10, {packet->itemClassification10, packet->gameIndex11});
            archipelago->setOverWorldStageConnection(packet->playerIndex11, {packet->itemIndex11, packet->itemClassification11});
            archipelago->setOverWorldStageConnection(packet->gameIndex12, {packet->playerIndex12, packet->itemIndex12});
            archipelago->setOverWorldStageConnection(packet->itemClassification12, {packet->gameIndex13, packet->playerIndex13});
            archipelago->setOverWorldStageConnection(packet->itemIndex13, {packet->itemClassification13, packet->gameIndex14});
            archipelago->setOverWorldStageConnection(packet->playerIndex14, {packet->itemIndex14, packet->itemClassification14});
            archipelago->setOverWorldStageConnection(packet->gameIndex15, {packet->playerIndex15, packet->itemIndex15});
            archipelago->setOverWorldStageConnection(packet->itemClassification15, {packet->gameIndex16, packet->playerIndex16});
            archipelago->setOverWorldStageConnection(packet->itemIndex16, {packet->itemClassification16, packet->gameIndex17});
            archipelago->setOverWorldStageConnection(packet->playerIndex17, {packet->itemIndex17, packet->itemClassification17});
            archipelago->setOverWorldStageConnection(packet->gameIndex18, {packet->playerIndex18, packet->itemIndex18});
            archipelago->setOverWorldStageConnection(packet->itemClassification18, {packet->gameIndex19, packet->playerIndex19});
            archipelago->setOverWorldStageConnection(packet->itemIndex19, {packet->itemClassification19, packet->gameIndex20});
            archipelago->setOverWorldStageConnection(packet->playerIndex20, {packet->itemIndex20, packet->itemClassification20});
            archipelago->setOverWorldStageConnection(packet->gameIndex21, {packet->playerIndex21, packet->itemIndex21});
            archipelago->setOverWorldStageConnection(packet->itemClassification21, {packet->gameIndex22, packet->playerIndex22});
            archipelago->setOverWorldStageConnection(packet->itemIndex22, {packet->itemClassification22, packet->gameIndex23});
            archipelago->setOverWorldStageConnection(packet->playerIndex23, {packet->itemIndex23, packet->itemClassification23});
            archipelago->setOverWorldStageConnection(packet->gameIndex24, {packet->playerIndex24, packet->itemIndex24});
            archipelago->setOverWorldStageConnection(packet->itemClassification24, {packet->gameIndex25, packet->playerIndex25});
            archipelago->setOverWorldStageConnection(packet->itemIndex25, {packet->itemClassification25, packet->gameIndex26});
            archipelago->setOverWorldStageConnection(packet->playerIndex26, {packet->itemIndex26, packet->itemClassification26});
            archipelago->setOverWorldStageConnection(packet->gameIndex27, {packet->playerIndex27, packet->itemIndex27});
            archipelago->setOverWorldStageConnection(packet->itemClassification27, {packet->gameIndex28, packet->playerIndex28});
            archipelago->setOverWorldStageConnection(packet->itemIndex28, {packet->itemClassification28, packet->gameIndex29});
            archipelago->setOverWorldStageConnection(packet->playerIndex29, {packet->itemIndex29, packet->itemClassification29});
            archipelago->setOverWorldStageConnection(packet->gameIndex30, {packet->playerIndex30, packet->itemIndex30});
            archipelago->setOverWorldStageConnection(packet->itemClassification30, {packet->gameIndex31, packet->playerIndex31});
            archipelago->setOverWorldStageConnection(packet->itemIndex31, {packet->itemClassification31, packet->gameIndex32});
            archipelago->setOverWorldStageConnection(packet->playerIndex32, {packet->itemIndex32, packet->itemClassification32});
            archipelago->setOverWorldStageConnection(packet->gameIndex33, {packet->playerIndex33, packet->itemIndex33});
            archipelago->setOverWorldStageConnection(packet->itemClassification33, {packet->gameIndex34, packet->playerIndex34});
            archipelago->setOverWorldStageConnection(packet->itemIndex34, {packet->itemClassification34, packet->gameIndex35});
            archipelago->setOverWorldStageConnection(packet->playerIndex35, {packet->itemIndex35, packet->itemClassification35});
            archipelago->setOverWorldStageConnection(packet->gameIndex36, {packet->playerIndex36, packet->itemIndex36});
            archipelago->setOverWorldStageConnection(packet->itemClassification36, {packet->gameIndex37, packet->playerIndex37});
        }

        // Sub Area
        if (type == 6) {
            sInstance->sendMessage("ER Sub Area");
            archipelago->setSubAreaStageConnection(packet->gameIndex0, {packet->playerIndex0, packet->itemIndex0});
            archipelago->setSubAreaStageConnection(packet->itemClassification0, {packet->gameIndex1, packet->playerIndex1});
            archipelago->setSubAreaStageConnection(packet->itemIndex1, {packet->itemClassification1, packet->gameIndex2});
            archipelago->setSubAreaStageConnection(packet->playerIndex2, {packet->itemIndex2, packet->itemClassification2});
            archipelago->setSubAreaStageConnection(packet->gameIndex3, {packet->playerIndex3, packet->itemIndex3});
            archipelago->setSubAreaStageConnection(packet->itemClassification3, {packet->gameIndex4, packet->playerIndex4});
            archipelago->setSubAreaStageConnection(packet->itemIndex4, {packet->itemClassification4, packet->gameIndex5});
            archipelago->setSubAreaStageConnection(packet->playerIndex5, {packet->itemIndex5, packet->itemClassification5});
            archipelago->setSubAreaStageConnection(packet->gameIndex6, {packet->playerIndex6, packet->itemIndex6});
            archipelago->setSubAreaStageConnection(packet->itemClassification6, {packet->gameIndex7, packet->playerIndex7});
            archipelago->setSubAreaStageConnection(packet->itemIndex7, {packet->itemClassification7, packet->gameIndex8});
            archipelago->setSubAreaStageConnection(packet->playerIndex8, {packet->itemIndex8, packet->itemClassification8});
            archipelago->setSubAreaStageConnection(packet->gameIndex9, {packet->playerIndex9, packet->itemIndex9});
            archipelago->setSubAreaStageConnection(packet->itemClassification9, {packet->gameIndex10, packet->playerIndex10});
            archipelago->setSubAreaStageConnection(packet->itemIndex10, {packet->itemClassification10, packet->gameIndex11});
            archipelago->setSubAreaStageConnection(packet->playerIndex11, {packet->itemIndex11, packet->itemClassification11});
            archipelago->setSubAreaStageConnection(packet->gameIndex12, {packet->playerIndex12, packet->itemIndex12});
            archipelago->setSubAreaStageConnection(packet->itemClassification12, {packet->gameIndex13, packet->playerIndex13});
            archipelago->setSubAreaStageConnection(packet->itemIndex13, {packet->itemClassification13, packet->gameIndex14});
            archipelago->setSubAreaStageConnection(packet->playerIndex14, {packet->itemIndex14, packet->itemClassification14});
            archipelago->setSubAreaStageConnection(packet->gameIndex15, {packet->playerIndex15, packet->itemIndex15});
            archipelago->setSubAreaStageConnection(packet->itemClassification15, {packet->gameIndex16, packet->playerIndex16});
            archipelago->setSubAreaStageConnection(packet->itemIndex16, {packet->itemClassification16, packet->gameIndex17});
            archipelago->setSubAreaStageConnection(packet->playerIndex17, {packet->itemIndex17, packet->itemClassification17});
            archipelago->setSubAreaStageConnection(packet->gameIndex18, {packet->playerIndex18, packet->itemIndex18});
            archipelago->setSubAreaStageConnection(packet->itemClassification18, {packet->gameIndex19, packet->playerIndex19});
            archipelago->setSubAreaStageConnection(packet->itemIndex19, {packet->itemClassification19, packet->gameIndex20});
            archipelago->setSubAreaStageConnection(packet->playerIndex20, {packet->itemIndex20, packet->itemClassification20});
            archipelago->setSubAreaStageConnection(packet->gameIndex21, {packet->playerIndex21, packet->itemIndex21});
            archipelago->setSubAreaStageConnection(packet->itemClassification21, {packet->gameIndex22, packet->playerIndex22});
            archipelago->setSubAreaStageConnection(packet->itemIndex22, {packet->itemClassification22, packet->gameIndex23});
            archipelago->setSubAreaStageConnection(packet->playerIndex23, {packet->itemIndex23, packet->itemClassification23});
            archipelago->setSubAreaStageConnection(packet->gameIndex24, {packet->playerIndex24, packet->itemIndex24});
            archipelago->setSubAreaStageConnection(packet->itemClassification24, {packet->gameIndex25, packet->playerIndex25});
            archipelago->setSubAreaStageConnection(packet->itemIndex25, {packet->itemClassification25, packet->gameIndex26});
            archipelago->setSubAreaStageConnection(packet->playerIndex26, {packet->itemIndex26, packet->itemClassification26});
            archipelago->setSubAreaStageConnection(packet->gameIndex27, {packet->playerIndex27, packet->itemIndex27});
            archipelago->setSubAreaStageConnection(packet->itemClassification27, {packet->gameIndex28, packet->playerIndex28});
            archipelago->setSubAreaStageConnection(packet->itemIndex28, {packet->itemClassification28, packet->gameIndex29});
            archipelago->setSubAreaStageConnection(packet->playerIndex29, {packet->itemIndex29, packet->itemClassification29});
            archipelago->setSubAreaStageConnection(packet->gameIndex30, {packet->playerIndex30, packet->itemIndex30});
            archipelago->setSubAreaStageConnection(packet->itemClassification30, {packet->gameIndex31, packet->playerIndex31});
            archipelago->setSubAreaStageConnection(packet->itemIndex31, {packet->itemClassification31, packet->gameIndex32});
            archipelago->setSubAreaStageConnection(packet->playerIndex32, {packet->itemIndex32, packet->itemClassification32});
            archipelago->setSubAreaStageConnection(packet->gameIndex33, {packet->playerIndex33, packet->itemIndex33});
            archipelago->setSubAreaStageConnection(packet->itemClassification33, {packet->gameIndex34, packet->playerIndex34});
            archipelago->setSubAreaStageConnection(packet->itemIndex34, {packet->itemClassification34, packet->gameIndex35});
            archipelago->setSubAreaStageConnection(packet->playerIndex35, {packet->itemIndex35, packet->itemClassification35});
            archipelago->setSubAreaStageConnection(packet->gameIndex36, {packet->playerIndex36, packet->itemIndex36});
            archipelago->setSubAreaStageConnection(packet->itemClassification36, {packet->gameIndex37, packet->playerIndex37});
        }
    }
}

// ===== Utility Functions =====

void Client::addMessage(const char* message) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }
    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    MessagePacket* packet = new MessagePacket();
    strcpy(packet->message, message);

    sInstance->updateMessages(packet);
}

void Client::sendMessage(const char* message) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }
    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    MessagePacket* packet = new MessagePacket();
    strcpy(packet->message, message);

    sInstance->mSocket->queuePacket(packet);
}

void Client::updateArchipelagoShines(GameDataHolderAccessor accessor, int shineID) {
    // Update to proper range when achievement support added
    if (shineID >= 2000 && shineID <= 2060) {
        if (!rs::checkGetAchievement(sInstance->mCurStageScene, toadetteMoons[shineID - 2000])) {
            accessor->getGameDataFile()->getAchievement(toadetteMoons[shineID - 2000]);
        }
        return;
    }

    GameDataFile::HintInfo* shineInfo = CustomGameDataFunction::getHintInfoByUniqueID(accessor, shineID);

    if (shineInfo) {
        if (!GameDataFunction::isGotShine(accessor, shineInfo->stageName.cstr(), shineInfo->objId.cstr())) {
            Shine* stageShine = findStageShine(shineID);

            if (stageShine) {
                if (al::isDead(stageShine)) {
                    stageShine->makeActorAlive();
                }

                // stageShine->onSwitchGet();
            }

            GameDataHolderAccessor(accessor)->getGameDataFile()->setGotShine(shineInfo);
        }
    }
}

void Client::apApplyOneCoinCollect(const char* placeID, int worldID, const char* stage) {
    const al::PlacementId placementId(placeID, nullptr, nullptr);
    GameDataHolderWriter writer(sInstance->mCurStageScene);
    writer.mData->getGameDataFile()->customAddCoinCollect(&placementId, worldID, stage);
    sead::FixedSafeString<128> recCoin = sead::FixedSafeString<128>();
    recCoin = "Received Coin at ";
    recCoin.append(placeID);
    recCoin.append(", ");
    recCoin.append(intToCstr(worldID));
    recCoin.append(", ");
    recCoin.append(stage);
    // addMessage(recCoin.cstr());
}

// ===== UPDATE UI =====

void Client::startShineCount() {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }
    sInstance->mCurStageScene->stageSceneLayout->startShineCountAnim(false);
    startShineChipCount();
}

void Client::startShineChipCount() {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }
    sInstance->mCurStageScene->stageSceneLayout->updateCounterParts();
}