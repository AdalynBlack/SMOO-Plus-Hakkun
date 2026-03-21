#include "server/archipelago/ArchipelagoHelpers.hpp"

#include "al/Library/Base/StringUtil.h"

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
// int getIndexStageIdList(const char* stageId) {
//    for (size_t i = 0; i < sizeof(changeStageIdList) / sizeof(changeStageIdList[0]); i++) {
//        if (al::isEqualString(changeStageIdList[i], stageId)) {
//            return i;
//        }
//    }
//    return -1;
//}
//
// int getIndexStageNameList(const char* stangeName) {
//    for (size_t i = 0; i < sizeof(changeStageNameList) / sizeof(changeStageNameList[0]); i++) {
//        if (al::isEqualString(changeStageNameList[i], stangeName)) {
//            return i;
//        }
//    }
//    return -1;
//}

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