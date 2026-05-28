#pragma once

// #include "al/Library/Layout/LayoutActor.h"
#include "game/Layout/CommonSelectParts.h"

namespace al {
class LiveActor;
class LayoutInitInfo;
class MessageTagDataHolder;
class EventFlowChoiceInfo;
class LayoutActor;
}  // namespace al

class TalkMessage : al::LayoutActor {
public:
    TalkMessage(char const*);
    void initLayoutTalk(al::LayoutInitInfo const&, char const*);
    void initLayoutWithArchiveName(al::LayoutInitInfo const&, char const*, char const*);
    void initLayoutImportant(al::LayoutInitInfo const&, char const*);
    void initLayoutOver(al::LayoutInitInfo const&, char const*);
    void initLayoutForEventTalk(al::LayoutInitInfo const&);
    void initLayoutForEventImportant(al::LayoutInitInfo const&);
    void startForNpc(al::LiveActor const*, char16_t const*, char16_t const*, al::MessageTagDataHolder const*, bool);
    void reset();
    void startForSystem(char16_t const*, al::MessageTagDataHolder const*, bool);
    void end();
    bool isIconWait() const;
    void kill();
    bool isWait() const;
    void startSelectWithChoiceTable(char16_t const**, int, int);
    void startSelectWithChoiceInfo(al::EventFlowChoiceInfo const*);
    bool isSelectDecide() const;
    int getSelectedChoiceIndex() const;
    void exeAppear();
    void exeAppearWithText();
    void exeTextAnim();
    void exeIconAppearDelay();
    void exeIconAppear();
    void exeIconWait();
    void exeIconWaitTriggered();
    void exeIconPageNext();
    void exeIconPageNextAndPlayNextPage();
    void exeIconPageNextAndLoadNextMessage();
    void exeIconPageEnd();
    void exeWait();
    void exeEnd();
    void appear();
    void control();
    void startIconPageNext();

    void* _130 = 0;
    void* _138 = 0;
    void* _140 = 0;
    CommonSelectParts* mCommonSelectParts = nullptr;
    void* _150 = 0;
    void* _158 = 0;
    int _160 = -1;
    int _164 = 0;
    short _168 = 0;
};