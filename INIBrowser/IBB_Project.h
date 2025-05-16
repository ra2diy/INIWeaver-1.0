#pragma once
#include "FromEngine/Include.h"
#include "cjson/cJSON.h"
#include "IBB_Components.h"
#include "IBB_Ini.h"

#ifndef _TEXT_UTF8
#define _TEXT_UTF8
#endif

struct IniToken;
struct ModuleClipData;



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
    std::unordered_map<std::string, std::wstring> LastOutputIniName;//上次导出的ini文件名

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
    std::vector<std::string> EnumVector;
    std::vector<std::string> EnumValue;

    std::vector<std::string> EnumSplit(const std::string& s, char delimiter);

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







