#pragma once

#include "FromEngine/Include.h"
#include "cjson/cJSON.h"

#ifndef _TEXT_UTF8
#define _TEXT_UTF8
#endif


/*
一个总纲：
back不负责Write和Update的综合
而由Front统御！
*/

struct IBG_SettingPack;

struct IBB_Section;
struct IBB_Ini;
struct IBB_Project;
struct IBB_Project_Index;
struct IBB_VariableList;
struct IBB_Link;

struct IBB_Module_Default;
struct IBB_Module;
struct IBB_Module_ParagraphList;

template<typename Str>
struct IBB_TDIndex
{
    bool UseIndex;
    int Index;
    Str Name;

    bool operator==(const IBB_TDIndex<Str>& A) const
    {
        if (A.UseIndex != UseIndex)return false;
        if (UseIndex)return Index == A.Index;
        else return Name == A.Name;
    }

    IBB_TDIndex() :UseIndex(true), Index(0), Name() {}
    IBB_TDIndex(int _Index) :UseIndex(true), Index(_Index), Name() {}
    IBB_TDIndex(const Str& _Name) :UseIndex(false), Index(0), Name(_Name) {}
    IBB_TDIndex<Str>& Assign(int _Index) { UseIndex = true; Index = _Index; return *this; }
    IBB_TDIndex<Str>& Assign(const Str& _Name) { UseIndex = false; Name = _Name; return *this; }
    bool Load(JsonObject FromJson);

    template<typename T>
    typename std::vector<T>::iterator Search(std::vector<T>& Source, bool CanUseByName, bool Update, const std::function<Str(const T&)>& GetName);
    template<typename T>
    typename std::unordered_map<Str, T>::iterator Search(std::unordered_map<Str, T>& Source, bool CanUseByName, bool Update);
    template<typename T>
    typename std::vector<T>::iterator Search(std::vector<T>& Source, bool CanUseByName, const std::function<Str(const T&)>& GetName) const;
    template<typename T>
    typename std::unordered_map<Str, T>::iterator Search(std::unordered_map<Str, T>& Source, bool CanUseByName) const;
    template<typename T>
    typename std::vector<T>::const_iterator Search(const std::vector<T>& Source, bool CanUseByName, bool Update, const std::function<Str(const T&)>& GetName);
    template<typename T>
    typename std::unordered_map<Str, T>::const_iterator Search(const std::unordered_map<Str, T>& Source, bool CanUseByName, bool Update);
    template<typename T>
    typename std::vector<T>::const_iterator Search(const std::vector<T>& Source, bool CanUseByName, const std::function<Str(const T&)>& GetName) const;
    template<typename T>
    typename std::unordered_map<Str, T>::const_iterator Search(const std::unordered_map<Str, T>& Source, bool CanUseByName) const;
};


struct IBB_DIndex :public IBB_TDIndex<std::string>
{
    std::string GetText() const;
    IBB_DIndex() :IBB_TDIndex<std::string>() {}
    IBB_DIndex(int _Index) :IBB_TDIndex<std::string>(_Index) {}
    IBB_DIndex(const std::string& _Name) :IBB_TDIndex<std::string>(_Name) {}
};

struct IBG_SettingPack
{
    int32_t FrameRateLimit;
    static constexpr int32_t ____FrameRateLimit_Min = 15;
    static constexpr int32_t ____FrameRateLimit_Max = 2000;
    static constexpr int32_t ____FrameRateLimit_SpV = -1;
    static constexpr int32_t ____FrameRateLimit_Def = 25;

    int32_t FontSize;
    static constexpr int32_t ____FontSize_Min = 12;
    static constexpr int32_t ____FontSize_Max = 48;
    static constexpr int32_t ____FontSize_Def = 24;

    int32_t MenuLinePerPage;
    static constexpr int32_t ____MenuLinePerPage_Min = 5;
    static constexpr int32_t ____MenuLinePerPage_Max = INT_MAX;
    static constexpr int32_t ____MenuLinePerPage_Def = 10;

    int32_t ScrollRateLevel;
    static constexpr int32_t ____ScrollRate_Min = 1;
    static constexpr int32_t ____ScrollRate_Max = 6;
    static constexpr int32_t ____ScrollRate_Def = 3;

    int32_t WindowTransparencyLevel;
    static constexpr int32_t ____WindowTransparency_Min = 1;
    static constexpr int32_t ____WindowTransparency_Max = 10;
    static constexpr int32_t ____WindowTransparency_Def = 8;

    bool DarkMode;
    static constexpr bool ____DarkMode_Def = false;

    bool OpenFolderOnOutput;
    static constexpr bool ____OpenFolderOnOutput_Def = true;

    bool OutputOnSave;
    static constexpr bool ____OutputOnSave_Def = false;

    void SetDefault()
    {
        FrameRateLimit = ____FrameRateLimit_Def;
        FontSize = ____FontSize_Def;
        MenuLinePerPage = ____MenuLinePerPage_Def;
        ScrollRateLevel = ____ScrollRate_Def;
        DarkMode = ____DarkMode_Def;
        OpenFolderOnOutput = ____OpenFolderOnOutput_Def;
        OutputOnSave = ____OutputOnSave_Def;
        WindowTransparencyLevel = ____WindowTransparency_Def;
    }

    static constexpr float ScrollRateArray[7]
    {
        0.0f, 0.8f, 1.10378f, 1.52292f, 2.10122f, 2.89912f, 4.0f
    };

    float GetScrollRate() const
    {
        return ScrollRateArray[ScrollRateLevel];
    }

    float GetTransparencyBase() const
    {
        return WindowTransparencyLevel * 0.1f;
    }
};

struct IBB_SettingType
{
    enum _Type
    {
        None, IntA, IntB, Bool
    }Type;

    std::string DescShort, DescLong;
    void* Data;
    std::vector<const void*> Limit;
    std::shared_ptr<bool> Changing{ new bool(false) };
};

struct IBB_SettingTypeList
{
    IBG_SettingPack Pack;
    std::string _TEXT_UTF8 LastOutputDir;

    std::vector<IBB_SettingType> Types;

    IBB_SettingTypeList();
    void PackSetDefault();
};
typedef std::unordered_map<IBB_SettingType::_Type, std::function<void(const IBB_SettingType&)>> SettingTypeMap;


void IBB_SettingRegisterRW(SaveFile& Save);

const IBG_SettingPack& IBG_GetSetting();
void IBB_SetGlobalSetting(const IBG_SettingPack& Pack);





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
    };

    std::string Name,DescShort,DescLong;
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

    bool Load(JsonObject FromJson,const std::unordered_map<std::string, IBB_IniLine_Default>& LineMap);
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
    IBB_SubSec(IBB_SubSec_Default* D,IBB_Section* R) : Default(D), Root(R) {}
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



struct IBB_Section_Desc
{
    std::string Ini, Sec;
    bool operator==(const IBB_Section_Desc& Ano) const { return Ini == Ano.Ini && Sec == Ano.Sec; }
    IBB_Section_Desc() = default;
    IBB_Section_Desc(const std::string& i, const std::string& s) : Ini(i), Sec(s) {};
    std::string GetText() const;
};
bool operator<(const IBB_Section_Desc& A, const IBB_Section_Desc& B);

struct IBB_Project_Index
{
    IBB_DIndex Ini;
    IBB_DIndex Section;

    IBB_Project_Index() {}
    IBB_Project_Index(const std::string& _Ini, const std::string& _Sec = "") :Ini(_Ini), Section(_Sec) {}
    IBB_Project_Index(const IBB_Section_Desc& Desc) :Ini(Desc.Ini), Section(Desc.Sec) {}

    const IBB_Ini* GetIni(const IBB_Project& Proj);
    const IBB_Section* GetSec(const IBB_Project& Proj);
    const IBB_Ini* GetIni(const IBB_Project& Proj) const;
    const IBB_Section* GetSec(const IBB_Project& Proj) const;
    IBB_Ini* GetIni(IBB_Project& Proj);
    IBB_Section* GetSec(IBB_Project& Proj);
    IBB_Ini* GetIni(IBB_Project& Proj) const;
    IBB_Section* GetSec(IBB_Project& Proj) const;

    bool operator==(const IBB_Project_Index& A) const
    {
        return A.Ini == Ini && A.Section == Section;
    }
    bool SameTarget(const IBB_Project& Proj, const IBB_Project_Index& A) const;

    std::string GetText() const;
};

struct IBB_Link_NameType
{
    std::string FromIni, FromSec, ToIni, ToSec;
    IBB_Link_NameType() {}
    IBB_Link_NameType(const std::string& fi, const std::string& fs, const std::string& ti, const std::string& ts) :
        FromIni(fi), FromSec(fs), ToIni(ti), ToSec(ts) {}

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
    size_t Order, OrderEx;//OrderEx=INT_MAX to a linkgroup

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

struct IBB_VariableList
{
    IBB_VariableList* UpValue{ nullptr };
    std::unordered_map<std::string, std::string>Value;

    bool Load(JsonObject FromJson);
    void FillKeys(const std::vector<std::string>& List,const std::string& Val);
    void Merge(const IBB_VariableList& Another, bool MergeUpValue);

    const std::string& GetVariable(const std::string& Name) const;
    bool CoverUpValue(const std::string& Name) const;
    bool HasValue(const std::string& Name) const;
    std::string GetText(bool ConsiderUpValue) const;
    void Flatten(IBB_VariableList& Target) const;

    void Read(const ExtFileClass& File);//不管UpValue
    void Write(const ExtFileClass& File)const;//不管UpValue
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

struct IniToken;
struct ModuleClipData;

struct IBB_Section
{
    IBB_Ini* Root;


    std::string Name;
    std::vector<IBB_SubSec> SubSecs;
    std::vector<IBB_Link> LinkedBy;
    IBB_VariableList VarList;
    IBB_VariableList UnknownLines;//不归属于任一SubSec

    bool IsLinkGroup{ false };//no subsec varlist unklines
    std::vector<IBB_Link> LinkGroup_LinkTo;
    std::unordered_map<std::string, std::string> DefaultLinkKey;
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

struct IBB_RegisterList_NameType
{
    std::string Type;
    std::string IniType;
    std::string TargetIniType;
    std::vector <std::string> List;

    std::vector <std::string> TargetIniTypeList;
    bool UseTargetIniTypeList{ false };

    void Read(const ExtFileClass& File);
    void Write(const ExtFileClass& File)const;
};

struct IBB_RegisterList
{
    IBB_Project* Root;

    std::string Type;
    std::string IniType;
    std::vector<IBB_Section*> List;

    bool ChangeRoot(IBB_Project* NewRoot) { Root = NewRoot; return true; }
    IBB_RegisterList& ChangeRootAndBack(IBB_Project* NewRoot) { Root = NewRoot; return *this; }

    bool Merge(const IBB_RegisterList& Another);

    std::string GetText(bool PrintExtraData) const;
    IBB_RegisterList_NameType GetNameType() const;
};

struct IBB_ModuleAlt;
struct ModuleClipData;

struct IBB_Project
{
    std::wstring ProjName;//Name or Path ? Unsure.
    std::wstring Path;//绝对路径
    std::wstring LastOutputDir;//上次导出的路径
    bool IsNewlyCreated{ false };
    bool ChangeAfterSave{ false };
    std::vector<IBB_RegisterList> RegisterLists;
    std::vector<IBB_Ini> Inis;
    uint64_t CreateTime;//Once created never change  (microseconds)
    uint64_t LastUpdate;//last time the project was saved (microseconds)
    int CreateVersionMajor, CreateVersionMinor, CreateVersionRelease;//Once created never change

    const IBB_Ini* GetIni(const IBB_Project_Index& Index) const;
    const IBB_Section* GetSec(const IBB_Project_Index& Index) const;
    IBB_Ini* GetIni(IBB_Project_Index& Index) const;
    IBB_Section* GetSec(IBB_Project_Index& Index) const;
    IBB_Project_Index GetSecIndex(const std::string& Name) const;

    bool CreateIni(const std::string& Name);
    bool AddIni(const IBB_Ini& Ini, bool IsDuplicate);
    bool CreateRegisterList(const std::string& Name, const std::string& IniName);
    bool AddRegisterList(const IBB_RegisterList& List);
    IBB_Section* AddNewSection(const IBB_Section_NameType& Paragraph);
    IBB_Section* CreateNewSection(const IBB_Section_Desc& Desc);
    bool AddNewLinkToLinkGroup(const IBB_Section_Desc& From, const IBB_Section_Desc& To);

    IBB_RegisterList& GetRegisterList(const std::string& Name, const std::string& IniName);//找不到就返回一个新建的
    bool RegisterSection(const std::string& Name, const std::string& IniName, IBB_Section& Section);
    bool RegisterSection(size_t RegListID, IBB_Section& Section);

    bool AddModule(const IBB_ModuleAlt& Module);
    bool AddModule(const ModuleClipData& Module);
    bool UpdateAll();//After AddModule - It affects the WHOLE project. Don't use it frequently.
    _TEXT_UTF8 std::string GetText(bool PrintExtraData) const;

    void Clear();
    bool IsEmpty() const;
private:
    IBB_Section* AddNewSectionEx(const IBB_Section_NameType& Paragraph);
};


struct IBB_DefaultTypeAlt
{
    std::string Name, DescLong, DescShort, LinkType;
    int LinkLimit{ 1 };
    ImU32 Color;

    bool Load(JsonObject FromJson);
    bool Load(const std::vector<std::string>& FromCSV);
};

struct IBB_DefaultTypeAltList
{
    std::vector<IBB_DefaultTypeAlt> List;

    bool Load(JsonObject FromJson);
    bool LoadFromJsonFile(const char* Name);
    bool LoadFromCSVFile(const char* Name);
};

struct IBB_DefaultTypeList
{
private:
public:
    // std::unordered_map<Name, Object>
    std::unordered_map<std::string, JsonObject> Require_Default;
    std::unordered_map<std::string, IBB_IniLine_Default> IniLine_Default;
    std::unordered_map<std::string, IBB_SubSec_Default> SubSec_Default;//一个IniLine只能属于一个SubSec
    std::unordered_map<std::string, IBB_Link_Default> Link_Default;

    struct _Query
    {
        // pair<match, line(to unordered_maps)>
        std::unordered_map<std::string, IBB_IniLine_Default*> IniLine_Default_Full;//Likely
        std::vector<std::pair<std::string, IBB_IniLine_Default*>> IniLine_Default_RegexFull;
        std::vector<std::pair<std::string, IBB_IniLine_Default*>> IniLine_Default_RegexNotFull;
        std::vector<std::pair<std::string, IBB_IniLine_Default*>> IniLine_Default_RegexNone;
        std::vector<std::pair<std::string, IBB_IniLine_Default*>> IniLine_Default_RegexNotNone;
        std::vector<std::pair<std::string, IBB_IniLine_Default*>> IniLine_Default_Special;
        /* Initalized in constructor */std::unordered_map<std::string, std::function<bool(const std::string&)>> IniLine_Default_Special_FunctionList;

        // <LineName,SubSec>
        std::unordered_map<std::string, IBB_SubSec_Default*> SubSec_Default_FromLineID;

        _Query();
    }Query;


    JsonFile RootJson;

    bool LoadFromAlt(const IBB_DefaultTypeAltList& AltList);

    bool BuildQuery();//call after load
    void EnsureType(const std::string& Key, const std::string& LinkType);
    void EnsureType(const IBB_DefaultTypeAlt& Alt, std::set<std::string>* pSet = nullptr);

    IBB_IniLine_Default* KeyBelongToLine(const std::string& KeyName) const;//Order:String-Special-Regex
    IBB_SubSec_Default* KeyBelongToSubSec(const std::string& KeyName) const;
};






















template<typename Str>
bool IBB_TDIndex<Str>::Load(JsonObject FromJson)
{
    auto Item = FromJson.GetObjectItem(u8"Index");
    if (Item.IsTypeString())
    {
        UseIndex = false;
        Name = Item.GetString();
        return true;
    }
    else if (Item.IsTypeNumber())
    {
        UseIndex = true;
        Index = Item.GetInt();
        return true;
    }
    else
    {
        return false;
    }
}

template<typename Str> template<typename T>
typename std::vector<T>::iterator IBB_TDIndex<Str>::Search(std::vector<T>& Source, bool CanUseByName, bool Update, const std::function<Str(const T&)>& GetName)
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            int i = 0;
            for (const auto& It : Source)
            {
                if (GetName(It) == Name)break;
                i++;
            }
            if (Update)Index = i;
            return Source.begin() + i;
        }
        else
        {
            Index = Source.size();
            return Source.end();
        }
    }
    else
    {
        typename std::vector<T>::iterator Data = Source.begin() + Index;
        if (Update)Name = GetName(*Data);
        return Data;
    }
}
template<typename Str> template<typename T>
typename std::unordered_map<Str, T>::iterator IBB_TDIndex<Str>::Search(std::unordered_map<Str, T>& Source, bool CanUseByName, bool Update)
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            auto Ret = Source.find(Name);
            if (Update)
            {
                if (Ret != Source.end())
                {
                    int i = 0;
                    for (const auto& It : Source)
                    {
                        if (It.first == Name)break;
                        i++;
                    }
                    Index = i;
                }
                else Index = Source.size();
            }
            return Ret;
        }
        else
        {
            Index = Source.size();
            return Source.end();
        }
    }
    else
    {
        int i = 0;
        Str Nr{};
        for (const auto& It : Source)
        {
            if (Index == i)
            {
                Nr = It.first;
                break;
            }
            i++;
        }
        if (Update && !Nr.empty())Name = Nr;
        return Source.find(Nr);
    }
}

template<typename Str> template<typename T>
typename std::vector<T>::iterator IBB_TDIndex<Str>::Search(std::vector<T>& Source, bool CanUseByName, const std::function<Str(const T&)>& GetName) const
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            int i = 0;
            for (const auto& It : Source)
            {
                if (GetName(It) == Name)break;
                i++;
            }
            return Source.begin() + i;
        }
        else return Source.end();
    }
    else
    {
        typename std::vector<T>::iterator Data = Source.begin() + Index;
        return Data;
    }
}
template<typename Str> template<typename T>
typename std::unordered_map<Str, T>::iterator IBB_TDIndex<Str>::Search(std::unordered_map<Str, T>& Source, bool CanUseByName) const
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            auto Ret = Source.find(Name);
            return Ret;
        }
        else return Source.end();
    }
    else
    {
        int i = 0;
        Str Nr{};
        for (const auto& It : Source)
        {
            if (Index == i)
            {
                Nr = It.first;
                break;
            }
            i++;
        }
        return Source.find(Nr);
    }
}

template<typename Str> template<typename T>
typename std::vector<T>::const_iterator IBB_TDIndex<Str>::Search(const std::vector<T>& Source, bool CanUseByName, bool Update, const std::function<Str(const T&)>& GetName)
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            int i = 0;
            for (const auto& It : Source)
            {
                if (GetName(It) == Name)break;
                i++;
            }
            if (Update)Index = i;
            return Source.cbegin() + i;
        }
        else
        {
            Index = Source.size();
            return Source.cend();
        }
    }
    else
    {
        typename std::vector<T>::const_iterator Data = Source.cbegin() + Index;
        if (Update)Name = GetName(*Data);
        return Data;
    }
}
template<typename Str> template<typename T>
typename std::unordered_map<Str, T>::const_iterator IBB_TDIndex<Str>::Search(const std::unordered_map<Str, T>& Source, bool CanUseByName, bool Update)
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            auto Ret = Source.find(Name);
            if (Update)
            {
                if (Ret != Source.cend())
                {
                    int i = 0;
                    for (const auto& It : Source)
                    {
                        if (It.first == Name)break;
                        i++;
                    }
                    Index = i;
                }
                else Index = Source.size();
            }
            return Ret;
        }
        else
        {
            Index = Source.size();
            return Source.cend();
        }
    }
    else
    {
        int i = 0;
        Str Nr{};
        for (const auto& It : Source)
        {
            if (Index == i)
            {
                Nr = It.first;
                break;
            }
            i++;
        }
        if (Update && !Nr.empty())Name = Nr;
        return Source.find(Nr);
    }
}

template<typename Str> template<typename T>
typename std::vector<T>::const_iterator IBB_TDIndex<Str>::Search(const std::vector<T>& Source, bool CanUseByName, const std::function<Str(const T&)>& GetName) const
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            int i = 0;
            for (const auto& It : Source)
            {
                if (GetName(It) == Name)break;
                i++;
            }
            return Source.cbegin() + i;
        }
        else return Source.cend();
    }
    else
    {
        typename std::vector<T>::const_iterator Data = Source.cbegin() + Index;
        return Data;
    }
}
template<typename Str> template<typename T>
typename std::unordered_map<Str, T>::const_iterator IBB_TDIndex<Str>::Search(const std::unordered_map<Str, T>& Source, bool CanUseByName) const
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            auto Ret = Source.find(Name);
            return Ret;
        }
        else return Source.cend();
    }
    else
    {
        int i = 0;
        Str Nr{};
        for (const auto& It : Source)
        {
            if (Index == i)
            {
                Nr = It.first;
                break;
            }
            i++;
        }
        return Source.find(Nr);
    }
}
