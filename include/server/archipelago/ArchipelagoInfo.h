#pragma once

#include "server/freeze/FreezeTagScore.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

enum ArchipelagoState {  // Client connection state
    NOT_CONNECTED = 0,
    CLIENT_CONNECTED = 1
};

struct ArchipelagoInfo : GameModeInfoBase {
    ArchipelagoInfo() { mMode = GameMode::ARCHIPELAGO; }

    ArchipelagoState mIsClientConnected = ArchipelagoState::NOT_CONNECTED;

    bool mIsDebugMode = false;
};