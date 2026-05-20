#pragma once

#include "sead/container/seadSafeArray.h"

#include "Keyboard.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"

// Forward declaration
struct ArchipelagoInfo;

class ArchipelagoConfigMenu : public GameModeConfigMenu, public sead::IDisposer {
public:
    ArchipelagoConfigMenu();
    ~ArchipelagoConfigMenu() override = default;

    const sead::WFixedSafeString<0x200>* getStringData() override;
    GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

    const int getMenuSize() override { return 7; }  // Fixed size for now

    void initMenu() override;

private:
    static constexpr int mItemCount = 7;
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount> mItems;
    Keyboard* mIPKeyboard = nullptr;
    Keyboard* mHostNameKeyboard = nullptr;
    Keyboard* mPortKeyboard = nullptr;
    Keyboard* mSlotKeyboard = nullptr;
    Keyboard* mPasswordKeyboard = nullptr;
};