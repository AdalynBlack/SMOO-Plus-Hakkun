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

    void setCheckIndex(int index);
    int getCheckIndex() { return mCheckIndex; };

    void setMessage(int num, const char* msg);

    // sead::FixedSafeString<0x4B> getAPChatMessage1() { return sInstance ? sInstance->apChatLine1 : sead::FixedSafeString<0x20>::cEmptyString; }
    // sead::FixedSafeString<0x4B> getAPChatMessage2() { return sInstance ? sInstance->apChatLine2 : sead::FixedSafeString<0x20>::cEmptyString; }
    // sead::FixedSafeString<0x4B> getAPChatMessage3() { return sInstance ? sInstance->apChatLine3 : sead::FixedSafeString<0x20>::cEmptyString; }
    void setRecentShine(Shine* curShine);
    Shine* getRecentShine() { return mRecentShine; }

    void setWorldUnlockCount(int worldId, int count);
    int getWorldUnlockCount(int worldId);
    void setRegionalsFlag(bool value) { mRegionalsEnabled = value; };
    bool getRegionalsFlag() { return mRegionalsEnabled; };
    void setCapturesFlag(bool value) { mCapturesEnabled = value; };
    bool getCapturesFlag() { return mCapturesEnabled; };

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

    const char* getShineReplacementText();
    int getShineColor(Shine* curShine);
    const char16_t* getShopReplacementText(const char* fileName, const char* key);

    void setDying(bool value);
    void setApDeath(bool value);
    bool isDying() { return mDying; }
    bool isApDeath() { return mApDeath; }

    // ===== Archipeligo Check Senders =====
    void sendMoonCheck(int uid);
    void sendShopCheck(const ShopItem::ItemInfo* itemInfo);
    void sendRegionalCoinCheck(const char* objId, const char* stageName);
    void sendCaptureCheck(const char* hackName);

    PlayerActorHakoniwa* getPlayerActorHakoniwa();  // Returns nullptr if the player is not a PlayerActorHakoniwa

    void setWipeHolder(al::WipeHolder* wipe) { mWipeHolder = wipe; };  // Called with HakoniwaSequence hook, wipe used in recovery event

    // ===== Archipelago Utility Methods =====
    void sendStage(GameDataHolderWriter writer, const ChangeStageInfo* stageInfo);

private:
    al::WipeHolder* mWipeHolder = nullptr;  // Pointer set by setWipeHolder on first step of hakoniwaSequence hook

    // ===== Scene Actors =====
    ArchipelagoHintArrow* mHintArrow = nullptr;  // Arrow that points to a targeted object
    // ptr to the targeted actor so arrow can be deactivated when coin is collected
    al::LiveActor* mCoinCollectHintTarget = nullptr;

    // ===== Archipeligo Heap =====
    sead::ExpHeap* mHeap = nullptr;

    // ===== Archipeligo Data =====
    sead::FixedSafeString<0x4B> apChatLine1;
    sead::FixedSafeString<0x4B> apChatLine2;
    sead::FixedSafeString<0x4B> apChatLine3;

    // shine pay counts
    sead::SafeArray<int, 17> mWorldPayCounts;
    bool mRegionalsEnabled = false;
    bool mCapturesEnabled = false;
    bool mIsRecordCapture = false;
    sead::SafeArray<int, 17> mWorldScenarios;
    bool mDying = false;
    bool mApDeath = false;
    int mCheckIndex = 0;

    // List of 37 ints to track which shine's have been grabbed
    sead::SafeArray<int, 37> collectedShines;

    // List of 11 u8s for tracking which caps and clothes have been grabbed
    sead::SafeArray<u8, 11> collectedOutfits;

    // List of 3 u8s for tracking which stickers have been grabbed
    sead::SafeArray<u8, 3> collectedStickers;

    // List of 4 u8s for tracking which souvenirs have been grabbed
    sead::SafeArray<u8, 4> collectedSouvenirs;

    // List of 7 u8s for tracking which captures have been grabbed
    sead::SafeArray<u8, 7> collectedCaptures;
    sead::SafeArray<u8, 7> checkedCaptures;

    // Moon Text Replacement Handling
    Shine* mRecentShine = nullptr;
    sead::SafeArray<shineReplaceText, 100> shineTextReplacements;
    sead::SafeArray<sead::FixedSafeString<40>, 100> mShineItemNames;

    // Moon Color Replacement
    sead::SafeArray<s8, 1170> shineColors;

    // Shop Text Replacement Handling
    sead::SafeArray<shopReplaceText, 44> shopCapTextReplacements;
    sead::SafeArray<shopReplaceText, 44> shopClothTextReplacements;
    sead::SafeArray<shopReplaceText, 17> shopStickerTextReplacements;
    sead::SafeArray<shopReplaceText, 26> shopGiftTextReplacements;
    sead::SafeArray<shopReplaceText, 13> shopMoonTextReplacements;
    sead::SafeArray<sead::WFixedSafeString<40>, 144> mGameNames;
    sead::SafeArray<sead::WFixedSafeString<40>, 144> mSlotNames;
    sead::SafeArray<sead::WFixedSafeString<40>, 144> mItemNames;
    int numApGames = 0;
    int numApSlots = 0;
    int numApItems = 0;

    // Loading Zone Replacement
    // 378 one for each StageId
    // May need more for alternate loading zones in sub areas that don't use a unique
    // stage id like forks exit
    // Flag for if Entrance Randomization is active
    // stageConnections are indexed by stageId
    bool isER = false;
    // Overworld connections
    sead::SafeArray<stageConnection, 378> overworldStageConnections;
    // Sub area connections
    sead::SafeArray<stageConnection, 378> subAreaStageConnections;

    // Esacape to last trasition or Odyssey
    // Could just use preexisting

    // Update Timers
    unsigned short mUpdateCounterTimer = 0;
    u8 mSoftlockTimer = 0;

    ArchipelagoInfo* mInfo = nullptr;
};