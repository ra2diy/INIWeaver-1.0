#pragma once

#include "FromEngine/Include.h"
#include "IBB_Components.h"

struct IBB_RegType
{

    std::string IniType; //如果从Register字段得到的可以直接访问；否则走GetIniTypeOfReg函数！

    ImColor FrameColor;         //Base Color
    ImColor FrameColorPlus1;    //Lightness  +
    ImColor FrameColorPlus2;    //Lightness  ++
    ImColor FrameColorH;        //Saturation -
    ImColor FrameColorL;
    ImColor FrameColorLPlus1;
    ImColor FrameColorLPlus2;
    ImColor FrameColorLH;
    ImColor FrameColorD;
    ImColor FrameColorDPlus1;
    ImColor FrameColorDPlus2;
    ImColor FrameColorDH;
    bool Export;
    bool RegNameAsDisplay;
    bool UseOwnName;
    bool ValidateOptions;
    std::string Name;
    std::string ExportName;
    int Count;
    std::unordered_map<StrPoolID, StrPoolID> DefaultLinks;
    std::unordered_map<std::string, std::string> Options;//AllowedValue : DisplayName ; if empty then any value allowed

    std::string GetNoName();
    std::string GetNoName(const std::string& Reg);
};

struct IBB_CompoundRegType
{
    std::string Name;
    std::string DisplayName;
    std::vector<std::string> Regs;
    IBB_VariableList DefaultLinks;
};

struct PairClipString;
struct LinkNodeSetting;
struct IBG_InputType;

extern const char* AnyTypeName;
extern const char* MyTypeName;
StrPoolID AnyTypeID();
StrPoolID MyTypeID();

ImColor LoadColorFromJson(JsonObject Obj, bool& Colored);
ImColor LoadColorFromJson(JsonObject Obj, const ImColor& Default);

namespace IBB_DefaultRegType
{
    extern const ImColor DefaultColor;
    //create type && create ini
    void EnsureRegType(const _TEXT_UTF8 std::string& Type);
    bool Load(JsonObject Obj);
    bool LoadFromFile(const wchar_t* FileName);
    bool HasRegType(const _TEXT_UTF8 std::string& Type);
    bool HasRegType(StrPoolID Type);
    IBB_RegType& GetRegType(const _TEXT_UTF8 std::string& Type);
    IBB_RegType& GetRegType(StrPoolID Type);
    const _TEXT_UTF8 std::string& GetIniTypeOfReg(const _TEXT_UTF8 std::string& Type);
    const _TEXT_UTF8 std::string& GetIniTypeOfReg(StrPoolID Type);
    bool HasInputType(const _TEXT_UTF8 std::string& Type);
    IBG_InputType& GetInputType(const _TEXT_UTF8 std::string& Type);
    IBG_InputType& GetDefaultInputType();
    ImColor GetDefaultNodeColor();
    LinkNodeSetting GetDefaultLinkNodeSetting();
    StrBoolType GetDefaultStrBoolType();
    IBG_InputType& SelectInputTypeByValue(const _TEXT_UTF8 std::string& Value);
    const bool MatchType(StrPoolID TypeA, StrPoolID TypeB);
    void GenerateDLK(const std::vector<PairClipString>& DLK1, StrPoolID Register, std::unordered_map<StrPoolID, StrPoolID>& DefaultLinkKey, std::unordered_map<StrPoolID, StrPoolID>*& UpValue);
    void SwitchLightColor();
    void SwitchDarkColor();
    void ClearModuleCount();
}
