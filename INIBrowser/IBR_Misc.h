#pragma once
#include "IBFront.h"
#include "FromEngine/RFBump.h"
#include "IBSave.h"
#include "IBG_InputType_Defines.h"
#include "IBR_IniLine.h"
#include "IBR_Project.h"
#include <any>


namespace IBR_RealCenter
{
    extern ImVec2 Center;
    extern dImVec2 FixedUL, WorkSpaceUL, WorkSpaceDR;
    bool Update();
}

namespace IBR_UICondition
{
    extern int CurrentScreenWidth, CurrentScreenHeight;
    extern bool MenuChangeShow;
    extern bool MenuCollapse;
    extern std::wstring WindowTitle;
    bool UpdateWindowTitle();
}

//按钮有特别的样式
struct IBR_MainMenu
{
    struct _Item
    {
        std::function<bool()> Button;
        std::function<void()> Menu;
    };
    std::vector<_Item> ItemList;
    IBR_MainMenu() {}
    IBR_MainMenu(const std::vector<_Item>& v) :ItemList(v) {}
    void RenderUIBar();
    void RenderUIMenu();
    void ChooseMenu(size_t S);
private:
    size_t Choice{ 0 };
public:
    size_t GetMenuItem() { return Choice; }
};
extern IBR_MainMenu IBR_Inst_Menu;
#define MenuItemID_FILE 0
#define MenuItemID_VIEW 1
#define MenuItemID_LIST 2
#define MenuItemID_EDIT 3
#define MenuItemID_SETTING 4
#define MenuItemID_ABOUT 5
#define MenuItemID_DEBUG 6



struct IBG_UndoStack
{
    struct _Item
    {
        _TEXT_ANSI std::string Id;
        std::function<void()> UndoAction, RedoAction;
        std::function<std::any()> Extra;
    };
    std::vector<_Item> Stack;
    //直接赋值得同时改Cursor
    int Cursor{ -1 };
    bool Undo();
    bool Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    void SomethingShouldBeHere();
    void Release();
    void Push(const _Item& a);
    void RenderUI();
    void Clear();
    _Item* Top();
};
extern IBG_UndoStack IBG_Undo;

namespace IBR_ProjectManager
{
    bool IsOpen();
    void OpenAction();
    void OpenRecentAction(const std::wstring& Path);
    void CreateAction();
    void CloseAction();
    void SaveAction();
    void SaveAsAction();
    void SaveOptAction();
    void ProjOpen_CreateAction();
    void ProjOpen_OpenAction();
    void ProjOpen_OpenRecentAction(const std::wstring& Path);
    void OpenRecentOptAction(const std::wstring& Path);
    void OutputAction();
    void AutoOutputAction();
    void OutputOnSaveAction();
    void OnDropFile(GLFWwindow* window, int argc, const char** argv);
    void ProjActionByKey();
};

namespace IBR_FullView
{
    extern ImVec2 EqCenter;
    extern float Ratio;
    extern ImVec2 ViewSize;
    extern ImVec2 CurrentEqMax;
    const float RatioMax = 200.0, RatioMin = 25.0;
    void ChangeOffsetPos(ImVec2 ClickRel);
    int EqXRange(const ImVec2& V);
    int EqYRange(const ImVec2& V);
    void EqPosFixRange(ImVec2& V);
    bool EqPosInRange(ImVec2 V);
    void RenderUI();
    ImVec2 GetEqMin();
    ImVec2 GetEqMax();
    ImVec2 GetDefaultEqMax();
    void UpdateCurrentEqMax();
}

struct IBB_ClipBoardData;
namespace IBR_WorkSpace
{
    extern ImVec2 EqCenterPrev, ReCenterPrev, DragStartEqMouse, DragCurEqMouse, InitialMassCenter;
    extern float RatioPrev;
    extern int UpdatePrevResult;
    extern float NewRatio;
    extern bool NeedChangeRatio, InitHolding, ShowRegName;
    extern bool IsBgDragging, HoldingModules, IsMassSelecting;
    extern std::vector<ModuleID_t> MassTarget;
    //包含了被缩合的块
    extern std::vector<ModuleID_t> MassTargetExtended;
    extern ImVec4 TempWbg;
    extern bool LastOperateOnText, OperateOnText;
    extern IBR_Section MouseOverSection;
    extern IBR_SectionData* MouseOverSecData;
    extern bool CurOnRender_Clicked;
    extern ModuleID_t CurOnRender_ID;
    extern IBR_SectionData* CurOnRender;

    void UpdatePrev();
    void  _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE UpdatePrevII();
    void  _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE RenderUI();
    enum _UpdatePrev
    {
        _UpdatePrev_None = 0,
        _UpdatePrev_Ratio = 1,
        _UpdatePrev_EqCenter = 2
    };
    inline ImVec2 EqPosToRePos(ImVec2 EqPos) { return (EqPos - IBR_FullView::EqCenter) * IBR_FullView::Ratio + IBR_RealCenter::Center; }
    inline ImVec2 RePosToEqPos(ImVec2 RePos) { return (RePos - IBR_RealCenter::Center) / IBR_FullView::Ratio + IBR_FullView::EqCenter; }
    bool InWorkSpace(ImVec2 RePos);
    void ProcessBackgroundOpr();
    void UpdateNewRatio();
    void Close();

    bool SelectedAllIgnored();
    void IgnoreSelected();
    void NoIgnoreSelected();
    void DeleteSelected();
    void CopySelected();
    void DuplicateSelected();
    void CutSelected();
    void Paste();
    void GenerateClipDataFromIDs(IBB_ClipBoardData& ClipData, const std::vector<ModuleID_t>& IDs);
    void MassSelect(const std::vector<ModuleID_t>& Target);
    void SelectAll();
    void OpenRightClick();
    void OutputSelectedImpl(const char* IPath, const char* IDescShort, const char* IDescLong);
    void OutputSelected();
    void ComposeSelected();

    dImVec2 GetMassCenter(const std::vector<ModuleID_t>& Target);
}

namespace IBR_EditFrame
{
    extern IBR_Section CurSection;
    extern ModuleID_t PrevId;
    extern bool Empty;
    extern std::unordered_map<StrPoolID, SidebarLine> EditLines;
    void SetActive(ModuleID_t id);
    void ActivateAndEdit(ModuleID_t id, bool TextMode);
    void RenderUI();
    void Clear();
    void SwitchToText();
};

namespace IBR_Color
{
    extern ImColor BackgroundColor;
    extern ImColor ViewFocusWindowColor;
    extern ImColor FocusWindowColor;
    extern ImColor TouchEdgeColor;
    extern ImColor CheckMarkColor;
    extern ImColor ClipFrameLineColor;
    extern ImColor CenterCrossColor;
    extern ImColor ForegroundCoverColor;
    extern ImColor ForegroundAltColor;
    extern ImColor ForegroundMarkColor;
    extern ImColor LegalLineColor;
    extern ImColor LinkingLineColor;
    extern ImColor IllegalLineColor;
    extern ImColor ErrorTextColor;
    extern ImColor FocusLineColor;
    extern ImColor FrozenSecColor;
    extern ImColor HiddenSecColor;
    extern ImColor FrozenMaskColor;

    void StyleLight();
    void StyleDark();
    void LoadLight(JsonObject Obj);
    void LoadDark(JsonObject Obj);
}

