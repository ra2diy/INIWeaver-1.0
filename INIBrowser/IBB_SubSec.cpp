#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_RegType.h"
#include <imgui_internal.h>
#include <ranges>
#include "IBB_IniLine.h"

extern const char* Internal_IniName;

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

std::string IBB_NewLink::GetText() const
{
    return "FROM " + From.operator IBB_Section_Desc().GetText() + "." + PoolStr(FromKey) + " TO " + To.operator IBB_Section_Desc().GetText();
}

bool IBB_NewLink::Empty() const
{
    return From.Empty() || To.Empty();
}

IBB_SubSec::IBB_SubSec(IBB_SubSec&& A) noexcept :
    Root(A.Root), Default(A.Default), Lines_ByName(std::move(A.Lines_ByName)), Lines(std::move(A.Lines)), NewLinkTo(std::move(A.NewLinkTo))
{}

bool IBB_SubSec::MergeLine(StrPoolID Key, const std::string& Value, bool InitOnShow, IBB_IniMergeMode Mode, bool NoUpdate)
{
    /*sprintf_s(LogBufB, __FUNCTION__ ":  %s=%s Mode=%d InitOnShow=%s NoUpdate=%s",
        Line.first.c_str(), Line.second.c_str(), Mode, IBD_BoolStr(InitOnShow), IBD_BoolStr(NoUpdate));
    GlobalLogB.AddLog(LogBufB);*/

    bool Ret = true;
    auto it = Lines.find(Key);
    if (it == Lines.end())
    {
        IBB_IniLine_Default* Def = IBF_Inst_DefaultTypeList.List.KeyBelongToLine(Key);
        if (Def == nullptr)return false;
        Lines_ByName.push_back(Key);

        auto rp = Lines.insert({ Key, IBB_IniLine(Value, Def) });
        if (InitOnShow && Root->OnShow[Key].empty())
        {
            auto Str = PoolStr(Def->LinkNode.LinkType);
            if (!Str.empty() && Str != "bool")
                Root->OnShow[Key] = EmptyOnShowDesc;
        }
    }
    else
    {
        Ret = it->second.Merge(Value, Mode);
    }

    if (NoUpdate) return Ret;
    else return UpdateAll() && Ret;
}

bool IBB_SubSec::ChangeRoot(IBB_Section* NewRoot)
{
    if (NewRoot != nullptr)
    {
        auto ti = NewRoot->GetThisID();
        for (auto& L : NewLinkTo)L.From = ti;
    }
    Root = NewRoot;
    return true;
}

std::vector<StrPoolID> IBB_SubSec::GetKeys(bool PrintExtraData) const
{
    std::vector<StrPoolID> Ret;
    if (PrintExtraData)Ret.push_back(NewPoolStr("_DEFAULT_NAME"));
    for (const auto& sn : Lines_ByName)
    {
        if (Lines.find(sn) == Lines.end())
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                auto sl = UTF8toUnicode(PoolStr(sn));
                GlobalLogB.AddLog(std::vformat(L"IBB_SubSec::GetKeys ：" + locw("Log_SubSecNotExist"), std::make_wformat_args(sl)));
            }
        Ret.push_back(sn);
    }
    return Ret;
}
IBB_VariableList IBB_SubSec::GetLineList(bool PrintExtraData, bool FromExport, std::vector<std::string>* TmpLineOrder) const
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
                auto sl = UTF8toUnicode(PoolStr(sn));
                GlobalLogB.AddLog(std::vformat(L"IBB_SubSec::GetLineList ：" + locw("Log_SubSecNotExist"), std::make_wformat_args(sl)));
            }
        auto& L = It->second;
        if (FromExport)
        {
            if (sn == InheritKeyID())continue;
            L.MakeKVForExport(Ret, Root, TmpLineOrder);
        }
        else
        {
            if (sn == InheritKeyID())continue;
            Ret.Value[PoolStr(sn)] = L.Data->GetString();
        }
    }
    if (PrintExtraData)for (const auto& L : NewLinkTo)
    {
        auto pf = L.From.GetSec(*(Root->Root->Root)), pt = L.To.GetSec(*(Root->Root->Root));
        if (pf != nullptr && pt != nullptr)
            Ret.Value["_LINK_FROM_" + pf->Name] = "_LINK_TO_" + pt->Name;
    }
    return Ret;
}

std::pair< std::multimap<uint64_t, size_t>::const_iterator, std::multimap<uint64_t, size_t>::const_iterator>
IBB_SubSec::GetLink(size_t LineIdx, size_t ComponentIdx) const
{
    uint64_t l = LineIdx;
    uint64_t c = ComponentIdx;
    uint64_t i = (l << 32) | c;
    return LinkSrc.equal_range(i);
}
void IBB_SubSec::ClaimLink(size_t LineIdx, size_t ComponentIdx, size_t LinkIdx)
{
    uint64_t l = LineIdx;
    uint64_t c = ComponentIdx;
    uint64_t i = (l << 32) | c;
    LinkSrc.insert({ i, LinkIdx });
}

bool IBB_SubSec::CanOwnKey(StrPoolID Key) const
{
    auto pSub = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(Key);
    return pSub == Default;
}

const std::vector<std::string>& SplitParamCached(const std::string& Text);

bool IBB_SubSec::RenameInLinkTo(size_t LinkIdx, const std::string& OldName, const std::string& NewName)
{
    //按照这个Link，找到所有from的地方并修改to到新name
    //不需要修改Link结构
    bool Ret = true;
    //auto& Link = NewLinkTo[LinkIdx];
    for (auto& [lc, lidx] : LinkSrc)
    {
        if (lidx == LinkIdx)
        {
            uint64_t l = lc >> 32;
            uint64_t c = lc & 0xFFFFFFFF;
            size_t LineIdx = (size_t)l;
            size_t CompIdx = (size_t)c;
            auto& Key = Lines_ByName[LineIdx];
            auto& line = Lines.find(Key)->second;
            line.Data->Replace(CompIdx, OldName, NewName);
        }
    }
    return Ret;
}

#include "IBR_Components.h"

void IBB_SubSec::UpdateNewLinkTo(std::vector<IBB_NewLink>&& NewLT)
{
    auto& Proj = *(Root->Root->Root);
    for (auto& L : NewLinkTo)
        if (L.FromKey == ImportKeyID())
        {
            auto pf = L.To.GetSec(Proj);
            if (!pf)continue;
            pf->Dynamic.ImportCount--;
        }
    for (auto& L : NewLT)
        if (L.FromKey == ImportKeyID())
        {
            auto pf = L.To.GetSec(Proj);
            if (!pf)continue;
            pf->Dynamic.ImportCount++;
        }
    NewLinkTo = std::move(NewLT);
}

bool IBB_SubSec::UpdateAll()
{
    bool Ret = true;
    if (Default == nullptr)Ret = false;
    std::vector<IBB_NewLink> NewLT;
    NewLT.reserve(NewLinkTo.size() + 1);
    LinkSrc.clear();
    bool LimitFix = false;

    for (auto&& [LineIdx, L] : std::views::zip(std::views::iota(0u), Lines_ByName))
    {
        auto& Line = Lines[L];
        if (!Line.Default)continue;
        auto& KeyName = L;

        if (auto pd = Line.GetData<IBB_IniLine_Data_IIF>(); pd)
        {
            auto& iif = pd->Value;

            auto ldd = Line.Default && IBB_DefaultRegType::HasRegType(Line.Default->LinkNode.LinkType);
            std::set<std::pair<int, int>> SelectValues;

            int i = -1;
            for (auto& iic : *iif->InputComponents)
            {
                i++;
                if (!iic->SupportLinks())continue;
                auto id = iic->GetCurrentTargetValueID();
                if (ldd)
                    SelectValues.insert({ id, i });
                else if (iic->UseCustomSetting)//确实是常驻的Node
                    SelectValues.insert({ id, i });
                else if (iic->InitialStatus.InputMethod == IICStatus::Link)
                    SelectValues.insert({ id, i });
                else if (iif->GetComponentStatus()[i].InputMethod == IICStatus::Link)
                    SelectValues.insert({ id, i });
            }

            auto& val = iif->GetValues();
            auto DefaultLinkLimit = Line.Default->GetLinkLimit();

            for (auto&& [id, cidx] : SelectValues)
            {
                if (!val.Values.contains(id))continue;
                auto& V = val.Values[id];

                auto& piic = iif->InputComponents->at(cidx);
                auto LinkLimit = piic->UseCustomSetting ? piic->NodeSetting.LinkLimit : DefaultLinkLimit;
                auto& spc = SplitParamCached(V.Value);
                if (LinkLimit != -1 && LinkLimit != 0 && (int)spc.size() > LinkLimit)
                {
                    LimitFix = true;
                    std::string NewStr;
                    if (LinkLimit == 1)NewStr = spc.back();
                    else NewStr = spc |
                        std::views::take(LinkLimit) |
                        std::views::join_with(',') |
                        std::ranges::to<std::string>();
                    V.Value = NewStr;
                    V.Dirty = false;
                    Line.Data->SetValue(iif->RegenFormattedString());
                }
                else if (!LimitFix)
                    for (auto&& str : spc)
                    {
                        auto& IniType = Line.Default->GetIniType();
                        auto toidx = IBF_Inst_Project.Project.GetSecID(str, IniType);

                        if (toidx.Empty())continue;//目标不存在，跳过

                        //sprintf_s(LogBufB, "New Link <%s->%u:%u> to %s, ", Root->Name.c_str(), LineIdx, cidx, toidx.operator IBB_Section_Desc().GetText().c_str());
                        //GlobalLogB.AddLog(LogBufB, false);
                        ClaimLink(LineIdx, cidx, NewLT.size());

                        ImU32 Col = piic->UseCustomSetting ? piic->NodeSetting.LinkCol :
                            (Line.Default ? Line.Default->LinkNode.LinkCol : (ImU32)IBB_DefaultRegType::GetDefaultNodeColor());
                        auto RootID = Root->GetThisID();
                        auto seid = IBR_NodeSession::GetSessionIdx(
                            RootID,
                            Default->Name,
                            LineIdx,
                            cidx
                        );
                        NewLT.emplace_back(
                            RootID,
                            toidx,
                            KeyName,
                            Col,
                            seid
                        );
                    }



                //GlobalLogB.AddLog("");
            }
        }
        if (auto pd = Line.GetData<IBB_IniLine_Data_String>(); pd)
        {
            auto& spc = SplitParamCached(pd->Value);
            for (auto&& str : spc)
            {
                auto& IniType = Line.Default->GetIniType();
                auto toidx = IBF_Inst_Project.Project.GetSecID(str, IniType);
                if (toidx.Empty())continue;//目标不存在，跳过
                if (!toidx.GetSec(IBF_Inst_Project.Project)) continue;//目标不存在，跳过
                ClaimLink(LineIdx, 0, NewLT.size());
                ImU32 Col = (Line.Default ? Line.Default->LinkNode.LinkCol : (ImU32)IBB_DefaultRegType::GetDefaultNodeColor());
                auto RootID = Root->GetThisID();
                auto seid = IBR_NodeSession::GetSessionIdx(
                    RootID,
                    Default->Name,
                    LineIdx,
                    0
                );
                NewLT.emplace_back(
                    RootID,
                    toidx,
                    KeyName,
                    Col,
                    seid
                );
            }
        }
    }

    if (Default->Type == IBB_SubSec_Default::Inherit)
    {
        auto it = Lines.find(InheritKeyID());
        if (it != Lines.end())Root->Inherit = it->second.Data->GetString(); 
    }

    UpdateNewLinkTo(std::move(NewLT));

    if (LimitFix)Ret &= UpdateAll();

    IBR_Inst_Project.TriggerRefreshLink();

    return Ret;
}



