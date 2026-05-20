#pragma once

#include "server/archipelago/ArchipelagoHelpers.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"

enum ArchipelagoState {  // Client connection state
    NOT_CONNECTED = 0,
    CLIENT_CONNECTED = 1
};

struct ArchipelagoInfo : GameModeInfoBase {
    ArchipelagoInfo() { mMode = GameMode::ARCHIPELAGO; }

    ArchipelagoState mIsClientConnected = ArchipelagoState::NOT_CONNECTED;
    bool isNeedArchipelagoConnect = true;
    bool mIsHintTargetValid = false;
    bool mIsDebugMode = false;
};