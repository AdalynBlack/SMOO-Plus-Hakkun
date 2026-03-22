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
}

const sead::WFixedSafeString<0x200>* ArchipelagoConfigMenu::getStringData() {
    mItems[0].copy(u"Client IP");
    mItems[1].copy(u"Reconnect to Client");

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
                    // Client::restartConnection();
                }
            }
        }
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    case 1: {
        Client::setConnectStatusMsg(u"Connecting to Client...");
        Client::restartConnection();
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    default:
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
}