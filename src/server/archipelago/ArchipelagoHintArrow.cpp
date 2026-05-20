#include "server/archipelago/ArchipelagoHintArrow.h"

#include "al/Library/LiveActor/ActorModelFunction.h"

#include "server/gamemode/GameModeManager.hpp"

ArchipelagoHintArrow::ArchipelagoHintArrow(const char* name) : GameModeHintArrow(name) {}

void ArchipelagoHintArrow::initAfterPlacement(void) {
    GameModeHintArrow::initAfterPlacement();

    if (GameModeManager::instance()->isMode(GameMode::ARCHIPELAGO))
        mInfo = GameModeManager::instance()->getInfo<ArchipelagoInfo>();
}

void ArchipelagoHintArrow::setupMaterials() {
    al::hideMaterial(this, "BodyRedMT00");
    al::showMaterial(this, "BodyYellowMT00");
    al::showMaterial(this, "BodyBlueMT00");
}

bool ArchipelagoHintArrow::shouldBeVisible() {
    if (!mInfo)
        return false;

    bool isInArchipelagoMode = GameModeManager::instance()->isModeAndActive(GameMode::ARCHIPELAGO);
    bool isTargetAlive = isInArchipelagoMode && mInfo->mIsHintTargetValid;
    return isTargetAlive;
    // return false;
}