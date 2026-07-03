#include "IBR_ImportIni.h"
#include "Global.h"
#include "IBR_Components.h"
#include "IBR_Misc.h"
#include "IBR_Combo.h"
#include "IBB_RegType.h"
#include "IBFront.h"
#include "IBR_Project.h"
#include <imgui.h>
#include <algorithm>
#include <format>
#include <ranges>

extern int FontHeight;

namespace IBR_ImportPreview
{
    // 内部状态
    static ImportedIniFile g_File;
    static std::function<void(const IBR_ImportResult&)> g_Callback;
    static bool g_OpenPending = false;
    static std::string g_FilterText;

    // 每个未匹配 section 的 RegType 选择缓存
    static std::unordered_map<size_t, std::string> g_SelectionCache;

    // 可用注册表类型列表（弹出下拉菜单用）
    static std::vector<std::string> g_AvailableRegTypes;

    void Open(ImportedIniFile&& File, const std::function<void(const IBR_ImportResult&)>& Callback)
    {
        g_File = std::move(File);
        g_Callback = Callback;
        g_OpenPending = true;
        g_SelectionCache.clear();
        g_FilterText.clear();

        // 收集所有可用注册表类型
        g_AvailableRegTypes.clear();
        for (auto& [Name, Reg] : IBB_DefaultRegType::RegisterTypes)
            g_AvailableRegTypes.push_back(Name);
        // 排序以便显示
        std::sort(g_AvailableRegTypes.begin(), g_AvailableRegTypes.end());
    }

    void RegenMatch(const std::string& Search)
    {
        // 判断 a 是否在 b 中出现（不区分大小写）
        bool contains_ignore_case(const std::string& a, const std::string& b);

        for (auto&& [Sec, Match] : std::views::zip(g_File.Sections, g_File.Matched))
        {
            if(contains_ignore_case(Search, Sec.SectionName))
                Match = true;
            else
                Match = false;
        }
    }

    void RenderUI()
    {
        auto Action = []()
        {
            ImGui::BeginChild("##ImportContent", ImVec2(0, -(FontHeight * 4.5F)));
            size_t MatchedCount = 0, LinkMatchedCount = 0, UnmatchedCount = 0;
            size_t MatchedSearch = 0, LinkMatchedSearch = 0, UnmatchedSearch = 0;
            for (auto&& [Sec, Match] : std::views::zip(g_File.Sections, g_File.Matched))
            {
                if (Sec.MatchStatus == IniImportMatchStatus::Matched)
                    MatchedCount++;
                else if (Sec.MatchStatus == IniImportMatchStatus::LinkMatched)
                    LinkMatchedCount++;
                else
                    UnmatchedCount++;

                if (Match)
                {
                    if (Sec.MatchStatus == IniImportMatchStatus::Matched)
                        MatchedSearch++;
                    else if (Sec.MatchStatus == IniImportMatchStatus::LinkMatched)
                        LinkMatchedSearch++;
                    else
                        UnmatchedSearch++;
                }
            }

            // ---- 摘要 + 全选 ----
            ImGui::Text(locc("GUI_ImportIni_Summary"),
                (int)g_File.Sections.size(), (int)MatchedCount, (int)LinkMatchedCount, (int)UnmatchedCount);

            // 全局全选/取消
            bool AllSelected = std::ranges::all_of(g_File.Sections, [](auto& S) { return !S.IsRegistryList ? S.Selected : true; });
            if (ImGui::Checkbox(locc("GUI_ImportIni_SelectAll"), &AllSelected))
            {
                for (auto& Sec : g_File.Sections)
                    if (!Sec.IsRegistryList)
                        Sec.Selected = AllSelected;
            }

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FontHeight * 2.0F);
            if (InputTextStdString(locc("GUI_SearchSection"), g_FilterText, ImGuiInputTextFlags_SearchIconBg))
            {
                RegenMatch(g_FilterText);
            }

            ImGui::Separator();
            ImGui::NewLine();

            // ---- 冲突检测 ----
            {
                std::vector<std::string> Conflicts;
                for (auto& Sec : g_File.Sections)
                {
                    if (Sec.IsRegistryList) continue;
                    IBB_Section_Desc Desc{ DefaultIniName, Sec.SectionName };
                    if (IBR_Inst_Project.HasSection(Desc))
                        Conflicts.push_back(Sec.SectionName);
                }

                if (!Conflicts.empty())
                {
                    ImGui::NewLine();
                    ImGui::TextColored(ImVec4{ 0.9F, 0.6F, 0.0F, 1.0F }, "%s",
                        locc("GUI_ImportIni_ConflictTitle"));
                    ImGui::Indent();
                    for (auto& Name : Conflicts)
                        ImGui::Text("  %s", Name.c_str());
                    ImGui::Unindent();
                    ImGui::NewLine();
                }
            }

            // ---- 已匹配列表（可收起的匹配方式标题，按类型分组，每组一个全选框） ----
            if (MatchedCount > 0)
            {
                bool AllMatchedSel = std::ranges::all_of(g_File.Sections,
                    [](auto& S) { return S.MatchStatus != IniImportMatchStatus::Matched || S.Selected; });
                if (ImGui::Checkbox("##SelAll_Matched", &AllMatchedSel))
                {
                    for (auto& Sec : g_File.Sections)
                        if (Sec.MatchStatus == IniImportMatchStatus::Matched)
                            Sec.Selected = AllMatchedSel;
                }
                ImGui::SameLine();
                ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.2F, 0.8F, 0.2F, 1.0F });
                if (
                    g_FilterText.empty() ?
                    ImGui::TreeNode("##Sec_Matched", "%s  (%d)", locc("GUI_ImportIni_Matched"), (int)MatchedCount) :
                    ImGui::TreeNode("##Sec_Matched", "%s  (%d/%d)", locc("GUI_ImportIni_Matched"), (int)MatchedSearch, (int)MatchedCount)
                    )
                {
                    ImGui::PopStyleColor();
                    // 按注册表类型分组
                    std::unordered_map<std::string, std::vector<ImportedIniSection*>> MatchedGroups;
                    for (auto&& [Sec, Match] : std::views::zip(g_File.Sections, g_File.Matched))
                    {
                        if (!Match)continue;
                        if (Sec.MatchStatus == IniImportMatchStatus::Matched)
                            MatchedGroups[Sec.MatchedRegType].push_back(&Sec);
                    }

                    for (auto& [TypeName, Secs] : MatchedGroups)
                    {
                        // 类型全选框（放在 TreeNode 外部，保证收起时仍可全选）
                        bool TypeAllSel = std::ranges::all_of(Secs, [](auto* S) { return S->Selected; });
                        if (ImGui::Checkbox(("##SelAll_" + TypeName).c_str(), &TypeAllSel))
                        {
                            for (auto* S : Secs)
                                S->Selected = TypeAllSel;
                        }
                        ImGui::SameLine();

                        // 可收起的类型标题
                        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
                        if (ImGui::TreeNode(("##TypeNode_" + TypeName).c_str(), "%s  (%d)", TypeName.c_str(), (int)Secs.size()))
                        {
                            for (auto* Sec : Secs)
                            {
                                ImGui::Checkbox(("##Sel_" + Sec->SectionName).c_str(), &Sec->Selected);
                                ImGui::SameLine();

                                auto DisplayText = std::format("{}  ({} {})",
                                    Sec->SectionName, Sec->KeyValues.size(), locc("GUI_ImportIni_KeyUnit"));
                                ImGui::Text("%s", DisplayText.c_str());

                                // 展开查看键值对
                                auto TreeNodeLabel = std::format("##KV_{}", Sec->SectionName);
                                if (ImGui::TreeNode(TreeNodeLabel.c_str()))
                                {
                                    for (auto& KV : Sec->KeyValues)
                                    {
                                        if (KV.Value.empty()) continue;
                                        ImGui::TextColored(ImVec4{ 0.6F, 0.8F, 1.0F, 1.0F }, "  %s", KV.Key.c_str());
                                        ImGui::SameLine();
                                        ImGui::Text("= %s", KV.Value.c_str());
                                    }
                                    ImGui::TreePop();
                                }
                            }
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                    ImGui::NewLine();
                }
                else
                {
                    ImGui::PopStyleColor();
                }
            }

            // ---- 链接匹配列表（可收起的匹配方式标题，按类型分组，每组一个全选框） ----
            if (LinkMatchedCount > 0)
            {
                bool AllLinkMatchedSel = std::ranges::all_of(g_File.Sections,
                    [](auto& S) { return S.MatchStatus != IniImportMatchStatus::LinkMatched || S.Selected; });
                if (ImGui::Checkbox("##SelAll_LinkMatched", &AllLinkMatchedSel))
                {
                    for (auto& Sec : g_File.Sections)
                        if (Sec.MatchStatus == IniImportMatchStatus::LinkMatched)
                            Sec.Selected = AllLinkMatchedSel;
                }
                ImGui::SameLine();
                ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.0F, 0.6F, 0.9F, 1.0F });
                if (g_FilterText.empty() ?
                    ImGui::TreeNode("##Sec_LinkMatched", "%s  (%d)", locc("GUI_ImportIni_LinkMatched"), (int)LinkMatchedCount) :
                    ImGui::TreeNode("##Sec_LinkMatched", "%s  (%d/%d)", locc("GUI_ImportIni_LinkMatched"), (int)LinkMatchedSearch, (int)LinkMatchedCount)
                    )
                {
                    ImGui::PopStyleColor();
                    // 按注册表类型分组
                    std::unordered_map<std::string, std::vector<ImportedIniSection*>> LinkMatchedGroups;
                    for (auto&& [Sec, Match] : std::views::zip(g_File.Sections, g_File.Matched))
                    {
                        if(!Match)continue;
                        if (Sec.MatchStatus == IniImportMatchStatus::LinkMatched)
                            LinkMatchedGroups[Sec.MatchedRegType].push_back(&Sec);
                    }

                    for (auto& [TypeName, Secs] : LinkMatchedGroups)
                    {
                        // 类型全选框（放在 TreeNode 外部，保证收起时仍可全选）
                        bool TypeAllSel = std::ranges::all_of(Secs, [](auto* S) { return S->Selected; });
                        if (ImGui::Checkbox(("##SelAll_LM_" + TypeName).c_str(), &TypeAllSel))
                        {
                            for (auto* S : Secs)
                                S->Selected = TypeAllSel;
                        }
                        ImGui::SameLine();

                        // 可收起的类型标题
                        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
                        if (ImGui::TreeNode(("##TypeNode_LM_" + TypeName).c_str(), "%s  (%d)", TypeName.c_str(), (int)Secs.size()))
                        {
                            for (auto* Sec : Secs)
                            {
                                ImGui::Checkbox(("##Sel_" + Sec->SectionName).c_str(), &Sec->Selected);
                                ImGui::SameLine();

                                auto DisplayText = std::format("{}  ({} {})",
                                    Sec->SectionName, Sec->KeyValues.size(), locc("GUI_ImportIni_KeyUnit"));
                                ImGui::Text("%s", DisplayText.c_str());
                                ImGui::SameLine();
                                ImGui::TextColored(ImVec4{ 0.5F, 0.5F, 0.5F, 1.0F }, "  <- %s", Sec->LinkMatchSource.c_str());

                                // 展开查看键值对
                                auto TreeNodeLabel = std::format("##KV_{}", Sec->SectionName);
                                if (ImGui::TreeNode(TreeNodeLabel.c_str()))
                                {
                                    for (auto& KV : Sec->KeyValues)
                                    {
                                        if (KV.Value.empty()) continue;
                                        ImGui::TextColored(ImVec4{ 0.6F, 0.8F, 1.0F, 1.0F }, "  %s", KV.Key.c_str());
                                        ImGui::SameLine();
                                        ImGui::Text("= %s", KV.Value.c_str());
                                    }
                                    ImGui::TreePop();
                                }
                            }
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                    ImGui::NewLine();
                }
                else
                {
                    ImGui::PopStyleColor();
                }
            }

            // ---- 未匹配列表（可收起的匹配方式标题） ----
            if (UnmatchedCount > 0)
            {
                // 未匹配组全选框（放在 TreeNode 外部，保证收起时仍可全选）
                bool AllUnmatchedSel = std::ranges::all_of(g_File.Sections,
                    [](auto& S) { return S.MatchStatus != IniImportMatchStatus::Unmatched || S.Selected; });
                if (ImGui::Checkbox("##SelAll_Unmatched", &AllUnmatchedSel))
                {
                    for (auto& Sec : g_File.Sections)
                        if (Sec.MatchStatus == IniImportMatchStatus::Unmatched)
                            Sec.Selected = AllUnmatchedSel;
                }
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.8F, 0.2F, 0.0F, 1.0F });
                ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
                if (g_FilterText.empty() ?
                    ImGui::TreeNode("##Sec_Unmatched", "%s  (%d)", locc("GUI_ImportIni_Unmatched"), (int)UnmatchedCount) :
                    ImGui::TreeNode("##Sec_Unmatched", "%s  (%d/%d)", locc("GUI_ImportIni_Unmatched"), (int)UnmatchedSearch, (int)UnmatchedCount)
                )
                {
                    ImGui::PopStyleColor();
                    for (auto&& [Sec, Match] : std::views::zip(g_File.Sections, g_File.Matched))
                    {
                        if (!Match)continue;
                        if (Sec.MatchStatus != IniImportMatchStatus::Unmatched)
                            continue;

                        ImGui::Checkbox(("##Sel_" + Sec.SectionName).c_str(), &Sec.Selected);
                        ImGui::SameLine();

                        auto DisplayText = std::format("{}  ({} {})",
                            Sec.SectionName, Sec.KeyValues.size(), locc("GUI_ImportIni_KeyUnit"));
                        ImGui::Text("%s", DisplayText.c_str());
                        ImGui::SameLine();

                        // 下拉选择注册表类型
                        std::string ComboLabel = "##RegType_" + Sec.SectionName;
                        auto& Selection = g_SelectionCache[Sec.Index];

                        // 确保初始选中第一个
                        if (Selection.empty() && !g_AvailableRegTypes.empty())
                            Selection = g_AvailableRegTypes[0];

                        if (ImGui::BeginCombo(ComboLabel.c_str(), Selection.c_str()))
                        {
                            for (auto& RegName : g_AvailableRegTypes)
                            {
                                bool IsSelected = (Selection == RegName);
                                if (ImGui::Selectable(RegName.c_str(), IsSelected))
                                    Selection = RegName;
                                if (IsSelected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }

                        // 展开查看键值对
                        auto TreeNodeLabel = std::format("##KV_{}", Sec.SectionName);
                        if (ImGui::TreeNode(TreeNodeLabel.c_str()))
                        {
                            for (auto& KV : Sec.KeyValues)
                            {
                                if (KV.Value.empty()) continue;
                                if (!IBF_Inst_DefaultTypeList.List.KeyBelongToLine(KV.Key)) continue;
                                ImGui::TextColored(ImVec4{ 0.6F, 0.8F, 1.0F, 1.0F }, "  %s", KV.Key.c_str());
                                ImGui::SameLine();
                                ImGui::TextWrapped("= %s", KV.Value.c_str());
                            }
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                    ImGui::NewLine();
                }
                else
                {
                    ImGui::PopStyleColor();
                }
            }

            ImGui::EndChild();
            ImGui::Separator();

            // ---- 底部按钮 ----
            float ButtonWidth = FontHeight * 8.0F;
            float WindowWidth = ImGui::GetWindowWidth();
            ImGui::SetCursorPosX((WindowWidth - ButtonWidth * 2.0F - ImGui::GetStyle().ItemSpacing.x) * 0.5F);

            if (ImGui::Button(locc("GUI_ImportIni_Confirm"), ImVec2{ ButtonWidth, FontHeight * 2.0F }))
            {
                // 将用户在未匹配 section 下拉菜单中选择的类型写回
                for (auto& Sec : g_File.Sections)
                {
                    if (Sec.MatchStatus == IniImportMatchStatus::Unmatched)
                    {
                        auto It = g_SelectionCache.find(Sec.Index);
                        if (It != g_SelectionCache.end() && !It->second.empty())
                        {
                            SetSectionRegType(Sec, It->second);
                        }
                        else
                        {
                            // 用户未选择则设为 _AnyType
                            SetSectionRegType(Sec, "_AnyType");
                            IBB_DefaultRegType::EnsureRegType("_AnyType");
                        }
                    }
                }

                // 构建结果并回调
                IBR_ImportResult Result;
                Result.Confirmed = true;
                Result.File = std::move(g_File);

                if (g_Callback)
                    g_Callback(Result);

                g_Callback = nullptr;
                IBR_PopupManager::ClearCurrentPopup();
                return;
            }

            ImGui::SameLine();

            if (ImGui::Button(locc("GUI_Cancel"), ImVec2{ ButtonWidth, FontHeight * 2.0F }))
            {
                IBR_ImportResult Result;
                Result.Confirmed = false;
                Result.File = std::move(g_File);

                if (g_Callback)
                    g_Callback(Result);

                g_Callback = nullptr;
                IBR_PopupManager::ClearCurrentPopup();
                return;
            }
        };

        // 首次打开时触发 OpenPopup（仅一次）
        // 注意：BeginPopupModal 必须每帧调用，不能用 g_OpenPending 提前 return
        if (g_OpenPending)
        {
            g_OpenPending = false;
            //ImGui::OpenPopup(loc("GUI_ImportIni_Title").c_str());
            IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}
                .CreateModal(loc("GUI_ImportIni_Title"), false)
                .SetSize(ImVec2{ FontHeight * 30.0F, FontHeight * 22.0F })
                .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)
                .PushMsgBack(Action)
            ));
        }

        // 没有待处理的导入时直接跳过
        if (g_File.Sections.empty() && !g_Callback)
            return;
    }
}
