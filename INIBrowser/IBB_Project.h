#pragma once
#include "FromEngine/Include.h"
#include "cjson/cJSON.h"
#include "IBB_Components.h"
#include "IBB_Ini.h"
#include "IBR_Combo.h"

#ifndef _TEXT_UTF8
#define _TEXT_UTF8
#endif

struct IniToken;
struct ModuleClipData;

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
};

struct IBB_ModuleAlt;
struct ModuleClipData;
struct IBB_DefaultTypeList;

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
    IBB_Project_Index GetSecIndex(const std::string& Name, const std::string& PriorIni) const;
    IBB_SectionID GetSecID(const std::string& Name, const std::string& PriorIni) const;
    IBB_LineLocation GetSecAndLineID(const std::string& KeyName, const std::string& PriorIni) const;

    bool CreateIni(const std::string& Name);
    bool CreateRegisterList(const std::string& Name, const std::string& IniName);
    bool AddRegisterList(const IBB_RegisterList& List);
    IBB_Section* CreateNewSection(const IBB_Section_Desc& Desc);
    bool AddNewLinkToLinkGroup(const IBB_Section_Desc& From, const IBB_Section_Desc& To);

    IBB_RegisterList& GetRegisterList(const std::string& Name, const std::string& IniName);//找不到就返回一个新建的
    bool RegisterSection(const std::string& Name, const std::string& IniName, IBB_Section& Section);
    bool RegisterSection(size_t RegListID, IBB_Section& Section);

    bool AddModule(const ModuleClipData& Module);
    bool UpdateAll();//After AddModule - It affects the WHOLE project. Don't use it frequently.
    _TEXT_UTF8 std::string GetText(bool PrintExtraData) const;

    void Clear();
    bool IsEmpty() const;
};


struct IBB_DefaultTypeAlt
{
    StrPoolID Name, LinkType, Input;
    DescPoolOffset DescLong, DescShort;
    int LinkLimit{ 1 };
    ImU32 Color{ 0xFF000000 };

    void Clear();
    bool Load(JsonObject FromJson);
    bool Load(const std::vector<std::string>& FromCSV);
};

struct IBB_DefaultTypeList
{
public:
    // std::unordered_map<Name, Object>
    std::unordered_map<StrPoolID, IBB_IniLine_Default> IniLine_Default;
    std::unordered_map<std::string, IBB_SubSec_Default> SubSec_Default;//一个IniLine只能属于一个SubSec

    bool LoadFromAlt();

    void EnsureType(const IBB_DefaultTypeAlt& Alt);
    void CreateUnknownType(StrPoolID KeyName);

    bool LoadFromJsonObject(JsonObject FromJson);
    bool LoadFromJsonFile(const wchar_t* Name);
    bool LoadFromCSVFile(const wchar_t* Name);

    IBB_IniLine_Default* KeyBelongToLine(const std::string& KeyName);
    IBB_SubSec_Default* KeyBelongToSubSec(const std::string& KeyName);
    IBB_IniLine_Default* KeyBelongToLine(StrPoolID KeyName);
    IBB_SubSec_Default* KeyBelongToSubSec(StrPoolID KeyName);
};







