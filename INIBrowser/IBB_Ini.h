#pragma once
#include "FromEngine/Include.h"
#include "cjson/cJSON.h"
#include "IBB_Components.h"
#include "IBB_Index.h"
#include "IBB_PropStringPool.h"
#include "IBG_InputType_Defines.h"
#include <variant>

#ifndef _TEXT_UTF8
#define _TEXT_UTF8
#endif

struct IniToken;
struct ModuleClipData;
struct LinkNodeSetting;

struct IBB_IniLine_Data_Base;
struct IBB_IniLine_Data_String;
struct IBB_RegType;
struct IBG_InputType;
enum class ValidateResult;
using LineData = std::shared_ptr<IBB_IniLine_Data_Base>;
using LineDV = std::variant<LineData, std::vector<LineData>>;
struct IBB_SubSec_Default;

enum class IBB_IniMergeMode
{
    Replace,
    Merge,
    Reserve
};


struct IBB_IniLine_Default
{
    bool Known;

    //从TypeAlt加载
    StrPoolID Name;
    DescPoolOffset DescShort;
    DescPoolOffset DescLong;
    StrPoolID InputName;
    const IBG_InputType* Input;
    LinkNodeSetting LinkNode;

    //SubSec关联
    IBB_SubSec_Default* InSubSec;

    LineData Create() const;
    const IBB_RegType& GetRegType() const;
    const std::string& GetIniType() const;
    const IBG_InputType& GetInputType() const;
    LinkNodeSetting GetNodeSetting() const;
    int GetLinkLimit() const;
    const IBG_InputType* GetInputTypeByValue(const std::string& Value) const;
    bool IsMultiple() const;
};

const inline size_t Index_AlwaysNew = static_cast<size_t>(-1);

struct IBB_SubSec_Default
{
    std::string Name;
    enum _Type
    {
        Default,
        Inherit,
        Import
    }Type;

    IBB_SubSec_Default();
};


struct IBB_IniLine_Data_Base
{
    bool _Empty{ true };
    bool Empty() { return _Empty; }

    virtual bool SetValue(const std::string&) = 0;
    virtual bool MergeValue(const std::string&) = 0;
    virtual bool Clear() = 0;
    virtual void RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace) = 0;
    virtual void Replace(size_t CompIdx, const std::string& OldName, const std::string& NewName) = 0;

    virtual bool FirstIsLink() const = 0;
    virtual IIFPtr GetNewIIF(IBB_IniLine_Default* Default) const = 0;
    virtual std::string GetString() const = 0;
    virtual std::string GetStringForExport() const = 0;

    IBB_IniLine_Data_Base() {}
    virtual ~IBB_IniLine_Data_Base() {}

    template<typename T> T* GetData() { return dynamic_cast<T*>(this); }
    template<typename T> const T* GetData() const { return dynamic_cast<const T*>(this); }
};

struct IBB_IniLine
{
    IBB_IniLine_Default* Default{ nullptr };
    LineDV Data;

    //暂时不需要Validate
    //ValidateResult ValidateValue() const;
    //ValidateResult ValidateAndSet(const std::string& Value);
    //ValidateResult ValidateAndMerge(const std::string& Another, IBB_IniMergeMode Mode);
    //ValidateResult ValidateAndMerge(const IBB_IniLine& Another, IBB_IniMergeMode Mode);

    static LineData& Null() { static LineData LD; LD.reset(); return LD; }
    bool IsMultiple() const { return Default->IsMultiple(); }
    LineData& Single() { return std::get<LineData>(Data); }
    const LineData& Single() const { return std::get<LineData>(Data); }
    std::vector<LineData>& Multis() { return std::get<std::vector<LineData>>(Data); }
    LineData& Multiple(size_t Index) { auto& V = Multis(); if (Index < V.size())return V.at(Index); else return Null(); }
    const LineData& Multiple(size_t Index) const { auto& V = Multis(); if (Index < V.size())return V.at(Index); else return Null(); }
    LineData& Indexed(size_t Index) { return IsMultiple() ? Multiple(Index) : Single(); }
    const LineData& Indexed(size_t Index) const { return IsMultiple() ? Multiple(Index) : Single(); }
    const std::vector<LineData>& Multis() const { return std::get<std::vector<LineData>>(Data); }
    template<typename T> T* GetData(size_t Index) const
    {
        return dynamic_cast<T*>(Indexed(Index).get());
    }
    template<typename _Act> void ForEach(const _Act& Action)
    {
        if (IsMultiple())for (auto& D : Multis())Action(D);
        else Action(Single());
    }
    template<typename _Act> void ForEach(const _Act& Action) const
    {
        if (IsMultiple())for (auto& D : Multis())Action(D);
        else Action(Single());
    }
    template<typename _Act> void ForEachWithIdx(const _Act& Action)
    {
        if (IsMultiple())for (auto&& [D, I] : std::views::zip(Multis(), std::views::iota(0)))Action(D, I);
        else Action(Single(), 0);
    }
    template<typename _Act> void ForEachWithIdx(const _Act& Action) const
    {
        if (IsMultiple())for (auto&& [D, I] : std::views::zip(Multis(), std::views::iota(0)))Action(D, I);
        else Action(Single(), 0);
    }

    void MakeKVForExport(IBB_VariableMultiList&, IBB_Section* AtSec, std::vector<std::string>* TmpLineOrder = nullptr) const;
    
    bool Merge(size_t Index, const std::string& Another, IBB_IniMergeMode Mode);

    void RenderUI(const LinkNodeSetting& LinkNode, bool IsWorkspace);
    void RenderUI(bool IsWorkspace);
    const void* GetComponentID();
    IIFPtr GetNewIIF() const;

    IBB_IniLine() {}
    IBB_IniLine(const std::string& Value, IBB_IniLine_Default* Def);
    IBB_IniLine(const IBB_IniLine& F) { Default = F.Default; Data = F.Data; }
    IBB_IniLine(IBB_IniLine&& F) noexcept;

    ~IBB_IniLine() = default;
};

struct IBB_NewLink
{
    IBB_SectionID From;
    IBB_SectionID To;
    StrPoolID FromKey;
    StrPoolID ToKey;
    size_t LineMult;
    ImU32 DefaultColor;
    uint64_t SessionID;

    std::string GetText() const;
    bool Empty() const;//From和To都不能为空，有一个是"":""就返回true
    std::string TargetValue() const;
};

std::string TargetValueStr(const std::string& ToSec, StrPoolID ToKey, size_t LineMult);

struct IBB_SubSec
{
    IBB_Section* Root{ nullptr };

    IBB_SubSec_Default* Default{ nullptr };
    std::vector<StrPoolID> Lines_ByName;//KeyName
    std::unordered_map<StrPoolID, IBB_IniLine> Lines;//<KeyName,LineData>
    std::vector<IBB_NewLink> NewLinkTo;
    std::multimap<LinkSrcIdx, size_t> LinkSrc;
    //Key复制了2遍：Lines_ByName / Lines.find(x)->first

    IBB_SubSec() {}
    IBB_SubSec(IBB_SubSec_Default* D, IBB_Section* R) : Default(D), Root(R) {}
    IBB_SubSec(const IBB_SubSec&) = default;
    IBB_SubSec(IBB_SubSec&& A) noexcept;

    std::vector<StrPoolID> GetKeys(bool PrintExtraData) const;//RARELY USED
    IBB_VariableMultiList GetLineList(bool PrintExtraData, bool FromExport, std::vector<std::string>* TmpLineOrder = nullptr) const;
    std::pair<LinkSrcCIter, LinkSrcCIter> GetLink(size_t LineIdx, size_t LineMult, size_t ComponentIdx) const;
    void ClaimLink(size_t LineIdx, size_t LineMult, size_t ComponentIdx, size_t LinkIdx);
    bool RenameInLinkTo(size_t LinkIdx, const std::string& OldName, const std::string& NewName);
    bool CanOwnKey(StrPoolID Key) const;

    bool UpdateAll();
    void UpdateNewLinkTo(std::vector<IBB_NewLink>&& NewLT);

    bool MergeLine(StrPoolID Key, size_t Index, const std::string& Value, IBB_IniMergeMode Mode, bool NoUpdate = false);
    bool ChangeRoot(IBB_Section* NewRoot);
    IBB_SubSec& ChangeRootAndBack(IBB_Section* NewRoot) { ChangeRoot(NewRoot); return *this; }
};

struct IBB_Section
{
    IBB_Ini* Root{ nullptr };


    std::string Name;
    std::vector<IBB_SubSec> SubSecs;
    std::vector<size_t> SubSecOrder;
    IBB_VariableList VarList;

    bool IsLinkGroup{ false };//no subsec varlist unklines
    std::vector<IBB_NewLink> LinkGroup_NewLinkTo;
    std::unordered_map<StrPoolID, StrPoolID> DefaultLinkKey;
    std::unordered_map<StrPoolID, StrPoolID>* DefaultLinkKey_UpValue;
    std::unordered_map<StrPoolID, std::string> OnShow;//EmptyOnShowDesc means no desc
    std::vector<StrPoolID> LineOrder;
    std::string Inherit;
    std::string Comment{};
    StrPoolID Register;
    bool CreateAsCommentBlock{ false };
    bool SingleVal{ false };//为True时只有一个值
    IBB_SectionID ID;

    struct _Dynamic
    {
        bool Selected{ false };
        int ImportCount{ 0 };
    }Dynamic;

    //增删改时相应修改移动构造函数

    // MergeType is unused to a LinkGroup
    bool RemoveLine(StrPoolID Key);
    bool MergeLine(StrPoolID Key, size_t Index, const std::string& Value, IBB_IniMergeMode Mode, bool NoUpdate = false);
    void OrderKey(StrPoolID Key, size_t NewOrder);
    void CheckSubsecOrder();

    bool UpdateAll();
    bool UpdateLineOrder();

    IBB_Section() {}
    IBB_Section(const std::string& N, IBB_Ini* R);
    IBB_Section(const IBB_Section&) = default;
    IBB_Section(IBB_Section&&) noexcept;

    bool ChangeRoot(const IBB_Ini* NewRoot);
    bool Rename(const std::string& NewName);//无效化指向它的所有IBB_Section_Desc
    bool AcceptNewNameInLinkTo(IBB_SectionID NewID, IBB_Section* Target, const std::string& NewName);//配合Rename
    bool AcceptNewNameInLinkedBy(IBB_SectionID OldIndex, const std::string& NewName);//配合Rename
    bool ChangeAddress();//用于不改变内容但是改变存储位置时,供移动构造
    bool RemoveNameInLinkTo(IBB_Section* Target);//配合Isolate
    bool Isolate();//切断所有Link

    IBB_IniLine* GetLineFromSubSecs(StrPoolID Name);
    std::pair <IBB_IniLine*, IBB_SubSec*> GetLineFromSubSecsEx2(StrPoolID Name);
    std::pair <IBB_IniLine*, size_t> GetLineFromSubSecsEx(StrPoolID Name);
    IBB_SubSec& GetSubSecByDef(IBB_SubSec_Default* Def);//返回Def对应的SubSec，若没有则构造一个新的SubSec并返回
    IBB_SubSec& GetSubSecByLine(const std::string& Key);//返回包含Key的SubSec，若没有则构造一个新的SubSec并返回

    std::vector<IBB_NewLink>& GetLinkedBy_NoCached() const;
    std::vector<IBB_NewLink>& GetLinkedBy_Cached() const;
    const IBB_IniLine* GetLineFromSubSecs(StrPoolID Name) const;
    IBB_Project_Index GetThisIndex() const;
    IBB_Section_Desc GetThisDesc() const;
    IBB_SectionID GetThisID() const;
    std::vector<StrPoolID> GetKeys(bool PrintExtraData) const;
    std::vector<std::string> GetLineOrderString() const;
    IBB_VariableMultiList GetLineList(bool PrintExtraData, bool FromExport, std::vector<std::string>* TmpLineOrder = nullptr) const;//RARELY USED
    std::string GetText(bool PrintExtraData, bool FromExport = false, bool ForEdit = false) const;
    std::string GetTextForEdit() const;
    std::vector<size_t> GetRegisteredPosition() const;//Project的RegList序号
    std::vector<std::pair<size_t, size_t>> GetRegisteredPositionAlt() const;//pair<Project的RegList序号,RegList的Sec*序号>
    bool IsComment() const { return CreateAsCommentBlock || !Comment.empty(); }
    bool HasLine(StrPoolID Key) const;
    bool IsOnShow(StrPoolID Key) const;
    const std::string& GetOnShow(StrPoolID Key) const;
    StrPoolID GetDLK(StrPoolID Reg) const;
    
    void SetOnShow(StrPoolID Key, const std::string& Value, bool AllowReapply);
    void SetOnShow(StrPoolID Key);
    void PushLineOrder(StrPoolID Key);
    void RecheckLineOrder();
    bool SetText(char* Text);//mess Text up
    bool SetText(const std::vector<IniToken>& Tokens);//do not consider section&inherit
    void GetClipData(ModuleClipData& Clip);
    bool Generate(const ModuleClipData& Clip);

};

struct IBB_Ini
{
    IBB_Project* Root;
    std::string Name;
    std::vector<std::string> Secs_ByName;
    std::unordered_map<std::string, IBB_Section> Secs;

    IBB_Ini() = default;
    IBB_Ini(const IBB_Ini&);
    IBB_Ini(IBB_Ini&&) noexcept;

    bool CreateSection(const std::string& _Name);
    bool DeleteSection(const std::string& Tg);//包含Update

    std::string GetText(bool PrintExtraData) const;

    bool ChangeRoot(IBB_Project* NewRoot) { Root = NewRoot; return true; }
    IBB_Ini& ChangeRootAndBack(IBB_Project* NewRoot) { Root = NewRoot; return *this; }

    bool UpdateAll();
};
