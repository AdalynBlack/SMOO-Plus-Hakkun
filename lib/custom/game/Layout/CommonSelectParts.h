#pragma once

namespace al {
class LayoutInitInfo;
class LayoutActor;
class EventFlowChoiceInfo;
}  // namespace al

class CommonSelectParts {
public:
    CommonSelectParts(char const*, al::LayoutActor*, al::LayoutInitInfo const&, int, bool);
    void activateAll();
    void startSelect2(char16_t const*, char16_t const*, int);
    void startSelectWithChoiceTable(char16_t const**, int, int);
    void startSelectWithChoiceTableWithoutPosAnim(char16_t const**, int, int);
    void startSelectWithChoiceInfo(al::EventFlowChoiceInfo const*);
    bool isActive() const;
    bool isDecideEnd() const;
    void setSelectPartsString(char16_t const*, int);
    void deactivate(int);
    void kill();
    void reset();
    void exeHide();
    void exeAppearBefore();
    void exeAppear();
    void exeAppearAfter();
    void exeAppearCursor();
    void exeSelect();
    void exeDecideParts();
    void exeDecideDeactiveParts();
    void exeDecide();
    void exeDecideAfter();
    void exeDecideEnd();
};