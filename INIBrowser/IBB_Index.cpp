#include "IBB_Index.h"
#include "IBB_Components.h"
#include "Global.h"
#include "IBB_ModuleAlt.h"


std::string IBB_DIndex::GetText() const
{
    return (UseIndex && Name.empty()) ? ("<IDX:" + std::to_string(Index) + ">") : Name;
}


bool IBB_Project_Index::SameTarget(const IBB_Project& Proj, const IBB_Project_Index& A) const
{
    return GetSec(Proj) == A.GetSec(Proj);
}
std::string IBB_Project_Index::GetText() const
{
    return Ini.GetText() + "->" + Section.GetText();
}

bool operator<(const IBB_Project_Index& A, const IBB_Project_Index& B)
{
    if (A.Ini.Name < B.Ini.Name)return true;
    else if (A.Ini.Name > B.Ini.Name)return false;
    return (A.Section.Name < B.Section.Name);
}

IBB_Project_Index::IBB_Project_Index(const IBB_Section_Desc& Desc) : Ini(Desc.Ini), Section(Desc.Sec) {}

IBB_Section_Desc IBB_Project_Index::ToDesc() const
{
    return { Ini.GetText(),Section.GetText() };
}
PairClipString IBB_Project_Index::ToClipPair() const
{
    return { Ini.GetText(),Section.GetText() };
}

IBB_Project_Index::operator IBB_Section_Desc() const { return ToDesc(); }
IBB_Project_Index::operator PairClipString() const { return ToClipPair(); }

IBB_Ini* IBB_Project_Index::GetIni(IBB_Project& Proj)
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetIni ：Func I Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, true, [](const IBB_Ini& F) {return F.Name; });
    return (Iter == Proj.Inis.end()) ? nullptr : std::addressof(*Iter);
}
IBB_Section* IBB_Project_Index::GetSec(IBB_Project& Proj)
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetSec ：Func I Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, true, [](const IBB_Ini& F) {return F.Name; });
    if (Iter == Proj.Inis.end())return nullptr;
    auto Iter1 = Section.Search(Iter->Secs, true, false);
    return (Iter1 == Iter->Secs.end()) ? nullptr : std::addressof(Iter1->second);
}
IBB_Ini* IBB_Project_Index::GetIni(IBB_Project& Proj) const
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetIni ：Func II Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, [](const IBB_Ini& F) {return F.Name; });
    return (Iter == Proj.Inis.end()) ? nullptr : std::addressof(*Iter);
}
IBB_Section* IBB_Project_Index::GetSec(IBB_Project& Proj) const
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetSec ：Func II Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, [](const IBB_Ini& F) {return F.Name; });
    if (Iter == Proj.Inis.end())return nullptr;
    auto Iter1 = Section.Search(Iter->Secs, true);
    return (Iter1 == Iter->Secs.end()) ? nullptr : std::addressof(Iter1->second);
}
const IBB_Ini* IBB_Project_Index::GetIni(const IBB_Project& Proj)
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetIni ：Func III Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, true, [](const IBB_Ini& F) {return F.Name; });
    return (Iter == Proj.Inis.end()) ? nullptr : std::addressof(*Iter);
}
const IBB_Section* IBB_Project_Index::GetSec(const IBB_Project& Proj)
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetSec ：Func III Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, true, [](const IBB_Ini& F) {return F.Name; });
    if (Iter == Proj.Inis.end())return nullptr;
    auto Iter1 = Section.Search(Iter->Secs, true, false);
    return (Iter1 == Iter->Secs.end()) ? nullptr : std::addressof(Iter1->second);
}
const IBB_Ini* IBB_Project_Index::GetIni(const IBB_Project& Proj) const
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetIni ：Func IV Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, [](const IBB_Ini& F) {return F.Name; });
    return (Iter == Proj.Inis.end()) ? nullptr : std::addressof(*Iter);
}
const IBB_Section* IBB_Project_Index::GetSec(const IBB_Project& Proj) const
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetSec ：Func IV Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, [](const IBB_Ini& F) {return F.Name; });
    if (Iter == Proj.Inis.end())return nullptr;
    auto Iter1 = Section.Search(Iter->Secs, true);
    return (Iter1 == Iter->Secs.end()) ? nullptr : std::addressof(Iter1->second);
}
