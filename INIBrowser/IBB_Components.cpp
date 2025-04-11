#include "IBB_Components.h"


std::string IBB_Section_Desc::GetText() const
{
    return Ini + "->" + Sec;
}



bool operator<(const IBB_Section_Desc& A, const IBB_Section_Desc& B)
{
    if (A.Ini < B.Ini)return true;
    else if (A.Ini > B.Ini)return false;
    return (A.Sec < B.Sec);
}

void IBB_VariableList::FillKeys(const std::vector<std::string>& List, const std::string& Val)
{
    for (const auto& s : List)Value[s] = Val;
}
void IBB_VariableList::Merge(const IBB_VariableList& Another, bool MergeUpValue)
{
    for (const auto& p : Another.Value)
        Value[p.first] = p.second;
    if (MergeUpValue && UpValue != nullptr && Another.UpValue != nullptr && UpValue != Another.UpValue)
        UpValue->Merge(*Another.UpValue, true);
}
const std::string& IBB_VariableList::GetVariable(const std::string& Name) const
{
    static std::string Null = "";
    auto It = Value.find(Name);
    if (It != Value.end())return It->second;
    else if (UpValue == nullptr)return Null;
    else return UpValue->GetVariable(Name);
}
bool IBB_VariableList::HasValue(const std::string& Name) const
{
    if ((Value.find(Name) != Value.end()))return true;
    else if (UpValue == nullptr)return false;
    else return UpValue->HasValue(Name);
}
bool IBB_VariableList::CoverUpValue(const std::string& Name) const
{
    if (UpValue == nullptr)return false;
    else return (Value.find(Name) != Value.end()) && UpValue->HasValue(Name);
}
std::string IBB_VariableList::GetText(bool ConsiderUpValue, bool FromExport) const
{
    if (!ConsiderUpValue)
    {
        std::string Ret;
        for (const auto& p : Value)
        {
            if (FromExport && p.second.empty())continue;
            else if (p.first.empty())continue;
            Ret += p.first;
            Ret.push_back('=');
            Ret += p.second;
            Ret.push_back('\n');
        }
        return Ret;
    }
    else
    {
        IBB_VariableList List;
        Flatten(List);
        return List.GetText(false, FromExport);
    }
}
void IBB_VariableList::Flatten(IBB_VariableList& Target) const
{
    if (UpValue != nullptr)UpValue->Flatten(Target);
    for (const auto& p : Value)
        Target.Value[p.first] = p.second;
}


bool IBB_VariableList::Load(JsonObject FromJson)
{
    Value = FromJson.GetMapString();
    return true;
}
