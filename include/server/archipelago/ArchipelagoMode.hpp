#pragma once

#include "al/Library/Camera/CameraTicket.h"

#include "game/Layout/ShopLayoutInfo.h"
#include "game/MapObj/CapMessageShowInfo.h"
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

enum CheckType {
    SentCheck = -11,
    ShopMoonScout = -10,
    MoonRockScout = -9,
    StickerScout = -8,
    SouvenirScout = -7,
    CapScout = -6,
    ClothesScout = -5,
    LifeUpHeart = -4,
    LifeHeart = -3,
    Coins = -2,
    Moon = -1,
    Clothes = 0,
    Cap = 1,
    Souvenir = 2,
    Sticker = 3,
    RegionalCoin = 4,
    Capture = 5,
    MoonRock = 6,
    HealthUpgrade = 7,
    WalletUpgrade = 8
};

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

    void addScoutedOutfit(int index);
    bool hasScoutedOutfit(int index);

    void addScoutedSticker(int index);
    bool hasScoutedSticker(int index);

    void addScoutedSouvenir(int index);
    bool hasScoutedSouvenir(int index);

    void addScoutedShopMoon(int index);
    bool hasScoutedShopMoon(int index);

    bool hasScoutedItem(int type, int index);
    void addScoutedItem(int type, int index);

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

    void setDefeatedBowserCloud(bool value) { mDefeatedBowserInCloud = value; };
    bool getDefeatedBowserCloud() { return mDefeatedBowserInCloud; };
    void setDefeatedKlepto(bool value) { mDefeatedKlepto = value; };
    bool getDefeatedKlepto() { return mDefeatedKlepto; };

    void enqueueCappyMessage(cappyMessage message);

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

    // ===== Talkatoo% mode =====
    // When enabled, Talkatoo's speech bubble names AP-pool moons drawn from
    // the per-kingdom named set (populated by markMoonNamed), and collecting
    // a moon that has NOT been named is silently rejected: the get-cinematic
    // plays cosmetically, sendMoonCheck is suppressed, and the cutscene's
    // title pane shows "Blocked by Talkatoo!". In smoo-plus-hakkun's
    // virtualization model, blocked moons remain re-collectible on stage
    // re-entry because Orig never flips the underlying GameDataFile bit in
    // AP mode (see sendShinePacketHook in main.cpp).
    //
    // The named set is bridge-populated via markMoonNamed. Until at least one
    // moon has been marked named, Talkatoo% mode is effectively unplayable
    // — that's the load-bearing wire contract the server side must satisfy
    // before flipping this flag on for a player.
    void setTalkatooMode(bool value) { mTalkatooMode = value; }
    bool getTalkatooMode() const { return mTalkatooMode; }
    void markMoonNamed(int uid);
    void clearNamedMoons();
    bool isMoonNamed(int uid) const;

    // Hook callback: pick a moon name Talkatoo should speak instead of his
    // vanilla pick. Fills `out` with up to `out_cap-1` ASCII bytes + NUL and
    // returns true; returns false if no substitute is available (caller falls
    // back to vanilla speech). The hook then widens `out` into its own static
    // UTF-16 buffer rotation — ArchipelagoMode does NOT own the lifetime of
    // the char16_t* passed to SMO.
    //
    // STUB: current implementation rotates through "Power Moon #N" probes so
    // bring-up is testable without a wire-format change. Replace with a real
    // per-(world_id, named-but-uncollected) pick once the server side ships
    // the named-moon list to the mod.
    bool chooseTalkatooSpokenUtf8(int world_id, int index, char* out, u32 out_cap);
    void setConnectInitFlag(bool value) { mIsConnectInit = value; };
    bool getConnectInitFlag() { return mIsConnectInit; };
    void setFirstConnectFlag(bool value) { mIsFirstConnect = value; };
    bool getFirstConnectFlag() { return mIsFirstConnect; };
    void setCurWorldShineList(int worldId) { mCurWorldShineList = worldId; };
    int getCurWorldShineList() { return mCurWorldShineList; };
    void setRelativeWorldCoinCollect(int worldId) { mRelativeWorldCoinCollect = worldId; };
    void setRelativeWorldCoinCollect(const char* stageName);
    int getRelativeWorldCoinCollect() { return mRelativeWorldCoinCollect; };
    void setIsNeedUpdateCounter(bool value) { mIsNeedUpdateCounter = value; };
    bool getIsNeedUpdateCounter() { return mIsNeedUpdateCounter; };
    void registerStoryShine(Shine* shine) { mStoryShineArray.pushBack(shine); };
    Shine* getStoryShine(int index) { return mStoryShineArray[index]; };
    void setGoal(u8 value) { mGoal = value; };
    u8 getGoal() { return mGoal; };

    void setGameName(int index, const char* name);
    void setSlotName(int index, const char* name);
    void setItemName(int index, const char* name);
    void setCappySlotName(const char* name) { mSlotNames[148 + mCurrentCappyQueue] = name; };
    void setCappyItemName(const char* name) { mItemNames[148 + mCurrentCappyQueue] = name; };

    void setShineTextReplacement(int index, replaceText replace);
    void setShineColors(int index, u8 replace);
    void setClothesTextReplacement(int index, shopReplaceText replace);
    void setCapTextReplacement(int index, shopReplaceText replace);
    void setRegionalTextReplacement(int index, shopReplaceText replace);
    void setShopMoonTextReplacement(shopReplaceText replace);
    void setOverWorldStageConnection(int index, stageConnection replace);
    void setSubAreaStageConnection(int index, stageConnection replace);

    const char* getShineReplacementText();
    int getShineColor(Shine* curShine);
    const char16_t* getShopReplacementText(const char* fileName, const char* key);

    void setDying(bool value);
    void setApDeath(bool value);
    bool isDying() { return mDying; }
    bool isApDeath() { return mApDeath; }

    const char* getLastERStageId() { return mLastERStageId.cstr(); };
    const char* getLastERStageName() { return mLastERStageName.cstr(); };
    ChangeStageInfo* getLastERTransition();

    ChangeStageInfo* handleER(const ChangeStageInfo* info);

    bool isTargetAlive();
    bool trySetHintTargetValid();

    void clearArrays();
    void clearCollectibles();
    void clearScenarios();

    // ===== Archipeligo Check Senders =====
    void sendMoonCheck(int uid);
    void sendShopCheck(const ShopItem::ItemInfo* itemInfo);
    void sendRegionalCoinCheck(const char* objId, const char* stageName);
    void sendCaptureCheck(const char* hackName);

    PlayerActorHakoniwa* getPlayerActorHakoniwa();  // Returns nullptr if the player is not a PlayerActorHakoniwa

    void setWipeHolder(al::WipeHolder* wipe) { mWipeHolder = wipe; };  // Called with HakoniwaSequence hook, wipe used in recovery event

    // ===== Cappy Messenger — in-game speech-bubble notifications =====
    // enqueueCappyMessage queues a UTF-8 string (truncated at kCappyTextCap
    // bytes) for display via the existing rs::tryShowCapMessagePriorityLow
    // pipeline. tryPumpCappyMessage is called once per frame from update().
    //
    // lookupCappyMessageSubstitution is consulted by the hooked al::*Message
    // accessors (isExistLabelInSystemMessage / getSystemMessageString /
    // isExistLabelIn­StageMessage / getStageMessageString): when CapMessageLayout
    // asks for kArchipelagoCappyLabel and we currently have a buffer ready,
    // we substitute our own char16_t* so SMO's native bubble pipeline renders
    // our text with no further plumbing.
    //
    // setCappyRsCalls is invoked from main.cpp::hkMain right after
    // hk::ro::lookupSymbol resolves the two rs:: entry points. Until both are
    // non-null, tryPumpCappyMessage is a no-op and queued entries accumulate
    // (capped at kCappyQueueCap).
    // void enqueueCappyMessage(const char* utf8_text);
    const char16_t* lookupCappyMessageSubstitution(const char* label) const;
    // STATIC because main.cpp::hkMain installs these at module init, before
    // any ArchipelagoMode instance is created.

    // Magic label CapMessageLayout queries when our enqueue is active. Keep
    // this distinctive — any vanilla MSBT key collision would route Nintendo's
    // own bubbles through our substitution and corrupt them.
    static constexpr const char* kArchipelagoCappyLabel = "ArchipelagoCappyMsg";

    // ===== Archipelago Utility Methods =====
    void sendStage(GameDataHolderWriter writer, const ChangeStageInfo* stageInfo);
    void sendBack();
    void isSubArea(GameDataHolderAccessor accessor, bool* isInSubArea, sead::FixedSafeString<32> stageId);
    void getCustomStageId(GameDataHolderAccessor accessor, const ChangeStageInfo* info, sead::FixedSafeString<64>* stageId);
    void correctCustomStageId(sead::FixedSafeString<64>* toStageId);
    int getNumGotShines();
    int getNumCoinCollect();
    void handleDeathLink(PlayerActorBase* playerBase, PlayerActorHakoniwa* playerHakoniwa, GameDataHolderWriter writer);
    void handleCaptureSanity(PlayerActorBase* playerBase, GameDataHolderAccessor accessor);
    void getNearestRegional(StageScene* stageScene, PlayerActorBase* playerBase);
    void handleSoftLocks(GameDataHolderAccessor accessor, GameDataHolderWriter writer);
    void updateCounter(PlayerActorBase* playerBase, GameDataHolderAccessor accessor);
    bool infoMenu();
    int getRelativeWorldCoinCollectCheckGotNum(GameDataHolderAccessor accessor);
    void calculateShineScenarios();
    int isMoonRockScenario(int worldId);
    int getSubAreaScenario(const char* toStageName);
    bool tryShowCappyMessage(StageScene* curScene);
    void buildCappyMessage();

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

    // Goal
    u8 mGoal = -1;

    // shine pay counts
    sead::SafeArray<int, 17> mWorldPayCounts;
    bool mDeathLinkEnabled = false;
    bool mCapturesEnabled = false;
    bool mIsRecordCapture = false;
    bool mIsEntranceRandomizationEnabled = false;
    bool mIsConnectInit = false;
    bool mIsFirstConnect = true;

    // ===== Talkatoo% state =====
    // mNamedShines mirrors collectedShines' packed-bitmap shape (1 bit per
    // uid; 148 bytes covers uid 0..1183 which is more than the apworld's
    // current max of 1166). bridge-populated via markMoonNamed; consulted by
    // sendMoonCheck's Talkatoo% block and by TalkatooSpeechHook's substitute
    // path.
    bool mTalkatooMode = false;
    sead::SafeArray<u8, 148> mNamedShines;
    // Consumed-on-read flag. sendMoonCheck sets this when blocking, then the
    // existing setShineLabel pipeline picks it up via getShineReplacementText
    // and emits "Blocked by Talkatoo!" once before clearing.
    bool mIsTalkatooBlockedLabelPending = false;
    sead::SafeArray<int, 17> mWorldScenarios;
    bool mDying = false;
    bool mApDeath = false;
    int mCheckIndex = 0;

    // List of 37 ints to track which shine's have been grabbed
    sead::SafeArray<u8, 148> mCollectedShines;

    // List of 11 u8s for tracking which caps and clothes have been grabbed
    sead::SafeArray<u8, 11> mCollectedOutfits;

    // List of 3 u8s for tracking which stickers have been grabbed
    sead::SafeArray<u8, 3> mCollectedStickers;

    // List of 4 u8s for tracking which souvenirs have been grabbed
    sead::SafeArray<u8, 4> mCollectedSouvenirs;

    // List of 11 u8s for tracking which caps and clothes have been scouted
    sead::SafeArray<u8, 11> mScoutedOutfits;

    // List of 3 u8s for tracking which stickers have been scouted
    sead::SafeArray<u8, 3> mScoutedStickers;

    // List of 4 u8s for tracking which souvenirs have been scouted
    sead::SafeArray<u8, 4> mScoutedSouvenirs;

    // List of 4 u8s for tracking which souvenirs have been scouted
    sead::SafeArray<u8, 2> mScoutedShopMoons;

    // List of 7 u8s for tracking which captures have been grabbed
    sead::SafeArray<u8, 7> mCollectedCaptures;
    sead::SafeArray<u8, 7> mCheckedCaptures;

    // List of 7 u8s for tracking which captures have been grabbed l trailing byte buffer
    sead::SafeArray<u8, 126> mCollectedRegionals;

    // List of 3 u8s for tracking which moon rocks have been collected
    sead::SafeArray<u8, 3> mCollectedMoonRocks;

    // List of 3 u8s for tracking which moon rocks have been scouted
    sead::SafeArray<u8, 3> mScoutedMoonRocks;

    // Moon Text Replacement Handling
    int mRecentShineHintIndex = 0;
    sead::SafeArray<replaceText, 100> shineTextReplacements;

    // Moon Color Replacement
    sead::SafeArray<s8, 1170> shineColors;

    // Moon model override
    // For when custom models and the like are added
    // sead::SafeArray<s8, 1170> mModelType;

    // 17 caps
    sead::SafeArray<shopReplaceText, 17> mShopCapTextReplacements;
    // 20 outfits
    sead::SafeArray<shopReplaceText, 20> mShopClothTextReplacements;
    // 9 slots to correspond with the maximum regional items in one shop
    sead::SafeArray<shopReplaceText, 9> mShopRegionalTextReplacements;
    // Offsets Regional Text Replacement index bu the number of Caps in a shop
    // This guarantees replacements are alligned with the correct item
    int mRegionalOffset = 0;
    // One shop moon slot per world
    shopReplaceText mShopMoonTextReplacements;
    // coin cap + outfit = 37 indexes  0 - 36
    // 46 with regionals 37 - 45
    // 47 with moon slot 46
    sead::SafeArray<sead::FixedSafeString<APNAMESIZE>, 47> mGameNames;
    // moon data 47 - 146
    // moon rock 147
    // cappy message parts 148 - 167
    // coin shop item and slot names would be treated as common due to there always available nature
    sead::SafeArray<sead::FixedSafeString<APNAMESIZE>, 168> mSlotNames;
    sead::SafeArray<sead::FixedSafeString<APNAMESIZE>, 168> mItemNames;

    // need duplicates of theese lists in player.py for tracking current cached names state might not be needed

    // this could free up space in a kingdoms shine replace space
    // leaving more room for caching other data
    // could be taken advantage of using the player.py parallel

    // additional entries in slot and item will be required for achievements
    // although they can be allieviated by just overwriting the shine
    // replace data with them while only in the castle sub area
    // just without removing the replace for the sub area moons

    // simplify shopReplaceText using replaceText

    int lastShopMoonReplaceIndex = -1;

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

    sead::FixedSafeString<128> mLastERStageId;
    sead::FixedSafeString<128> mLastERStageName;

    sead::FixedSafeString<128> mLastExitStageId;
    sead::FixedSafeString<128> mLastExitStageName;

    // Array for storing all story shines and multi moons in a given stage
    // For activating onSwitchGet behaviors when loading into a stage
    sead::PtrArray<Shine> mStoryShineArray;

    // World index used by shine list total count fetching
    int mCurWorldShineList = 0;

    // The world relative to the type of
    // regionals coins found in a stage
    int mRelativeWorldCoinCollect = -1;

    bool mDefeatedBowserInCloud = false;
    bool mDefeatedKlepto = false;

    static const int kCappyMessageQueueSize = 20;
    sead::SafeArray<cappyMessage, kCappyMessageQueueSize> mCappyMessages;
    int mCurrentCappyMessage = 0;
    int mCurrentCappyQueue = 0;
    bool mIsCappyMessageActive = false;
    int mCappyMessageFrameTimer = 0;
    int mCappyCheckForMessageTimer = 0;

    sead::WFixedSafeString<APNAMESIZE * 3> mSafeCappyBuffer;
    bool mCappyBufferInUse = false;
};