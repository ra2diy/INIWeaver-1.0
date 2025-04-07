#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include<imgui_internal.h>

void DrawNoEntrySymbol(ImVec2 pos, float size, ImU32 col) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 计算中心点和半径
    ImVec2 center = ImVec2(pos.x + size / 2, pos.y + size / 2);

    size *= 0.9F;

    float radius = size / 2;

    // 绘制圆圈
    drawList->AddCircle(center, radius, col, 0, 2.0f);

    // 绘制斜线
    float offset = radius * 0.7071f; // 45度斜线的偏移量
    drawList->AddLine(
        ImVec2(center.x - offset, center.y - offset),
        ImVec2(center.x + offset, center.y + offset),
        col,
        2.0f
    );
}

void DrawCross(ImVec2 pos, float size, ImU32 col) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 计算线条的起点和终点
    ImVec2 p1 = ImVec2(pos.x + size * 0.15f, pos.y + size * 0.15f);
    ImVec2 p2 = ImVec2(pos.x + size * 0.85f, pos.y + size * 0.85f);
    ImVec2 p3 = ImVec2(pos.x + size * 0.15f, pos.y + size * 0.85f);
    ImVec2 p4 = ImVec2(pos.x + size * 0.85f, pos.y + size * 0.15f);

    // 绘制第一条线
    drawList->AddLine(p1, p2, col, 2.0f);

    // 绘制第二条线
    drawList->AddLine(p3, p4, col, 2.0f);
}


void DrawCheckmark(ImVec2 pos, float size, ImU32 col) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 计算线条的起点和终点
    ImVec2 p1 = ImVec2(pos.x + size * 0.2f, pos.y + size * 0.5f); // 第一条线的起点
    ImVec2 p2 = ImVec2(pos.x + size * 0.4f, pos.y + size * 0.7f); // 第一条线的终点
    ImVec2 p3 = ImVec2(pos.x + size * 0.8f, pos.y + size * 0.3f); // 第二条线的终点

    // 绘制第一条线
    drawList->AddLine(p1, p2, col, 2.0f);

    // 绘制第二条线
    drawList->AddLine(p2, p3, col, 2.0f);
}

namespace ImGui
{
    ImVec2 GetLineEndPos();
    ImVec2 GetLineBeginPos();
    bool IsWindowClicked(ImGuiMouseButton Button);
}

extern wchar_t CurrentDirW[];
extern const char* DefaultAltPropType;
extern const char* LinkAltPropType;
extern int RFontHeight;

void IBR_SectionData::RenameDisplay()
{
    const size_t BufSize = 1000;
    char* Str = new char[BufSize] {};
    strcpy(Str, DisplayName.c_str());
    IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}.CreateModal(loc("GUI_ModuleRename"), true, [this, Str]
        {
            delete[]Str;
        })
        .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize).PushMsgBack([this, Str]()
            {
                ImGui::SetWindowSize({ FontHeight * 20.0f,FontHeight * 10.0f });
                ImGui::Text(locc("GUI_EnterNewModuleName"));
                ImGui::InputText("", Str, BufSize);
                if (Str != DisplayName && IBF_Inst_Project.HasDisplayName(Str))
                {
                    ImGui::TextDisabled(locc("GUI_OK"));
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, IBR_Color::ErrorTextColor.Value);
                    ImGui::Text(locc("GUI_ModuleRename_Error1"));
                    ImGui::PopStyleColor(1);
                }
                else
                {
                    if (ImGui::Button(locc("GUI_OK")))
                    {
                        IBF_Inst_Project.DisplayNames.erase(DisplayName);
                        IBF_Inst_Project.DisplayNames[Str] = Desc;
                        IBG_Undo.SomethingShouldBeHere();
                        DisplayName = Str;
                        delete[]Str;
                        IBR_PopupManager::ClearPopupDelayed();
                    }
                }
            })));
}

void IBR_SectionData::RenameDisplayImpl(const std::string& Name)
{
    IBF_Inst_Project.DisplayNames.erase(DisplayName);
    auto it = IBF_Inst_Project.DisplayNames.find(Name);
    int I = 0;
    while (it != IBF_Inst_Project.DisplayNames.end())
        it = IBF_Inst_Project.DisplayNames.find(Name + "_" + std::to_string(++I));
    if (I) DisplayName = Name + "_" + std::to_string(I);
    IBF_Inst_Project.DisplayNames[DisplayName] = Desc;
    IBG_Undo.SomethingShouldBeHere();
}



bool IBR_SectionData::OnLineEdit(const std::string& Name, bool OnLink)
{
    auto& Line = ActiveLines[Name];
    bool HasInput{ false };
    if (Line.Edit.Input)HasInput = true;
    auto sec = IBR_Inst_Project.GetSection(Desc);
    auto back = sec.GetBack();
    auto line = back->GetLineFromSubSecs(Name);
    if (!line)
    {
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            auto W = UTF8toUnicode(Desc.GetText() + u8" : " + Name);
            GlobalLog.AddLog(std::vformat(L"IBR_SectionData::OnLineEdit ： "+locw("Log_INILineNotExist"),
                std::make_wformat_args(W)));
        }
        return true;
    }
    if (!OnLink)
    {
        if (line->Default->Property.TypeAlt == "bool")
        {
            Line.Buffer = line->Data->GetString();
            bool X = (Line.Buffer == "yes" || Line.Buffer == "true");
            if(!IBR_WorkSpace::ShowRegName)ImGui::Checkbox(line->Default->DescShort.c_str(), &X);
            else ImGui::Checkbox(Name.c_str(), &X);
            if (!line->Default->DescLong.empty() && ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(line->Default->DescLong.c_str());
                ImGui::EndTooltip();
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                line->Data->SetValue(!X ? "yes" : "no");
                if (IBR_Inst_Project.IBR_Rev_SectionMap[Desc] == IBR_EditFrame::CurSection.ID)
                {
                    IBR_EditFrame::EditLines[Name].Buffer = !X ? "yes" : "no";
                }
            }
            return true;
        }
    }
    if (Line.Edit.NeedInit())
    {
        IBR_IniLine::InitType It{ line->Data->GetString() ,
            "##" + RandStr(8),[Name, desc = this->Desc, line](char* S)
                {
                    IBG_Undo.SomethingShouldBeHere();
                    line->Data->SetValue(S);
                    if (IBR_Inst_Project.IBR_Rev_SectionMap[desc] == IBR_EditFrame::CurSection.ID)
                    {
                        IBR_EditFrame::EditLines[Name].Buffer = S;
                    }
                } };
        if (!IBR_WorkSpace::ShowRegName)Line.Edit.RenderUI(line->Default->DescShort, line->Default->DescLong, &It);
        else Line.Edit.RenderUI(Name, line->Default->DescLong, &It);
    }
    else
    {
        if (!IBR_WorkSpace::ShowRegName)Line.Edit.RenderUI(line->Default->DescShort, line->Default->DescLong);
        else Line.Edit.RenderUI(Name, line->Default->DescLong);
    }
    return line->Default->Property.TypeAlt.empty() ? true : HasInput;
}

void IBR_SectionData::CopyToClipBoard()
{
    IBB_ClipBoardData ClipData;
    std::vector<IBB_Section_Desc> Sel{ Desc };
    EqDelta = { 0.0f,0.0f };
    ClipData.Generate(Sel);
    ImGui::SetClipboardText(ClipData.GetString().c_str());
    auto c = 1;
    IBR_HintManager::SetHint(UnicodetoUTF8(std::vformat(locw("GUI_CopySuccess"), std::make_wformat_args(c))), HintStayTimeMillis);
}

void DrawDragPreviewIcon()
{
    //ImGui::Text(u8"DRAWN");
    if (ImGui::IsDragDropPayloadBeingAccepted())
    {
        auto& T = IBR_Inst_Project.DragConditionText;
        auto P = ImGui::GetCursorScreenPos();
        if (T.empty())DrawCross(P, float(RFontHeight), IBR_Color::ErrorTextColor);
        else if (T == "HOLY_SHIT\nWHAT_HAD_JUST_HAPPENED\nONE_MINUTE_AGO")DrawNoEntrySymbol(P, float(RFontHeight), IBR_Color::ErrorTextColor);
        else DrawCheckmark(P, float(RFontHeight), IBR_Color::CheckMarkColor);
        ImGui::Dummy(ImVec2{ float(RFontHeight),float(RFontHeight) });
        ImGui::SameLine();
        if (IBR_Inst_Project.DragConditionText.empty())ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_Preview_InvalidLink"));
        else if (T != "HOLY_SHIT\nWHAT_HAD_JUST_HAPPENED\nONE_MINUTE_AGO")
        {
            auto T2 = UTF8toUnicode(T);
            ImGui::Text(UnicodetoUTF8(std::vformat(locw("GUI_Preview_LinkTo"), std::make_wformat_args(T2))).c_str());
        }
        else ImGui::Text(IBR_Inst_Project.DragConditionTextAlt.c_str());
    }
    //else ImGui::Text(u8"NOT ACCEPTED");
}

void IBR_SectionData::RenderUI()
{
    auto HalfLine = ImGui::GetTextLineHeightWithSpacing() * 0.5F;
    FinalY = ImGui::GetCursorPosY();
    auto Rsec = IBR_Inst_Project.GetSection(Desc);
    bool TriggeredRightMenu = false;
    auto Pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({ 0.0f,0.0f });
    ImGui::Dummy(ImGui::GetWindowSize());
    if (ImGui::BeginDragDropTarget())
    {
        auto payload = ImGui::AcceptDragDropPayload("IBR_SecDrag", ImGuiDragDropFlags_AcceptBeforeDelivery);
        if (payload)
        {
            if (payload->IsPreview() || payload->IsDelivery())
            {
                std::string s = (char*)payload->Data;
                const auto& desc = IBR_Inst_Project.IBR_SecDragMap[s].Desc;
                auto sec = IBR_Inst_Project.GetSection(Desc);
                auto back = sec.GetBack();
                auto srcsec = IBR_Inst_Project.GetSection(desc);
                auto srcback = srcsec.GetBack();

                if (back && srcback)
                {
                    auto it = srcback->DefaultLinkKey.find(back->Register);
                    if (it != srcback->DefaultLinkKey.end())
                    {
                        if (payload->IsPreview())
                            IBR_Inst_Project.DragConditionText = Desc.Ini + " -> " + DisplayName + " : " + it->second;
                        if (payload->IsDelivery())
                        {
                            IBG_Undo.SomethingShouldBeHere();
                            IBB_Section_NameType NT;
                            NT.IsLinkGroup = false;
                            NT.IniType = back->Root->Name;
                            NT.Name = back->Name;
                            NT.Lines.Value[it->second] = desc.Sec;
                            //MessageBoxA(NULL, (it->second + " = " + desc.Sec).c_str(), "efrsgdfg", MB_OK);
                            {
                                IBD_RInterruptF(x);
                                back->Merge(IBB_Section(NT, back->Root), u8"Merge", false);
                            }
                            IBF_Inst_Project.UpdateAll();
                        }
                    }
                    else if (payload->IsPreview())
                    {
                        IBR_Inst_Project.DragConditionText = "HOLY_SHIT\nWHAT_HAD_JUST_HAPPENED\nONE_MINUTE_AGO";
                        auto W = UTF8toUnicode(back->Register);
                        IBR_Inst_Project.DragConditionTextAlt = UnicodetoUTF8(std::vformat(locw("GUI_Preview_NoDefaultLink"),
                            std::make_wformat_args(W)));
                    }

                }
                else if (payload->IsPreview())
                    IBR_Inst_Project.DragConditionText.clear();
                if (payload->IsDelivery())
                    IBR_Inst_Project.IBR_LineDragMap.erase(s);
            }
            ImGui::EndDragDropTarget();
        }
        else if ((payload = ImGui::AcceptDragDropPayload("IBR_LineDrag", ImGuiDragDropFlags_AcceptBeforeDelivery)))
            //如果是故意赋值，则可以将其括在括号中 "(e1 = e2)"，以消除此警告
        {
            if (payload->IsPreview() || payload->IsDelivery())
            {
                std::string s = (char*)payload->Data;
                const auto& lin = IBR_Inst_Project.IBR_LineDragMap[s];
                auto sec = IBR_Inst_Project.GetSection(lin.Desc);
                auto back = sec.GetBack();
                auto tgsec = IBR_Inst_Project.GetSection(Desc);
                auto tgback = tgsec.GetBack();
                auto Line = back->GetLineFromSubSecs(lin.Line);

                if (back && tgback)
                {
                    bool Check = true;
                    if (!back->Register.empty())
                    {
                        auto& alt = Line->Default->Property.TypeAlt;
                        if (!alt.empty())
                            Check = IBB_DefaultRegType::MatchType(alt, tgback->Register);
                    }
                    if (Check)
                    {
                        if (payload->IsPreview())
                            IBR_Inst_Project.DragConditionText = Desc.Ini + " -> " + DisplayName;
                        if (payload->IsDelivery())
                        {
                            IBG_Undo.SomethingShouldBeHere();
                            IBB_Section_NameType NT;
                            NT.IsLinkGroup = false;
                            NT.IniType = back->Root->Name;
                            NT.Name = back->Name;
                            NT.Lines.Value[lin.Line] = Desc.Sec;
                            {
                                IBD_RInterruptF(x);
                                back->Merge(IBB_Section(NT, back->Root), u8"Merge", false);
                            }
                            IBF_Inst_Project.UpdateAll();
                        }
                    }
                    else if (payload->IsPreview())
                    {
                        IBR_Inst_Project.DragConditionText = "HOLY_SHIT\nWHAT_HAD_JUST_HAPPENED\nONE_MINUTE_AGO";
                        auto W1 = UTF8toUnicode(Line->Default->Property.TypeAlt);
                        auto W2 = UTF8toUnicode(tgback->Register);
                        IBR_Inst_Project.DragConditionTextAlt = UnicodetoUTF8(std::vformat(locw("GUI_Preview_WrongType"),
                            std::make_wformat_args(W1, W2)));
                    }

                }
                else if (payload->IsPreview())
                    IBR_Inst_Project.DragConditionText.clear();
                if (payload->IsDelivery())
                    IBR_Inst_Project.IBR_LineDragMap.erase(s);
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImVec2 CurL{ Pos.x,Pos.y - 0.2f * FontHeight };
    ImGui::SetCursorPos({ 0.0f,CurL.y });
    ImGui::Dummy({ ImGui::GetWindowWidth(), ImGui::GetTextLineHeightWithSpacing() });
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        TriggeredRightMenu = true;
        if (IsComment)
            IBR_PopupManager::SetRightClickMenu(std::move(
                IBR_PopupManager::Popup{}.Create(DisplayName + "__MODULE").PushMsgBack([this]() {
                    if (Ignore)
                    {
                        if (ImGui::SmallButtonAlignLeft(locc("GUI_NoIgnore"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                        {
                            Ignore = false;
                            IBR_PopupManager::ClearRightClickMenu();
                        }
                    }
                    else
                    {
                        if (ImGui::SmallButtonAlignLeft(locc("GUI_Ignore"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                        {
                            Ignore = true;
                            IBR_PopupManager::ClearRightClickMenu();
                        }
                    }
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_Rename"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        RenameDisplay();
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_Copy"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        CopyToClipBoard();
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_Delete"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                        IBRF_CoreBump.SendToR({ [this]()
                        {IBR_PopupManager::ClearRightClickMenu(); IBR_Inst_Project.DeleteSection(Desc); },nullptr });

                    })
            ),ImGui::GetMousePos());
        else IBR_PopupManager::SetRightClickMenu(std::move(
            IBR_PopupManager::Popup{}.Create(DisplayName + "__MODULE").PushMsgBack([this]() {
                if (Ignore)
                {
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_NoIgnore"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        Ignore = false;
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                }
                else
                {
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_Ignore"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        Ignore = true;
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                }
                if (ImGui::SmallButtonAlignLeft(locc("GUI_Rename"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                {
                    RenameDisplay();
                    IBR_PopupManager::ClearRightClickMenu();
                }
                if (ImGui::SmallButtonAlignLeft(locc("GUI_Copy"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                {
                    CopyToClipBoard();
                    IBR_PopupManager::ClearRightClickMenu();
                }
                if (ImGui::SmallButtonAlignLeft(locc("GUI_EditText"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                {
                    IBR_EditFrame::SetActive(IBR_Inst_Project.IBR_Rev_SectionMap[Desc]);
                    IBR_EditFrame::SwitchToText();
                    IBR_Inst_Menu.ChooseMenu(MenuItemID_EDIT);
                    IBR_PopupManager::ClearRightClickMenu();
                }
                if (ImGui::SmallButtonAlignLeft(locc("GUI_Edit"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                {
                    IBR_EditFrame::SetActive(IBR_Inst_Project.IBR_Rev_SectionMap[Desc]);
                    IBR_Inst_Menu.ChooseMenu(MenuItemID_EDIT);
                    IBR_PopupManager::ClearRightClickMenu();
                }
                if (ImGui::SmallButtonAlignLeft(locc("GUI_Delete"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    IBRF_CoreBump.SendToR({ [this]()
                    {IBR_PopupManager::ClearRightClickMenu(); IBR_Inst_Project.DeleteSection(Desc); },nullptr });

                })
        ), ImGui::GetMousePos());
    }
    {
        auto Col = Rsec.GetRegTypeColor();
        auto PPos = ImGui::GetWindowPos();
        ImU32 UCol = Col.Value.w < 1e-6 ? ImGui::GetColorU32(ImGuiCol_TitleBg) : (ImU32)Col;
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2{ PPos.x, PPos.y + CurL.y },
            ImVec2{ PPos.x + ImGui::GetWindowWidth(), PPos.y + CurL.y + ImGui::GetTextLineHeightWithSpacing() * 1.1f }, UCol);
        ImGui::SetCursorPos(CurL);
    }

    if (!IsComment)
    {
        if(Ignore)ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_WorkSpace::TempWbg);
        Rsec.SetReOffset(ImGui::GetCursorPos() + ImVec2{ FontHeight * 0.7f, HalfLine });
        ImGui::RadioButton("##MODULE", true);
        if (Ignore)ImGui::PopStyleColor();
        if (ImGui::BeginDragDropSource())
        {
            //
            ImGui::Text((Desc.Ini + " -> " + DisplayName).c_str());
            DrawDragPreviewIcon();
            auto s = Desc.GetText();
            //IBR_HintManager::SetHint(s, 1000);
            ImGui::SetDragDropPayload("IBR_SecDrag", s.c_str(), s.size() + 1);
            IBR_Inst_Project.IBR_SecDragMap[s] = { Desc };
            ImGui::EndDragDropSource();
        }
        ImGui::SameLine();
        if (IBR_WorkSpace::ShowRegName)
        {
            IBD_RInterruptF(x);
            auto Bsec = Rsec.GetBack();
            if (Bsec)ImGui::Text(Bsec->Name.c_str());
            else ImGui::TextColored(IBR_Color::ErrorTextColor, locc("Back_GunMu"));
        }
        else ImGui::Text(DisplayName.c_str());
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            IBR_EditFrame::SetActive(IBR_Inst_Project.IBR_Rev_SectionMap[Desc]);
            IBR_EditFrame::SwitchToText();
            IBR_Inst_Menu.ChooseMenu(MenuItemID_EDIT);
        }
    }
    else
    {
        ImGui::Text(DisplayName.c_str());
    }

    
    ImVec2 HeadLineRN = ImGui::GetLineEndPos() - ImVec2{ FontHeight * 1.5f, HalfLine };


    {
        IBD_RInterruptF(x);
        auto Bsec = Rsec.GetBack();
        if (Bsec == nullptr)
        {
            ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_MissingSectionLink"));
        }
        else if (IsComment)
        {
            if(!CommentEdit)ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_MissingCommentBuffer"));
            else
            {
                if (ImGui::InputTextMultiline("##COMMENT", CommentEdit.get(), MAX_STRING_LENGTH,
                    ImVec2(14.6f * FontHeight, 8.0f * FontHeight),
                    ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_WrappedText))
                {
                    Bsec->Comment = CommentEdit.get();
                }
            }
        }
        else
        {
            if (!Bsec->Inherit.empty())ImGui::Text(locc("GUI_InheritFrom"), Bsec->Inherit.c_str());
            for (const auto& sub : Bsec->SubSecs)
            {
                std::string Last{};
                std::unordered_set<std::string> Used;
                for (const auto& lt : sub.LinkTo)
                {
                    Used.insert(lt.FromKey);
                    auto _F = Bsec->OnShow.find(lt.FromKey);
                    IBB_Section_Desc _Desc = { lt.To.Ini.GetText(),lt.To.Section.GetText() };
                    bool IsLinkingToSelf = (Bsec->GetThisDesc() == _Desc);
                    auto TypeAlt = IBF_Inst_DefaultTypeList.GetDefault(lt.FromKey);
                    ImU32 LineCol = TypeAlt ? TypeAlt->Color : 0;
                    ImU32 LineColII = (IBR_EditFrame::CurSection.ID == Rsec.ID
                        && !ImGui::GetCurrentContext()->OpenPopupStack.Size) ? (ImU32)IBR_Color::FocusLineColor : LineCol;


#define _PB(x) IBR_Inst_Project.LinkList.push_back({((ImVec2)x), _Desc, LineColII, IsLinkingToSelf, Rsec.Dragging()});
                    if (_F != Bsec->OnShow.end() && _F->second.empty())
                    {
                        _PB(HeadLineRN);continue;
                    }
                    else if (Last == lt.FromKey)
                    {
                        auto _G = ImGui::GetLineEndPos() - ImVec2{ FontHeight * 1.5f, HalfLine };
                        _PB(_G);continue;
                    }
                    else
                    {
                        auto _G = ImGui::GetLineEndPos() + ImVec2{ -FontHeight * 1.5f, HalfLine };
                        _PB(_G);
                    }
#undef _PB

                    bool HasInput{ OnLineEdit(lt.FromKey, true) };
                    if (!HasInput)
                    {
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - FontHeight * 2.0f);

                        auto rsc = IBR_Inst_Project.GetSection(IBB_Section_Desc{ lt.To.Ini.GetText(),lt.To.Section.GetText() });

                        ImColor BtnColor{ LineCol };

                        bool Illegal = !rsc.HasBack();
                        bool HasBtnCol = BtnColor.Value.w > 0.0001f;
                        if (Ignore)ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_WorkSpace::TempWbg);
                        else if (Illegal)ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_Color::IllegalLineColor.Value);
                        else if(HasBtnCol)ImGui::PushStyleColor(ImGuiCol_CheckMark, BtnColor.Value);

                        if (ImGui::RadioButton(("##" + lt.FromKey).c_str(), true))
                        {
                            auto sec = IBR_Inst_Project.GetSection(Desc);
                            auto back = sec.GetBack();
                            {
                                IBRF_CoreBump.SendToR({ [back,K = lt.FromKey]()
                                    {
                                        auto Line = back->GetLineFromSubSecs(K);
                                        IBG_Undo.SomethingShouldBeHere();
                                        if (Line && Line->Default->Property.Type == LinkAltPropType && reinterpret_cast<int>(Line->Default->Property.Lim.GetRaw()) == 1)
                                        {
                                            Line->Data->Clear();
                                            IBF_Inst_Project.UpdateAll();
                                        }
                                    } });
                            }
                        }
                        if (!Illegal && ImGui::IsItemHovered())
                        {
                            auto Sec = rsc.GetSectionData();
                            if (Sec)
                            {
                                ImGui::BeginTooltip();
                                auto wd = UTF8toUnicode(Sec->DisplayName);
                                ImGui::Text(UnicodetoUTF8(std::vformat(locw("GUI_Preview_LinkTo"), std::make_wformat_args(wd))).c_str());
                                ImGui::EndTooltip();
                            }
                        }
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                        {
                            TriggeredRightMenu = true;
                            auto sec = IBR_Inst_Project.GetSection(Desc);
                            auto back = sec.GetBack();
                            auto Line = back->GetLineFromSubSecs(lt.FromKey);
                            if (Line && Line->Default->Property.Type == LinkAltPropType && reinterpret_cast<int>(Line->Default->Property.Lim.GetRaw()) == 1)
                            {
                                IBR_PopupManager::SetRightClickMenu(std::move(
                                    IBR_PopupManager::Popup{}.Create(DisplayName + "__LINK__" + lt.FromKey).PushMsgBack([this, Line]() {
                                        if (ImGui::SmallButtonAlignLeft(locc("GUI_Unlink"), ImVec2{FontHeight * 7.0f, ImGui::GetTextLineHeight()}))
                                        {
                                            IBG_Undo.SomethingShouldBeHere();
                                            Line->Data->Clear();
                                            IBF_Inst_Project.UpdateAll();
                                            IBR_PopupManager::ClearRightClickMenu();
                                        }
                                        })), ImGui::GetMousePos());

                            }
                            else if (Line)
                            {
                                auto li = Line->GetData<IBB_IniLine_DataList>();
                                auto& Vals = li->GetValue();
                                std::vector<uint8_t> Res;
                                std::vector<std::string> Dis;
                                Res.resize(Vals.size(), true);
                                Dis.reserve(Vals.size());
                                auto& Reg = IBB_DefaultRegType::GetRegType(Line->Default->Property.TypeAlt);
                                for (auto& v : Vals)
                                {
                                    IBB_Section_Desc ds = { Reg.IniType, v };
                                    if (IBR_Inst_Project.HasSection(ds))Dis.push_back(IBR_Inst_Project.GetSection(ds).GetSectionData()->DisplayName);
                                    else Dis.push_back("");
                                }
                                IBR_PopupManager::SetRightClickMenu(std::move(
                                    IBR_PopupManager::Popup{}.Create(DisplayName + "__LINK__" + lt.FromKey).PushMsgBack([this, li, V = Vals, Res, Dis]() mutable {

                                        for (size_t i = 0; i < V.size(); i++)
                                        {
                                            if (Dis[i].empty())ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_Color::IllegalLineColor.Value);
                                            if (ImGui::RadioButton(("##POP_RIGHT_" + std::to_string(i)).c_str(), Res[i]))
                                            {
                                                if (Res[i])
                                                {
                                                    IBG_Undo.SomethingShouldBeHere();
                                                    li->RemoveValue(V[i]);
                                                }
                                                else
                                                {
                                                    IBG_Undo.SomethingShouldBeHere();
                                                    size_t idx = 0;
                                                    for (size_t j = 0; j < i; j++)if (Res[j])++idx;
                                                    li->InsertValue(V[i], idx);
                                                }
                                                Res[i] ^= 1;
                                                IBF_Inst_Project.UpdateAll();
                                            }
                                            ImGui::SameLine();
                                            if (Dis[i].empty())
                                            {
                                                ImGui::PopStyleColor();
                                                ImGui::Text(V[i].c_str());
                                            }
                                            else ImGui::Text(Dis[i].c_str());
                                        }
                                        if (ImGui::SmallButton(u8"断开所有链接       "))
                                        {
                                            IBG_Undo.SomethingShouldBeHere();
                                            li->Clear();
                                            IBF_Inst_Project.UpdateAll();
                                            IBR_PopupManager::ClearRightClickMenu();
                                        }
                                        }))
                                    , ImGui::GetMousePos());
                            }

                        }
                        if (ImGui::BeginDragDropSource())
                        {
                            ImGui::Text((Desc.Ini + " -> " + DisplayName + " : " + lt.FromKey).c_str());
                            DrawDragPreviewIcon();
                            auto s = Desc.GetText() + " : " + lt.FromKey;
                            ImGui::SetDragDropPayload("IBR_LineDrag", s.c_str(), s.size() + 1);
                            IBR_Inst_Project.IBR_LineDragMap[s] = { Desc, lt.FromKey };
                            ImGui::EndDragDropSource();
                        }
                        if (Ignore || Illegal || HasBtnCol)ImGui::PopStyleColor(1);
                    }

                    //REMOVED 25/01/22
                    /*
                    if (ImGui::ArrowButton((lt.FromKey + u8"_WTF_" + lt.To.Section.GetText()).c_str(), ImGuiDir_Right))
                    {
                        auto dat = rsc.GetSectionData();
                        if (dat != nullptr)
                        {
                            IBR_EditFrame::SetActive(rsc.ID);
                            IBRF_CoreBump.SendToR({ [=]() {IBR_FullView::EqCenter = dat->EqPos + (dat->EqSize / 2.0); } });
                        }
                    }
                    */

                    Last = lt.FromKey;
                }
                for (const auto& [k, l] : sub.Lines)
                {
                    if (Used.count(k) > 0)continue;
                    auto _F = Bsec->OnShow.find(k);
                    if (_F != Bsec->OnShow.end() && _F->second.empty())continue;
                    bool HasInput{ OnLineEdit(k, false) };
                    if (!HasInput)
                    {
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - FontHeight * 2.0f);
                        if (Ignore)ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_WorkSpace::TempWbg);
                        else ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_Color::IllegalLineColor.Value);
                        ImGui::RadioButton(("##" + k).c_str(), true);
                        if (ImGui::BeginDragDropSource())
                        {
                            ImGui::Text((Desc.Ini + " -> " + DisplayName + " : " + k).c_str());
                            DrawDragPreviewIcon();
                            auto s = RandStr(16);
                            ImGui::SetDragDropPayload("IBR_LineDrag", s.c_str(), 17);
                            IBR_Inst_Project.IBR_LineDragMap[s] = { Desc, k };
                            ImGui::EndDragDropSource();
                        }
                        ImGui::PopStyleColor(1);
                    }
                }
            }
            for (const auto& [k, l] : Bsec->UnknownLines.Value)
            {
                auto _F = Bsec->OnShow.find(k);
                if (_F == Bsec->OnShow.end() || _F->second.empty())continue;
                auto& Line = ActiveLines[k];
                auto& V = Bsec->UnknownLines.Value[k];
                Line.Buffer = V;
                auto K2 = k + " = " + V;
                if (V == "yes" || V == "no" || V == "true" || V == "false")
                {
                    bool X = (V == "yes" || V == "true");
                    if (ImGui::Checkbox((k).c_str(), &X))
                    {
                        IBG_Undo.SomethingShouldBeHere();
                        V = X ? "yes" : "no";
                        if (IBR_Inst_Project.IBR_Rev_SectionMap[Desc] == IBR_EditFrame::CurSection.ID)
                        {
                            IBR_EditFrame::EditLines[k].Buffer = V;
                        }
                    }
                }
                else
                {
                    /*
                    if (Line.Edit.NeedInit())
                    {
                        IBR_IniLine::InitType It{ l ,"##" + RandStr(8), [desc = Desc,Bsec,Str = k](char* S)
                            {
                                IBG_Undo.SomethingShouldBeHere();
                                Bsec->UnknownLines.Value[Str] = S;
                                if (IBR_Inst_Project.IBR_Rev_SectionMap[desc] == IBR_EditFrame::CurSection.ID)
                                {
                                    IBR_EditFrame::EditLines[Str].Buffer = S;
                                }
                            } };
                        Line.Edit.RenderUI(k, "", &It);
                    }
                    else
                    {
                        Line.Edit.RenderUI(Line.Edit.HasInput ? k : K2, "");
                    }
                    */
                    if (!Line.Edit.Input)
                    {
                        Line.Edit.Input.reset(new IBR_InputManager(l, "##" + RandStr(8), [desc = Desc, Bsec, Str = k](char* S)
                            {
                                IBG_Undo.SomethingShouldBeHere();
                                Bsec->UnknownLines.Value[Str] = S;
                                if (IBR_Inst_Project.IBR_Rev_SectionMap[desc] == IBR_EditFrame::CurSection.ID)
                                {
                                    IBR_EditFrame::EditLines[Str].Buffer = S;
                                }
                            }));
                    }
                    ImGui::TextEx(k.c_str());
                    ImGui::SameLine();
                    Line.Edit.Input->RenderUI();
                }
            }
        }
    }
    //TODO

    FinalY = ImGui::GetCursorPosY() - FinalY;
}
