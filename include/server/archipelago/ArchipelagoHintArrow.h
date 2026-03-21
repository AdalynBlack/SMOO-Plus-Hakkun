#pragma once

#include "server/archipelago/ArchipelagoInfo.h"
#include "server/gamemode/GameModeHintArrow.h"

class ArchipelagoHintArrow : public GameModeHintArrow {
public:
    ArchipelagoHintArrow(const char* name);
    void initAfterPlacement(void) override;

protected:
    bool shouldBeVisible() override;
    void setupMaterials() override;

private:
    ArchipelagoInfo* mInfo = nullptr;
};