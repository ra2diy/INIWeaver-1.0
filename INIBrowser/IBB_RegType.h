#pragma once

#include "FromEngine/Include.h"
#include "IBB_Components.h"

struct IBB_RegType
{
    std::string IniType;
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
    int Count;
    IBB_VariableList DefaultLinks;
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

struct IBG_InputType;

namespace IBB_DefaultRegType
{
    extern const ImColor DefaultColor;
    //create type && create ini
    void EnsureRegType(const _TEXT_UTF8 std::string& Type);
    bool Load(JsonObject Obj);
    bool LoadFromFile(const wchar_t* FileName);
    bool HasRegType(const _TEXT_UTF8 std::string& Type);
    IBB_RegType& GetRegType(const _TEXT_UTF8 std::string& Type);
    bool HasInputType(const _TEXT_UTF8 std::string& Type);
    IBG_InputType& GetInputType(const _TEXT_UTF8 std::string& Type);
    const bool MatchType(const _TEXT_UTF8 std::string& TypeA, const _TEXT_UTF8 std::string& TypeB);
    void GenerateDLK(const std::vector<PairClipString>& DLK1, const std::string& Register, IBB_VariableList& DefaultLinkKey);
    void SwitchLightColor();
    void SwitchDarkColor();
    void ClearModuleCount();
}
