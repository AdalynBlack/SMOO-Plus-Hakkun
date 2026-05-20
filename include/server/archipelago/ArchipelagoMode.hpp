#pragma once

#include "al/Library/Camera/CameraTicket.h"

#include "game/Layout/ShopLayoutInfo.h"
#include "game/Sequence/ChangeStageInfo.h"
#include "game/System/GameDataHolderWriter.h"

#include <math.h>
#include <stdint.h>

#include "container/seadSafeArray.h"
#include "heap/seadExpHeap.h"
#include "server/archipelago/ArchipelagoHelpers.hpp"
#include "server/archipelago/ArchipelagoHintArrow.h"
#include "server/archipelago/ArchipelagoInfo.h"
#include "server/gamemode/GameMode.h"
#include "server/gamemode/GameModeBase.hpp"

enum CheckType { Coins = -2, Moon = -1, Clothes = 0, Cap = 1, Souvenir = 2, Sticker = 3, RegionalCoin = 4, Capture = 5 };

class ArchipelagoMode : public GameModeBase {
public:
    ArchipelagoMode(const char* name);

    void init(GameModeInitInfo const& info) override;

    virtual void begin() override;
    virtual void update() override;
    virtual void end() override;

    void pause() override;
    void unpause() override;
    void debugMenuControls() override;

    bool isUseNormalUI() const override { return true; }

    // ===== Arcipelago Setters / Getters =====
    void setScenario(int worldID, int scenario);
    bool setScenario(const char* worldName, int scenario);
    int getScenario(const char* worldName);
    int getScenario(int worldID);
    void sendCorrectScenario(const ChangeStageInfo* info);

    void addShine(int uid);
    bool hasShine(int uid);
    int getShineChecks(int index);
    void setShineChecks(int index, int checks);

    void addOutfit(const ShopItem::ItemInfo* info);
    bool hasOutfit(const ShopItem::ItemInfo* info);
    int getOutfitChecks(int index);
    void setOutfitChecks(int index, int checks);

    void addSticker(const ShopItem::ItemInfo* info);
    bool hasSticker(const ShopItem::ItemInfo* info);
    int getStickerChecks(int index);
    void setStickerChecks(int index, int checks);

    void addSouvenir(const ShopItem::ItemInfo* info);
    bool hasSouvenir(const ShopItem::ItemInfo* info);
    int getSouvenirChecks(int index);
    void setSouvenirChecks(int index, int checks);

    bool hasItem(const ShopItem::ItemInfo* info);
    void addItem(const ShopItem::ItemInfo* info);

    void addCapture(const char* capture);
    bool hasCapture(const char* capture);
    int getCaptureChecks(int index);
    void setCaptureChecks(int index, int checks);
    void addCaptureCheck(const char* capture);
    bool hasCaptureCheck(const char* capture);
    void setIsRecordCapture(bool value);

    void addRegionalCoin(const char* placementId);
    void addRegionalCoin(int index);
    bool hasRegionalCoin(const char* placementId);
    bool hasRegionalCoin(int index);

    void setCheckIndex(int index);
    int getCheckIndex() { return mCheckIndex; };

    void setRecentShineHintIndex(int index);
    int getRecentShineHintIndex() { return mRecentShineHintIndex; }

    void setWorldUnlockCount(int worldId, int count);
    int getWorldUnlockCount(int worldId);
    void setDeathLinkFlag(bool value) { mDeathLinkEnabled = value; };
    bool getRegionalsFlag() { return mDeathLinkEnabled; };
    void setCapturesFlag(bool value) { mCapturesEnabled = value; };
    bool getCapturesFlag() { return mCapturesEnabled; };
    void setERFlag(bool value) { mIsEntranceRandomizationEnabled = value; };
    bool getERFlag() { return mIsEntranceRandomizationEnabled; };
    void setConnectInitFlag(bool value) { mIsConnectInit = value; };
    bool getConnectInitFlag() { return mIsConnectInit; };
    void setCurWorldShineList(int worldId) { mCurWorldShineList = worldId; };
    int getCurWorldShineList() { return mCurWorldShineList; };
    void setRelativeWorldCoinCollect(int worldId) { mRelativeWorldCoinCollect = worldId; };
    void setRelativeWorldCoinCollect(const char* stageName);
    int getRelativeWorldCoinCollect() { return mRelativeWorldCoinCollect; };
    void setIsNeedUpdateCounter(bool value) { mIsNeedUpdateCounter = value; };
    bool getIsNeedUpdateCounter() { return mIsNeedUpdateCounter; };

    void setGameName(int index, const char16_t* name);
    void setSlotName(int index, const char16_t* name);
    void setItemName(int index, const char16_t* name);
    void setShineItemName(int index, const char* name);

    void setShineTextReplacement(int index, shineReplaceText replace);
    void setShineColors(int index, u8 replace);
    void setClothesTextReplacement(int index, shopReplaceText replace);
    void setCapTextReplacement(int index, shopReplaceText replace);
    void setSouvenirTextReplacement(int index, shopReplaceText replace);
    void setStickerTextReplacement(int index, shopReplaceText replace);
    void setShopMoonTextReplacement(int index, shopReplaceText replace);
    void setOverWorldStageConnection(int index, stageConnection replace);
    void setSubAreaStageConnection(int index, stageConnection replace);

    const char* getShineReplacementText();
    int getShineColor(Shine* curShine);
    const char16_t* getShopReplacementText(const char* fileName, const char* key);

    void setDying(bool value);
    void setApDeath(bool value);
    bool isDying() { return mDying; }
    bool isApDeath() { return mApDeath; }

    ChangeStageInfo* handleER(const ChangeStageInfo* info);

    bool isTargetAlive();
    bool trySetHintTargetValid();

    // ===== Archipeligo Check Senders =====
    void sendMoonCheck(int uid);
    void sendShopCheck(const ShopItem::ItemInfo* itemInfo);
    void sendRegionalCoinCheck(const char* objId, const char* stageName);
    void sendCaptureCheck(const char* hackName);

    PlayerActorHakoniwa* getPlayerActorHakoniwa();  // Returns nullptr if the player is not a PlayerActorHakoniwa

    void setWipeHolder(al::WipeHolder* wipe) { mWipeHolder = wipe; };  // Called with HakoniwaSequence hook, wipe used in recovery event

    // ===== Archipelago Utility Methods =====
    void sendStage(GameDataHolderWriter writer, const ChangeStageInfo* stageInfo);
    void sendBack();
    void isSubArea(GameDataHolderAccessor accessor, bool* isInSubArea, sead::FixedSafeString<32> stageId);
    void getCustomStageId(GameDataHolderAccessor accessor, const ChangeStageInfo* info, sead::FixedSafeString<32>* stageId);
    void correctCustomStageId(sead::FixedSafeString<32>* toStageId);
    int getNumGotShines();
    int getNumCoinCollect();
    void handleDeathLink(PlayerActorBase* playerBase, PlayerActorHakoniwa* playerHakoniwa, GameDataHolderWriter writer);
    void handleCaptureSanity(PlayerActorBase* playerBase, GameDataHolderAccessor accessor);
    void getNearestRegional(StageScene* stageScene, PlayerActorBase* playerBase);
    void handleSoftLocks(GameDataHolderAccessor accessor, GameDataHolderWriter writer);
    void updateCounter(PlayerActorBase* playerBase, GameDataHolderAccessor accessor);
    void infoMenu();
    int getRelativeWorldCoinCollectCheckGotNum(GameDataHolderAccessor accessor);

private:
    al::WipeHolder* mWipeHolder = nullptr;  // Pointer set by setWipeHolder on first step of hakoniwaSequence hook

    // ===== Scene Actors =====
    ArchipelagoHintArrow* mHintArrow = nullptr;  // Arrow that points to a targeted object
    // ptr to the targeted actor so arrow can be deactivated when coin is collected
    al::LiveActor* mCoinCollectHintTarget = nullptr;

    // ===== Archipeligo Data =====

    // Esacape to last trasition or Odyssey
    // Could just use preexisting

    // Update Timers
    unsigned short mUpdateCounterTimer = 0;
    bool mIsNeedUpdateCounter = false;
    u8 mSoftlockTimer = 0;

    ArchipelagoInfo* mInfo = nullptr;
    bool mIsInfoMenuOpen = false;
    short mInfoMenuPageNum = 0;
    const short mInfoMenuPageMax = 3;

    // shine pay counts
    sead::SafeArray<int, 17> mWorldPayCounts;
    bool mDeathLinkEnabled = false;
    bool mCapturesEnabled = false;
    bool mIsRecordCapture = false;
    bool mIsEntranceRandomizationEnabled = false;
    bool mIsConnectInit = false;
    sead::SafeArray<int, 17> mWorldScenarios;
    bool mDying = false;
    bool mApDeath = false;
    int mCheckIndex = 0;

    // List of 37 ints to track which shine's have been grabbed
    sead::SafeArray<u8, 148> collectedShines;

    // List of 11 u8s for tracking which caps and clothes have been grabbed
    sead::SafeArray<u8, 11> collectedOutfits;

    // List of 3 u8s for tracking which stickers have been grabbed
    sead::SafeArray<u8, 3> collectedStickers;

    // List of 4 u8s for tracking which souvenirs have been grabbed
    sead::SafeArray<u8, 4> collectedSouvenirs;

    // List of 11 u8s for tracking which caps and clothes have been scouted
    sead::SafeArray<u8, 11> mScoutedOutfits;

    // List of 3 u8s for tracking which stickers have been scouted
    sead::SafeArray<u8, 3> mScoutedStickers;

    // List of 4 u8s for tracking which souvenirs have been scouted
    sead::SafeArray<u8, 4> mScoutedSouvenirs;

    // List of 7 u8s for tracking which captures have been grabbed
    sead::SafeArray<u8, 7> collectedCaptures;
    sead::SafeArray<u8, 7> checkedCaptures;

    // List of 7 u8s for tracking which captures have been grabbed
    sead::SafeArray<u8, 126> mCollectedRegionals;

    // List of 3 u8s for tracking which moon rocks have been collected
    sead::SafeArray<u8, 3> mCollectedMoonRocks;

    // List of 3 u8s for tracking which moon rocks have been scouted
    sead::SafeArray<u8, 3> mScoutedMoonRocks;

    // Moon Text Replacement Handling
    int mRecentShineHintIndex = 0;
    sead::SafeArray<shineReplaceText, 100> shineTextReplacements;
    sead::SafeArray<sead::FixedSafeString<APNAMESIZE>, 100> mShineItemNames;
    sead::SafeArray<sead::FixedSafeString<APNAMESIZE>, 100> mShineSlotNames;

    // Moon Color Replacement
    sead::SafeArray<s8, 1170> shineColors;

    // Shop Text Replacement Handling
    sead::SafeArray<shopReplaceText, 44> shopCapTextReplacements;
    sead::SafeArray<shopReplaceText, 44> shopClothTextReplacements;
    sead::SafeArray<shopReplaceText, 17> shopStickerTextReplacements;
    sead::SafeArray<shopReplaceText, 26> shopGiftTextReplacements;
    sead::SafeArray<shopReplaceText, 13> shopMoonTextReplacements;
    sead::SafeArray<sead::WFixedSafeString<APNAMESIZE>, 144> mGameNames;
    sead::SafeArray<sead::WFixedSafeString<APNAMESIZE>, 144> mSlotNames;
    sead::SafeArray<sead::WFixedSafeString<APNAMESIZE>, 144> mItemNames;

    // With only 9 slots for regional coin items that are updated upon entering a shop
    // Add 100 to merge with shine slots and items
    // sead::SafeArray<sead::WFixedSafeString<APNAMESIZE>, 72> mGameNames;
    // sead::SafeArray<sead::WFixedSafeString<APNAMESIZE>, 72> mSlotNames;
    // sead::SafeArray<sead::WFixedSafeString<APNAMESIZE>, 72> mItemNames;

    int numApGames = 0;
    int numApSlots = 0;
    int numApItems = 0;

    // Loading Zone Replacement
    // 239 one for each StageId
    // May need more for alternate loading zones in sub areas that don't use a unique
    // stage id like forks exit
    // Flag for if Entrance Randomization is active
    // stageConnections are indexed by stageId
    // Overworld connections
    sead::SafeArray<stageConnection, 239> mOverworldStageConnections;
    // Sub area connections
    sead::SafeArray<stageConnection, 239> mSubAreaStageConnections;

    ChangeStageInfo* mLastERTransition = nullptr;

    int mCurWorldShineList = 0;
    int mRelativeWorldCoinCollect = -1;
};