#include "IBB_IniImport.h"
#include "Global.h"
#include "IBB_RegType.h"
#include "IBB_ModuleAlt.h"
#include "IBFront.h"
#include "FromEngine/RFBump.h"

#include <algorithm>
#include <fstream>
#include <queue>
#include <format>
#include <ranges>
#include <sstream>
#include <unordered_set>

extern int FontHeight;

// ============================================================
//  Step 1: Parse INI File
// ============================================================

std::vector<std::string> SplitParam(const std::string& Text);

ImportedIniFile ParseIniFile(const std::wstring& FilePath)
{
    ImportedIniFile Result;
    Result.FilePath = FilePath;

    auto Secs = SplitTokens(GetTokens(GetLines(GetStringFromFile(FilePath.c_str()))));

    for (auto& sec : Secs)
    {
        if (sec.empty() || !sec[0].IsSection) continue;
        Result.Sections.emplace_back();
        Result.Matched.push_back(true);
        auto& imp = Result.Sections.back();

        imp.SectionName = sec[0].Key;
        imp.Inherit = sec[0].Value;
        imp.KeyValues.assign(sec.begin() + 1, sec.end());
    }

    for (size_t i = 0; i < Result.Sections.size(); i++)
        Result.Sections[i].Index = i;

    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog((u8"[ImportINI] ParseIniFile: found " + std::to_string(Result.Sections.size()) + u8" sections").c_str());
        for (auto& S : Result.Sections)
            GlobalLogB.AddLog((u8"  Section='" + S.SectionName + u8"' keys=" + std::to_string(S.KeyValues.size())).c_str());
    }

    return Result;
}

// ============================================================
//  Step 2: Match Registry Type
//  逻辑：先找与注册表同名的块（注册表列表块），
//        其键值就是各实例的块名，再为这些实例块分配注册表类型。
// ============================================================

void MatchSectionToRegType(ImportedIniFile& File)
{
    if (EnableLog)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog(u8"[ImportINI] MatchSectionToRegType: start");
    }

    auto& RegTypes = IBB_DefaultRegType::RegisterTypes;

    // 第1步：找出注册表列表块（section name == RegisterTypes key）
    //        并建立 注册表名 → 实例名列表 的映射
    std::unordered_map<std::string, std::vector<std::string>> RegToInstanceNames;
    std::unordered_set<size_t> RegistryListIndices; // 注册表列表块的索引，稍后跳过

    for (size_t i = 0; i < File.Sections.size(); i++)
    {
        auto& Sec = File.Sections[i];
        auto It = RegTypes.find(Sec.SectionName);

        // 也尝试匹配 Reg.Name 字段
        if (It == RegTypes.end())
        {
            for (auto& [RN, R] : RegTypes)
            {
                if (R.Name == Sec.SectionName || R.ExportName == Sec.SectionName)
                {
                    It = RegTypes.find(RN);
                    break;
                }
            }
        }

        if (It != RegTypes.end())
        {
            // 这是个注册表列表块 → 收集实例名
            RegistryListIndices.insert(i);
            std::vector<std::string> Instances;
            for (auto& KV : Sec.KeyValues)
            {
                if (!KV.Value.empty())
                    Instances.push_back(KV.Value);
            }
            RegToInstanceNames[It->first] = std::move(Instances);

            if (EnableLog)
                GlobalLogB.AddLog((u8"[ImportINI] Registry list: '" + Sec.SectionName + u8"' -> " + It->first + u8" (" + std::to_string(Instances.size()) + u8" instances)").c_str());

            // 标记注册表列表块，后续跳过导入
            Sec.IsRegistryList = true;
            Sec.MatchStatus = IniImportMatchStatus::Matched;
            Sec.MatchedRegType = It->first;
        }
    }

    // 第2步：为实例块分配注册表类型
    for (size_t i = 0; i < File.Sections.size(); i++)
    {
        if (RegistryListIndices.count(i))
            continue; // 跳过注册表列表块

        auto& Sec = File.Sections[i];
        if (Sec.MatchStatus == IniImportMatchStatus::Matched)
            continue; // 已经被匹配

        bool Found = false;
        for (auto& [RegName, Instances] : RegToInstanceNames)
        {
            for (auto& InstName : Instances)
            {
                if (Sec.SectionName == InstName)
                {
                    Sec.MatchStatus = IniImportMatchStatus::Matched;
                    Sec.MatchedRegType = RegName;
                    Found = true;
                    if (EnableLog)
                        GlobalLogB.AddLog((u8"[ImportINI] Instance match: [" + Sec.SectionName + u8"] -> RegType=[" + RegName + u8"]").c_str());
                    break;
                }
            }
            if (Found) break;
        }

        if (!Found)
        {
            // 第3步（兜底）：直接匹配 section name 到 RegisterTypes key
            auto It = RegTypes.find(Sec.SectionName);
            if (It != RegTypes.end())
            {
                Sec.MatchStatus = IniImportMatchStatus::Matched;
                Sec.MatchedRegType = It->first;
                if (EnableLog)
                    GlobalLogB.AddLog((u8"[ImportINI] Direct match: [" + Sec.SectionName + u8"] -> RegType=[" + It->first + u8"]").c_str());
                Found = true;
            }
        }

        if (!Found)
        {
            // 再尝试匹配 Reg.Name 字段
            for (auto& [RN, R] : RegTypes)
            {
                if (R.Name == Sec.SectionName || R.ExportName == Sec.SectionName)
                {
                    Sec.MatchStatus = IniImportMatchStatus::Matched;
                    Sec.MatchedRegType = RN;
                    Found = true;
                    if (EnableLog)
                        GlobalLogB.AddLog((u8"[ImportINI] Name match: [" + Sec.SectionName + u8"] -> RegType=[" + RN + u8"]").c_str());
                    break;
                }
            }
        }

        if (!Found)
        {
            Sec.MatchStatus = IniImportMatchStatus::Unmatched;
            Sec.MatchedRegType.clear();
            if (EnableLog)
                GlobalLogB.AddLog((u8"[ImportINI] Unmatched: [" + Sec.SectionName + u8"]").c_str());
        }
    }

    // 第4步：基于画布链接规则的自动匹配（迭代多轮，直到不再新增匹配）
    // 源块无论是否已匹配，只要键是链接键就参与匹配
    {
        int Step4Iter = 0;
        bool NewMatch = true;
        while (NewMatch)
        {
            NewMatch = false;
            if (EnableLogEx)
            {
                int uc = 0; for (auto& s : File.Sections) if (s.MatchStatus == IniImportMatchStatus::Unmatched) uc++;
                GlobalLogB.AddLog((u8"[ImportINI] Step4 iter " + std::to_string(++Step4Iter) + u8": " + std::to_string(uc) + u8" unmatched remaining").c_str());
            }

            // 重新建立 未匹配块名 → 索引 的映射
            std::unordered_map<std::string, size_t> UnmatchedNames;
            for (size_t i = 0; i < File.Sections.size(); i++)
            {
                if (File.Sections[i].MatchStatus == IniImportMatchStatus::Unmatched
                    && !File.Sections[i].IsRegistryList)
                    UnmatchedNames[File.Sections[i].SectionName] = i;
            }

            if (UnmatchedNames.empty())
                break;

            for (size_t i = 0; i < File.Sections.size(); i++)
            {
                auto& SrcSec = File.Sections[i];
                if (SrcSec.IsRegistryList)
                    continue;

                for (auto& KV : SrcSec.KeyValues)
                {
                    // 通过 TypeAlt 获取该键的画布链接规则
                    auto pLine = IBF_Inst_DefaultTypeList.List.KeyBelongToLine(KV.Key);
                    if (!pLine) continue;
                    auto& LinkNode = pLine->LinkNode;
                    if (LinkNode.LinkType == EmptyPoolStr)
                        continue;

                    std::string LinkTypeStr = PoolStr(LinkNode.LinkType);
                    if (LinkTypeStr == "_AnyType")
                        continue;

                    // 确定目标类型：_MyType 时源块必须有已注册类型
                    std::string TargetRegType;
                    if (LinkTypeStr == "_MyType")
                    {
                        if (SrcSec.MatchStatus != IniImportMatchStatus::Matched)
                            continue;
                        TargetRegType = SrcSec.MatchedRegType;
                    }
                    else
                    {
                        TargetRegType = LinkTypeStr;
                    }

                    // 检查此键的值是否匹配某个未匹配的块名（支持逗号分隔的多值）
                    auto Parts = SplitParam(KV.Value);
                    for (auto& V : Parts)
                    {
                        auto UnmatchedIt = UnmatchedNames.find(V);
                        if (UnmatchedIt == UnmatchedNames.end()) continue;

                        auto& TargetSec = File.Sections[UnmatchedIt->second];
                        if (TargetSec.MatchStatus == IniImportMatchStatus::Unmatched)
                        {
                            TargetSec.MatchStatus = IniImportMatchStatus::LinkMatched;
                            TargetSec.MatchedRegType = TargetRegType;
                            TargetSec.LinkMatchSource = SrcSec.SectionName + "." + KV.Key;
                            NewMatch = true;

                            if (EnableLog)
                                GlobalLogB.AddLog((u8"[ImportINI] Link match: [" + TargetSec.SectionName + u8"] -> RegType=[" + TargetRegType + u8"] (via " + TargetSec.LinkMatchSource + u8")").c_str());
                        }
                    }
                }
            }
        }
    }

    if (EnableLog)
    {
        int Matched = 0, LinkMatched = 0, Unmatched = 0;
        for (auto& S : File.Sections)
        {
            if (S.MatchStatus == IniImportMatchStatus::Matched) Matched++;
            else if (S.MatchStatus == IniImportMatchStatus::LinkMatched) LinkMatched++;
            else Unmatched++;
        }
        GlobalLogB.AddLog((u8"[ImportINI] MatchSectionToRegType done: " + std::to_string(Matched) + u8" matched, " + std::to_string(LinkMatched) + u8" link-matched, " + std::to_string(Unmatched) + u8" unmatched").c_str());
    }
}

void SetSectionRegType(ImportedIniSection& Section, const std::string& RegType)
{
    Section.MatchedRegType = RegType;
    Section.MatchStatus = RegType.empty() ? IniImportMatchStatus::Unmatched : IniImportMatchStatus::Matched;
}

// ============================================================
//  Step 3: Detect Link Relations (via DefaultLinks)
// ============================================================

std::vector<IniImportLinkRelation> DetectLinkRelations(const ImportedIniFile& File)
{
    std::vector<IniImportLinkRelation> Result;

    if (EnableLogEx)
        GlobalLogB.AddLog((u8"[ImportINI] DetectLinkRelations: " + std::to_string(File.Sections.size()) + u8" sections").c_str());

    // 建立：section 索引 -> MatchedRegType 的快速映射（含 LinkMatched）
    std::unordered_map<size_t, std::string> SecRegTypes;
    for (auto& Sec : File.Sections)
    {
        if (Sec.MatchStatus == IniImportMatchStatus::Matched
            || Sec.MatchStatus == IniImportMatchStatus::LinkMatched)
            SecRegTypes[Sec.Index] = Sec.MatchedRegType;
    }

    // 第1轮：画布链接规则值匹配——如果某键在画布中有链接规则且值等于另一个 section 的名字，建立链接
    // 同时用 LinkMatchSource 建立 LinkMatched 块的链接
    // 建立 section 名到索引的映射
    std::unordered_map<std::string, size_t> NameToIdx;
    for (size_t j = 0; j < File.Sections.size(); j++)
    {
        auto& Dst = File.Sections[j];
        if (Dst.MatchStatus != IniImportMatchStatus::Matched
            && Dst.MatchStatus != IniImportMatchStatus::LinkMatched)
            continue;
        NameToIdx[Dst.SectionName] = j;
    }

    for (size_t i = 0; i < File.Sections.size(); i++)
    {
        auto& Src = File.Sections[i];
        if (Src.MatchStatus != IniImportMatchStatus::Matched
            && Src.MatchStatus != IniImportMatchStatus::LinkMatched)
            continue;
        if (Src.IsRegistryList)
            continue;

        for (auto& KV : Src.KeyValues)
        {
            // 通过 TypeAlt 获取该键的画布链接规则
            auto pLine = IBF_Inst_DefaultTypeList.List.KeyBelongToLine(KV.Key);
            if (!pLine) continue;
            if (pLine->LinkNode.LinkType == EmptyPoolStr)
                continue;
            auto LinkTypeStr = PoolStr(pLine->LinkNode.LinkType);
            if (LinkTypeStr == "_AnyType")
                continue;

            // 检查此键的值是否匹配另一个 section 的名字（支持逗号分隔的多值）
            auto Parts = SplitParam(KV.Value);
            for (auto& V : Parts)
            {
                auto NameIt = NameToIdx.find(V);
                if (NameIt == NameToIdx.end())
                {
                    // 没有找到目标也可能是 LinkMatchSource 方式（LinkMatched 块）
                    // 跳过，因为 LinkMatched 块的名已在 Unmatched 里
                    continue;
                }
                // 避免重复添加
                bool Dup = false;
                for (auto& L : Result)
                    if (L.FromSectionIdx == i && L.ToSectionIdx == NameIt->second && L.FromKey == KV.Key)
                        { Dup = true; break; }
                if (!Dup)
                    Result.push_back({ i, NameIt->second, KV.Key, "" });
            }
        }
    }

    // 第2轮：利用 LinkMatchSource（MatchSectionToRegType 第4步记录的来源）建立链接
    for (size_t i = 0; i < File.Sections.size(); i++)
    {
        auto& Sec = File.Sections[i];
        if (Sec.MatchStatus != IniImportMatchStatus::LinkMatched)
            continue;
        if (Sec.LinkMatchSource.empty())
            continue;
        auto DotPos = Sec.LinkMatchSource.find('.');
        if (DotPos == std::string::npos)
            continue;
        std::string SrcName = Sec.LinkMatchSource.substr(0, DotPos);
        std::string KeyName = Sec.LinkMatchSource.substr(DotPos + 1);
        auto SrcIt = NameToIdx.find(SrcName);
        if (SrcIt == NameToIdx.end()) continue;
        // 避免重复
        bool Dup = false;
        for (auto& L : Result)
            if (L.FromSectionIdx == SrcIt->second && L.ToSectionIdx == i && L.FromKey == KeyName)
                { Dup = true; break; }
        if (!Dup)
            Result.push_back({ SrcIt->second, i, KeyName, "" });
    }

    if (EnableLogEx)
        GlobalLogB.AddLog((u8"[ImportINI] Round1 done: " + std::to_string(Result.size()) + u8" links, Round2 done: via LinkMatchSource").c_str());

    // 第3轮：通用值匹配——仅对已匹配的源块，键的值若等于另一个 section 的名字建立链接（兜底）
    int Round3Links = 0;
    for (size_t i = 0; i < File.Sections.size(); i++)
    {
        auto& Src = File.Sections[i];
        if (Src.IsRegistryList) continue;
        if (Src.MatchStatus != IniImportMatchStatus::Matched
            && Src.MatchStatus != IniImportMatchStatus::LinkMatched)
            continue;
        for (auto& KV : Src.KeyValues)
        {
            auto Parts = SplitParam(KV.Value);
            for (auto& V : Parts)
            {
                auto NameIt = NameToIdx.find(V);
                if (NameIt == NameToIdx.end()) continue;
                if (NameIt->second == i) continue;
                bool Dup = false;
                for (auto& L : Result)
                    if (L.FromSectionIdx == i && L.ToSectionIdx == NameIt->second)
                        { Dup = true; break; }
                if (!Dup)
                {
                    Result.push_back({ i, NameIt->second, KV.Key, "" });
                    Round3Links++;
                }
            }
        }
    }

    if (EnableLogEx)
        GlobalLogB.AddLog((u8"[ImportINI] Round3 done: " + std::to_string(Round3Links) + u8" links").c_str());

    if (EnableLogEx)
        GlobalLogB.AddLog_CurTime(false);

    if (EnableLog)
        GlobalLogB.AddLog((u8"[ImportINI] DetectLinkRelations: found " + std::to_string(Result.size()) + u8" links").c_str());

    for (auto& L : Result)
        if (EnableLog)
            GlobalLogB.AddLog((u8"  Link: Section[" + std::to_string(L.FromSectionIdx) + u8"]." + L.FromKey + u8" -> Section[" + std::to_string(L.ToSectionIdx) + u8"]." + L.ToKey).c_str());

    return Result;
}

// ============================================================
//  Step 4: Calculate Layout
// ============================================================

void CalculateLayout(ImportedIniFile& File, const std::vector<IniImportLinkRelation>& Links)
{
    const float ColGap = FontHeight * 24.0F;   // 列间距
    const float RowGap = FontHeight * 4.0F;    // 行间距（块间垂直间距）
    const float DefaultWidth = FontHeight * 18.0F;
    const float DefaultHeight = FontHeight * 6.0F;
    const float ComponentGap = DefaultHeight * 0.5F; // 连通分量间距（按块大小）
    const float KeyHeight = FontHeight * 1.6F;     // 每行键的估算高度

    size_t SectionCount = File.Sections.size();
    if (SectionCount == 0) return;

    // 1. 按 regType 分组（含 LinkMatched）
    std::unordered_map<std::string, std::vector<size_t>> TypeGroups;
    std::vector<size_t> UnmatchedIndices;
    for (size_t i = 0; i < SectionCount; i++)
    {
        auto& Sec = File.Sections[i];
        if ((Sec.MatchStatus == IniImportMatchStatus::Matched
            || Sec.MatchStatus == IniImportMatchStatus::LinkMatched)
            && !Sec.MatchedRegType.empty())
            TypeGroups[Sec.MatchedRegType].push_back(i);
        else
            UnmatchedIndices.push_back(i);
    }

    // 2. 构建邻接表（链接关系图）
    std::unordered_map<size_t, std::unordered_set<size_t>> Adj;
    for (auto& Link : Links)
    {
        Adj[Link.FromSectionIdx].insert(Link.ToSectionIdx);
        Adj[Link.ToSectionIdx].insert(Link.FromSectionIdx);
    }

    // 3. BFS 找连通分量：只要有连线关系就在同一个分量中
    std::unordered_set<size_t> Visited;
    std::vector<std::unordered_set<size_t>> Components;

    for (size_t i = 0; i < SectionCount; i++)
    {
        if (Visited.count(i))
            continue;
        if (Adj[i].empty())
            continue;

        // BFS
        std::unordered_set<size_t> Comp;
        std::vector<size_t> Queue = { i };
        Visited.insert(i);
        while (!Queue.empty())
        {
            auto Cur = Queue.back();
            Queue.pop_back();
            Comp.insert(Cur);
            for (auto Nbr : Adj[Cur])
            {
                if (!Visited.count(Nbr))
                {
                    Visited.insert(Nbr);
                    Queue.push_back(Nbr);
                }
            }
        }
        Components.push_back(std::move(Comp));
    }

    if (EnableLogEx)
    {
        GlobalLogB.AddLog((u8"[ImportINI] CalculateLayout: " + std::to_string(Components.size()) + u8" components, " + std::to_string(Links.size()) + u8" links").c_str());
        for (auto& C : Components)
            if (C.size() > 100)
                GlobalLogB.AddLog((u8"  Large component: " + std::to_string(C.size()) + u8" sections").c_str());
    }

    // 4. 分配位置
    // 连通分量上下排（纵），分量内左右排（横）
    float BaseX = 0.0F;
    float CurrentY = 0.0F;

    for (auto& Comp : Components)
    {
        if (EnableLogEx && Comp.size() > 100)
            GlobalLogB.AddLog((u8"[ImportINI]  Processing component with " + std::to_string(Comp.size()) + u8" sections...").c_str());

        // 计算分量内每节的级数（拓扑深度）：BFS 从根节点（无入链）开始
        std::unordered_map<size_t, int> Depth;
        for (auto Idx : Comp)
            Depth[Idx] = 0;
        // 预过滤 Links 只保留当前分量内的，避免每次遍历全量
        std::vector<IniImportLinkRelation> CompLinks;
        for (auto& Link : Links)
            if (Comp.count(Link.FromSectionIdx) && Comp.count(Link.ToSectionIdx))
                CompLinks.push_back(Link);

        // Kahn 拓扑排序求深度（入度归零时处理，天然防环）
        std::unordered_map<size_t, int> InDeg;
        for (auto Idx : Comp) InDeg[Idx] = 0;
        for (auto& Link : CompLinks)
            InDeg[Link.ToSectionIdx]++;
        std::queue<size_t> Q;
        for (auto Idx : Comp)
            if (InDeg[Idx] == 0) Q.push(Idx);
        while (!Q.empty())
        {
            auto Cur = Q.front(); Q.pop();
            for (auto& Link : CompLinks)
            {
                if (Link.FromSectionIdx != Cur) continue;
                int nd = Depth[Cur] + 1;
                if (nd > Depth[Link.ToSectionIdx])
                    Depth[Link.ToSectionIdx] = nd;
                if (--InDeg[Link.ToSectionIdx] == 0)
                    Q.push(Link.ToSectionIdx);
            }
        }
        // 环中节点（入度未归零）赋予最大深度
        int MaxDepthV = 0;
        for (auto& [k, v] : Depth) if (v > MaxDepthV) MaxDepthV = v;
        for (auto Idx : Comp)
            if (InDeg[Idx] > 0)
                Depth[Idx] = MaxDepthV + 1;

        if (EnableLogEx && Comp.size() > 100)
        {
            int maxd = 0; for (auto& [k, v] : Depth) if (v > maxd) maxd = v;
            GlobalLogB.AddLog((u8"[ImportINI]  Depth done, max depth = " + std::to_string(maxd)).c_str());
        }

        // 按级数分列：同级竖向排，不同级左右横排
        // 同级内按引用键的先后顺序排列
        std::unordered_map<int, std::vector<size_t>> LevelCols;
        int MaxDepth = 0;
        for (auto Idx : Comp)
        {
            int d = Depth[Idx];
            LevelCols[d].push_back(Idx);
            if (d > MaxDepth) MaxDepth = d;
        }

        // 对 level >= 1，按父块在上一级的顺序 + 键在父块中的顺序重排
        for (int d = 1; d <= MaxDepth; d++)
        {
            auto& Indices = LevelCols[d];
            std::vector<size_t> Sorted;
            std::unordered_set<size_t> Placed;
            // 按上一级中父块的顺序遍历
            for (auto ParentIdx : LevelCols[d - 1])
            {
                // 收集该父块中所有链接键 → 子块，按键在 KeyValues 中的顺序
                auto& ParentKV = File.Sections[ParentIdx].KeyValues;
                // 先收集此父块发出的所有链接，按键顺序
                std::vector<std::pair<int, size_t>> ChildrenFromParent; // (keyPos, childIdx)
                for (auto& Link : Links)
                {
                    if (!Comp.count(Link.FromSectionIdx) || !Comp.count(Link.ToSectionIdx))
                        continue;
                    if (Link.FromSectionIdx != ParentIdx) continue;
                    if (Depth[Link.ToSectionIdx] != d) continue;
                    if (Placed.count(Link.ToSectionIdx)) continue;
                    // 找这个键在父块中的位置
                    for (size_t ki = 0; ki < ParentKV.size(); ki++)
                    {
                        if (ParentKV[ki].Key == Link.FromKey)
                        {
                            ChildrenFromParent.push_back({ (int)ki, Link.ToSectionIdx });
                            break;
                        }
                    }
                }
                std::stable_sort(ChildrenFromParent.begin(), ChildrenFromParent.end(),
                    [](auto& a, auto& b) { return a.first < b.first; });
                for (auto& [kp, child] : ChildrenFromParent)
                {
                    if (Placed.count(child)) continue;
                    Sorted.push_back(child);
                    Placed.insert(child);
                }
            }
            // 未通过父块入链的块放在最后
            for (auto Idx : Indices)
                if (!Placed.count(Idx))
                    Sorted.push_back(Idx);
            Indices = std::move(Sorted);
        }

        // 分量内横排：第 0 级 BaseX，第 1 级 BaseX + ColGap，依此类推
        float CurX = BaseX;
        for (int d = 0; d <= MaxDepth; d++)
        {
            auto& Indices = LevelCols[d];
            if (Indices.empty()) { CurX += ColGap; continue; }
            float CurY = CurrentY;
            for (auto Idx : Indices)
            {
                auto& Sec = File.Sections[Idx];
                float SecHeight = DefaultHeight + KeyHeight * std::max(0, (int)Sec.KeyValues.size() - 3);
                Sec.EqPos = ImVec2{ CurX, CurY };
                Sec.EqSize = ImVec2{ DefaultWidth, SecHeight };
                CurY += SecHeight + RowGap;
            }
            CurX += ColGap;
        }

        // 计算分量的高度（所有列最大值），然后 CurrentY 下移
        float CompHeight = 0.0F;
        for (auto Idx : Comp)
        {
            auto& Sec = File.Sections[Idx];
            float SecBottom = Sec.EqPos.y + Sec.EqSize.y;
            if (SecBottom > CompHeight) CompHeight = SecBottom;
        }
        CurrentY = CompHeight + ComponentGap;

        Visited.insert(Comp.begin(), Comp.end());
    }

    // 处理无链接的 section（已匹配但无链接）：单列竖向排列
    {
        float CurY = CurrentY;
        float CurX = BaseX;
        bool HasAny = false;
        for (auto& [Type, Indices] : TypeGroups)
        {
            for (auto Idx : Indices)
            {
                if (Adj[Idx].empty() && !Visited.count(Idx))
                {
                    auto& Sec = File.Sections[Idx];
                    float SecHeight = DefaultHeight + KeyHeight * std::max(0, (int)Sec.KeyValues.size() - 3);
                    Sec.EqPos = ImVec2{ CurX, CurY };
                    Sec.EqSize = ImVec2{ DefaultWidth, SecHeight };
                    CurY += SecHeight + RowGap;
                    Visited.insert(Idx);
                    HasAny = true;
                }
            }
        }
        if (HasAny)
        {
            CurrentY = CurY + RowGap;
            BaseX += ColGap;
        }
    }

    // 未匹配的 section 放在最下方，单列竖向排列
    {
        float CurY = CurrentY;
        float CurX = BaseX;
        bool HasAny = false;
        for (auto Idx : UnmatchedIndices)
        {
            auto& Sec = File.Sections[Idx];
            float SecHeight = DefaultHeight + KeyHeight * std::max(0, (int)Sec.KeyValues.size() - 3);
            Sec.EqPos = ImVec2{ CurX, CurY };
            Sec.EqSize = ImVec2{ DefaultWidth, SecHeight };
            CurY += SecHeight + RowGap;
            HasAny = true;
        }
        if (HasAny)
            CurrentY = CurY + RowGap;
    }

    if (EnableLog)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog(u8"[ImportINI] CalculateLayout done:");
        for (auto& Sec : File.Sections)
            GlobalLogB.AddLog((u8"  Section='" + Sec.SectionName + u8"' RegType='" + Sec.MatchedRegType + u8"' EqPos=(" + std::to_string(Sec.EqPos.x) + u8"," + std::to_string(Sec.EqPos.y) + u8") EqSize=(" + std::to_string(Sec.EqSize.x) + u8"," + std::to_string(Sec.EqSize.y) + u8")").c_str());
    }
}

// ============================================================
//  Step 5: Convert to ModuleClipData
// ============================================================

std::vector<ModuleClipData> ImportedSectionsToModuleClipData(
    const ImportedIniFile& File,
    const IniImportOptions& Options)
{
    std::vector<ModuleClipData> Result;

    const std::string& TargetIni = Options.TargetIniName.empty()
        ? DefaultIniName
        : Options.TargetIniName;

    for (auto& Sec : File.Sections)
    {
        // 跳过注册表列表块（如 [ArmorTypes]，其键值只是实例名列表）
        if (Sec.IsRegistryList)
            continue;

        // 跳过用户未勾选的块
        if (!Sec.Selected)
            continue;

        ModuleClipData Clip;
        Clip.IsComment = false;
        Clip.Ignore = false;
        Clip.FromClipBoard = false;
        Clip.CollapsedInComposed = false;
        Clip.Frozen = false;
        Clip.Hidden = false;
        Clip.SingleVal = false;
        Clip.SkipExport = false;
        Clip.SkipTitle = false;
        Clip.WidthRatio = 1.0F;

        // Desc: Ini + Sec name
        Clip.Desc.A = TargetIni;
        Clip.Desc.B = Sec.SectionName;

        // Display name
        Clip.DisplayName = Sec.SectionName;

        // 继承
        Clip.Inherit = Sec.Inherit;

        // Register type（含 LinkMatched）
        if ((Sec.MatchStatus == IniImportMatchStatus::Matched
            || Sec.MatchStatus == IniImportMatchStatus::LinkMatched)
            && !Sec.MatchedRegType.empty())
        {
            Clip.Register = Sec.MatchedRegType;
        }
        else
        {
            Clip.Register = "_Unknown";
            // 确保 _Unknown 类型存在
            IBB_DefaultRegType::EnsureRegType("_Unknown");
        }

        // 位置
        Clip.EqDelta = Sec.EqPos;
        Clip.EqSize = Sec.EqSize;

        // 转换为 IniToken 列表（TypeAlt 中有定义的键默认展开，否则默认收起）
        Clip.Lines.clear();
        for (auto& KV : Sec.KeyValues)
        {
            IniToken Token;
            Token.Key = KV.Key;
            Token.Value = KV.Value;
            bool KnownKey = IBF_Inst_DefaultTypeList.List.KeyBelongToLine(KV.Key) != nullptr;
            Token.HasDesc = KnownKey;
            Token.Desc = KnownKey ? EmptyOnShowDesc : "";
            Token.IsSection = false;
            Token.Empty = false;
            Clip.Lines.push_back(std::move(Token));
        }

        Result.push_back(std::move(Clip));
    }

    if (EnableLog)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog((u8"[ImportINI] ImportedSectionsToModuleClipData: created " + std::to_string(Result.size()) + u8" ModuleClipData entries").c_str());
        for (auto& M : Result)
            GlobalLogB.AddLog((u8"  Module: Ini='" + M.Desc.A + u8"' Sec='" + M.Desc.B + u8"' Reg='" + M.Register + u8"' Lines=" + std::to_string(M.Lines.size())).c_str());
    }

    return Result;
}
