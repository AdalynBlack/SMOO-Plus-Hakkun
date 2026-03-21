#include "hk/types.h"

#include "nn/os.h"
#include "nn/socket.h"

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"

#include "game/Info/ShineInfo.h"
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

// ===== Packet Senders =====
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

    int itemType = packet->itemType;

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

    switch (itemType) {
    case -2:
        // setMessage(3, "Coins Received");
        if (GameModeManager::instance()->getMode<ArchipelagoMode>()->getCheckIndex() < packet->index) {
            GameDataFunction::addCoin(writer, packet->amount);
            updateIndex = true;
        }
        break;
    case -1:
        if (collectedShineCount < curCollectedShines.size() - 1) {
            curCollectedShines[collectedShineCount] = packet->locationId;
            collectedShineCount++;
        }
        break;
    case 0:
        strcpy(info.name, costumeNames[packet->locationId]);
        info.type = static_cast<ShopItem::ItemType>(itemType);
        infoPtr = &info;
        writer.mData->mPlayingFile->buyItem(infoPtr, false);
        if (GameModeManager::instance()->getMode<ArchipelagoMode>()->getCheckIndex() < packet->index) {
            GameDataFunction::wearCostume(writer, info.name);
            updateIndex = true;
        }
        break;
    case 1:
        strcpy(info.name, costumeNames[packet->locationId]);
        info.type = static_cast<ShopItem::ItemType>(itemType);
        infoPtr = &info;
        writer.mData->mPlayingFile->buyItem(infoPtr, false);
        if (GameModeManager::instance()->getMode<ArchipelagoMode>()->getCheckIndex() < packet->index) {
            GameDataFunction::wearCap(writer, info.name);
            updateIndex = true;
        }
        break;
    case 2:
        strcpy(info.name, souvenirNames[packet->locationId]);
        info.type = static_cast<ShopItem::ItemType>(itemType);
        infoPtr = &info;
        writer.mData->mPlayingFile->buyItem(infoPtr, false);
        break;
    case 3:
        strcpy(info.name, stickerNames[packet->locationId]);
        info.type = static_cast<ShopItem::ItemType>(itemType);
        infoPtr = &info;
        writer.mData->mPlayingFile->buyItem(infoPtr, false);
        break;

    case 4: {
        const al::PlacementId placementId(packet->objId, nullptr, nullptr);
        writer.mData->mPlayingFile->customAddCoinCollect(&placementId, packet->amount, packet->stage);
        break;
    }

    case 5:
        addCapture(captureListNames[packet->locationId]);
        GameDataFunction::addHackDictionary(writer, captureListNames[packet->locationId]);
        break;
    }

    if (updateIndex) {
        GameModeManager::instance()->getMode<ArchipelagoMode>()->setCheckIndex(packet->index);
    }
}

// ===== Utility Functions =====

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