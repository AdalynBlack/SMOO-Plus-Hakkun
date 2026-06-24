#include "server/archipelago/ArchipelagoConfigMenu.hpp"

#include <stdint.h>

#include "Layout/CommonVerticalList.h"
#include "Library/Layout/LayoutActionFunction.h"
#include "Library/Memory/HeapUtil.h"
#include "Library/Play/Layout/RollParts.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "server/archipelago/ArchipelagoInfo.h"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"

ArchipelagoConfigMenu::ArchipelagoConfigMenu() : GameModeConfigMenu() {
    mIPKeyboard = new Keyboard(15);
    if (mIPKeyboard) {
        mIPKeyboard->setHeaderText(u"Set Client IP Address");
        mIPKeyboard->setSubText(u"");
    }

    mHostNameKeyboard = new Keyboard(64);
    if (mHostNameKeyboard) {
        mHostNameKeyboard->setHeaderText(u"Set Archipelago Host Name");
        mHostNameKeyboard->setSubText(u"");
    }

    mPortKeyboard = new Keyboard(5);
    if (mPortKeyboard) {
        mPortKeyboard->setHeaderText(u"Set Archipelago Port");
        mPortKeyboard->setSubText(u"0 - 65535");
    }

    mSlotKeyboard = new Keyboard(64);
    if (mSlotKeyboard) {
        mSlotKeyboard->setHeaderText(u"Set Archipelago Slot Name");
        mSlotKeyboard->setSubText(u"");
    }

    mPasswordKeyboard = new Keyboard(64);
    if (mPasswordKeyboard) {
        mPasswordKeyboard->setHeaderText(u"Set Archipelago Password");
        mPasswordKeyboard->setSubText(u"Leave Blank for No Password");
    }
    // mPortKeyboard = new Keyboard(5);
    // if (mPortKeyboard) {
    //     mPortKeyboard->setHeaderText(u"Set Client Port");
    //     mPortKeyboard->setSubText(u"Default 1027");
    // }
}

void ArchipelagoConfigMenu::initMenu() {
    ArchipelagoInfo* curMode = GameModeManager::instance()->getInfo<ArchipelagoInfo>();
    StageSceneStateModConfig::setMenuItemBase(mList->mListPartsArr[1]);
    StageSceneStateModConfig::setMenuItemBase(mList->mListPartsArr[2]);
    StageSceneStateModConfig::setMenuItemBase(mList->mListPartsArr[3]);
    StageSceneStateModConfig::setMenuItemBase(mList->mListPartsArr[4]);
    StageSceneStateModConfig::setMenuItemBase(mList->mListPartsArr[5]);
    StageSceneStateModConfig::setMenuItemBase(mList->mListPartsArr[6]);
    StageSceneStateModConfig::setMenuItemCheck(mList->mListPartsArr[7]);
}

const sead::WFixedSafeString<0x200>* ArchipelagoConfigMenu::getStringData() {
    ArchipelagoInfo* info = GameModeManager::instance()->getInfo<ArchipelagoInfo>();

    mItems[0].copy(u"Client IP");
    mItems[1].copy(u"Reconnect to Client");
    mItems[2].copy(u"Archipelago Host Name");
    mItems[3].copy(u"Archipelago Port");
    mItems[4].copy(u"Archipelago Slot Name");
    mItems[5].copy(u"Archipelago Password");
    mItems[6].copy(u"Default Mode");

    if (info) {
        al::startAction(mList->mListPartsArr[7], info->mIsClientConnected == ArchipelagoState::CLIENT_CONNECTED ? "On" : "Off", "State");
    }

    return mItems.mBuffer;
}

GameModeConfigMenu::UpdateAction ArchipelagoConfigMenu::updateMenu(int selectIndex) {
    ArchipelagoInfo* curMode = GameModeManager::instance()->getInfo<ArchipelagoInfo>();

    if (!curMode) {
        return GameModeConfigMenu::UpdateAction::NOOP;
    }

    switch (selectIndex) {
    case 0: {
        // Set Client IP
        if (mIPKeyboard) {
            mIPKeyboard->openKeyboard(Client::getApClientIP(), [](nn::swkbd::KeyboardConfig& config) {
                config.keyboardMode = nn::swkbd::KeyboardMode::ModeASCII;
                config.textMaxLength = 15;
                config.textMinLength = 7;
                config.isUseUtf8 = true;
                config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
            });

            while (!mIPKeyboard->isThreadDone()) {
                nn::os::YieldThread();
            }

            if (!mIPKeyboard->isKeyboardCancelled()) {
                const char* result = mIPKeyboard->getResult();
                if (result && result[0] != '\0') {
                    // set APIP memember in client here
                    Client::setApClientIP(result);
                    Client::setConnectStatusMsg(u"Connecting to Client...");
                }
            }
        }
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    case 1: {
        Client::setConnectStatusMsg(u"Connecting to Client...");
        Client::instance()->startReconnectThread();
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    case 2: {
        // Set Host
        if (mHostNameKeyboard) {
            mHostNameKeyboard->openKeyboard(Client::getArchipelagoHost(), [](nn::swkbd::KeyboardConfig& config) {
                config.keyboardMode = nn::swkbd::KeyboardMode::ModeASCII;
                config.textMaxLength = 64;
                config.textMinLength = 0;
                config.isUseUtf8 = true;
                config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
            });

            while (!mHostNameKeyboard->isThreadDone()) {
                nn::os::YieldThread();
            }

            if (!mHostNameKeyboard->isKeyboardCancelled()) {
                const char* result = mHostNameKeyboard->getResult();
                if (result && result[0] != '\0') {
                    Client::setArchipelagoHost(result);
                    curMode->isNeedArchipelagoConnect = true;
                }
            }
        }
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    case 3: {
        // Set Archipelago Port
        if (mPortKeyboard) {
            mPortKeyboard->openKeyboard(intToCstr(Client::getArchipelagoPort()), [](nn::swkbd::KeyboardConfig& config) {
                config.keyboardMode = nn::swkbd::KeyboardMode::ModeASCII;
                config.textMaxLength = 5;
                config.textMinLength = 1;
                config.isUseUtf8 = true;
                config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
            });

            while (!mPortKeyboard->isThreadDone()) {
                nn::os::YieldThread();
            }

            if (!mPortKeyboard->isKeyboardCancelled()) {
                const char* result = mPortKeyboard->getResult();
                if (result && result[0] != '\0') {
                    Client::setArchipelagoPort(::atoi(result));
                    curMode->isNeedArchipelagoConnect = true;
                }
            }
        }
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    case 4: {
        // Set Archipelago Slot
        if (mSlotKeyboard) {
            mSlotKeyboard->openKeyboard(Client::getArchipelagoSlot(), [](nn::swkbd::KeyboardConfig& config) {
                config.keyboardMode = nn::swkbd::KeyboardMode::ModeASCII;
                config.textMaxLength = 64;
                config.textMinLength = 1;
                config.isUseUtf8 = true;
                config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
            });

            while (!mSlotKeyboard->isThreadDone()) {
                nn::os::YieldThread();
            }

            if (!mSlotKeyboard->isKeyboardCancelled()) {
                const char* result = mSlotKeyboard->getResult();
                if (result && result[0] != '\0') {
                    Client::setArchipelagoSlot(result);
                    curMode->isNeedArchipelagoConnect = true;
                }
            }
        }
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    case 5: {
        // Set Archipelago Password
        if (mPasswordKeyboard) {
            mPasswordKeyboard->openKeyboard(Client::getArchipelagoPassword(), [](nn::swkbd::KeyboardConfig& config) {
                config.keyboardMode = nn::swkbd::KeyboardMode::ModeASCII;
                config.textMaxLength = 64;
                config.textMinLength = 0;
                config.isUseUtf8 = true;
                config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
                config.passwordMode = nn::swkbd::PasswordMode::Hide;
            });

            while (!mPasswordKeyboard->isThreadDone()) {
                nn::os::YieldThread();
            }

            if (!mPasswordKeyboard->isKeyboardCancelled()) {
                const char* result = mPasswordKeyboard->getResult();
                if (result) {
                    Client::setArchipelagoPassword(result);
                    curMode->isNeedArchipelagoConnect = true;
                }
            }
        }
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    case 6: {
        Client::setDefaultGameMode(Client::getDefaultGameMode() == GameMode::ARCHIPELAGO ? GameMode::HIDEANDSEEK : GameMode::ARCHIPELAGO);
        al::startAction(mList->mListPartsArr[7], Client::getDefaultGameMode() == GameMode::ARCHIPELAGO ? "On" : "Off", "State");

        return GameModeConfigMenu::UpdateAction::REFRESH;
    }
    default:
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
}