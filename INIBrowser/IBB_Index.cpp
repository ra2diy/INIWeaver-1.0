#include "IBB_Index.h"
#include "IBB_Components.h"
#include "Global.h"
#include "IBB_ModuleAlt.h"


std::string IBB_DIndex::GetText() const
{
    return (UseIndex && Name.empty()) ? ("<IDX:" + std::to_string(Index) + ">") : Name;
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
bool IBB_Project_Index::Empty() const
{
    if (Ini.UseIndex)return false;
    if (Section.UseIndex)return false;
    if (Ini.Name.empty() && Section.Name.empty())return true;
    return false;
}


std::unordered_map<IBB_SectionID, IBB_Project_Index> IBB_SectionID_Index;

uint64_t DescToID(const std::string& Ini, const std::string& Sec)
{
    size_t h1 = Ini.empty() ? 0 : std::hash<std::string>{}(Ini);
    size_t h2 = Sec.empty() ? 0 : std::hash<std::string>{}(Sec);
    return (static_cast<uint64_t>(h1) << 32) | h2;
}
uint64_t DescToID(const IBB_Section_Desc& Desc)
{
    return DescToID(Desc.Ini, Desc.Sec);
}

IBB_SectionID::IBB_SectionID(const IBB_Section_Desc& Desc)
    : ID(DescToID(Desc))
{
    if(!IBB_SectionID_Index.contains(*this))
        IBB_SectionID_Index.emplace(*this, Desc);
}
IBB_SectionID::IBB_SectionID(const std::string& Ini, const std::string& Sec)
    : ID(DescToID(Ini, Sec))
{
    if (!IBB_SectionID_Index.contains(*this))
        IBB_SectionID_Index.emplace(*this, IBB_Project_Index(Ini, Sec));
}

bool IBB_SectionID::Empty() const
{
    return ID == 0;
}

IBB_SectionID::operator IBB_Section_Desc() const
{
    return ToDesc();
}
IBB_SectionID::operator PairClipString() const
{
    return ToClipPair();
}

const IBB_Ini* IBB_SectionID::GetIni(const IBB_Project& Proj)
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.GetIni(Proj);
    return nullptr;
}
const IBB_Section* IBB_SectionID::GetSec(const IBB_Project& Proj)
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.GetSec(Proj);
    return nullptr;
}
const IBB_Ini* IBB_SectionID::GetIni(const IBB_Project& Proj) const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.GetIni(Proj);
    return nullptr;
}
const IBB_Section* IBB_SectionID::GetSec(const IBB_Project& Proj) const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.GetSec(Proj);
    return nullptr;
}
IBB_Ini* IBB_SectionID::GetIni(IBB_Project& Proj)
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.GetIni(Proj);
    return nullptr;
}
IBB_Section* IBB_SectionID::GetSec(IBB_Project& Proj)
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.GetSec(Proj);
    return nullptr;
}
IBB_Ini* IBB_SectionID::GetIni(IBB_Project& Proj) const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.GetIni(Proj);
    return nullptr;
}
IBB_Section* IBB_SectionID::GetSec(IBB_Project& Proj) const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.GetSec(Proj);
    return nullptr;
}

std::string IBB_SectionID::GetText() const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.GetText();
    return "<InvalidID>";
}
std::string IBB_SectionID::Ini() const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.Ini.GetText();
    static const std::string EmptyStr;
    return EmptyStr;
}
std::string IBB_SectionID::Section() const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.Section.GetText();
    static const std::string EmptyStr;
    return EmptyStr;
}

IBB_Section_Desc IBB_SectionID::ToDesc() const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.ToDesc();
    return { "", "" };
}
PairClipString IBB_SectionID::ToClipPair() const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second.ToClipPair();
    return { "", "" };
}
IBB_Project_Index& IBB_SectionID::ToIndex() const
{
    auto it = IBB_SectionID_Index.find(*this);
    if (it != IBB_SectionID_Index.end())
        return it->second;
    static IBB_Project_Index EmptyIndex;
    return EmptyIndex;
}

void IBB_SectionID::ClearCache()
{
    IBB_SectionID_Index.clear();
}
