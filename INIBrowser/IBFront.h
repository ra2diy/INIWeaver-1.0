#pragma once

#include "IBB_Setting.h"
#include "IBB_Project.h"
#include "SaveFile.h"

void IBF_Thr_FrontLoop();

struct IBF_SettingType
{
    std::string DescLong;
    std::function<bool()> Action;
};

class IBF_Setting
{
    SaveFile SettingFile;
public:
    IBB_SettingTypeList List;
    bool ReadSetting(const wchar_t* Name);
    bool SaveSetting(const wchar_t* Name);
    void UploadSettingBoard(std::function<void(const std::vector<IBF_SettingType>&)> Callback);
    bool IsDarkMode() { return List.Pack.DarkMode; }
    bool OpenFolderOnOutput() { return List.Pack.OpenFolderOnOutput; }
    bool OutputOnSave() { return List.Pack.OutputOnSave; }
    float TransparencyBase() { return List.Pack.GetTransparencyBase(); }
    float ScrollRate() { return List.Pack.GetScrollRate(); }
    std::string _TEXT_UTF8& OutputDir() { return List.LastOutputDir; }
};









struct IBF_DefaultTypeList
{
    IBB_DefaultTypeList List;
    void EnsureType(const std::string& Key, const std::string& LinkType);
    bool ReadAltSetting(const wchar_t* Name);
    const IBB_IniLine_Default* GetDefault(const std::string& Key) const;
};


struct IBS_Project;

struct IBF_Project
{
    IBB_Project Project;
    uint32_t CurrentProjectRID;
    std::unordered_map<std::string, IBB_Section_Desc> DisplayNames;

    bool HasDisplayName(const std::string& Name)
    {
        return DisplayNames.find(Name) != DisplayNames.end();
    }

    bool UpdateAll();
    bool UpdateCreateSection(const IBB_Section_Desc& Desc);

    void Load(const IBS_Project&);
    void Save(IBS_Project&);

    _TEXT_UTF8 std::string GetText(bool PrintExtraData) const;
};



