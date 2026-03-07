#include "IBR_LinkNode.h"
#include "IBB_RegType.h"
#include "Global.h"
#include "IBB_Ini.h"
#include <ranges>
#include "IBR_Components.h"
#include "IBR_Combo.h"
#include "FromEngine/global_tool_func.h"
#include <imgui_internal.h>

namespace LinkNodeContext
{
    IBB_SubSec* CurSub;
    size_t LineIndex;
    size_t CompIndex;
    ImVec2 CollapsedCenter;
    bool CurLineChangeCompStatus;
    LineDragData CurDragData;
    ImVec2 CurDragStart;
    ImVec2 CurDragStartEqCenter;
    ImU32 CurDragCol;
    bool HasDragNow;
}

namespace ExportContext
{
    std::string Key;
    size_t SameKeyIdx;//用于当Key重复时区分不同的Key
    std::set<IBB_Section_Desc> MergedDescs;//被Import而合并的Section列表
    bool OnExport;
}

bool LinkNodeSetting::Load(JsonObject Obj, bool* HasCustom)
{
    if (!Obj)return false;
    auto LNS = IBB_DefaultRegType::GetDefaultLinkNodeSetting();
    LinkType = Obj.ItemStringOr("Type", LNS.LinkType);
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

    std::map<SessionKey, SessionValue> SessionData;

    SessionValue& NewSessionValue(const std::string& Ini, const std::string& Sec, const std::string& Sub, size_t Line, size_t Comp)
    {
        SessionKey key{ Ini, Sec, Sub, Line, Comp };
        auto& val = SessionData[key];
        val.Renew();
        return val;
    }

    SessionValue& GetSessionValue(const std::string& Ini, const std::string& Sec, const std::string& Sub, size_t Line, size_t Comp)
    {
        SessionKey key{ Ini, Sec, Sub, Line, Comp };
        return SessionData[key];
    }

    size_t SourceNodeKey::ID() const
    {
        auto S = Ini + "\n" + Sec + "\n" + Line + "\n" + std::to_string(Comp);
        return std::hash<std::string>{}(S);
    }
}

namespace IBR_LinkNode
{
    void PushIdx(
        size_t LineIdx,
        size_t CompIdx,
        std::unordered_set<uint64_t>* Pushed
    )
    {
        if (Pushed)
        {
            uint64_t l = LineIdx;
            uint64_t c = CompIdx;
            uint64_t i = (l << 32) | c;
            Pushed->insert(i);
        }
    }

    ImVec2 DefaultCenter()
    {
        return ImGui::GetLineEndPos() - ImVec2{ FontHeight * 1.5f, ImGui::GetTextLineHeightWithSpacing() * 0.5F };
    }

    bool UpdateLinkInitial()
    {
        if (LinkNodeContext::CurSub && LinkNodeContext::LineIndex != UINT_MAX)
        {
            auto& ln = LinkNodeContext::CurSub->Lines_ByName[LinkNodeContext::LineIndex];
            auto& Line = LinkNodeContext::CurSub->Lines[ln];
            if (Line.Default && IBB_DefaultRegType::HasRegType(Line.Default->Property.TypeAlt))
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
        size_t CompIdx,
        std::unordered_set<uint64_t>* Pushed
    )
    {
        PushIdx(LineIdx, CompIdx, Pushed);
        auto&& [LinkBegin, LinkEnd] = FromSub.GetLink(LineIdx, CompIdx);
        if (LinkBegin == LinkEnd)return;
        auto Links =
            std::ranges::subrange(LinkBegin, LinkEnd) |
            std::views::transform([&](auto p) -> IBB_NewLink& {
                return FromSub.NewLinkTo[p.second];
            });

        bool IsImport = (FromSub.Default->Type == IBB_SubSec_Default::Import);
        auto Center = IsImport ? ImportCenter() : DefaultCenter();

        //sprintf_s(LogBufB, "<%p->%u:%u>%s PushLink : ", &FromSub, LineIdx, CompIdx, FromSub.Lines_ByName[LineIdx].c_str());
        //GlobalLogB.AddLog(LogBufB, false);
        for (auto& L : Links)
        {
            //GlobalLogB.AddLog((L.To.operator IBB_Section_Desc().GetText() + " , ").c_str(), false);
            PushLinkForDraw(
                Center,
                L.To,
                L.DefaultColor,
                L.To.GetSec(IBF_Inst_Project.Project) == FromSub.Root,
                false
            );
        }
        //GlobalLogB.AddLog("");
    }

    IBB_UpdateResult RenderUI_Node(
        const std::string& Hint,
        const IBB_UpdateResult& DefaultResult,
        const LinkNodeSetting& LinkNode,
        const std::function<IBB_UpdateResult(const std::string& NewValue, bool Active)>& ModifyFunc
    )
    {
        auto psd = IBR_WorkSpace::CurOnRender;
        auto pss = LinkNodeContext::CurSub;
        auto cidx = LinkNodeContext::CompIndex;
        auto lidx = LinkNodeContext::LineIndex;
        if (
            psd &&
            pss &&
            cidx != -1
        )
        {
            return IBR_LinkNode::RenderUI_Node(
                *psd,
                *pss,
                lidx,
                cidx,
                Hint,
                DefaultResult,
                LinkNode,
                ModifyFunc
            );
        }
        return DefaultResult;
    }

    IBB_UpdateResult RenderUI_Node(
        IBR_SectionData& Data,
        IBB_SubSec& FromSub,
        size_t LineIdx,
        size_t CompIdx,
        const std::string& Hint,
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
                ImGui::SameLine();
            }
        }


        bool IsInherit = (FromSub.Default->Type == IBB_SubSec_Default::Inherit);
        bool IsImport = (FromSub.Default->Type == IBB_SubSec_Default::Import);
        auto UR = DefaultResult;
        auto&& [LinkBegin, LinkEnd] = FromSub.GetLink(LineIdx, CompIdx);
        auto Links =
            std::ranges::subrange(LinkBegin, LinkEnd) |
            std::views::transform([&](auto p) { return FromSub.NewLinkTo[p.second]; });
        bool Empty = (LinkBegin == LinkEnd);
        auto Col = AdjustNodeCol(LinkNode.LinkCol, Empty, IsInherit);
        auto& KeyName = FromSub.Lines_ByName[LineIdx];
        auto& Session = IBR_NodeSession::GetSessionValue(
            Data.Desc.Ini, Data.Desc.Sec, FromSub.Default->Name, LineIdx, CompIdx
        );
        auto ModifyAndShow = [&](const std::string& NewValue, bool Active) -> auto
            {
                FromSub.Root->SetOnShow(KeyName);
                return ModifyFunc(NewValue, Active);
            };

        ImGui::PushStyleColor(ImGuiCol_CheckMark, Col.Value);

        auto DC = IsImport ? ImportCenter() : DefaultCenter();
        auto Center = IsImport ? ImportCenterInWindow() : DefaultCenterInWindow();
        auto Style = IsInherit ? ImGuiRadioButtonFlags_RoundedSquare : GlobalNodeStyle;
        ImGui::SetCursorPosX(Center.x - FontHeight * 0.5f);

        ImGui::PushID(LineIdx << 16 | CompIdx);

        auto w = ImGui::GetCurrentWindow();
        auto mx = w->DC.CursorMaxPos;
        bool Clicked = ImGui::RadioButton("", true, Style);
        w->DC.CursorMaxPos = mx;
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
            auto PopupName = Data.DisplayName + "__LINK__" + KeyName;
            if (LinkNode.LinkLimit == 1)
            {
                Session.Renew();
                IBR_PopupManager::SetRightClickMenu(std::move(
                    IBR_PopupManager::Popup{}.Create(PopupName).PushMsgBack([&Session]() {
                        if (ImGui::SmallButtonAlignLeft(locc("GUI_Unlink"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                        {
                            Session.NewValue = "";
                            Session.NotifyNewValue = true;
                            IBR_PopupManager::ClearRightClickMenu();
                        }
                        })), ImGui::GetMousePos());
            }
            else
            {
                Session.Renew();

                Session.LinkList = Links |
                        std::views::transform([&](auto p) {
                        auto Sec = IBR_Inst_Project.GetSection(p.To);
                        auto T = p.To.Section.GetText();
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
                            IBR_PopupManager::ClearRightClickMenu();
                        }
                    })), ImGui::GetMousePos());
            }
        }
        else if (Hovered && !Empty)
        {
            std::string Str;
            if (ShowReg)
                Str = Links |
                std::views::transform([&](auto p) {return p.To.Section.GetText(); }) |
                std::views::filter([&](auto&& s) { return !s.empty(); }) |
                std::views::join_with(',') |
                std::ranges::to<std::string>();
            else
                Str = Links |
                std::views::transform([&](auto p) {
                auto Sec = IBR_Inst_Project.GetSection(p.To);
                return Sec.HasBack() ? Sec.GetDisplayName() : p.To.Section.GetText();
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
                auto w = UTF8toUnicode((Data.Desc.Ini + " -> " + Data.DisplayName));
                ImGui::Text(UnicodetoUTF8(std::vformat(locw("GUI_InheritTo"), std::make_wformat_args(w))).c_str());
            }
            else ImGui::Text((Data.Desc.Ini + " -> " + Data.DisplayName + " : " + KeyName).c_str());

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
                    std::views::transform([&](auto p) {return p.To.Section.GetText(); }) |
                    std::views::filter([&](auto&& s) { return !s.empty(); }) |
                    std::ranges::to<std::vector>();
                bool Used = false;
                for (auto& ss : s)if (ss == Session.ValueToMerge)
                {
                    Used = true;
                    break;
                }
                if (!Used)s.push_back(Session.ValueToMerge);
                auto str = s |
                    std::views::join_with(',') |
                    std::ranges::to<std::string>();
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
        auto& OnShow = Bsec.OnShow;
        for (auto& link : FromSub.NewLinkTo)
        {
            auto it = OnShow.find(link.FromKey);
            //这个时候连自己的会折成一条线
            if (link.To.GetSec(IBF_Inst_Project.Project) == FromSub.Root)continue;
            if (it == OnShow.end() || it->second.empty())
            {
                PushLinkForDraw(
                    Center,
                    link.To,
                    link.DefaultColor,
                    false
                );
            }
        }
    }

    void PushRestLinkForDraw(
        IBB_SubSec& FromSub,
        const std::unordered_set<uint64_t>& Pushed,
        size_t LineIdx,
        ImVec2 Center,
        bool SrcDragging
    )
    {
        for (auto&& [idx, lidx] : FromSub.LinkSrc)
        {
            /*
            uint64_t l = LineIdx;
            uint64_t c = CompIdx;
            uint64_t i = (l << 32) | c;
            */
            size_t Line = idx >> 32;
            if (Line != LineIdx)continue;
            if (Pushed.contains(idx))continue;
            auto& link = FromSub.NewLinkTo[lidx];
            PushLinkForDraw(
                Center,
                link.To,
                link.DefaultColor,
                link.To.GetSec(IBF_Inst_Project.Project) == FromSub.Root,
                SrcDragging
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
