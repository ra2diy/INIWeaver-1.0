#pragma once
#include "FromEngine/Include.h"
#include "cjson/cJSON.h"
#include "IBB_Components.h"
#include "IBB_Index.h"

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
//using LineData = std::shared_ptr<IBB_IniLine_Data_Base>;
using LineData = std::shared_ptr<IBB_IniLine_Data_String>;
struct IIFWrapper_Wrapper;

enum class IBB_IniMergeMode
{
    Replace,
    Merge,
    Reserve
};


struct IBB_IniLine_Default
{
    struct _Limit//按照什么匹配
    {
        std::string Type;
        std::string Lim;
    };

    struct _Property//存储的是什么
    {
        std::string Type;
        JsonObject Lim;//限制数据，对link来说是LinkLimit
        std::string TypeAlt;//一个RegType的名字
    };

    //从TypeAlt加载
    std::string Name, DescShort, DescLong;

    //保留备用
    std::vector<std::string> Platform;
    _Limit Limit;
    _Property Property;
    
    //从TypeAlt加载
    ImU32 Color;
    std::string InputName;
    const IBG_InputType* Input;

    LineData Create() const;
    const IBB_RegType& GetRegType() const;
    const IBG_InputType& GetInputType() const;
    LinkNodeSetting GetNodeSetting() const;
    int GetLinkLimit() const;
    //bool Load(JsonObject FromJson);
};

struct IBB_SubSec_Default
{
    struct _Require
    {
        std::vector<JsonObject> RequiredValues;
        std::vector<JsonObject> ForbiddenValues;
    }Require;

    std::string Name, DescShort, DescLong;
    std::vector<std::string> Platform;
    std::vector<std::string> Lines_ByName;
    std::unordered_map<std::string, IBB_IniLine_Default> Lines;
    bool IsInherit;

    bool Load(JsonObject FromJson, const std::unordered_map<std::string, IBB_IniLine_Default>& LineMap);
};


struct IBB_IniLine_Data_Base
{
    bool _Empty{ true };
    bool Empty() { return _Empty; }

    virtual bool SetValue(const std::string&) = 0;
    virtual bool MergeValue(const std::string&) = 0;
    virtual bool MergeData(const IBB_IniLine_Data_Base*) = 0;
    virtual std::string GetString() const = 0;
    virtual std::string GetStringForExport() const = 0;
    virtual LineData Duplicate() const = 0;
    virtual void UpdateAsDuplicate() = 0;
    virtual const char* GetName() const = 0;
    virtual bool Clear() = 0;

    IBB_IniLine_Data_Base() {}
    virtual ~IBB_IniLine_Data_Base() {}
};

struct IBB_IniLine_Data_String final : public IBB_IniLine_Data_Base
{
    static constexpr const char* TypeName{ "String" };
    std::string Value{};

    IBB_IniLine_Data_String() {}

    bool SetValue(const std::string& Val);
    bool MergeValue(const std::string& Val);
    bool MergeData(const IBB_IniLine_Data_Base* data);
    bool Clear();
    void UpdateAsDuplicate();
    LineData Duplicate() const;

    std::string GetString() const { return Value; }
    std::string GetStringForExport() const { return GetString(); }

    virtual const char* GetName() const { return TypeName; }

    virtual ~IBB_IniLine_Data_String() {}
};

struct IBB_IniLine
{
    IBB_IniLine_Default* Default{ nullptr };
    LineData Data;

    //暂时不需要Validate
    //ValidateResult ValidateValue() const;
    //ValidateResult ValidateAndSet(const std::string& Value);
    //ValidateResult ValidateAndMerge(const std::string& Another, IBB_IniMergeMode Mode);
    //ValidateResult ValidateAndMerge(const IBB_IniLine& Another, IBB_IniMergeMode Mode);

    void MakeKVForExport(IBB_VariableList&, std::vector<std::string>* TmpLineOrder = nullptr) const;

    template<typename T> T* GetData() const { return dynamic_cast<T*>(Data.get()); }

    bool Merge(const IBB_IniLine& Another, IBB_IniMergeMode Mode);
    bool Merge(const std::string& Another, IBB_IniMergeMode Mode);

    bool Generate(const std::string& Value, IBB_IniLine_Default* Def = nullptr);//don't change Default if Def == nullptr 

    IBB_IniLine() {}
    IBB_IniLine(const std::string& Value, IBB_IniLine_Default* Def) { Generate(Value, Def); }
    IBB_IniLine(const IBB_IniLine& F) { Default = F.Default; Data = F.Data; }
    IBB_IniLine(IBB_IniLine&& F) noexcept;

    IBB_IniLine Duplicate() const;//这个才是深复制，鉴于深复制用处远低浅复制，故默认浅复制

    ~IBB_IniLine() = default;
};

struct IBB_NewLink
{
    IBB_Project_Index From, To;
    std::string FromKey;
    ImU32 DefaultColor;

    std::string GetText() const;
};

struct IBB_SubSec
{
    IBB_Section* Root{ nullptr };

    IBB_SubSec_Default* Default{ nullptr };
    std::vector<std::string> Lines_ByName;//KeyName
    std::unordered_map<std::string, IBB_IniLine> Lines;//<KeyName,LineData>
    std::vector<IBB_NewLink> NewLinkTo;
    std::multimap<uint64_t, size_t> LinkSrc;
    //Key复制了2遍：Lines_ByName / Lines.find(x)->first

    IBB_SubSec() {}
    IBB_SubSec(IBB_SubSec_Default* D, IBB_Section* R) : Default(D), Root(R) {}
    IBB_SubSec(const IBB_SubSec&) = default;
    IBB_SubSec(IBB_SubSec&& A);

    bool Merge(const IBB_SubSec& Another, const std::unordered_map<std::string, IBB_IniMergeMode>& MergeType, bool IsDuplicate);
    bool Merge(const IBB_SubSec& Another, IBB_IniMergeMode Mode, bool IsDuplicate);

    std::vector<std::string> GetKeys(bool PrintExtraData) const;//RARELY USED
    IBB_VariableList GetLineList(bool PrintExtraData, bool FromExport, std::vector<std::string>* TmpLineOrder = nullptr) const;//RARELY USED
    std::string GetFullVariable(const std::string& Name) const;
    IBB_SubSec Duplicate() const;//这个才是深复制，鉴于深复制用处远低浅复制，故默认浅复制
    void GenerateAsDuplicate(const IBB_SubSec& Src);//从Src深复制
    std::pair< std::multimap<uint64_t, size_t>::const_iterator, std::multimap<uint64_t, size_t>::const_iterator>
        GetLink(size_t LineIdx, size_t ComponentIdx) const;
    void ClaimLink(size_t LineIdx, size_t ComponentIdx, size_t LinkIdx);
    bool RenameInLinkTo(size_t LinkIdx, const std::string& NewName);

    bool UpdateAll();
    bool TriggerUpdate();

    bool AddLine(const std::pair<std::string, std::string>& Line, bool InitOnShow, IBB_IniMergeMode Mode, bool NoUpdate = false);
    bool ChangeRoot(IBB_Section* NewRoot);
    IBB_SubSec& ChangeRootAndBack(IBB_Section* NewRoot) { ChangeRoot(NewRoot); return *this; }
};


struct IBB_Section_NameType
{
    std::string Name;
    std::string IniType;
    IBB_VariableList VarList;
    IBB_VariableList Lines;
    bool IsLinkGroup;//no VarList Lines

    void Read(const ExtFileClass& File);
    void Write(const ExtFileClass& File)const;
};



struct IBB_Section
{
    IBB_Ini* Root{ nullptr };


    std::string Name;
    std::vector<IBB_SubSec> SubSecs;
    std::vector<size_t> SubSecOrder;
    std::vector<IBB_NewLink> NewLinkedBy;
    IBB_VariableList VarList;
    IBB_VariableList UnknownLines;//不归属于任一SubSec

    bool IsLinkGroup{ false };//no subsec varlist unklines
    std::vector<IBB_NewLink> LinkGroup_NewLinkTo;
    IBB_VariableList DefaultLinkKey;
    std::unordered_map<std::string, std::string> OnShow;//EmptyOnShowDesc means no desc
    std::vector<std::string> LineOrder;
    std::string Inherit;
    std::string Comment{};
    std::string Register;
    bool CreateAsCommentBlock{ false };

    struct _Dynamic
    {
        bool Selected{ false };
    }Dynamic;

    //增删改时相应修改移动构造函数

    // MergeType is unused to a LinkGroup
    bool Merge(const IBB_Section& Another, const std::unordered_map<std::string, IBB_IniMergeMode>& MergeType, bool IsDuplicate);
    bool Merge(const IBB_Section& Another, IBB_IniMergeMode MergeType, bool IsDuplicate);//they share the same merge type
    bool MergeLine(const std::string& Key, const std::string& Value, IBB_IniMergeMode Mode, bool NoUpdate = false);
    bool Generate(const IBB_Section_NameType& Paragraph);
    bool GenerateLines(const IBB_VariableList& Lines, const std::vector<std::string>& Order = {}, bool InitOnShow = true);
    bool GenerateAsDuplicate(const IBB_Section& Src);
    void OrderKey(const std::string& Key, size_t NewOrder);
    void CheckSubsecOrder();

    bool UpdateAll();
    bool UpdateLineOrder();

    IBB_Section() {}
    IBB_Section(const std::string& N, IBB_Ini* R);
    IBB_Section(const IBB_Section_NameType& Paragraph, IBB_Ini* R) : Root(R) { Generate(Paragraph); }
    IBB_Section(const IBB_Section&) = default;
    IBB_Section(IBB_Section&&) noexcept;

    bool ChangeRoot(const IBB_Ini* NewRoot);
    IBB_Section& ChangeRootAndBack(const IBB_Ini* R) { ChangeRoot(R); return *this; }
    bool Rename(const std::string& NewName);//无效化指向它的所有IBB_Section_Desc
    bool AcceptNewNameInLinkTo(IBB_Section* Target, const std::string& NewName);//配合Rename
    bool AcceptNewNameInLinkedBy(const IBB_Project_Index& OldIndex, const std::string& NewName);//配合Rename
    bool ChangeAddress();//用于不改变内容但是改变存储位置时,供移动构造
    bool RemoveNameInLinkTo(IBB_Section* Target);//配合Isolate
    bool Isolate();//切断所有Link
    void RedirectLinkAsDupicate();

    const IBB_IniLine* GetLineFromSubSecs(const std::string& Name) const;
    IBB_IniLine* GetLineFromSubSecs(const std::string& Name);
    std::pair <IBB_IniLine*, IBB_SubSec*> GetLineFromSubSecsEx2(const std::string& Name);
    std::pair <IBB_IniLine*, size_t> GetLineFromSubSecsEx(const std::string& Name);
    IBB_Project_Index GetThisIndex() const;
    IBB_Section_Desc GetThisDesc() const;
    std::vector<std::string> GetKeys(bool PrintExtraData) const;//RARELY USED
    IBB_VariableList GetLineList(bool PrintExtraData, bool FromExport, std::vector<std::string>* TmpLineOrder = nullptr) const;//RARELY USED
    IBB_VariableList GetSimpleLines() const;//RARELY USED
    std::string GetText(bool PrintExtraData, bool FromExport = false, bool ForEdit = false) const;
    std::string GetTextForEdit() const;
    std::string GetFullVariable(const std::string& Name) const;//如果对其使用_SECTION_NAME则返回字段名
    std::vector<size_t> GetRegisteredPosition() const;//Project的RegList序号
    std::vector<std::pair<size_t, size_t>> GetRegisteredPositionAlt() const;//pair<Project的RegList序号,RegList的Sec*序号>
    IBB_Section_NameType GetNameType() const;
    IIFWrapper_Wrapper GetLineIIF(const std::string& Key) const;
    IIFWrapper_Wrapper GetNewLineIIF(const std::string& Key) const;
    bool IsComment() const { return CreateAsCommentBlock || !Comment.empty(); }
    bool HasLine(const std::string& Key) const;
    bool IsOnShow(const std::string& Key) const;
    const std::string& GetOnShow(const std::string& Key) const;
    void SetOnShow(const std::string& Key, const std::string& Value, bool AllowReapply);
    void SetOnShow(const std::string& Key);

    bool SetText(char* Text);//mess Text up
    bool SetText(const std::vector<IniToken>& Tokens);//do not consider section&inherit
    void GetClipData(ModuleClipData& Clip);
    bool Generate(const ModuleClipData& Clip);

};

struct IBB_Ini
{
    IBB_Project* Root;
    /*
    enum _Type
    {
        Rule,Art,AI,Sound,EVA    etc.
    }Name;
    */
    _TEXT_ANSI std::string Name;
    std::vector<_TEXT_ANSI std::string> Secs_ByName;
    std::unordered_map<_TEXT_ANSI std::string, IBB_Section> Secs;

    bool Merge(const IBB_Ini& Another, bool IsDuplicate);
    bool CreateSection(const _TEXT_ANSI std::string& _Name);
    bool AddSection(const IBB_Section& Section, bool IsDuplicate);
    bool DeleteSection(const _TEXT_ANSI std::string& Tg);//包含Update

    std::string GetText(bool PrintExtraData) const;

    bool ChangeRoot(IBB_Project* NewRoot) { Root = NewRoot; return true; }
    IBB_Ini& ChangeRootAndBack(IBB_Project* NewRoot) { Root = NewRoot; return *this; }

    bool UpdateAll();
};
