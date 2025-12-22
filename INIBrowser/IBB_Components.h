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


struct IBB_VariableList
{
    IBB_VariableList* UpValue{ nullptr };
    std::unordered_map<std::string, std::string>Value;

    bool Load(JsonObject FromJson);
    void FillKeys(const std::vector<std::string>& List, const std::string& Val);
    void Merge(const IBB_VariableList& Another, bool MergeUpValue);

    const std::string& GetVariable(const std::string& Name) const;
    bool CoverUpValue(const std::string& Name) const;
    bool HasValue(const std::string& Name) const;
    std::string GetText(bool ConsiderUpValue, bool FromExport) const;
    void Flatten(IBB_VariableList& Target) const;

    void Read(const ExtFileClass& File);//不管UpValue
    void Write(const ExtFileClass& File)const;//不管UpValue
};

struct IBB_Section_Desc
{
    std::string Ini, Sec;
    bool operator==(const IBB_Section_Desc& Ano) const { return Ini == Ano.Ini && Sec == Ano.Sec; }
    IBB_Section_Desc() = default;
    IBB_Section_Desc(const IBB_Section_Desc&) = default;
    IBB_Section_Desc& operator=(const IBB_Section_Desc&) = default;
    IBB_Section_Desc(IBB_Section_Desc&& r) noexcept : Ini(std::move(r.Ini)), Sec(std::move(r.Sec)) {}
    IBB_Section_Desc(const std::string& i, const std::string& s) : Ini(i), Sec(s) {};
    std::string GetText() const;
};
bool operator<(const IBB_Section_Desc& A, const IBB_Section_Desc& B);


