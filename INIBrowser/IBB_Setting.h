#pragma once
#include "FromEngine/Include.h"
#include "cjson/cJSON.h"

#ifndef _TEXT_UTF8
#define _TEXT_UTF8
#endif

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
        None, IntA, IntB, Bool, Lang
    }Type;

    std::string DescShortOri, DescLongOri;
    void* Data;
    std::vector<const void*> Limit;
    std::shared_ptr<bool> Changing{ new bool(false) };
    std::string DescShort, DescLong;
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

