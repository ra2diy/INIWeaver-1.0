#pragma once

#include "FromEngine/Include.h"
#include "IBB_Components.h"

struct IBB_RegType
{
    std::string IniType;
    ImColor FrameColor;
    ImColor FrameColorL;
    ImColor FrameColorD;
    bool Export;
    bool RegNameAsDisplay;
    bool UseOwnName;
    std::string Name;
    int Count;
    IBB_VariableList DefaultLinks;

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

namespace IBB_DefaultRegType
{
    extern const ImColor DefaultColor;
    //create type && create ini
    void EnsureRegType(const _TEXT_UTF8 std::string& Type);
    bool Load(JsonObject Obj);
    bool LoadFromFile(const wchar_t* FileName);
    IBB_RegType& GetRegType(const _TEXT_UTF8 std::string& Type);
    const bool MatchType(const _TEXT_UTF8 std::string& TypeA, const _TEXT_UTF8 std::string& TypeB);
    void GenerateDLK(const std::vector<PairClipString>& DLK1, const std::string& Register, IBB_VariableList& DefaultLinkKey);
    void SwitchLightColor();
    void SwitchDarkColor();
    void ClearModuleCount();
}
