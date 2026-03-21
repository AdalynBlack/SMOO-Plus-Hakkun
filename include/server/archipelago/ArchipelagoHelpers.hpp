#pragma once

#include "game/Player/PlayerActorHakoniwa.h"

#include "types.h"

__attribute__((used)) static const char* costumeNamesByCheckId[] = {
    "Mario",          "MarioTailCoat",  "MarioPrimitiveMan", "MarioPoncho",       "MarioGunman",     "MarioSwimwear",   "MarioExplorer",     "MarioScientist",
    "MarioPilot",     "MarioMaker",     "MarioGolf",         "MarioSnowSuit",     "MarioAloha",      "MarioSailor",     "MarioCook",         "MarioPainter",
    "MarioArmor",     "MarioHappi",     "MarioSpaceSuit",    "Mario64",           "MarioShopman",    "MarioNew3DS",     "MarioMechanic",     "MarioSuit",
    "MarioPirate",    "MarioClown",     "MarioFootball",     "MarioColorClassic", "MarioColorLuigi", "MarioColorWario", "MarioColorWaluigi", "MarioColorGold",
    "MarioDoctor",    "MarioDiddyKong", "MarioKoopa",        "MarioPeach",        "Mario64Metal",    "MarioKing",       "MarioTuxedo",       "MarioCaptain",
    "MarioUnderwear", "MarioHakama",    "MarioBone",         "MarioInvisible"};

__attribute__((used)) static const char* stickerNames[] = {
    "StickerCap",  "StickerWaterfall", "StickerSand", "StickerLake", "StickerForest",     "StickerClash",     "StickerCity",       "StickerSnow",
    "StickerSea",  "StickerLava",      "StickerSky",  "StickerMoon", "StickerPeachDokan", "StickerPeachCoin", "StickerPeachBlock", "StickerPeachBlockQuestion",
    "StickerPeach"};

__attribute__((used)) static const char* souvenirNames[] = {
    "SouvenirHat1",  "SouvenirHat2",    "SouvenirFall1",   "SouvenirFall2",  "SouvenirSand1",  "SouvenirSand2", "SouvenirLake1",
    "SouvenirLake2", "SouvenirForest1", "SouvenirForest2", "SouvenirCrash1", "SouvenirCrash2", "SouvenirCity1", "SouvenirCity2",
    "SouvenirSnow1", "SouvenirSnow2",   "SouvenirSea1",    "SouvenirSea2",   "SouvenirLava1",  "SouvenirLava2", "SouvenirSky1",
    "SouvenirSky2",  "SouvenirMoon1",   "SouvenirMoon2",   "SouvenirPeach1", "SouvenirPeach2"};

__attribute__((used)) static const char* moonItemNames[] = {
    "MoonCity",       // 101
    "MoonForest",     // 138
    "MoonWaterfall",  // 211
    "MoonCap",        // 230
    "MoonLava",       // 294
    "MoonSky",        // 360
    "MoonClash",      // 398
    "MoonLake",       // 430
    "MoonSea",        // 460
    "MoonSand",       // 565
    "MoonSnow",       // 868
    "MoonPeach",      // 933
    "MoonMoon"        // 1157
};

__attribute__((used)) static const char* captureListNames[] = {
    "Frog",
    "ElectricWire",  // Spark pylon
    "KuriboWing",    // Paragoomba
    "Wanwan",        // Chain Chomp
    "WanwanBig",     // Big Chain Chomp
    "BreedaWanwan",  // Broode's Chain Chomp
    "TRex",
    "Fukankun",  // Binoculars
    "Killer",    // Bullet Bill
    "Megane",    // Moe-eye
    "Cactus",
    "Kuribo",           // Goomba
    "BossKnuckleHand",  // Knucklotec's Fist
    "BazookaElectric",  // Mini Rocket
    "Kakku",            // Glydon
    "JugemFishing",     // Lakitu
    "Fastener",         // Zipper
    "Pukupuku",         // Cheep Cheep
    "GotogotonLake",    // Puzzle Part (Lake Kingdom)
    "PackunPoison",     // Poison Pirana Plant
    "Senobi",           // Uproot
    "FireBros",         // Fire Bro
    "Tank",             // Sherm
    "Gamane",           // Coin Coffer
    "Tree",
    "RockForest",                // Boulder
    "FukuwaraiFacePartsKuribo",  // Gooma Picture Match Piece
    "Imomu",                     // Tropical Wiggler
    "Guidepost",                 // Pole
    "Manhole",
    "Car",                       // Taxi
    "Radicon",                   // RC Car
    "Byugo",                     // Ty-foo
    "Yukimaru",                  // Shiverian Racer
    "PukupukuSnow",              // Cheep Cheep (Snow Kingdom)
    "Hosui",                     // Gushen
    "Bubble",                    // Lava Bubble
    "HackFork",                  // Volbonan
    "HammerBros",                // Hammer and Pan Bros
    "CarryMeat",                 // Meat
    "PackunFire",                // Fire Pirana Plant
    "Tsukkun",                   // Pokio
    "Statue",                    // Jizo
    "StatueKoopa",               // Bowser Statue
    "KaronWing",                 // Para Bones
    "KillerMagnum",              // Bonsai Bill
    "Bull",                      // Chargin' Chuck
    "Koopa",                     // Bowser
    "AnagramAlphabetCharacter",  // Letter
    "GotogotonCity",             // Puzzle Part (Metro Kingdom)
    "FukuwaraiFacePartsMario",   // Mario Picture Match Piece
    "Yoshi",
};

struct stageConnection {
    // short fromStageIdIndex;
    // short fromStageNameIndex;
    short toStageIdIndex;
    short toStageNameIndex;
};

struct shineReplaceText {
    s8 itemType;
    u8 shineItemNameIndex;
    // s8 color;
};

struct shopReplaceText {
    u8 gameIndex;
    u8 slotIndex;
    u8 apItemNameIndex;
    u8 itemClassification;
};

bool isInApCostumeList(const char* costumeName);
int getIndexApCostumeList(const char* costumeName);

int getIndexStickerList(const char* stickerName);
int getIndexSouvenirList(const char* souvenirName);
int getIndexCaptureList(const char* captureName);
int getIndexMoonItemList(const char* moonItemName);

const char* intToCstr(int number);