#include "IBR_LinkNode.h"
#include "IBB_RegType.h"
#include "Global.h"
#include "IBB_Ini.h"
#include <ranges>
#include "IBR_Components.h"
#include "IBR_Combo.h"
#include "FromEngine/global_tool_func.h"
#include <imgui_internal.h>
#include "IBR_Misc.h"

namespace LinkNodeContext
{
    IBB_SubSec* CurSub;
    size_t LineIndex;
    size_t LineMult;
    size_t CompIndex;
    ImVec2 CollapsedCenter;
    bool CurLineChangeCompStatus;
    LineDragData CurDragData;
    ImVec2 CurDragStart;
    ImVec2 CurDragStartEqCenter;
    ImU32 CurDragCol;
    bool HasDragNow;
    std::vector<ImVec2> AcceptEdge;
}

namespace ExportContext
{
    StrPoolID Key;
    std::set<IBB_Section_Desc> MergedDescs;//被Import而合并的Section列表
    bool OnExport;
    const IBB_IniLine* ExportingLine;
    const IBB_Section* ExportingSection;
}

bool LinkNodeSetting::Load(JsonObject Obj, bool* HasCustom)
{
    if (!Obj)return false;
    auto LNS = IBB_DefaultRegType::GetDefaultLinkNodeSetting();
    auto TypeStr = Obj.ItemStringOr("Type", "");
    LinkType = TypeStr.empty() ? LNS.LinkType : NewPoolStr(TypeStr);
    LinkLimit = Obj.ItemIntOr("Limit", LNS.LinkLimit);
    auto oDNC = Obj.GetObjectItem(u8"Color");
    if (oDNC.Available())
    {
        auto V = oDNC.GetArrayInt();
        if (V.size() == 3)LinkCol = ImColor(V[0], V[1], V[2]);
        else if (V.size() >= 4)LinkCol = ImColor(V[0], V[1], V[2], V[3]);
        else LinkCol = IBB_DefaultRegType::GetDefaultNodeColor();
    }
    else LinkCol = IBB_DefaultRegType::GetDefaultNodeColor();

    if (HasCustom)
    {
        *HasCustom = Obj.HasItem("Type") || Obj.HasItem("Limit") || Obj.HasItem("Color");
    }

    return true;
}

std::vector<std::string> SplitParam(const std::string& Text);
void DrawDragPreviewIcon();
void DrawDragPreviewIcon_LinkLim0();

namespace ImGui
{
    ImVec2 GetLineEndPos();
    ImVec2 GetLineBeginPos();
    bool IsWindowClicked(ImGuiMouseButton Button);
    const char* FindRenderedTextEnd(const char* text, const char* text_end);
    void Dummy(const ImVec2& size, bool AffectsLayout);
}

namespace IBR_NodeSession
{
    bool operator<(const SessionKey& a, const SessionKey& b)
    {
        if (a.Ini != b.Ini)return a.Ini < b.Ini;
        if (a.Sec != b.Sec)return a.Sec < b.Sec;
        if (a.Sub != b.Sub)return a.Sub < b.Sub;
        if (a.Line != b.Line)return a.Line < b.Line;
        return a.Comp < b.Comp;
    }

    bool operator<(const SourceNodeKey& a, const SourceNodeKey& b)
    {
        if (a.Ini != b.Ini)return a.Ini < b.Ini;
        if (a.Sec != b.Sec)return a.Sec < b.Sec;
        if (a.Line != b.Line)return a.Line < b.Line;
        return a.Comp < b.Comp;
    }

    uint64_t GetSessionIdx(const std::string& Ini, const std::string& Sec, const std::string& Sub, size_t Line, size_t Mult, size_t Comp)
    {
        //Very hot path
        //as fast as possible
        IBB_SectionID SecID(Ini, Sec);
        return GetSessionIdx(SecID, Sub, Line, Mult, Comp);
    }
    uint64_t GetSessionIdx(IBB_SectionID SecID, const std::string& Sub, size_t Line, size_t Mult, size_t Comp)
    {
        size_t i = 0, j = 0, h;
        h = std::hash<std::string>{}(Sub);
        i ^= h + 0x9e3779b9 + (i << 6) + (i >> 2);
        j ^= h + 0x9ddfea08 + (j << 6) + (j >> 2);
        h = std::hash<size_t>{}(Line);
        i ^= h + 0x9e3779b9 + (i << 6) + (i >> 2);
        j ^= h + 0x9ddfea08 + (j << 6) + (j >> 2);
        h = std::hash<size_t>{}(Mult);
        i ^= h + 0x9e3779b9 + (i << 6) + (i >> 2);
        j ^= h + 0x9ddfea08 + (j << 6) + (j >> 2);
        h = std::hash<size_t>{}(Comp);
        i ^= h + 0x9e3779b9 + (i << 6) + (i >> 2);
        j ^= h + 0x9ddfea08 + (j << 6) + (j >> 2);
        auto Com = (uint64_t(i) << 32) | uint64_t(j);
        Com ^= SecID.ID + 0x9e3779b97f4a7c15 + (Com << 12) + (Com >> 4);
        return Com;
    }

    std::unordered_map<uint64_t, SessionValue> SessionData;

    SessionValue& NewSessionValue(const std::string& Ini, const std::string& Sec, const std::string& Sub, size_t Line, size_t Mult, size_t Comp)
    {
        auto key = GetSessionIdx(Ini, Sec, Sub, Line, Mult, Comp);
        auto& val = SessionData[key];
        val.Renew();
        return val;
    }

    SessionValue& GetSessionValue(const std::string& Ini, const std::string& Sec, const std::string& Sub, size_t Line, size_t Mult, size_t Comp)
    {
        auto key = GetSessionIdx(Ini, Sec, Sub, Line, Mult, Comp);
        return SessionData[key];
    }

    SessionValue& GetSessionValue(IBB_SectionID SecID, const std::string& Sub, size_t Line, size_t Mult, size_t Comp)
    {
        auto key = GetSessionIdx(SecID, Sub, Line, Mult, Comp);
        return SessionData[key];
    }

    SessionValue& GetSessionValue(uint64_t SessionID)
    {
        return SessionData[SessionID];
    }

    ImVec2 GetSessionBeginR(uint64_t SessionID)
    {
        return SessionData[SessionID].LastCenter;
    }
    void SetSessionBeginR(uint64_t SessionID, ImVec2 Center)
    {
        SessionData[SessionID].LastCenter = Center;
    }
    bool GetSessionCollapsed(uint64_t SessionID)
    {
        return SessionData[SessionID].Collapsed;
    }

    void SetSessionStatus(uint64_t SessionID, ImVec2 Center, bool Collapsed)
    {
        auto& Sess = SessionData[SessionID];
        Sess.LastCenter = Center;
        Sess.Collapsed = Collapsed;
    }

    size_t SourceNodeKey::ID() const
    {
        auto S = Ini + "\n" + Sec + "\n" + Line + "\n" + std::to_string(Comp);
        return std::hash<std::string>{}(S);
    }

    void ClearSession()
    {
        SessionData.clear();
    }
}

namespace IBR_LinkNode
{
    void PushIdx(
        size_t LineIdx,
        size_t LineMult,    
        size_t CompIdx,
        std::unordered_set<LinkSrcIdx>* Pushed
    )
    {
        if (Pushed)
        {
            Pushed->insert(LinkSrcIdx{LineIdx, LineMult, CompIdx});
        }
    }

    ImVec2 DefaultCenter()
    {
        return ImGui::GetLineEndPos() - ImVec2{ FontHeight * 1.5f, ImGui::GetTextLineHeightWithSpacing() * 0.5F };
    }

    bool UpdateLinkInitial()
    {
        if (LinkNodeContext::CurSub)
        {
            auto& ln = LinkNodeContext::CurSub->Lines_ByName[LinkNodeContext::LineIndex];
            auto& Line = LinkNodeContext::CurSub->Lines[ln];
            if (Line.Default && IBB_DefaultRegType::HasRegType(Line.Default->LinkNode.LinkType))
                return true;
        }
        return false;
    }

    ImVec2 DefaultCenterInWindow()
    {
        auto dd = DefaultCenter();
        auto wp = ImGui::GetWindowPos();
        return { dd.x - wp.x, dd.y - wp.y };
    }

    ImVec2 ImportCenter()
    {
        auto wp = ImGui::GetWindowPos();
        return
        {
            wp.x + ImGui::GetWindowWidth() * 0.5f,
            wp.y + ImGui::GetCursorPos().y - ImGui::GetTextLineHeightWithSpacing() * 0.5F
        };
    }

    ImVec2 ImportCenterInWindow()
    {
        return
        {
            ImGui::GetWindowWidth() * 0.5f,
            ImGui::GetCursorPos().y - ImGui::GetTextLineHeightWithSpacing() * 0.5F
        };
    }

    void UpdateLink(
        IBB_SubSec& FromSub,
        size_t LineIdx,
        size_t LineMult,
        size_t CompIdx,
        std::unordered_set<LinkSrcIdx>* Pushed
    )
    {
        bool IsImport = (FromSub.Default->Type == IBB_SubSec_Default::Import);
        auto Center = IsImport ? ImportCenter() : DefaultCenter();
        auto SecID = FromSub.Root->GetThisID();
        auto SessID = IBR_NodeSession::GetSessionIdx(
            SecID, FromSub.Default->Name, LineIdx, LineMult, CompIdx
        );
        IBR_NodeSession::SetSessionStatus(SessID, Center, false);
        PushIdx(LineIdx, LineMult, CompIdx, Pushed);
        if (!IBR_Inst_Project.RefreshLinkList) return;

        auto&& [LinkBegin, LinkEnd] = FromSub.GetLink(LineIdx, LineMult, CompIdx);
        if (LinkBegin == LinkEnd)return;
        auto Links =
            std::ranges::subrange(LinkBegin, LinkEnd) |
            std::views::transform([&](auto p) -> IBB_NewLink& {
                return FromSub.NewLinkTo[p.second];
            });

        

        //sprintf_s(LogBufB, "<%p->%u:%u>%s PushLink : ", &FromSub, LineIdx, CompIdx, FromSub.Lines_ByName[LineIdx].c_str());
        //GlobalLogB.AddLog(LogBufB, false);
        for (auto& L : Links)
        {
            //OutputDebugStringA(std::format("{} ; {}.{} -> {}",L.GetText(), L.From.ID, L.FromKey, L.To.ID).c_str());
            PushLinkForDraw(
                Center,
                L.ToLoc.Sec,
                L.ToLoc.Key,
                L.ToLoc.Mult,
                L.SessionID,
                L.DefaultColor,
                L.FromLoc.Key == ImportKeyID(),
                L.ToLoc.Sec == L.FromLoc.Sec,
                false
            );
        }
        //GlobalLogB.AddLog("");
    }

    IBB_UpdateResult RenderUI_Node(
        IICStatus& Status,
        const std::string& Hint,
        const std::string& DescLong,
        const IBB_UpdateResult& DefaultResult,
        const LinkNodeSetting& LinkNode,
        const std::function<IBB_UpdateResult(const std::string& NewValue, bool Active)>& ModifyFunc
    )
    {
        auto pss = LinkNodeContext::CurSub;
        auto psd = IBR_Inst_Project.GetSection(pss->Root->GetThisID()).GetSectionData();
        auto cidx = LinkNodeContext::CompIndex;
        auto lidx = LinkNodeContext::LineIndex;
        auto lmul = LinkNodeContext::LineMult;
        if (
            psd &&
            pss &&
            cidx != -1
        )
        {
            return IBR_LinkNode::RenderUI_Node(
                Status,
                *psd,
                *pss,
                lidx,
                lmul,
                cidx,
                Hint,
                DescLong,
                DefaultResult,
                LinkNode,
                ModifyFunc
            );
        }
        return DefaultResult;
    }

    IBB_UpdateResult RenderUI_Node(
        IICStatus& Status,
        IBR_SectionData& Data,
        IBB_SubSec& FromSub,
        size_t LineIdx,
        size_t LineMult,
        size_t CompIdx,
        const std::string& Hint,
        const std::string& DescLong,
        const IBB_UpdateResult& DefaultResult,
        const LinkNodeSetting& LinkNode,
        const std::function<IBB_UpdateResult(const std::string& NewValue, bool Active)>& ModifyFunc
    )
    {
        {
            auto End = ImGui::FindRenderedTextEnd(Hint.c_str(), nullptr);
            auto Len = End - Hint.c_str();
            if (Len)
            {
                ImGui::TextUnformatted(Hint.c_str(), End);
                if (ImGui::IsItemHovered())IBR_ToolTip(DescLong);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))Status.InputMethod = IICStatus::Input;
                ImGui::SameLine();
            }
        }

        {
            auto XLeft = ImGui::GetCursorPos().x;
            auto XRight = ImGui::GetWindowContentRegionWidth();
            auto YUp = ImGui::GetCursorPos().y;
            auto YDown = YUp + ImGui::GetTextLineHeight();
            ImGui::Dummy(ImVec2(XRight - XLeft, YDown - YUp), true);
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))Status.InputMethod = IICStatus::Input;
            ImGui::SetCursorPos({ XLeft , YUp });
        }

        bool IsInherit = (FromSub.Default->Type == IBB_SubSec_Default::Inherit);
        bool IsImport = (FromSub.Default->Type == IBB_SubSec_Default::Import);
        auto Style = IsInherit ? ImGuiRadioButtonFlags_RoundedSquare : GlobalNodeStyle;
        bool Clicked;
        auto UR = DefaultResult;
        auto&& [LinkBegin, LinkEnd] = FromSub.GetLink(LineIdx, LineMult, CompIdx);
        auto Links =
            std::ranges::subrange(LinkBegin, LinkEnd) |
            std::views::transform([&](auto p) { return FromSub.NewLinkTo[p.second]; });
        bool Empty = (LinkBegin == LinkEnd);
        auto Col = AdjustNodeCol(LinkNode.LinkCol, Empty, IsInherit);
        auto KeyName = FromSub.Lines_ByName[LineIdx];
        auto& Session = IBR_NodeSession::GetSessionValue(
            FromSub.Root->GetThisID(), FromSub.Default->Name, LineIdx, LineMult, CompIdx
        );
        auto DC = IsImport ? ImportCenter() : DefaultCenter();
        auto ModifyAndShow = [&](const std::string& NewValue, bool Active) -> auto
            {
                FromSub.Root->SetOnShow(KeyName);
                return ModifyFunc(NewValue, Active);
            };

        ImGui::PushStyleColor(ImGuiCol_CheckMark, Col.Value);

        {
            auto Center = IsImport ? ImportCenterInWindow() : DefaultCenterInWindow();
            auto wbase = FromSub.Root->GetWidthBase();
            if (Session.LastCenterRatio < wbase - 2.5f)Session.LastCenterRatio = wbase - 2.5f;
            auto CenterRatio = Center.x / FontHeight;
            if (CenterRatio > Session.LastCenterRatio + 1.0f)CenterRatio = Session.LastCenterRatio + 1.0f;
            auto Oldx = Center.x;
            Center.x = CenterRatio * FontHeight;
            DC.x = DC.x + (Center.x - Oldx);
            ImGui::SetCursorPosX(Center.x - FontHeight * 0.5f);
            Session.LastCenterRatio = CenterRatio;
        }

        ImGui::PushID(LineIdx << 16 | CompIdx);

        {
            auto w = ImGui::GetCurrentWindow();
            auto mx = w->DC.CursorMaxPos;
            auto mxi = w->DC.IdealMaxPos;
            Clicked = ImGui::RadioButton("", true, Style);
            w->DC.CursorMaxPos = mx;
            w->DC.IdealMaxPos = mxi;
            void AdjustCursor();
            AdjustCursor();
        }

        bool RightClicked = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
        bool Hovered = ImGui::IsItemHovered();
        bool ShowReg = IBR_WorkSpace::ShowRegName;
        UR.Active = ImGui::IsItemActive();

        if (Clicked && !Empty)
        {
            if (LinkNode.LinkLimit == 1)
                UR = ModifyAndShow("", UR.Active);
        }
        else if (RightClicked && !Empty)
        {
            //TriggeredRightMenu = true;
            //原来这里是有一行这个的，但是这个变量本就没有引用，所以不知道是干什么的，先注释掉了
            auto PopupName = Data.DisplayName + "__LINK__" + PoolStr(KeyName);
            if (LinkNode.LinkLimit == 1)
            {
                Session.Renew();
                IBR_PopupManager::SetRightClickMenu(std::move(
                    IBR_PopupManager::Popup{}.Create(PopupName).PushMsgBack([&Session]() {
                        if (ImGui::SmallButtonAlignLeft(locc("GUI_Unlink"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                        {
                            Session.NewValue = "";
                            Session.NotifyNewValue = true;
                            return IBR_PopupManager::ClearRightClickMenu();
                        }
                        })), ImGui::GetMousePos());
            }
            else
            {
                Session.Renew();

                Session.LinkList = Links |
                        std::views::transform([&](auto p) {
                        auto Sec = IBR_Inst_Project.GetSection(p.ToLoc.Sec);
                        auto T = p.TargetValue();
                        return IBR_NodeSession::SessionLinkList{ Sec.HasBack() ? Sec.GetDisplayName() : T, T, true };
                    }) |
                    std::views::filter([&](auto&& s) { return !s.Section.empty(); }) |
                    std::ranges::to<std::vector>();

                    IBR_PopupManager::SetRightClickMenu(std::move(
                        IBR_PopupManager::Popup{}.Create(PopupName).PushMsgBack([&Session]() {
                        for (auto& ll : Session.LinkList)
                        {
                            bool PushCol = !ll.UseLink;
                            if (PushCol)ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_Color::IllegalLineColor.Value);
                            ImGui::PushID(ll.Section.c_str());
                            if (ImGui::RadioButton("", ll.UseLink, GlobalNodeStyle))
                            {
                                ll.UseLink = !ll.UseLink;
                                Session.NewValue = Session.LinkList |
                                    std::views::filter([&](auto&& s) { return s.UseLink; }) |
                                    std::views::transform([&](auto&& s) { return s.Section; }) |
                                    std::views::join_with(',') |
                                    std::ranges::to<std::string>();
                                Session.NotifyNewValue = true;
                            }
                            ImGui::PopID();
                            if (PushCol)ImGui::PopStyleColor();
                            ImGui::SameLine();
                            ImGui::Text((IBR_WorkSpace::ShowRegName ? ll.Section : ll.Display).c_str());
                        }
                        if (ImGui::SmallButtonAlignLeft(locc("GUI_UnlinkAll"), ImVec2{ FontHeight * 8.0f, ImGui::GetTextLineHeight() }))
                        {
                            Session.NewValue = "";
                            Session.NotifyNewValue = true;
                            return IBR_PopupManager::ClearRightClickMenu();
                        }
                    })), ImGui::GetMousePos());
            }
        }
        else if (Hovered && !Empty)
        {
            std::string Str;
            if (ShowReg)
                Str = Links |
                std::views::transform([&](auto p) {return p.TargetValue(); }) |
                std::views::filter([&](auto&& s) { return !s.empty(); }) |
                std::views::join_with(',') |
                std::ranges::to<std::string>();
            else
                Str = Links |
                std::views::transform([&](auto p) {
                auto Sec = IBR_Inst_Project.GetSection(p.ToLoc.Sec);
                return Sec.HasBack() ? Sec.GetDisplayName() : p.ToLoc.Section();
                    }) |
                std::views::filter([&](auto&& s) { return !s.empty(); }) |
                        std::views::join_with(',') |
                        std::ranges::to<std::string>();
                    if (!Str.empty())
                    {
                        auto W = UTF8toUnicode(Str);
                        IBR_ToolTip(std::vformat(locw("GUI_Preview_LinkTo"), std::make_wformat_args(W)));
                    }
        }

        if (ImGui::BeginDragDropSource())
        {
            if (IsInherit)
            {
                auto ws = UTF8toUnicode((Data.Desc.Ini + " -> " + Data.DisplayName));
                ImGui::Text(UnicodetoUTF8(std::vformat(locw("GUI_InheritTo"), std::make_wformat_args(ws))).c_str());
            }
            else ImGui::Text((Data.Desc.Ini + " -> " + Data.DisplayName + " : " + PoolStr(KeyName)).c_str());

            if (LinkNode.LinkLimit == 0)DrawDragPreviewIcon_LinkLim0();
            else DrawDragPreviewIcon();

            LinkNodeContext::CurDragStart = { DC.x , DC.y + ImGui::GetTextLineHeight() };
            LinkNodeContext::CurDragCol = LinkNode.LinkCol;
            LinkNodeContext::CurDragStartEqCenter = IBR_WorkSpace::RePosToEqPos(IBR_RealCenter::Center);
            LinkNodeContext::HasDragNow = true;

            auto& cdd = LinkNodeContext::CurDragData;
            cdd = { Data.Desc, LinkNode.LinkType, &Session };
            auto cpdd = &cdd;
            ImGui::SetDragDropPayload("IBR_LineDrag", &cpdd, sizeof(cpdd));

            ImGui::EndDragDropSource();
        }

        if (Session.NotifyNewValue)
        {
            UR = ModifyAndShow(Session.NewValue, UR.Active);
            Session.NotifyNewValue = false;
        }
        if (Session.NotifyValueToMerge)
        {
            if (LinkNode.LinkLimit != 0)
            {
                auto s = Links |
                    std::views::transform([&](auto p) {return p.TargetValue(); }) |
                    std::views::filter([&](auto&& s) { return !s.empty(); }) |
                    std::ranges::to<std::vector>();
                bool Used = false;
                for (auto& ss : s)if (ss == Session.ValueToMerge)
                {
                    Used = true;
                    break;
                }
                if (!Used)s.push_back(Session.ValueToMerge);
                std::string str;
                if ((int)s.size() <= LinkNode.LinkLimit)
                {
                    str = s |
                        std::views::join_with(',') |
                        std::ranges::to<std::string>();
                }
                else if (LinkNode.LinkLimit == 1)
                {
                    str = s.back();
                }
                else
                {
                    str = s |
                        std::views::take(LinkNode.LinkLimit) |
                        std::views::join_with(',') |
                        std::ranges::to<std::string>();
                }
                UR = ModifyAndShow(str, UR.Active);
            }
            Session.NotifyValueToMerge = false;
        }

        ImGui::PopID();
        ImGui::PopStyleColor();

        return UR;
    }

    void PushInactiveLines(
        IBB_SubSec& FromSub,
        ImVec2 Center
    )
    {
        auto& Bsec = *FromSub.Root;
        for (auto& link : FromSub.NewLinkTo)
        {
            if (!Bsec.IsOnShow(link.FromLoc.Key))
            {
                if (IBR_Inst_Project.RefreshLinkList)
                    PushLinkForDraw(
                        Center,
                        link.ToLoc.Sec,
                        link.ToLoc.Key,
                        link.ToLoc.Mult,
                        link.SessionID,
                        link.DefaultColor,
                        (link.FromLoc.Key == ImportKeyID()),
                        link.ToLoc.Sec == link.FromLoc.Sec,
                        true
                    );
                else
                    IBR_NodeSession::SetSessionStatus(
                        link.SessionID,
                        Center,
                        true
                    );
            }
        }
    }

    void PushRestLinkForDraw(
        IBB_SubSec& FromSub,
        const std::unordered_set<LinkSrcIdx>& Pushed,
        size_t LineIdx,
        ImVec2 Center,
        bool SrcDragging
    )
    {
        for (auto&& [idx, lidx] : FromSub.LinkSrc)
        {
            size_t Line = GetLineIdx(idx);
            if (Line != LineIdx)continue;
            if (Pushed.contains(idx))continue;
            auto& link = FromSub.NewLinkTo[lidx];
            if (IBR_Inst_Project.RefreshLinkList)
                PushLinkForDraw(
                    Center,
                    link.ToLoc.Sec,
                    link.ToLoc.Key,
                    link.ToLoc.Mult,
                    link.SessionID,
                    link.DefaultColor,
                    link.ToLoc.Sec == link.FromLoc.Sec,
                    SrcDragging,
                    false
                );
            else
                IBR_NodeSession::SetSessionStatus(
                    link.SessionID,
                    Center,
                    false
                );
        }
    }
}


namespace SPCached
{
    std::unordered_map<std::string, std::vector<std::string>> Cache;
    std::map<uint64_t, std::string> CacheOrder;
    uint64_t MaxCacheID{ 0 };//assume it never reaches ULL_MAX

    size_t CacheSize{ 0 };

    std::vector<std::string>& CacheResult(const std::string& Str, std::vector<std::string>&& Value)
    {
        if (CacheSize < 2)CacheSize = 2;//minimum at 2
        if (CacheOrder.size() > CacheSize - 1)
        {
            auto EraseCount = CacheOrder.size() - CacheSize + 1;
            if (EraseCount == 1)
                CacheOrder.erase(CacheOrder.begin());
            else
            {
                auto From = CacheOrder.begin();
                auto To = std::next(From, EraseCount);
                CacheOrder.erase(From, To);
            }
        }
        Cache[Str] = std::move(Value);
        CacheOrder[MaxCacheID++] = Str;
        return Cache.at(Str);
    }
}

size_t& SPCacheSize()
{
    return SPCached::CacheSize;
}

const std::vector<std::string>& SplitParamCached(const std::string& Text)
{
    auto it = SPCached::Cache.find(Text);
    if (it == SPCached::Cache.end())
    {
        return SPCached::CacheResult(Text, SplitParam(Text));
    }
    else
    {
        return it->second;
    }
}
