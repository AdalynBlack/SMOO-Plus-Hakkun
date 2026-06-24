#include "server/archipelago/ArchipelagoHelpers.hpp"

#include "al/Library/Base/StringUtil.h"

#include "game/System/GameDataFunction.h"

#include "helpers.hpp"

bool isInApCostumeList(const char* costumeName) {
    for (size_t i = 0; i < sizeof(costumeNamesByCheckId) / sizeof(costumeNamesByCheckId[0]); i++) {
        if (al::isEqualString(costumeNamesByCheckId[i], costumeName)) {
            return true;
        }
    }
    return false;
}

int getIndexApCostumeList(const char* costumeName) {
    for (size_t i = 0; i < sizeof(costumeNamesByCheckId) / sizeof(costumeNamesByCheckId[0]); i++) {
        if (al::isEqualString(costumeNamesByCheckId[i], costumeName)) {
            return i;
        }
    }
    return -1;
}

int getIndexStickerList(const char* stickerName) {
    for (size_t i = 0; i < sizeof(stickerNames) / sizeof(stickerNames[0]); i++) {
        if (al::isEqualString(stickerNames[i], stickerName)) {
            return i;
        }
    }
    return -1;
}

int getIndexSouvenirList(const char* souvenirName) {
    for (size_t i = 0; i < sizeof(souvenirNames) / sizeof(souvenirNames[0]); i++) {
        if (al::isEqualString(souvenirNames[i], souvenirName)) {
            return i;
        }
    }
    return -1;
}

int getIndexCaptureList(const char* captureName) {
    for (size_t i = 0; i < sizeof(captureListNames) / sizeof(captureListNames[0]); i++) {
        if (al::isEqualString(captureListNames[i], captureName)) {
            return i;
        }
    }
    return -1;
}

int getIndexMoonItemList(const char* moonItem) {
    for (size_t i = 0; i < sizeof(moonItemNames) / sizeof(moonItemNames[0]); i++) {
        if (al::isEqualString(moonItemNames[i], moonItem)) {
            return i;
        }
    }
    return -1;
}

// Stage stuff for ER
int getIndexStageIdList(const char* stageId) {
    for (size_t i = 0; i < sizeof(stageIdList) / sizeof(stageIdList[0]); i++) {
        if (al::isEqualString(stageIdList[i], stageId)) {
            return i;
        }
    }
    return -1;
}

int getIndexStageNameList(const char* stageName) {
    for (size_t i = 0; i < sizeof(stageNameList) / sizeof(stageNameList[0]); i++) {
        if (al::isEqualString(stageNameList[i], stageName)) {
            return i;
        }
    }
    return -1;
}

int getIndexRegionalCoinStageList(const char* stageName) {
    for (size_t i = 0; i < sizeof(regionalCoinStages) / sizeof(regionalCoinStages[0]); i++) {
        if (al::isEqualString(regionalCoinStages[i], stageName)) {
            return i;
        }
    }
    return -1;
}

int getIndexRegionalCoinId(const char* stageName, const char* placementId) {
    int stageIndex = getIndexRegionalCoinStageList(stageName);
    if (stageIndex == -1) {
        return -2;
    }

    int totalIndex = 0;
    for (int k = 0; k < stageIndex; k++) {
        totalIndex += regionalCoinListLengths[k];
    }

    for (size_t i = 0; i < regionalCoinListLengths[stageIndex]; i++) {
        if (al::isEqualString(regionalCoinsByCheckId[stageIndex][i], placementId)) {
            return i + totalIndex;
        }
    }
    return -1;
}

const char* getWorldStageNameByRegionalCoinStageList(const char* stageName) {
    int indexLastHomeStage = 0;
    int index = -1;
    for (size_t i = 0; i < sizeof(regionalCoinStages) / sizeof(regionalCoinStages[0]); i++) {
        if (isPartOf(regionalCoinStages[i], "WorldHomeStage")) {
            indexLastHomeStage = i;
        }
        if (al::isEqualString(regionalCoinStages[i], stageName)) {
            index = i;
            break;
        }
    }

    if (index >= 0) {
        return regionalCoinStages[indexLastHomeStage];
    }

    return "CapWorldHomeStage";
}

int getIndexRegionalItemList(int worldId, const char* itemName) {
    if (worldId > GameDataFunction::getWorldIndexPeach())
        worldId = GameDataFunction::getWorldIndexPeach();
    if (worldId > GameDataFunction::getWorldIndexBoss())
        worldId -= 1;
    if (worldId > GameDataFunction::getWorldIndexCloud())
        worldId -= 1;

    for (size_t i = 0; i < regionalShopItemsSizes[worldId]; i++) {
        if (al::isEqualString(regionalShopItems[worldId][i], itemName)) {
            return i;
        }
    }
    return -1;
}

const char* intToCstr(int number) {
    sead::FixedSafeString<40> numberStr;
    numberStr = "";
    int trim = number;
    int magnitude = 1;
    while (number % (magnitude * 10) != number) {
        magnitude *= 10;
    }
    while (magnitude > 0) {
        numberStr.append(static_cast<char>(48 + trim / magnitude));
        trim %= magnitude;
        magnitude /= 10;
    }

    return numberStr.cstr();
}

const char16_t* utf8ToUtf16(const char* src) {
    sead::WFixedSafeString<APNAMESIZE> out = sead::WFixedSafeString<APNAMESIZE>();

    u32 i = 0;
    u32 o = 0;
    while (src[i] != '\0' && o + 1 < out.getBufferSize()) {
        const unsigned char b0 = static_cast<unsigned char>(src[i]);
        if (b0 < 0x80) {
            out.append(static_cast<char16_t>(b0));
            ++i;
        } else if ((b0 & 0xE0) == 0xC0 && src[i + 1] != '\0') {
            const unsigned char b1 = static_cast<unsigned char>(src[i + 1]);
            out.append(static_cast<char16_t>(((b0 & 0x1F) << 6) | (b1 & 0x3F)));
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0 && src[i + 1] != '\0' && src[i + 2] != '\0') {
            const unsigned char b1 = static_cast<unsigned char>(src[i + 1]);
            const unsigned char b2 = static_cast<unsigned char>(src[i + 2]);
            out.append(static_cast<char16_t>(((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F)));
            i += 3;
        } else {
            // Malformed lead byte — skip and continue rather than UB.
            ++i;
        }
    }

    return out.cstr();
}

void appendUtf8ToUtf16(const char* src, sead::WFixedSafeString<APNAMESIZE * 3>* dest) {
    sead::WFixedSafeString<APNAMESIZE> out = sead::WFixedSafeString<APNAMESIZE>();

    u32 i = 0;
    u32 o = 0;
    while (src[i] != '\0' && o + 1 < out.getBufferSize()) {
        const unsigned char b0 = static_cast<unsigned char>(src[i]);
        if (b0 < 0x80) {
            out.append(static_cast<char16_t>(b0));
            ++i;
        } else if ((b0 & 0xE0) == 0xC0 && src[i + 1] != '\0') {
            const unsigned char b1 = static_cast<unsigned char>(src[i + 1]);
            out.append(static_cast<char16_t>(((b0 & 0x1F) << 6) | (b1 & 0x3F)));
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0 && src[i + 1] != '\0' && src[i + 2] != '\0') {
            const unsigned char b1 = static_cast<unsigned char>(src[i + 1]);
            const unsigned char b2 = static_cast<unsigned char>(src[i + 2]);
            out.append(static_cast<char16_t>(((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F)));
            i += 3;
        } else {
            // Malformed lead byte — skip and continue rather than UB.
            ++i;
        }
    }

    dest->append(out.cstr());
}

const char16_t* getRegionalCoinIcon(int worldId) {
    sead::WFixedSafeString<APNAMESIZE> icon = sead::WFixedSafeString<8>();
    icon.append(0x000e);
    icon.append(0x0008);
    icon.append(regionalIcons1[worldId]);
    icon.append(0x0004);
    icon.append(0x0006);
    icon.append(regionalIcons2[worldId]);
    return icon.cstr();
}

void getColor(ProjectTextColors color, sead::WBufferedSafeString* str) {
    // sead::WFixedSafeString<APNAMESIZE> colorTag = sead::WFixedSafeString<8>();
    str->append(0x0000);
    str->append(0x000e);
    str->append(0x0003);
    str->append(0x0002);
    str->append(color);

    // return colorTag.cstr();
}