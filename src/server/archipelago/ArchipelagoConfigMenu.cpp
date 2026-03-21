#include "server/archipelago/ArchipelagoConfigMenu.hpp"

#include <stdint.h>

#include "server/archipelago/ArchipelagoInfo.h"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"

ArchipelagoConfigMenu::ArchipelagoConfigMenu() : GameModeConfigMenu() {
    mIPKeyboard = new Keyboard(15);
    if (mIPKeyboard) {
        mIPKeyboard->setHeaderText(u"Set Client IP Address");
        mIPKeyboard->setSubText(u"This is the Local Address of the Computer the Client is running on.");
    }

    // mPortKeyboard = new Keyboard(5);
    // if (mPortKeyboard) {
    //     mPortKeyboard->setHeaderText(u"Set Client Port");
    //     mPortKeyboard->setSubText(u"Default 1027");
    // }
}

const sead::WFixedSafeString<0x200>* ArchipelagoConfigMenu::getStringData() {
    mItems[0].copy(u"Client IP");

    return mItems.mBuffer;
}

GameModeConfigMenu::UpdateAction ArchipelagoConfigMenu::updateMenu(int selectIndex) {
    ArchipelagoInfo* curMode = GameModeManager::instance()->getInfo<ArchipelagoInfo>();

    if (!curMode) {
        return GameModeConfigMenu::UpdateAction::NOOP;
    }

    switch (selectIndex) {
    case 0: {
        if (mIPKeyboard) {
            char buf[15];

            mIPKeyboard->openKeyboard(buf, [](nn::swkbd::KeyboardConfig& config) {
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
                }
            }
        }
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    // case 1: {
    //     if (mPortKeyboard) {
    //         curMode->mIsHostMode = true;

    //        uint8_t oldTime = curMode->mRoundLength;

    //        char buf[4];
    //        nn::util::SNPrintf(buf, 4, "%u", oldTime);

    //        mPortKeyboard->openKeyboard(buf, [](nn::swkbd::KeyboardConfig& config) {
    //            config.keyboardMode = nn::swkbd::KeyboardMode::ModeNumeric;
    //            config.textMaxLength = 5;
    //            config.textMinLength = 1;
    //            config.isUseUtf8 = true;
    //            config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
    //        });

    //        while (!mPortKeyboard->isThreadDone()) {
    //            nn::os::YieldThread();
    //        }

    //        if (!mPortKeyboard->isKeyboardCancelled()) {
    //            const char* result = mPortKeyboard->getResult();
    //            if (result && result[0] != '\0') {
    //                int newTime = atoi(result);
    //                if (newTime >= 2 && newTime <= 60) {
    //                    curMode->mRoundLength = (uint8_t)newTime;
    //                }
    //            }
    //        }
    //    }
    //    return GameModeConfigMenu::UpdateAction::NOOP;
    //}
    default:
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
}