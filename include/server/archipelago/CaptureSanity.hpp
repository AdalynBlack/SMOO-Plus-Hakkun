#pragma once
#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "game/Player/PlayerHackKeeper.h"
#include "game/System/GameDataFunction.h"
#include "game/Util/Hack.h"

#include "server/archipelago/ArchipelagoMode.hpp"
#include "server/gamemode/GameModeManager.hpp"

// ===== Hack Data Replacement =====
// isExistInHackDictionary for capture tracking
static void onAddHack(GameDataHolderWriter writer, const char* hackName) {
    GameModeManager* manager = GameModeManager::instance();
    if (manager->isModeAndActive(GameMode::ARCHIPELAGO) && manager->getMode<ArchipelagoMode>()->getCapturesFlag()) {
        // Client::setMessage(2, hackName);
        manager->getMode<ArchipelagoMode>()->sendCaptureCheck(hackName);
        manager->getMode<ArchipelagoMode>()->setIsRecordCapture(true);
    } else {
        GameDataFunction::addHackDictionary(writer, hackName);
    }
}

static void canEndHack(al::LiveActor* actor) {
    if (actor != nullptr) {
        ((PlayerHackKeeper*)actor)->endHackStartDemo(actor);
    }
}