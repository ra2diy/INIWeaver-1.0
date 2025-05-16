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

struct IBB_IniLine_Data_Base;
using LineData = std::shared_ptr<IBB_IniLine_Data_Base>;

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
        JsonObject Lim;
        std::string TypeAlt;
        std::vector<std::string> Enum;
        std::vector<std::string> EnumValue;
    };

    std::string Name, DescShort, DescLong;
    std::vector<std::string> Platform;
    _Limit Limit;
    _Property Property;
    ImU32 Color;

    LineData Create() const;
    bool IsLinkAlt() const;
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


struct IBB_IniLine_DataList : public IBB_IniLine_Data_Base
{
    static constexpr const char* TypeName{ "List" };
    std::vector<std::string> Value{};

    IBB_IniLine_DataList() {}

    virtual bool SetValue(const std::string& Val);
    virtual bool MergeValue(const std::string& Val);
    virtual void UpdateAsDuplicate();
    virtual bool MergeData(const IBB_IniLine_Data_Base* Data);
    virtual LineData Duplicate() const;
    virtual bool Clear();

    virtual std::string GetString() const;
    virtual std::string GetStringForExport() const;

    void RemoveValue(const std::string& Val);
    void InsertValue(const std::string& Val, size_t Idx);
    void ReplaceValue(const std::string& Old, const std::string& New);

    typedef std::vector<std::string> type;
    typedef size_t alt_type;
    type& GetValue() { return Value; }
    alt_type GetAltValue() { return Value.size(); }
    virtual const char* GetName() const { return TypeName; }

    virtual ~IBB_IniLine_DataList() {}
};

struct IBB_IniLine
{
    IBB_IniLine_Default* Default{ nullptr };
    LineData Data;

    template<typename T> T* GetData() const { return dynamic_cast<T*>(Data.get()); }

    bool Merge(const IBB_IniLine& Another, const std::string& Mode);
    bool Merge(const std::string& Another, const std::string& Mode);

    bool Generate(const std::string& Value, IBB_IniLine_Default* Def = nullptr);//don't change Default if Def == nullptr 

    IBB_IniLine() {}
    IBB_IniLine(const std::string& Value, IBB_IniLine_Default* Def) { Generate(Value, Def); }
    IBB_IniLine(const IBB_IniLine& F) { Default = F.Default; Data = F.Data; }
    IBB_IniLine(IBB_IniLine&& F);

    IBB_IniLine Duplicate() const;//这个才是深复制，鉴于深复制用处远低浅复制，故默认浅复制

    ~IBB_IniLine() = default;
};


struct IBB_SubSec
{
    IBB_Section* Root{ nullptr };

    IBB_SubSec_Default* Default{ nullptr };
    std::vector<std::string> Lines_ByName;//KeyName
    std::unordered_map<std::string, IBB_IniLine> Lines;//<KeyName,LineData>
    std::vector<IBB_Link> LinkTo;
    //Key复制了2遍：Lines_ByName / Lines.find(x)->first

    IBB_SubSec() {}
    IBB_SubSec(IBB_SubSec_Default* D, IBB_Section* R) : Default(D), Root(R) {}
    IBB_SubSec(const IBB_SubSec&) = default;
    IBB_SubSec(IBB_SubSec&& A);

    bool Merge(const IBB_SubSec& Another, const IBB_VariableList& MergeType, bool IsDuplicate);
    bool Merge(const IBB_SubSec& Another, const std::string& Mode, bool IsDuplicate);

    std::string GetText(bool PrintExtraData, bool FromExport = false) const;//RARELY USED 这个GetText自带换行符
    std::vector<std::string> GetKeys(bool PrintExtraData) const;//RARELY USED
    IBB_VariableList GetLineList(bool PrintExtraData) const;//RARELY USED
    std::string GetFullVariable(const std::string& Name) const;
    IBB_SubSec Duplicate() const;//这个才是深复制，鉴于深复制用处远低浅复制，故默认浅复制
    void GenerateAsDuplicate(const IBB_SubSec& Src);//从Src深复制

    bool UpdateAll();

    bool AddLine(const std::pair<std::string, std::string>& Line);
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
    std::vector<IBB_Link> LinkedBy;
    IBB_VariableList VarList;
    IBB_VariableList UnknownLines;//不归属于任一SubSec

    bool IsLinkGroup{ false };//no subsec varlist unklines
    std::vector<IBB_Link> LinkGroup_LinkTo;
    IBB_VariableList DefaultLinkKey;
    std::unordered_map<std::string, std::string> OnShow;//EmptyOnShowDesc means no desc
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
    bool Merge(const IBB_Section& Another, const IBB_VariableList& MergeType, bool IsDuplicate);
    bool Merge(const IBB_Section& Another, const std::string& MergeType, bool IsDuplicate);//they share the same merge type
    bool Generate(const IBB_Section_NameType& Paragraph);
    bool GenerateLines(const IBB_VariableList& Lines);
    bool GenerateAsDuplicate(const IBB_Section& Src);


    bool UpdateAll();

    IBB_Section() {}
    IBB_Section(const std::string& N, IBB_Ini* R) : Name(N), Root(R), IsLinkGroup(false) {}
    IBB_Section(const IBB_Section_NameType& Paragraph, IBB_Ini* R) : Root(R) { Generate(Paragraph); }
    IBB_Section(const IBB_Section&) = default;
    IBB_Section(IBB_Section&&);

    bool ChangeRoot(const IBB_Ini* NewRoot);
    IBB_Section& ChangeRootAndBack(const IBB_Ini* R) { ChangeRoot(R); return *this; }
    bool Rename(const std::string& NewName);//无效化指向它的所有IBB_Section_Desc
    bool ChangeAddress();//用于不改变内容但是改变存储位置时,供移动构造
    bool Isolate();//切断所有Link
    void RedirectLinkAsDupicate();

    IBB_IniLine* GetLineFromSubSecs(const std::string& Name) const;
    IBB_Project_Index GetThisIndex() const;
    IBB_Section_Desc GetThisDesc() const;
    std::vector<IBB_Link> GetLinkTo() const;//RARELY USED
    std::vector<std::string> GetKeys(bool PrintExtraData) const;//RARELY USED
    IBB_VariableList GetLineList(bool PrintExtraData) const;//RARELY USED
    IBB_VariableList GetSimpleLines() const;//RARELY USED
    std::string GetText(bool PrintExtraData, bool FromExport = false) const;//RARELY USED
    std::string GetFullVariable(const std::string& Name) const;//如果对其使用_SECTION_NAME则返回字段名
    std::vector<size_t> GetRegisteredPosition() const;//Project的RegList序号
    std::vector<std::pair<size_t, size_t>> GetRegisteredPositionAlt() const;//pair<Project的RegList序号,RegList的Sec*序号>
    IBB_Section_NameType GetNameType() const;
    bool SetText(char* Text);//mess Text up
    bool SetText(const std::vector<IniToken>& Tokens);//do not consider section&inherit
    bool IsComment() const { return CreateAsCommentBlock || !Comment.empty(); }
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

struct IBB_Link_Default
{
    std::vector<JsonObject>
        LinkFromRequired,
        LinkFromForbidden,
        LinkToRequired,
        LinkToForbidden;
    std::string Name;//链接没有Desc
    bool NameOnlyAsRegister{ false };

    bool Load(JsonObject FromJson);
};

struct IBB_Link_NameType
{
    std::string FromIni, FromSec, ToIni, ToSec;
    IBB_Link_NameType() {}
    IBB_Link_NameType(const std::string& fi, const std::string& fs, const std::string& ti, const std::string& ts) :
        FromIni(fi), FromSec(fs), ToIni(ti), ToSec(ts) {
    }

    void Read(const ExtFileClass& File);
    void Write(const ExtFileClass& File)const;
};

//TODO:它的析构似乎有莫名的问题，可能导致崩溃,但是至今未曾复现。。
struct IBB_Link
{
    IBB_Link_Default* Default{ nullptr };
    IBB_Project_Index From, To;

    IBB_Link* Another{ nullptr };
    std::string FromKey;
    size_t Order{ 0 }, OrderEx{ 0 };//OrderEx=INT_MAX to a linkgroup

    bool operator==(const IBB_Link& A) const
    {
        return A.Default == Default && A.From == From && A.To == To;
    }

    struct _Dynamic
    {
        enum LinkStatus
        {
            Incomplete, Illegal, Correct, Mixed
        }Legal;
    }Dynamic;

    IBB_Link() {}
    IBB_Link(IBB_Link_Default* D, const IBB_Project_Index& F, const IBB_Project_Index& T) :Default(D), From(F), To(T) {}

    void FillData(IBB_Link* a, const std::string& s) { Another = a; FromKey = s; }

    void DynamicCheck_Legal(const IBB_Project& Proj);
    void DynamicCheck_UpdateNewLink(const IBB_Project& Proj);
    bool ChangeAddress();

    std::string GetText(const IBB_Project& Proj) const;//这个GetText自带换行符
    IBB_Link_NameType GetNameType() const;
};
