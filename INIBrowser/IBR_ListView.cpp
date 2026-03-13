#include "IBR_ListView.h"
#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "IBB_RegType.h"
#include "IBR_Combo.h"
#include "imgui_internal.h"
#include <ranges>
#include <locale>


namespace ImGui
{
    void PushOrderFront(ImGuiWindow* Window);
}

bool StrCmpZHCN(const std::string& l, const std::string& r)
{
    static std::locale zh_CN("zh-CN.UTF-8");
    return std::use_facet<std::collate<char>>(zh_CN).compare(
        l.data(), l.data() + l.size(),
        r.data(), r.data() + r.size()) < 0;
}

bool StringMatch(std::string str, std::string match, bool Full, bool CaseSensitive, bool Regex)
{
    if (!CaseSensitive)
    {
#pragma warning(push)
#pragma warning(disable:4244)//屏蔽有关tolower的警告
        std::ranges::transform(str, str.begin(), ::tolower);
        std::ranges::transform(match, match.begin(), ::tolower);
#pragma warning(pop)
    }
    if (Full)
    {
        if (Regex)
        {
            return RegexFull_Nothrow(str, match);
        }
        else
        {
            return str == match;
        }
    }
    else
    {
        if (Regex)
        {
            return RegexNotNone_Nothrow(str, match);
        }
        else
        {
            return str.find(match) != std::string::npos;
        }
    }
}

namespace IBR_ListView
{
    std::vector<IBB_SectionID> CurrentList;
    bool Search_Full{ false }, Search_CaseSensitive{ false }, Search_Regex{ false }, Search_ByRegistry{ false };
    bool NeedsUpdateV{ false };

    inline namespace __
    {
        extern bool ReversedFlag;
        extern SortBy CurrentSortBy;
    }

    void RenderUI()
    {
        int SelectN = 0;
        int TotalSections = 0;
        static int SelAndFrozenN = 0;
        static int SelAndHiddenN = 0;
        bool FullSelected = true;

        IBD_RInterruptF(x);

        if (CurrentList.empty())InitSort();
        if (NeedsUpdateV)
        {
            bool Rev = IsReversed();
            auto Sort = GetCurrentSortBy();
            RemakeSort();
            CurrentSortBy = SortBy::COUNT;
            ReversedFlag = Rev;
            SetSortBy(Sort);
            NeedsUpdateV = false;
        }
        for (auto& ini : IBF_Inst_Project.Project.Inis)
        {
            if (ini.Secs_ByName.empty())continue;
            for (auto& sec : ini.Secs)
            {
                TotalSections++;
                if (sec.second.Dynamic.Selected)
                {
                    ++SelectN;
                }
                else FullSelected = false;
            }
        }

        bool SelectAll{ false }, SelectNone{ false }, Delete{ false }, Duplicate{ false };
        bool Freeze{ false }, Unfreeze{ false }, Hide{ false }, Show{ false };
        const bool UseUnfreeze = SelectN && (SelAndFrozenN == SelectN);
        const bool UseShow = SelectN && (SelAndHiddenN == SelectN);

        auto lw = ImGui::GetWindowContentRegionWidth() - FontHeight * 0.9F;
        auto lh = FontHeight * 1.6F;
        auto BUTTON = [&](const char* Str, bool Disabled) -> bool
            {
                if (Disabled)
                {
                    ImGui::BeginDisabled();
                    ImGui::Button(Str, { lw / 5.0F, lh });
                    ImGui::EndDisabled();
                    return false;
                }
                else return ImGui::Button(Str, { lw / 5.0F, lh });
            };

        {
            if (FullSelected)
            {
                if (SelectN == 0)BUTTON(locc("GUI_SelectAll"), true);
                else if (BUTTON(locc("GUI_SelectNone"), false))SelectNone = true;
            }
            else if (BUTTON(locc("GUI_SelectAll"), false))SelectAll = true;
            ImGui::SameLine();
            if (SelectN == 0)
            {
                BUTTON(locc("GUI_Delete"), true); ImGui::SameLine();
                BUTTON(locc("GUI_Duplicate"), true);  ImGui::SameLine();
                BUTTON(locc("GUI_FreezeSec"), true);  ImGui::SameLine();
                BUTTON(locc("GUI_HideSec"), true);
            }
            else
            {
                if (BUTTON(locc("GUI_Delete"), false))Delete = true; ImGui::SameLine();
                if (BUTTON(locc("GUI_Duplicate"), false))Duplicate = true; ImGui::SameLine();

                if (UseUnfreeze) { if (BUTTON(locc("GUI_UnfreezeSec"), false)) Unfreeze = true; }
                else { if (BUTTON(locc("GUI_FreezeSec"), false))Freeze = true; }
                ImGui::SameLine();
                if (UseShow) { if (BUTTON(locc("GUI_ShowSec"), false)) Show = true; }
                else { if (BUTTON(locc("GUI_HideSec"), false)) Hide = true; }
            }
            
            if (TotalSections)
            {
                ImGui::Text(locc("GUI_SelectedCount"));
                ImGui::SameLine();
                ImGui::Text(u8" %d/%d ", SelectN, TotalSections);

                //-----NEW LINE-----

                IBR_Combo(locc("GUI_SortBy"), GetCurrentSortName(), 0, [&] {
                    for (int i = 0; i < SortTypeCount; i++)
                    {
                        auto CurSort = static_cast<SortBy>(i);
                        if (ImGui::Selectable(GetSortName(CurSort), GetCurrentSortBy() == CurSort))
                        {
                            SetSortBy(CurSort);
                        }
                    }
                });
                ImGui::SameLine();

                //IsReversed() false 为升序 true 为降序
                if(ImGui::ArrowButton("##REVERSE_LIST", IsReversed() ? ImGuiDir_Down : ImGuiDir_Up))
                {
                    Reverse();
                }

                //-----NEW LINE-----
                ImGui::InputText(locc("GUI_SearchSection"), GetSearchBuffer(), MAX_STRING_LENGTH, ImGuiInputTextFlags_SearchIconBg);

                //-----NEW LINE-----
                //bool Search_Full{ false }, Search_CaseSensitive{ false }, Search_Regex{ false }, Search_ByRegistry{ false };
                if (ImGui::Button(Search_ByRegistry ? locc("GUI_SearchSec_ByRegistry") : locc("GUI_SearchSec_ByDisplayName")))
                    Search_ByRegistry = !Search_ByRegistry;
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FontHeight * 1.0F);
                ImGui::Checkbox(locc("GUI_SearchSec_Full"), &Search_Full);

                //-----NEW LINE-----
                ImGui::Checkbox(locc("GUI_SearchSec_CaseSensitive"), &Search_CaseSensitive);
                ImGui::SameLine();
                ImGui::Checkbox(locc("GUI_SearchSec_Regex"), &Search_Regex);
            }
        }
        if (SelectAll || SelectNone || Delete || Duplicate)
        {
            std::vector<IBB_Section_Desc> ToDel;
            if (Duplicate)
            {
                std::vector<ModuleID_t> Tg;
                for (auto& ini : IBF_Inst_Project.Project.Inis)
                {
                    if (ini.Secs_ByName.empty())continue;
                    for (auto& sec : ini.Secs)
                        if (sec.second.Dynamic.Selected)
                            if (auto s = IBR_Inst_Project.GetSectionID(sec.second.GetThisDesc()); s)
                                Tg.push_back(*s);
                }
                if (!Tg.empty())
                {
                    IBR_WorkSpace::MassSelect(Tg);
                    IBR_WorkSpace::DuplicateSelected();
                }
            }
            for (auto& ini : IBF_Inst_Project.Project.Inis)
            {
                if (ini.Secs_ByName.empty())continue;
                for (auto& sec : ini.Secs)
                {
                    if (SelectAll)sec.second.Dynamic.Selected = true;
                    if (SelectNone)sec.second.Dynamic.Selected = false;
                    if (sec.second.Dynamic.Selected && Delete)ToDel.push_back({ ini.Name,sec.second.Name });
                }
            }
            if (Delete)
                IBRF_CoreBump.SendToR({ [=]() { IBR_Inst_Project.DeleteSection(ToDel); },nullptr });
            else IBRF_CoreBump.SendToR({ [=]() {IBF_Inst_Project.UpdateAll(); },nullptr });


        }

        std::set<IBB_SectionID> InvalidSet;
        SelAndFrozenN = 0;
        SelAndHiddenN = 0;

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        ImGui::BeginChildFrame(67656, {
            ImGui::GetWindowWidth() - ImGui::GetCursorPosX(),
            ImGui::GetWindowHeight() - ImGui::GetCursorPosY() - FontHeight * 2.5F });
        ImGui::PopStyleColor();

        for (auto& idx : CurrentList)
        {
            auto rsc = IBR_Inst_Project.GetSection(idx);
            auto secName = idx.Section();
            auto psec = idx.GetSec(IBF_Inst_Project.Project);
            if (psec == nullptr)
            {
                InvalidSet.insert(idx);
                continue;
            }
            auto& sec = *psec;
            auto dat = rsc.GetSectionData();
            if (dat == nullptr)
            {
                InvalidSet.insert(idx);
                continue;
            }

            if (sec.Dynamic.Selected)
            {
                if (Freeze)dat->Frozen = true;
                if (Unfreeze)dat->Frozen = false;
                if (Hide)dat->Hidden = true;
                if (Show)dat->Hidden = false;

                if (dat->Frozen)SelAndFrozenN++;
                if (dat->Hidden)SelAndHiddenN++;
            }

            auto& searchsrc = Search_ByRegistry ? sec.Name : dat->DisplayName;
            if (GetSearchBuffer()[0] &&//不为空
                !StringMatch(searchsrc, GetSearchBuffer(), Search_Full, Search_CaseSensitive, Search_Regex))
                continue;

            auto HasCol = IBB_DefaultRegType::HasRegType(sec.Register);
            auto& Reg = IBB_DefaultRegType::GetRegType(sec.Register);
            if (HasCol)
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, Reg.FrameColor.Value);
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Reg.FrameColorPlus1.Value);
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Reg.FrameColorPlus2.Value);
                ImGui::PushStyleColor(ImGuiCol_CheckMark, Reg.FrameColorH.Value);
            }
            bool Styled = dat->Ignore || dat->Frozen || dat->Hidden;
            if (dat->Hidden) ImGui::PushStyleColor(ImGuiCol_Text, IBR_Color::HiddenSecColor.Value);
            else if (dat->Ignore) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            else if (dat->Frozen) ImGui::PushStyleColor(ImGuiCol_Text, IBR_Color::FrozenSecColor.Value);
            auto& BoxText = IBR_WorkSpace::ShowRegName ? sec.Name : dat->DisplayName;

            ImGui::Checkbox(BoxText.c_str(), &sec.Dynamic.Selected);

            if (HasCol)ImGui::PopStyleColor(4);
            if (Styled)ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - FontHeight * 2.0f));//4.5个字符是右侧内容的预留空间
            if (ImGui::ArrowButton((secName + "_ub_arr").c_str(), ImGuiDir_Right))
            {
                IBR_EditFrame::ActivateAndEdit(rsc.ID, false);
                IBR_FullView::EqCenter = dat->EqPos + (dat->EqSize / 2.0);
            }
        }
        std::erase_if(CurrentList, [&](const IBB_SectionID& idx) { return InvalidSet.count(idx) != 0; });

        ImGui::NewLine();

        ImGui::EndChildFrame();
        
    }

    inline namespace __
    {
        SortBy CurrentSortBy = SortBy::COUNT;
        bool ReversedFlag = false;
        BufString _TEXT_UTF8 SearchBuffer;

        BufString _TEXT_UTF8& GetSearchBuffer()
        {
            return SearchBuffer;
        }
        void NeedsUpdate()
        {
            NeedsUpdateV = true;
        }
        void RemakeSort()
        {
            if (!CurrentList.empty())
            {
                CurrentList.clear();
                ReversedFlag = false;
            }
            for (auto& ini : IBF_Inst_Project.Project.Inis)
            {
                if (ini.Secs_ByName.empty())continue;
                for (auto& sec : ini.Secs)
                    CurrentList.emplace_back(ini.Name, sec.second.Name);
            }
        }

        bool SortOrderByDefault(const IBB_SectionID& l, const IBB_SectionID& r)
        {
            auto sl = IBR_Inst_Project.GetSection(l);
            auto sr = IBR_Inst_Project.GetSection(r);
            auto& rl = sl.GetRegTypeName();
            auto& rr = sr.GetRegTypeName();
            if (rl != rr)return rl < rr;
            return StrCmpZHCN(sl.GetDisplayName(), sr.GetDisplayName());
        }
        bool SortOrderByRegName(const IBB_SectionID& l, const IBB_SectionID& r)
        {
            return l < r;
        }
        bool SortOrderByDisplayName(const IBB_SectionID& l, const IBB_SectionID& r)
        {
            return StrCmpZHCN(IBR_Inst_Project.GetSection(l).GetDisplayName(),IBR_Inst_Project.GetSection(r).GetDisplayName());
        }
        bool SortOrderByRegType(const IBB_SectionID& l, const IBB_SectionID& r)
        {
            return IBR_Inst_Project.GetSection(l).GetRegTypeName() < IBR_Inst_Project.GetSection(r).GetRegTypeName();
        }
       

        void SortByImpl(bool (*fn)(const IBB_SectionID& l, const IBB_SectionID& r))
        {
            if (CurrentList.empty())RemakeSort();
            std::ranges::sort(CurrentList, fn);
        }

        void SortByDefault() { SortByImpl(SortOrderByDefault); }
        void SortByRegName() { SortByImpl(SortOrderByRegName); }
        void SortByDisplayName() { SortByImpl(SortOrderByDisplayName); }
        void SortByRegType() { SortByImpl(SortOrderByRegType); }

        void InitSort()
        {
            CurrentSortBy = SortBy::COUNT;
            SetSortBy(SortBy::Default);
        }
        void ClearSort()
        {
            CurrentList.clear();
            CurrentSortBy = SortBy::COUNT;
            ReversedFlag = false;
            SearchBuffer[0] = '\0';
        }
        void SetSortBy(SortBy Type)
        {
            if (Type == CurrentSortBy)
                return;
            CurrentSortBy = Type;
            if (ReversedFlag)
            {
                Reverse();
            }
            switch (Type)
            {
            case SortBy::Default:
                SortByDefault();
                break;
            case SortBy::RegName:
                SortByRegName();
                break;
            case SortBy::DisplayName:
                SortByDisplayName();
                break;
            case SortBy::RegType:
                SortByRegType();
                break;
            default:
                CurrentSortBy = SortBy::Default;
                SortByDefault();
                break;
            }
        }
        const char* GetSortName(SortBy Type)
        {
            switch (Type)
            {
            case SortBy::Default:
                return locc("GUI_SortByDefault");
            case SortBy::RegName:
                return locc("GUI_SortByRegName");
            case SortBy::DisplayName:
                return locc("GUI_SortByDisplayName");
            case SortBy::RegType:
                return locc("GUI_SortByRegType");
            default:
                return locc("Error_SortByUnknown");
            }
        }
        const char* GetCurrentSortName()
        {
            return GetSortName(CurrentSortBy);
        }
        SortBy GetCurrentSortBy()
        {
            return CurrentSortBy;
        }
        void Reverse()
        {
            ReversedFlag = !ReversedFlag;
            std::reverse(CurrentList.begin(), CurrentList.end());
        }
        bool IsReversed()
        {
            return ReversedFlag;
        }
    }
}
