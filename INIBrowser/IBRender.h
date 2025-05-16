#pragma once
#include "IBFront.h"
#include "FromEngine/RFBump.h"
#include "IBSave.h"
#include <any>

extern int HintStayTimeMillis;
extern bool IsEnumHovered;

class IBR_Setting
{
public:
    const wchar_t* SettingName;
    void SetSettingName(const wchar_t* Name);
    void CallReadSetting();
    bool IsReadSettingComplete();
    void CallSaveSetting();//call every 5 second with [setting] window
    bool IsSaveSettingComplete();

    void RefreshSetting();
    void RenderUI();
};

struct IBR_InputManager
{
    static constexpr size_t InputSize = 5000;
    std::unique_ptr<char[]> Input;
    std::string ID;
    std::function<void(char*)> AfterInput;
    
    IBR_InputManager(const std::string& InitialText, const std::string& id, const std::function<void(char*)>& Fn);
    bool RenderUI();
    ~IBR_InputManager() = default;
};

struct IBR_IniLine
{
    std::shared_ptr<IBR_InputManager> Input;
    bool HasInput{ false }, UseInput{ false };
    struct InitType
    {
        std::string InitText;
        std::string ID;
        std::function<void(char*)> AfterInput;
    };
    bool NeedInit() { return HasInput && !Input; }
    void RenderUI(const std::string& Line, const std::string& Hint, const InitType* Init = nullptr);
    void CloseInput();
};

struct BufferedLine
{
    IBR_IniLine Edit;
    std::string Buffer;
    std::string Hint;
    bool Known;
    bool AltRes;
    bool IsAltBool;
    bool IsAltEnum;
    std::vector<std::string> Enum;
    std::vector<std::string> EnumValue;
};

struct ActiveLine
{
    IBR_IniLine Edit;
    std::string Buffer;
};

dImVec2 operator+(const dImVec2 a, const dImVec2 b);
dImVec2 operator-(const dImVec2 a, const dImVec2 b);
ImVec4 operator+(const ImVec4 a, const ImVec4 b);
dImVec2 operator/(const dImVec2 a, const double b);
dImVec2 operator*(const dImVec2 a, const double b);

int IBG_ExitProcess();
std::wstring RemoveSpec(std::wstring W);







struct IBR_Debug
{
    struct _UICond
    {
        bool LoopShow{ true }, OnceShow{ true };
    }UICond;
    struct _Data
    {
        bool Nope;
    }Data;

    std::vector<StdMessage>DebugVec, DebugVecOnce;
    void AddMsgCycle(const StdMessage& Msg);
    void ClearMsgCycle();
    void AddMsgOnce(const StdMessage& Msg);

    void DebugLoad();

    void RenderUI();
    void RenderUIOnce();
    void RenderOnWorkSpace();
};

#define _PROJ_CMD_UPDATE
#define _PROJ_CMD_CAN_UNDO
#define _PROJ_CMD_WRITE
#define _PROJ_CMD_READ
#define _PROJ_CMD_NOINTERRUPT //此操作不挂起后端线程
#define _PROJ_CMD_BACK_CONST

#define _CALL_CMD

#define _RETURN_BACK_DATA //使用相应返回值时请先RInterruptF锁住后端线程再读写



struct IBR_Section;
struct IBR_Project;
struct IBB_Section_Desc;
extern const IBB_Section_Desc IBB_Section_DescNull;

struct IBR_SectionData
{
    IBB_Section_Desc Desc;
    std::string DisplayName;
    ImVec2 EqPos, EqSize, EqDelta;
    bool Exists{ false };
    bool Hovered{ false };
    bool First{ true };
    bool IsOpen{ true };
    bool Dragging{ false };
    bool Ignore{ false };
    bool IsComment{ false };
    float FinalY{ 0.0f };
    float WidthFix{ 0.0f };
    std::unordered_map<std::string, ActiveLine> ActiveLines;
    std::shared_ptr<BufString> CommentEdit;
    ImVec2 ReOffset;
    //int UpdateResult;


    IBR_SectionData();
    IBR_SectionData(const IBB_Section_Desc& D);
    IBR_SectionData(const IBB_Section_Desc& D, std::string&& Name);

    void RenameDisplayImpl(const std::string& Name);
    void RenameRegisterImpl(const std::string& Name);
    void RenameDisplay();
    void RenameRegister();
    bool OnLineEdit(const std::string& Name, bool OnLink);
    void RenderUI();
    void RenderUI_TitleBar(bool& TriggeredRightMenu);
    void CopyToClipBoard();
};

struct ModuleClipData;

struct IBR_Section
{
    IBR_Project* Root;
    uint64_t ID;
    //改动其中存储内容应修改IBR_Project::GetSection

    
    _RETURN_BACK_DATA IBB_Section* _PROJ_CMD_READ GetBack() ;
    _RETURN_BACK_DATA const IBB_Section* _PROJ_CMD_READ GetBack() _PROJ_CMD_BACK_CONST const;
    _RETURN_BACK_DATA const IBB_Section* _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ GetBack_Unsafe() const;

    //若Sec不存在给Function传入nullptr
    template<typename T>
    T _PROJ_CMD_READ _PROJ_CMD_WRITE OperateBackData(const std::function<T(IBB_Section*)>& Function);
    /*template<typename T>
    T _PROJ_CMD_READ _PROJ_CMD_WRITE OperateBackDataWithUndo(
        const std::function<T(IBB_Section*)>& Undo,
        const std::function<T(IBB_Section*)>& Redo);*/

    const IBB_Section_Desc& _PROJ_CMD_NOINTERRUPT GetSectionDesc() _PROJ_CMD_BACK_CONST const;

    //此Sec是否存在
    bool _PROJ_CMD_READ HasBack() const;

    _RETURN_BACK_DATA IBB_VariableList* _PROJ_CMD_READ GetVarList() _PROJ_CMD_BACK_CONST const;
    IBB_VariableList _PROJ_CMD_READ GetVarListCopy() _PROJ_CMD_BACK_CONST const;
    IBB_VariableList _PROJ_CMD_READ GetVarListFullCopy(bool PrintExtraData) _PROJ_CMD_BACK_CONST const;
    bool _PROJ_CMD_READ GetClipData(ModuleClipData& Data, bool UsePosAsDelta) _PROJ_CMD_BACK_CONST;
    ImColor _PROJ_CMD_READ GetRegTypeColor() _PROJ_CMD_BACK_CONST const;

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE SetVarList(const IBB_VariableList& NewList);

    //不建议跨INI类型复制，除非你确定你在做什么，并且为字段设置正确的变量表，以符合模板的整体约定
    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DuplicateSection(const IBB_Section_Desc& NewDesc) _PROJ_CMD_BACK_CONST const;
    IBR_Section  _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DuplicateSectionAndBack(const IBB_Section_Desc& NewDesc) _PROJ_CMD_BACK_CONST const;

    //如果名字冲突则啥也不干并返回false
    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE Rename(const std::string& NewName);

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO Register(const std::string& Name, const std::string& IniName) _PROJ_CMD_BACK_CONST const;

    IBR_SectionData* _PROJ_CMD_READ _PROJ_CMD_NOINTERRUPT GetSectionData() const;//可能为空

    _TEXT_UTF8 std::string _PROJ_CMD_NOINTERRUPT GetDisplayName() const;

    IBB_Section_Desc* _PROJ_CMD_NOINTERRUPT GetDesc() const;

    bool _PROJ_CMD_NOINTERRUPT Dragging() const;

    void _PROJ_CMD_NOINTERRUPT SetReOffset(const ImVec2& Offset);
private:
    IBB_Section* GetBack_Inl() const;
};

struct SectionDragData
{
    IBB_Section_Desc Desc;
};

struct LineDragData
{
    IBB_Section_Desc Desc;
    std::string Line;
};

struct IBB_ModuleAlt;

struct IBR_Project
{
    typedef uint64_t id_t;
    id_t MaxID{ 0 };//TODO:你还能使用超过ULL_MAX个ID？要是真的如此那就修一修
    std::map<id_t, IBR_SectionData> IBR_SectionMap;
    std::map<IBB_Section_Desc, id_t> IBR_Rev_SectionMap;
    std::unordered_map<std::string, SectionDragData> IBR_SecDragMap;
    std::unordered_map<std::string, LineDragData> IBR_LineDragMap;
    std::unordered_map<std::string, std::string> CopyTransform;
    std::string DragConditionText;
    std::string DragConditionTextAlt;

    struct _Plink
    {
        ImVec2 BeginR;
        IBB_Section_Desc Dest;
        ImU32 Color;
        bool IsSelfLinked;
        bool IsSrcDragging;
    };
    std::vector<_Plink> LinkList;

    std::optional<std::vector<id_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE AddModule(const IBB_ModuleAlt& Module, const std::string& Argument, bool UseMouseCenter = true);
    std::optional<std::vector<id_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE AddModule(const std::vector<ModuleClipData>& Modules, bool UseMouseCenter = true);
    std::optional<id_t> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE AddModule(const ModuleClipData& Modules, bool UseMouseCenter = true);

    bool _PROJ_CMD_WRITE _PROJ_CMD_UPDATE UpdateAll();

    bool _PROJ_CMD_WRITE _PROJ_CMD_UPDATE ForceUpdate();

    /*template<typename T>
    T _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO OperateBackDataWithUndo(
        const std::function<T(void)>& Undo,
        const std::function<T(void)>& Redo);*/

    //bool _PROJ_CMD_READ WriteTextToFolder();

    //它是UTF8纯粹是因为方便统一INI存储格式
    _TEXT_UTF8 std::string _PROJ_CMD_READ GetText(bool PrintExtraData) _PROJ_CMD_BACK_CONST;

    //这个东西没啥开销，除了复制一份Desc ； 不保证是否存在
    IBR_Section _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ GetSection(const IBB_Section_Desc& Desc) _PROJ_CMD_BACK_CONST;

    //不建议，会缺少模板里面约定的一部分Variable（包括类型标记）
    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE CreateSection(const IBB_Section_Desc& Desc);
    IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE CreateSectionAndBack(const IBB_Section_Desc& Desc);
    IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE CreateCommentBlock(ImVec2 InitialEqPos, std::string_view InitialText = "", ImVec2 InitialEqSize = {0.0F,0.0F});

    //同GetSection的HasBack
    bool _PROJ_CMD_READ HasSection(const IBB_Section_Desc& Desc) _PROJ_CMD_BACK_CONST;

    //同GetSection的HasBack
    bool _PROJ_CMD_READ HasSection(id_t id) _PROJ_CMD_BACK_CONST;

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DeleteSection(const std::vector<IBB_Section_Desc>& Descs);

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DeleteSection(const IBB_Section_Desc& Desc);

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DeleteSection(id_t id);

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DeleteSection(const std::vector <IBR_Project::id_t>& ids);

    //不保证ID有效
    inline IBR_Section _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ GetSectionFromID(id_t id) _PROJ_CMD_BACK_CONST { return { this,id }; }

    std::optional<id_t> _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ GetSectionID(const IBB_Section_Desc& Desc) _PROJ_CMD_BACK_CONST;
    void _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ EnsureSection(const IBB_Section_Desc& Desc, const std::string& DisplayName = "") _PROJ_CMD_BACK_CONST;

    bool _PROJ_CMD_UPDATE DataCheck();

    void Load(const IBS_Project&);
    void Save(IBS_Project&);
private:
    std::optional<id_t> _PROJ_CMD_WRITE _PROJ_CMD_UPDATE AddModule_Impl(const ModuleClipData& Modules, bool UseMouseCenter = true);
};


namespace IBR_RealCenter
{
    extern ImVec2 Center;
    extern dImVec2 WorkSpaceUL, WorkSpaceDR;
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
    size_t Choice;
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
    void OnDropFile(GLFWwindow* window, int argc, const char** argv);
    void ProjActionByKey();
};

namespace IBR_FullView
{
    extern ImVec2 EqCenter;
    extern float Ratio;
    extern ImVec2 ViewSize;
    const float RatioMax = 200.0, RatioMin = 25.0;
    void ChangeOffsetPos(ImVec2 ClickRel);
    int EqXRange(const ImVec2& V);
    int EqYRange(const ImVec2& V);
    void EqPosFixRange(ImVec2& V);
    bool EqPosInRange(ImVec2 V);
    void RenderUI();
    ImVec2 GetEqMin();
    ImVec2 GetEqMax();
}

namespace IBR_WorkSpace
{
    extern ImVec2 EqCenterPrev, ReCenterPrev, DragStartEqMouse, DragCurEqMouse, InitialMassCenter;
    extern float RatioPrev;
    extern int UpdatePrevResult;
    extern float NewRatio;
    extern bool NeedChangeRatio, InitHolding, ShowRegName;
    extern bool IsBgDragging, HoldingModules, IsMassSelecting;
    extern std::vector<IBR_Project::id_t> MassTarget;
    extern ImVec4 TempWbg;
    extern bool LastOperateOnText, OperateOnText;

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
    void CutSelected();
    void Paste();
    void MassSelect(const std::vector<IBR_Project::id_t>& Target);
    void SelectAll();
    void OpenRightClick();
    void OutputSelectedImpl(const char* IPath, const char* IDescShort, const char* IDescLong);
    void OutputSelected();
}
namespace IBR_SelectMode
{
    struct Mode
    {
        bool RestrictedToSections;
        std::function<void(IBR_Section, ImVec2)>Exit;
        StdMessage Cancel;
    };
    const std::vector<IBR_Project::id_t>& GetMassSelected();
    bool IsWindowMassSelected(const IBB_Section_Desc& Desc);
    void EnterSelectMode(const Mode& Mode);
    void ExitSelectMode(IBR_Section Section, ImVec2 ClickRePos);
    void CancelSelectMode();
    void RenderUI();
    bool InSelectMode();
    const Mode& CurrentSelectMode();
}

namespace IBR_DynamicData
{
    void Read(int DefaultResX, int DefaultResY);
    void SetRandom();
    void Open();
    void Save();
    void Close();
    void SetDefaultWidth(int W);
    void SetDefaultHeight(int H);
}

namespace IBR_RecentManager
{
    extern std::wstring Path;
    void RenderUI();
    void Load();
    void Push(const std::wstring& Path);
    void Save();
    void WanDuZiLe();
}

namespace IBR_EditFrame
{
    extern IBR_Section CurSection;
    extern IBR_Project::id_t PrevId;
    extern bool Empty;
    extern std::unordered_map<std::string, BufferedLine> EditLines;
    void SetActive(IBR_Project::id_t id);
    void UpdateSection();
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

    void StyleLight();
    void StyleDark();
    void LoadLight(JsonObject Obj);
    void LoadDark(JsonObject Obj);
}


namespace IBR_PopupManager
{
    struct Popup
    {
        bool CanClose;//Only when Modal==true
        bool Modal;
        bool HasOwnStyle{ false };
        bool InstantClose{ false };
        _TEXT_UTF8 std::string Title;
        ImGuiWindowFlags Flag{ ImGuiWindowFlags_None };
        StdMessage Show;
        StdMessage Close;//run when it can be closed and it is closed

        ImVec2 Size{ 0,0 };

        Popup& Create(const _TEXT_UTF8 std::string& title);
        Popup& CreateModal(const _TEXT_UTF8 std::string& title, bool canclose, StdMessage close = []() {});
        Popup& SetTitle(const _TEXT_UTF8 std::string& title);
        Popup& SetFlag(ImGuiWindowFlags flag);
        Popup& UnsetFlag(ImGuiWindowFlags flag);
        Popup& ClearFlag();
        Popup& UseMyStyle();
        Popup& SetSize(ImVec2 NewSize = { 0,0 });//{0,0}=default/auto
        Popup& PushTextPrev(const _TEXT_UTF8 std::string& Text);
        Popup& PushTextBack(const _TEXT_UTF8 std::string& Text);
        Popup& PushMsgPrev(StdMessage Msg);
        Popup& PushMsgBack(StdMessage Msg);
        Popup& EnableInstantClose(bool Enable = true) { InstantClose = Enable; return *this; }
    };
    extern Popup CurrentPopup;
    extern Popup RightClickMenu;
    extern bool HasPopup;
    extern bool HasRightClickMenu;
    extern bool FirstRightClick;
    extern ImVec2 RightClickMenuPos;
    inline void SetCurrentPopup(Popup&& sc) { HasPopup = true; CurrentPopup = std::move(sc); }
    inline void ClearCurrentPopup() { HasPopup = false; }
    inline void ClearRightClickMenu() { HasRightClickMenu = false; }
    inline void SetRightClickMenu(Popup&& sc, ImVec2 Pos) { RightClickMenuPos = Pos; FirstRightClick = HasRightClickMenu = true; RightClickMenu = std::move(sc); }
    Popup SingleText(const _TEXT_UTF8 std::string& StrId, const _TEXT_UTF8 std::string& Text, bool Modal);
    Popup MessageModal(const _TEXT_UTF8 std::string& Title, const _TEXT_UTF8 std::string& Text, ImVec2 Size, bool CanClose, bool UseDefaultOK, StdMessage Close = []() {});
    void RenderUI();
    void ClearPopupDelayed();
    bool IsMouseOnPopup();
    void AddJsonParseErrorPopup(std::string&& ErrorStr, const std::string& Info);
}


template<typename Cont>
class IBR_ListMenu
{
    std::vector<Cont>& List;
    int Page;
public:
    typedef Cont Type;
    typedef std::function<void(Cont&, int, int)> ActionType;
    std::string Tag;
    ActionType Action;

    IBR_ListMenu() = delete;
    IBR_ListMenu(std::vector<Cont>& L, const std::string& t, const ActionType& a) :
        List(L), Page(0), Tag(t), Action(a) {}

    void Rewind()
    {
        Page = 0;
    }
    void RenderUI();
};

namespace IBR_HintManager
{
    void Clear();
    void RenderUI();
    void SetHint(const _TEXT_UTF8 std::string& Str, int TimeLimitMillis = -1);
    void SetHintCustom(const std::function<bool(_TEXT_UTF8 std::string&)>& Fn);//返回true继续，false停止并Clear
    const std::string& GetHint();
    void Load();
}


struct BrowseParamList
{
    int RenderF;
    int RenderN;
    int Sz;
    bool HasPrev;
    bool HasNext;
    int RealRF;
    int RealNF;
    int PageN;
};
BrowseParamList MakeParamList(size_t size, int Page);
void Browse_ShowList_Impl(const std::string& suffix, int* Page, BrowseParamList& List);

template<typename Cont>
void Browse_ShowList(const std::string& suffix, std::vector<Cont>& Ser, int* Page, const std::function<void(Cont&, int, int)>& Callback)
{
    BrowseParamList L{ MakeParamList(Ser.size(), *Page) };
    for (int On = L.RealRF; On < L.RealNF; On++)
        Callback(Ser.at(On), On - L.RealRF + 1, On);
    Browse_ShowList_Impl(suffix, Page, L);
}

template<typename Cont>
void IBR_ListMenu<Cont>::RenderUI()
{
    Browse_ShowList(Tag, List, &Page, Action);
}
