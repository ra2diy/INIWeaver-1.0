#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_RegType.h"
#include<imgui_internal.h>

extern const char* LinkGroup_IniName;

std::string CutSpace(const std::string& ss)//REPLACE ORIG
{
    auto fp = ss.find_first_not_of(" \011\r\n\t"), bp = ss.find_last_not_of(" \011\r\n\t");
    return std::string(ss.begin() + (fp == ss.npos ? 0 : fp),
        ss.begin() + (bp == ss.npos ? 0 : bp + 1));
}
std::vector<std::string> SplitParam(const std::string& Text)//ORIG
{
    if (Text.empty())return {};
    size_t cur = 0, crl;
    std::vector<std::string> ret;
    while ((crl = Text.find_first_of(',', cur)) != Text.npos)
    {
        ret.push_back(CutSpace(std::string(Text.begin() + cur, Text.begin() + crl)));
        cur = crl + 1;
    }
    auto U = CutSpace(std::string(Text.begin() + cur, Text.end()));
    if(!U.empty())ret.push_back(std::move(U));
    return ret;
}

void MergeList(std::vector<std::string>& Value, const std::string& Str)
{
    auto S = SplitParam(Str);
    Value.reserve(Value.size() + S.size());
    for (auto&& V : S)
        Value.push_back(std::move(V));
    //std::set<std::string> SS;
    //Value.erase(std::remove_if(Value.begin(), Value.end(), [&](const std::string& s)->bool {return !SS.insert(s).second; }), Value.end());
}

std::string DecodeListForExport(const std::string& Val)
{
    if (Val.empty())return "";
    IBB_Section_Desc Desc{ LinkGroup_IniName, Val };
    auto pSec = IBF_Inst_Project.Project.GetSec(Desc);
    if (pSec && pSec->IsLinkGroup)
    {
        std::string R;
        for (auto& V : pSec->LinkGroup_LinkTo)
        {
            auto pp = V.To.GetSec(IBF_Inst_Project.Project);
            if (pp)
            {
                R += pp->Name;
                R += ',';
            }
        }
        if (!R.empty())R.pop_back();
        return R;
    }
    else
    {
        return Val;
    }
}

LineData IBB_IniLine_DataList::Duplicate() const
{
    std::shared_ptr<IBB_IniLine_Data_Base> R{ new IBB_IniLine_DataList };
    R->MergeData(this);
    return R;
}

bool IBB_IniLine_DataList::SetValue(const std::string& Val)
{
    Value = SplitParam(Val);
    _Empty = Value.empty();
    return true;
}
bool IBB_IniLine_DataList::MergeValue(const std::string& Val)
{
    if (!Val.empty())MergeList(Value, Val);
    _Empty = Value.empty();
    return true;
}
void IBB_IniLine_DataList::UpdateAsDuplicate()
{
    for (auto& V : Value)
    {
        auto it = IBR_Inst_Project.CopyTransform.find(V);
        if (it != IBR_Inst_Project.CopyTransform.end())
            V = it->second;
    }
}
bool IBB_IniLine_DataList::MergeData(const IBB_IniLine_Data_Base* data)
{
    auto Data = dynamic_cast<const IBB_IniLine_DataList*>(data);
    if (Data == nullptr)return false;
    if (Data->_Empty)return true;
    Value.insert(Value.end(), Data->Value.begin(), Data->Value.end());
    _Empty = Value.empty();
    return true;
}
bool IBB_IniLine_DataList::Clear()
{
    _Empty = true;
    Value.clear();
    return true;
}

std::string IBB_IniLine_DataList::GetString() const
{
    std::string Ret;
    for (auto& V : Value)
    {
        Ret += V;
        Ret.push_back(',');
    }
    if (!Ret.empty())Ret.pop_back();
    return Ret;
}
std::string IBB_IniLine_DataList::GetStringForExport() const
{
    std::string Ret;
    for (auto& V : Value)
    {
        Ret += DecodeListForExport(V);
        Ret.push_back(',');
    }
    if (!Ret.empty())Ret.pop_back();
    return Ret;
}

void IBB_IniLine_DataList::RemoveValue(const std::string& Val)
{
    Value.erase(std::remove_if(Value.begin(), Value.end(), [&](const std::string& V)->bool {return V == Val; }), Value.end());
}
void IBB_IniLine_DataList::RemoveValue(size_t Idx)
{
    Value.erase(Value.begin() + Idx);
}
void IBB_IniLine_DataList::InsertValue(const std::string& Val, size_t Idx)
{
    Value.insert(Value.begin() + Idx, Val);
}
void IBB_IniLine_DataList::ReplaceValue(const std::string& Old, const std::string& New)
{
    for (auto& s : Value)if (s == Old)s = New;
}


extern const char* DefaultAltPropType;
extern const char* LinkAltPropType;

IBB_SubSec::IBB_SubSec(IBB_SubSec&& A) :
    Root(A.Root), Default(A.Default), Lines_ByName(std::move(A.Lines_ByName)), Lines(std::move(A.Lines)), LinkTo(std::move(A.LinkTo))
{}

bool IBB_SubSec::Merge(const IBB_SubSec& Another, const std::unordered_map<std::string, IBB_IniMergeMode>& MergeType, bool IsDuplicate)
{
    bool Ret = true;
    for (auto p : Another.Lines)
    {
        auto it = Lines.find(p.first);
        if (it == Lines.end())
        {
            Lines_ByName.push_back(p.first);
            if (IsDuplicate)
            {
                IBB_IniLine np = p.second.Duplicate();
                Lines.insert({ p.first,np });
            }
            else Lines.insert(p);
        }
        else
        {
            const auto& Mode = MergeType.find(p.first);
            if (!it->second.Merge(p.second, Mode == MergeType.end() ? IBB_IniMergeMode::Replace : Mode->second))Ret = false;
        }
    }
    if (Root != nullptr)
    {
        //auto pproj = Root->Root->Root;
        auto ti = Root->GetThisIndex();
        for (const auto& p : Another.LinkTo)
        {
            LinkTo.push_back(p);
            LinkTo.back().From = ti;
        }
    }
    return Ret;
}
bool IBB_SubSec::Merge(const IBB_SubSec& Another, IBB_IniMergeMode Mode, bool IsDuplicate)
{
    bool Ret = true;
    for (auto p : Another.Lines)
    {
        auto it = Lines.find(p.first);
        if (it == Lines.end())
        {
            Lines_ByName.push_back(p.first);
            if (IsDuplicate)
            {
                IBB_IniLine np = p.second.Duplicate();
                Lines.insert({ p.first,np });
            }
            else
            {
                Lines.insert(p);
            }
        }
        else
        {
            if (!it->second.Merge(p.second, Mode))Ret = false;
        }
    }
    if (Root != nullptr)
    {
        //auto pproj = Root->Root->Root;
        auto ti = Root->GetThisIndex();
        for (const auto& p : Another.LinkTo)
        {
            LinkTo.push_back(p);
            LinkTo.back().From = ti;

        }
    }
    return Ret;
}
bool IBB_SubSec::AddLine(const std::pair<std::string, std::string>& Line)
{
    auto it = Lines.find(Line.first);
    if (it == Lines.end())
    {
        IBB_IniLine_Default* Def = IBF_Inst_DefaultTypeList.List.KeyBelongToLine(Line.first);
        if (Def == nullptr)return false;
        Lines_ByName.push_back(Line.first);
        auto rp = Lines.insert({ Line.first,IBB_IniLine(Line.second, Def) });
        if (!Def->Property.TypeAlt.empty() && Def->Property.TypeAlt != "bool")
            Root->OnShow[Line.first] = EmptyOnShowDesc;
        return true;
    }
    else
    {
        return it->second.Merge(Line.second, IBB_IniMergeMode::Merge);
    }
}

bool IBB_SubSec::ChangeRoot(IBB_Section* NewRoot)
{
    if (NewRoot != nullptr)
    {
        auto ti = NewRoot->GetThisIndex();
        for (auto& L : LinkTo)L.From = ti;
    }
    Root = NewRoot;
    return true;
}

std::string IBB_SubSec::GetText(bool PrintExtraData, bool FromExport) const
{
    std::string Text;
    if (PrintExtraData)
    {
        Text.push_back(';'); Text += (Default == nullptr ? std::string{ "MISSING SubSec Default" } : Default->Name); Text.push_back('\n');
    }
    for (const auto& sn : Lines_ByName)
    {
        auto It = Lines.find(sn);
        if (It == Lines.end())
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                auto sl = UTF8toUnicode(sn);
                GlobalLogB.AddLog(std::vformat(L"IBB_SubSec::GetText ：" + locw("Log_SubSecNotExist"), std::make_wformat_args(sl)));
            }
        auto& L = It->second;
        if (FromExport)
        {
            auto ex = L.Data->GetStringForExport(/*Root->Root->Name*/);
            if (ex.empty())continue;
            if (sn == "__INHERIT__")continue;
            Text += sn;
            Text += '=';
            Text += ex;
            Text += '\n';
        }
        else
        {
            if (sn == "__INHERIT__")continue;
            Text += sn;
            Text += '=';
            Text += L.Data->GetString();
            Text += '\n';
        }
    }
    if (PrintExtraData)
    {
        //Text += ";Extra Data - Link To\n";
        //for (const auto& to : LinkTo)
        //    Text += to.GetText(*(Root->Root->Root));//SubSec->Sec->Ini->Project

    }
    return Text;
}
std::vector<std::string> IBB_SubSec::GetKeys(bool PrintExtraData) const
{
    std::vector<std::string> Ret;
    if (PrintExtraData)Ret.push_back("_DEFAULT_NAME");
    for (const auto& sn : Lines_ByName)
    {
        if (Lines.find(sn) == Lines.end())
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                auto sl = UTF8toUnicode(sn);
                GlobalLogB.AddLog(std::vformat(L"IBB_SubSec::GetKeys ：" + locw("Log_SubSecNotExist"), std::make_wformat_args(sl)));
            }
        Ret.push_back(sn);
    }
    if (PrintExtraData)for (const auto& L : LinkTo)
    {
        auto pf = L.From.GetSec(*(Root->Root->Root));
        if (pf != nullptr)Ret.push_back("_LINK_FROM_" + pf->Name);
    }
    return Ret;
}
IBB_VariableList IBB_SubSec::GetLineList(bool PrintExtraData) const
{
    IBB_VariableList Ret;
    if (PrintExtraData)Ret.Value["_DEFAULT_NAME"] = (Default == nullptr ? std::string{ "MISSING SubSec Default" } : Default->Name);
    for (const auto& sn : Lines_ByName)
    {
        auto It = Lines.find(sn);
        if (It == Lines.end())
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                auto sl = UTF8toUnicode(sn);
                GlobalLogB.AddLog(std::vformat(L"IBB_SubSec::GetLineList ：" + locw("Log_SubSecNotExist"), std::make_wformat_args(sl)));
            }
        auto& L = It->second;
        if (L.Default == nullptr)Ret.Value[sn] = "MISSING Line Default";
        else
        {
            Ret.Value[sn] = L.Data->GetString();
        }
    }
    if (PrintExtraData)for (const auto& L : LinkTo)
    {
        auto pf = L.From.GetSec(*(Root->Root->Root)), pt = L.To.GetSec(*(Root->Root->Root));
        if (pf != nullptr && pt != nullptr)
            Ret.Value["_LINK_FROM_" + pf->Name] = "_LINK_TO_" + pt->Name;
    }
    return Ret;
}

std::string IBB_SubSec::GetFullVariable(const std::string& Name) const
{
    auto it = Lines.find(Name);
    if (it == Lines.end())return "";
    if (!it->second.Data)return "";
    return it->second.Data->GetString();
}

IBB_SubSec IBB_SubSec::Duplicate() const
{
    IBB_SubSec Ret;
    Ret.Root = Root;
    Ret.Default = Default;
    Ret.Lines_ByName = Lines_ByName;
    Ret.LinkTo = LinkTo;
    Ret.Lines.reserve(Lines.size());
    for (const auto& p : Lines)Ret.Lines.insert({ p.first,p.second.Duplicate() });
    return Ret;
}

void IBB_SubSec::GenerateAsDuplicate(const IBB_SubSec& Src)
{
    Root = Src.Root;
    Default = Src.Default;
    Lines_ByName = Src.Lines_ByName;
    LinkTo = Src.LinkTo;
    Lines.reserve(Src.Lines.size());
    for (const auto& p : Src.Lines)Lines.insert({ p.first,p.second.Duplicate() });
}

bool IBB_SubSec::UpdateAll()
{
    bool Ret = true;
    if (Default == nullptr)Ret = false;
    std::vector<IBB_Link> NewLT;
    NewLT.reserve(LinkTo.size());
    //auto pproj = Root->Root->Root;
    for (auto& L : Lines)
    {
        if (EnableLogEx)
        {
            GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_SubSec::UpdateAll Line : ", false); GlobalLogB.AddLog(L.first.c_str());//BREAKPOINT
        }
        auto def = L.second.Default;
        if (def == nullptr)Ret = false;
        else
        {
            if (def->Property.Type == LinkAltPropType)
            {
                auto It = IBF_Inst_DefaultTypeList.List.Link_Default.find(def->Property.TypeAlt);
                auto pList = L.second.GetData<IBB_IniLine_DataList>();
                if (pList)
                {
                    auto& pV = pList->GetValue();
                    //留下无效的内容（
                    //pV.erase(std::remove_if(pV.begin(), pV.end(), [](const std::string& v)->bool {return IBB_Project_Index{ DefaultIniName ,v }.GetSec(IBF_Inst_Project.Project) == nullptr; }), pV.end());
                    auto Limit = reinterpret_cast<int>(def->Property.Lim.GetRaw());

                    //改成了扔掉最新的链接作“连不上”状
                    if (Limit > 1)while ((int)pV.size() > Limit)pV.pop_back();
                    if (Limit == 1 && (int)pV.size() > Limit)pV.erase(pV.begin());
                    //if (Limit > 0 && (int)pV.size() > Limit)pV.erase(pV.begin(), pV.begin() + pV.size() - Limit);
                    //auto& RegType = IBB_DefaultRegType::GetRegType(L.second.Default->Property.TypeAlt);
                    for (auto& V : pV)
                    {
                        //MessageBoxA(MainWindowHandle, V.c_str(), L.first.c_str(), MB_OK);
                        NewLT.emplace_back(
                            (It == IBF_Inst_DefaultTypeList.List.Link_Default.end()) ? nullptr : (&(It->second)),
                            Root->GetThisIndex(),
                            IBF_Inst_Project.Project.GetSecIndex(V)//TODO : TEST!!
                        );
                        NewLT.back().FromKey = L.first;
                        if (EnableLogEx)
                        {
                            GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_SubSec::UpdateAll Line II Type : ", false);//BREAKPOINT
                            GlobalLogB.AddLog(NewLT.back().GetText(IBF_Inst_Project.Project).c_str());//BREAKPOINT
                         }
                    }
                }
                else Ret = false;
            }
            //TODO: Alt
            //TODO: LinkList
        }
    }
    LinkTo = NewLT;
    return Ret;
}
