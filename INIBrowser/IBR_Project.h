#pragma once
#include "IBFront.h"
#include "FromEngine/RFBump.h"
#include "IBSave.h"
#include "IBG_InputType_Defines.h"
#include "IBR_IniLine.h"
#include <any>

extern int HintStayTimeMillis;

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


dImVec2 operator+(const dImVec2 a, const dImVec2 b);
dImVec2 operator-(const dImVec2 a, const dImVec2 b);
ImVec4 operator+(const ImVec4 a, const ImVec4 b);
dImVec2 operator/(const dImVec2 a, const double b);
dImVec2 operator*(const dImVec2 a, const double b);

int IBG_ExitProcess();
std::wstring RemoveSpec(std::wstring W);






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
struct IBB_ClipBoardData;
extern const IBB_Section_Desc IBB_Section_DescNull;

#define INVALID_MODULE_ID UINT64_MAX

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
    bool CollapsedInComposed{ false };
    bool Frozen{ false };
    bool Hidden{ false };
    bool UpdatePosByEq{ false };
    float FinalY{ 0.0f };
    float WidthFix{ 0.0f };
    std::shared_ptr<BufString> CommentEdit;
    ImVec2 ReWindowUL, ReOffset;
    IBB_Section* BackPtr_Cached;
    std::optional<ImColor> TitleCol_Cached;

    std::string ModuleStrID;
    uint64_t IncludedByModule{ INVALID_MODULE_ID };
    std::vector<uint64_t> IncludingModules;
    IBB_Section_Desc IncludedByModule_TmpDesc;
    std::vector<IBB_Section_Desc> IncludingModules_TmpDesc;
    std::unordered_map<StrPoolID, WorkSpaceLine> ActiveLines;

    bool IsVirtualBlock() const;

    ~IBR_SectionData();
    IBR_SectionData();
    IBR_SectionData(const IBB_Section_Desc& D);
    IBR_SectionData(const IBB_Section_Desc& D, std::string&& Name);
    IBR_SectionData(IBR_SectionData&&) noexcept;

    void RenameDisplayImpl(const std::string& Name);
    void RenameRegisterImpl(const std::string& Name);
    void RenameDisplay();
    void RenameRegister();
    bool RenderUI_Line(const std::string& OnShow, StrPoolID Name);

    void RenderUI();
    void RenderUI_Acceptor(float LastFinalY);
    void RenderUI_TitleBar(IBR_Section Rsec, IBB_Section* Bsec, bool& TriggeredRightMenu, float LastFinalY);
    void RenderUI_Error();
    void RenderUI_Comment(IBB_Section*);
    void RenderUI_Collapsed(IBB_Section*, ImVec2 HeadLineRN, IBR_Section Rsec);
    void RenderUI_Virtual();
    void RenderUI_Composed();
    void RenderUI_Lines(IBB_Section* Bsec);

    bool IsComposedAllFold() const;
    void FoldComposed();
    void UnfoldComposed();

    IBB_ClipBoardData GetClipBoardData(int& Copied);
    void CopyToClipBoard();
    void CutToClipBoard();
    void Duplicate();

    bool Decomposable() const;
    void Decompose();
    bool IsIncluded() const;

    IBB_Section* GetBack_Inl();
};

struct ModuleClipData;

struct IBR_Section
{
    IBR_Project* Root;
    uint64_t ID;
    //改动其中存储内容应修改IBR_Project::GetSection

    
    _RETURN_BACK_DATA IBB_Section* _PROJ_CMD_READ GetBack() ;
    _RETURN_BACK_DATA const IBB_Section* _PROJ_CMD_READ GetBack() _PROJ_CMD_BACK_CONST const;
    _RETURN_BACK_DATA IBB_Section* _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ GetBack_Unsafe();
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
    bool _PROJ_CMD_READ GetClipData(ModuleClipData& Data, bool UsePosAsDelta) _PROJ_CMD_BACK_CONST;
    ImColor _PROJ_CMD_READ GetRegTypeColor() _PROJ_CMD_BACK_CONST const;
    const std::string& _PROJ_CMD_READ GetRegTypeName() _PROJ_CMD_BACK_CONST const;

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE SetVarList(const IBB_VariableList& NewList);


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
    StrPoolID TypeAlt;
    IBR_NodeSession::SessionValue* pSession;
};

struct IBB_ModuleAlt;

struct IBR_Project
{
    struct _Plink
    {
        IBB_SectionID Dest;
        StrPoolID DestKey;
        size_t LineMult;
        uint64_t SourceID;
        ModuleID_t SrcModuleID;
        ImU32 Color;
        bool FromImport;
        bool IsSelfLinked;
        bool IsSrcDragging;
    };

    ModuleID_t MaxID{ 0 };//TODO:你还能使用超过ULL_MAX个ID？要是真的如此那就修一修
    std::map<ModuleID_t, IBR_SectionData> IBR_SectionMap;
    std::map<IBB_Section_Desc, ModuleID_t> IBR_Rev_SectionMap;
    std::map<IBB_SectionID, ModuleID_t> IBR_Rev_SectionMapII;
    std::unordered_map<std::string, SectionDragData> IBR_SecDragMap;
    std::string DragConditionText;
    std::string DragConditionTextAlt;
    std::vector<_Plink> LinkList;
    bool RefreshLinkList{ true };

    void _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE TriggerRefreshLink();

    std::pair<bool, std::vector<ModuleID_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE AddModule(const IBB_ModuleAlt& Module, const std::string& Argument, bool UseMouseCenter = true);
    std::pair<bool, std::vector<ModuleID_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE AddModule(const std::vector<ModuleClipData>& Modules, bool UseMouseCenter = true);
    std::optional<ModuleID_t> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE AddModule(const ModuleClipData& Modules, bool UseMouseCenter = true);

    bool _PROJ_CMD_WRITE SetModuleIncludeLink(ModuleID_t ID);
    bool _PROJ_CMD_WRITE SetModuleIncludeLink(const std::vector<ModuleID_t>& IDs);

    //Assume IDs are checked
    std::optional<ModuleID_t> _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE ComposeSections(const std::vector<ModuleID_t>& IDs);
    //Assume IDs are checked
    std::optional<std::vector<ModuleID_t>> _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DecomposeSection(ModuleID_t ID);
    std::optional<std::vector<ModuleID_t>> _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DecomposeSection(IBB_Section_Desc Desc);

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
    IBR_Section _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ GetSection(IBB_SectionID id) _PROJ_CMD_BACK_CONST;

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE RenameAll();

    //不建议，会缺少模板里面约定的一部分Variable（包括类型标记）
    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE CreateSection(const IBB_Section_Desc& Desc);
    IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE CreateSectionAndBack(const IBB_Section_Desc& Desc, _TEXT_UTF8 const std::string& Register, _TEXT_UTF8 const std::string& DisplayName);

    //特殊类型
    IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE CreateCommentBlock(ImVec2 InitialEqPos, std::string_view InitialText = "", ImVec2 InitialEqSize = {0.0F,0.0F});
    IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE CreateSingleValBlock(ImVec2 InitialEqPos, const std::string& InitialValue = "");

    //同GetSection的HasBack
    bool _PROJ_CMD_READ HasSection(const IBB_Section_Desc& Desc) _PROJ_CMD_BACK_CONST;

    //同GetSection的HasBack
    bool _PROJ_CMD_READ HasSection(ModuleID_t id) _PROJ_CMD_BACK_CONST;

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DeleteSection(const std::vector<IBB_Section_Desc>& Descs);

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DeleteSection(const IBB_Section_Desc& Desc);

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DeleteSection(ModuleID_t id);

    bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE DeleteSection(const std::vector <ModuleID_t>& ids);

    //不保证ID有效
    inline IBR_Section _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ GetSectionFromID(ModuleID_t id) _PROJ_CMD_BACK_CONST { return { this,id }; }

    std::optional<ModuleID_t> _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ GetSectionID(const IBB_Section_Desc& Desc) _PROJ_CMD_BACK_CONST;
    void _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ EnsureSection(const IBB_Section_Desc& Desc, _TEXT_UTF8 const std::string& Register, const std::string& DisplayName = "") _PROJ_CMD_BACK_CONST;

    bool _PROJ_CMD_UPDATE DataCheck();

    void Load(const IBS_Project&);
    void Save(IBS_Project&);
    void Clear();
private:
    std::optional<ModuleID_t> _PROJ_CMD_WRITE _PROJ_CMD_UPDATE AddModule_Impl(const ModuleClipData& Modules, bool UseMouseCenter = true);
};
