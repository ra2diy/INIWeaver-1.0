#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include<imgui_internal.h>
#include <ranges>

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



IBB_Section* IBR_SectionData::GetBack_Inl()
{
    if (!BackPtr_Cached)
    {
        BackPtr_Cached = IBR_Inst_Project.GetSection(Desc).GetBack();
    }
    return BackPtr_Cached;
}

bool IBR_SectionData::IsVirtualBlock() const
{
    return !IncludingModules.empty();
}




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
                if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
                if (Str != DisplayName && IBF_Inst_Project.HasDisplayName(Str))
                {
                    ImGui::TextDisabled(locc("GUI_OK"));
                    ImGui::SameLine();
                    ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_ModuleRename_Error1"));
                }
                else if (!strlen(Str))
                {
                    ImGui::TextDisabled(locc("GUI_OK"));
                    ImGui::SameLine();
                    ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_ModuleRename_Error2"));
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

void IBR_SectionData::RenameRegister()
{
    const size_t BufSize = 1000;
    char* Str = new char[BufSize] {};
    strcpy(Str, Desc.Sec.c_str());
    bool DoRenameDisplay = true;
    IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}.CreateModal(loc("GUI_RegRename"), true, [this, Str]
        {
            delete[]Str;
        })
        .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize).PushMsgBack([this, Str, DoRenameDisplay]() mutable
            {
                ImGui::SetWindowSize({ FontHeight * 20.0f,FontHeight * 10.0f });
                ImGui::Text(locc("GUI_EnterNewRegName"));
                ImGui::InputText("", Str, BufSize);
                if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
                ImGui::Checkbox(locc("GUI_RenameModuleTogether"), &DoRenameDisplay);
                if (Str != Desc.Sec && IBF_Inst_Project.Project.GetSec({Desc.Ini, Str}))
                {
                    ImGui::TextDisabled(locc("GUI_OK"));
                    ImGui::SameLine();
                    ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_RegRename_Error1"));
                }
                else if (!strlen(Str))
                {
                    ImGui::TextDisabled(locc("GUI_OK"));
                    ImGui::SameLine();
                    ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_RegRename_Error2"));
                }
                else if (DoRenameDisplay && Str != DisplayName && IBF_Inst_Project.HasDisplayName(Str))
                {
                    ImGui::TextDisabled(locc("GUI_OK"));
                    ImGui::SameLine();
                    ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_ModuleRename_Error1"));
                }
                else
                {
                    if (ImGui::Button(locc("GUI_OK")))
                    {
                        IBG_Undo.SomethingShouldBeHere();
                        if (DoRenameDisplay)RenameDisplayImpl(Str);
                        RenameRegisterImpl(Str);
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
    else DisplayName = Name;
    IBF_Inst_Project.DisplayNames[DisplayName] = Desc;
    IBG_Undo.SomethingShouldBeHere();
}

void IBR_SectionData::RenameRegisterImpl(const std::string& Name)
{
    auto RSec = IBR_Inst_Project.GetSection(Desc);
    if (!RSec.Rename(Name))
        IBR_HintManager::SetHint(locc("GUI_RegRenameFailed"), HintStayTimeMillis);
    BackPtr_Cached = nullptr;
}

bool IBR_SectionData::OnLineEdit(const std::string& Name, bool OnLink)
{

    auto& Line = ActiveLines[Name];
    bool HasInput{ false };
    if (Line.Edit.Input)HasInput = true;
    auto back = GetBack_Inl();
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
            "##" + RandStr(8),[Name, desc = this->Desc, line](const std::string& S)
                {
                    IBG_Undo.SomethingShouldBeHere();
                    line->Data->SetValue(S);
                    if (IBR_Inst_Project.IBR_Rev_SectionMap[desc] == IBR_EditFrame::CurSection.ID)
                    {
                        IBR_EditFrame::EditLines[Name].Buffer = S;
                    }
                }, line->Default->Input->WorkSpace };
        if (Name == InheritKeyName)
        {
            auto Q = line->Data->GetString();
            auto w = IBR_WorkSpace::ShowRegName ? UTF8toUnicode(Q) :
                (IBR_Inst_Project.HasSection({ Desc.Ini, Q }) ? UTF8toUnicode(IBR_Inst_Project.GetSection({ Desc.Ini, Q }).GetDisplayName()) : UTF8toUnicode(Q));
            std::wstring Nul;
            if(w.empty())Line.Edit.RenderUI(locc("GUI_NoInherit"), line->Default->DescLong, &It);
            else Line.Edit.RenderUI(UnicodetoUTF8(std::vformat(locw("GUI_InheritFrom"), std::make_wformat_args(Line.Edit.HasInput ? Nul : w))), line->Default->DescLong, &It);
        }
        else if (!IBR_WorkSpace::ShowRegName)Line.Edit.RenderUI(line->Default->DescShort, line->Default->DescLong, &It);
        else Line.Edit.RenderUI(Name, line->Default->DescLong, &It);
    }
    else
    {
        if (Name == InheritKeyName)
        {
            auto Q = line->Data->GetString();
            auto w = IBR_WorkSpace::ShowRegName ? UTF8toUnicode(Q) :
                (IBR_Inst_Project.HasSection({ Desc.Ini, Q }) ? UTF8toUnicode(IBR_Inst_Project.GetSection({ Desc.Ini, Q }).GetDisplayName()) : UTF8toUnicode(Q));
            std::wstring Nul;
            if (w.empty())Line.Edit.RenderUI(locc("GUI_NoInherit"), line->Default->DescLong);
            else Line.Edit.RenderUI(UnicodetoUTF8(std::vformat(locw("GUI_InheritFrom"), std::make_wformat_args(Line.Edit.HasInput ? Nul : w))), line->Default->DescLong);
        }
        else if (!IBR_WorkSpace::ShowRegName)Line.Edit.RenderUI(line->Default->DescShort, line->Default->DescLong);
        else Line.Edit.RenderUI(Name, line->Default->DescLong);
    }
    return line->Default->Property.TypeAlt.empty() ? true : HasInput;
}

void IBR_SectionData::CopyToClipBoard()
{
    IBB_ClipBoardData ClipData;
    int Copied;

    if (IsVirtualBlock())
    {
        auto IDs = IncludingModules;
        if(auto ido = IBR_Inst_Project.GetSectionID(Desc); ido)
            IDs.push_back(*ido);
        Copied = IDs.size();
        IBR_WorkSpace::GenerateClipDataFromIDs(ClipData, IDs);
    }
    else
    {
        std::vector<IBB_Section_Desc> Sel{ Desc };
        EqDelta = { 0.0f,0.0f };
        ClipData.Generate(Sel);
        Copied = 1;
    }

    ImGui::SetClipboardText(ClipData.GetString().c_str());
    IBR_HintManager::SetHint(UnicodetoUTF8(std::vformat(locw("GUI_CopySuccess"), std::make_wformat_args(Copied))), HintStayTimeMillis);
}

bool IBR_SectionData::Decomposable() const
{
    return !IncludingModules.empty();
}

void IBR_SectionData::Decompose()
{
    if (!Decomposable())return;
    IBRF_CoreBump.SendToR({ [Desc = this->Desc]()
    {
        auto Res = IBR_Inst_Project.DecomposeSection(Desc);
        if (Res)
        {
            IBR_WorkSpace::MassSelect(*Res);
            auto c = Res->size();
            IBR_HintManager::SetHint(UnicodetoUTF8(std::vformat(locw("GUI_DecomposeSuccess"), std::make_wformat_args(c))), HintStayTimeMillis);
        }
        else
        {
            IBR_HintManager::SetHint(locc("GUI_DecomposeFailed"), HintStayTimeMillis);
        }
    } });
}

bool IBR_SectionData::IsIncluded() const
{
    return IncludedByModule != INVALID_MODULE_ID && IBR_Inst_Project.HasSection(IncludedByModule);
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

void IBR_SectionData::RenderUI_TitleBar(bool &TriggeredRightMenu, float LastFinalY)
{
    auto Rsec = IBR_Inst_Project.GetSection(Desc);
    auto HalfLine = ImGui::GetTextLineHeightWithSpacing() * 0.5F;
    auto Pos = ImGui::GetCursorPos();
    auto Virtual = IsVirtualBlock();
    auto Included = IsIncluded();
    ImGui::SetCursorPos({ 0.0f, FinalY });

    ImGui::Dummy({ ImGui::GetWindowWidth(), Included ? LastFinalY : ImGui::GetWindowHeight()});
    if (!Virtual && ImGui::BeginDragDropTarget())
    {
        auto payload = ImGui::AcceptDragDropPayload("IBR_SecDrag", ImGuiDragDropFlags_AcceptBeforeDelivery);
        if (payload)
        {
            if (payload->IsPreview() || payload->IsDelivery())
            {
                std::string s = (char*)payload->Data;
                const auto& desc = IBR_Inst_Project.IBR_SecDragMap[s].Desc;
                auto back = GetBack_Inl();
                auto srcsec = IBR_Inst_Project.GetSection(desc);
                auto srcback = srcsec.GetBack();

                if (back && srcback)
                {
                    if (srcback->DefaultLinkKey.HasValue(back->Register))
                    {
                        auto& Val = srcback->DefaultLinkKey.GetVariable(back->Register);
                        if (payload->IsPreview())
                            IBR_Inst_Project.DragConditionText = Desc.Ini + " -> " + DisplayName + " : " + Val;
                        if (payload->IsDelivery())
                        {
                            IBG_Undo.SomethingShouldBeHere();
                            IBRF_CoreBump.SendToF([=]{
                                    back->MergeLine(Val, desc.Sec, IBB_IniMergeMode::Merge);
                                    IBF_Inst_Project.UpdateAll();
                                });
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
                    IBR_Inst_Project.IBR_SecDragMap.erase(s);
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
                auto tgback = GetBack_Inl();
                auto Line = back->GetLineFromSubSecs(lin.Line);

                if (back && tgback)
                {
                    bool Check = true;
                    if (!back->Register.empty())
                    {
                        auto& alt = Line->Default->Property.TypeAlt;
                        if (!alt.empty())
                        {
                            if (alt == "_MyType")Check = IBB_DefaultRegType::MatchType(back->Register, tgback->Register);
                            else if (alt == "_AnyType")Check = true;
                            else Check = IBB_DefaultRegType::MatchType(alt, tgback->Register);

                        }
                    }
                    if (Check)
                    {
                        if (payload->IsPreview())
                            IBR_Inst_Project.DragConditionText = Desc.Ini + " -> " + DisplayName;
                        if (payload->IsDelivery())
                        {
                            IBG_Undo.SomethingShouldBeHere();
                            IBRF_CoreBump.SendToF([=] {
                                back->MergeLine(lin.Line, Desc.Sec, IBB_IniMergeMode::Merge);
                                IBF_Inst_Project.UpdateAll();
                                });
                        }
                    }
                    else if (payload->IsPreview())
                    {
                        IBR_Inst_Project.DragConditionText = "HOLY_SHIT\nWHAT_HAD_JUST_HAPPENED\nONE_MINUTE_AGO";
                        auto& alt = Line->Default->Property.TypeAlt;
                        std::wstring W1;
                        if (alt == "_MyType")W1 = UTF8toUnicode(back->Register);
                        else if (alt == "_AnyType")W1 = UTF8toUnicode(back->Register);
                        else W1 = UTF8toUnicode(alt);
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
        auto id = IBR_Inst_Project.GetSectionID(Desc);
        if(id)IBR_EditFrame::SetActive(id.value());
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
                    //DO NOT CHANGE THE REGISTER NAME OF COMMENT BLOCK
                    //OK BUT UNNECESSARY
                    /*
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_RegRename"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        RenameRegister();
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                    */
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_Copy"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        CopyToClipBoard();
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_Delete"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                        IBRF_CoreBump.SendToR({ [this]()
                        {IBR_PopupManager::ClearRightClickMenu(); IBR_Inst_Project.DeleteSection(Desc); },nullptr });

                    })
            ), ImGui::GetMousePos());
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
                if (ImGui::SmallButtonAlignLeft(locc("GUI_RegRename"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                {
                    RenameRegister();
                    IBR_PopupManager::ClearRightClickMenu();
                }
                if (ImGui::SmallButtonAlignLeft(locc("GUI_Copy"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                {
                    CopyToClipBoard();
                    IBR_PopupManager::ClearRightClickMenu();
                }
                if (Decomposable())
                {
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_Decompose"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        Decompose();
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                }
                if (ImGui::SmallButtonAlignLeft(locc("GUI_EditText"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                {
                    IBR_EditFrame::ActivateAndEdit(IBR_Inst_Project.IBR_Rev_SectionMap[Desc], true);
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

    if (!IsComment && !Virtual)
    {
        if (Ignore)ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_WorkSpace::TempWbg);
        Rsec.SetReOffset(ImVec2{ FontHeight * 0.7f, HalfLine });
        ImGui::RadioButton(("##MODULE" + ModuleStrID).c_str(), true, GlobalNodeStyle);
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
            else
            {
                ImGui::TextColored(IBR_Color::ErrorTextColor, locc("Back_GunMu"));
                BackPtr_Cached = nullptr;
            }
        }
        else ImGui::Text(DisplayName.c_str());

        auto UPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos({ 0.0f,CurL.y });
        ImGui::Dummy({ ImGui::GetWindowWidth(), ImGui::GetTextLineHeightWithSpacing() });
        ImGui::SetCursorPos(UPos);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            IBR_EditFrame::ActivateAndEdit(IBR_Inst_Project.IBR_Rev_SectionMap[Desc], true);
        }
    }
    else
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FontHeight * 1.5F);
        ImGui::Text(DisplayName.c_str());
    }
}

extern bool EnableDebugList;

void IBR_SectionData::RenderUI()
{
    if (!First && Hidden)
    {
        FinalY = 0;
        return;
    }

    ReWindowUL = ImGui::GetCursorScreenPos();
    auto HalfLine = ImGui::GetTextLineHeightWithSpacing() * 0.5F;
    auto LastFinalY = FinalY;
    FinalY = ImGui::GetCursorPosY();
    WidthFix = 0.0f;
    auto Rsec = IBR_Inst_Project.GetSection(Desc);
    bool TriggeredRightMenu = false;
    bool Included = IsIncluded();
    
    RenderUI_TitleBar(TriggeredRightMenu, LastFinalY);
    
    ImVec2 HeadLineRN = ImGui::GetLineEndPos() - ImVec2{ FontHeight * 1.5f, HalfLine };
    {
        IBD_RInterruptF(x);
        auto Bsec = GetBack_Inl();
        if (Bsec == nullptr)
        {
            ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_MissingSectionLink"));
        }
        else if (IsComment)
        {
            if(!CommentEdit)ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_MissingCommentBuffer"));
            else
            {
                auto TSize = ImGui::CalcTextSize(CommentEdit.get());
                TSize.x = std::max(TSize.x + FontHeight * 1.2f , FontHeight * 14.6f);
                TSize.y = std::max(TSize.y + FontHeight, FontHeight * 8.0f);
                //IBR_HintManager::SetHint(std::to_string(TSize.x) + ","+ std::to_string(TSize.y),HintStayTimeMillis);
                if (ImGui::InputTextMultiline(("##COMMENT" + ModuleStrID).c_str(), CommentEdit.get(), MAX_STRING_LENGTH, 
                    TSize,
                    ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_WrappedText))
                {
                    Bsec->Comment = CommentEdit.get();
                }
                if(ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
                if (TSize.x > FontHeight * 14.6f)WidthFix = TSize.x;
            }
        }
        else if (Included && CollapsedInComposed && !First)
        {
            for (auto i : Bsec->SubSecOrder)
            {
                const auto& sub = Bsec->SubSecs[i];
                for (const auto& lt : sub.LinkTo)
                {
                    IBB_Section_Desc _Desc = lt.To;
                    auto TypeAlt = IBF_Inst_DefaultTypeList.GetDefault(lt.FromKey);
                    ImU32 LineCol = TypeAlt ? TypeAlt->Color : 0;
                    ImU32 LineColII = (IBR_EditFrame::CurSection.ID == Rsec.ID
                        && !ImGui::GetCurrentContext()->OpenPopupStack.Size) ? (ImU32)IBR_Color::FocusLineColor : LineCol;
                    bool IsLinkingToSelf = (Bsec->GetThisDesc() == _Desc);
                    IBR_Inst_Project.LinkList.push_back({ HeadLineRN, _Desc, LineColII, IsLinkingToSelf, Rsec.Dragging() });
                }
            }
            auto FrameBgCol = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
            ImGui::SetCursorPos({ ImGui::GetWindowWidth() - FontHeight * 4.0f, FinalY - FontHeight * 0.15f });
            if (ImGui::SmallButton(("+##ExpandBtn" + ModuleStrID).c_str(), {FontHeight * 1.5f,FontHeight * 1.2f}))
            {
                CollapsedInComposed = false;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(locc("GUI_UnfoldModule"));
                ImGui::EndTooltip();
            }
            
            //CANCEL THE CHECKBOX APPEARANCE
            // 2025/09/23
            //ImGui::SetCursorPos({ ImGui::GetWindowWidth() - FontHeight * 2.0f, FinalY - FontHeight * 0.15f });
            //ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_WorkSpace::TempWbg);
            //ImGui::PushStyleColor(ImGuiCol_FrameBgActive, FrameBgCol);
            //ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, FrameBgCol);
            //ImGui::RadioButton(("##" + ModuleStrID).c_str(), true, GlobalNodeStyle);
            //ImGui::PopStyleColor(3);
        }
        else
        {
            if (IsIncluded())
            {
                ImGui::SetCursorPos({ ImGui::GetWindowWidth() - FontHeight * 4.0f, FinalY - FontHeight * 0.15f });
                if (ImGui::SmallButton(("-##ExpandBtn" + ModuleStrID).c_str(), { FontHeight * 1.5f,FontHeight * 1.2f }))
                {
                    CollapsedInComposed = true;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text(locc("GUI_FoldModule"));
                    ImGui::EndTooltip();
                }
            }
            
            //GlobalLog.AddLog(( Desc.GetText() + ":" + DisplayName).c_str());
            if (Bsec->SubSecOrder.size() != Bsec->SubSecs.size())
            {
                Bsec->SubSecOrder.clear();
                for (size_t i = 0; i < Bsec->SubSecs.size(); i++)Bsec->SubSecOrder.push_back(i);
                std::ranges::sort(Bsec->SubSecOrder, [&](size_t a, size_t b) {
                        return Bsec->SubSecs[a].Default->Name < Bsec->SubSecs[b].Default->Name;
                    });
            }
            for (auto i : Bsec->SubSecOrder)
            {
                auto& sub = Bsec->SubSecs[i];
                std::string Last{};
                std::unordered_set<std::string> Used;
                bool IsInheritSubSec = (sub.Default->Name == "  INHERIT_SUBSEC__");

                if (IsInheritSubSec && Bsec->Inherit.empty())
                    continue;

                auto BLinkCmp = [](const IBB_Link& a, const IBB_Link& b) { return a.FromKey < b.FromKey; };
                if (!std::ranges::is_sorted(sub.LinkTo, BLinkCmp))
                    std::ranges::sort(sub.LinkTo, BLinkCmp);

                for (const auto& lt : sub.LinkTo)
                {
                    Used.insert(lt.FromKey);
                    auto _F = Bsec->OnShow.find(lt.FromKey);
                    IBB_Section_Desc _Desc = lt.To;
                    bool IsLinkingToSelf = (Bsec->GetThisDesc() == _Desc);
                    auto TypeAlt = IBF_Inst_DefaultTypeList.GetDefault(lt.FromKey);
                    ImU32 LineCol = TypeAlt ? TypeAlt->Color : 0;
                    ImU32 LineColII = (IBR_EditFrame::CurSection.ID == Rsec.ID
                        && !ImGui::GetCurrentContext()->OpenPopupStack.Size) ? (ImU32)IBR_Color::FocusLineColor : LineCol;

                    float _w = FinalY + ImGui::GetWindowPos().y + HalfLine;

#define _PB IBR_Inst_Project.LinkList.push_back({(ImVec2{(float)_G.x, IsInheritSubSec?_w:(float)_G.y}), _Desc, LineColII, IsLinkingToSelf, Rsec.Dragging()});
                    if (_F != Bsec->OnShow.end() && _F->second.empty())
                    {
                        auto _G = HeadLineRN;
                        _PB;continue;
                    }
                    else if (Last == lt.FromKey)
                    {
                        auto _G = ImGui::GetLineEndPos() - ImVec2{ FontHeight * 1.5f, HalfLine };
                        _PB;continue;
                    }
                    else
                    {
                        auto _G = ImGui::GetLineEndPos() + ImVec2{ -FontHeight * 1.5f, HalfLine };
                        _PB;
                    }
#undef _PB

                    bool HasInput{ OnLineEdit(lt.FromKey, true) };
                    if (!HasInput)
                    {
                        auto NewLineCursor = ImGui::GetCursorPos();
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - FontHeight * 2.0f);

                        auto rsc = IBR_Inst_Project.GetSection(lt.To);

                        ImColor BtnColor{ LineCol };
                        ImU32 BtnColorW = (LineCol >> IM_COL32_A_SHIFT) & 0xFF;

                        bool Illegal = !rsc.HasBack();
                        bool HasBtnCol = BtnColorW > 0;
                        if (Ignore)ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_WorkSpace::TempWbg);
                        else if (Illegal)
                        {
                            if (IsInheritSubSec) ImGui::PushStyleColor(ImGuiCol_CheckMark, BtnColor.Value);
                            else ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_Color::IllegalLineColor.Value);
                        }
                        else if(HasBtnCol)ImGui::PushStyleColor(ImGuiCol_CheckMark, BtnColor.Value);

                        if (IsInheritSubSec)ImGui::SetCursorPosY(FinalY);
                        bool _K = ImGui::RadioButton(("##" + lt.FromKey + "\n" + ModuleStrID).c_str(), true,
                            IsInheritSubSec ? ImGuiRadioButtonFlags_RoundedSquare : GlobalNodeStyle );
                        if (_K)
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
                                            if (ImGui::RadioButton(("##POP_RIGHT_" + std::to_string(i)).c_str(), Res[i], GlobalNodeStyle))
                                            {
                                                if (Res[i])
                                                {
                                                    IBG_Undo.SomethingShouldBeHere();
                                                    size_t idx = 0;
                                                    for (size_t j = 0; j < i; j++)if (Res[j])++idx;
                                                    li->RemoveValue(idx);
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
                                            else if(IBR_WorkSpace::ShowRegName)
                                                ImGui::Text(V[i].c_str());
                                            else
                                                ImGui::Text(Dis[i].c_str());
                                        }
                                        if (ImGui::SmallButtonAlignLeft(locc("GUI_UnlinkAll"), ImVec2{ FontHeight * 8.0f, ImGui::GetTextLineHeight() }))
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
                            if (lt.FromKey == InheritKeyName)
                            {
                                auto w = UTF8toUnicode((Desc.Ini + " -> " + DisplayName));
                                ImGui::Text(UnicodetoUTF8(std::vformat(locw("GUI_InheritTo"), std::make_wformat_args(w))).c_str());
                            }
                            else ImGui::Text((Desc.Ini + " -> " + DisplayName + " : " + lt.FromKey).c_str());
                            DrawDragPreviewIcon();
                            auto s = Desc.GetText() + " : " + lt.FromKey;
                            ImGui::SetDragDropPayload("IBR_LineDrag", s.c_str(), s.size() + 1);
                            IBR_Inst_Project.IBR_LineDragMap[s] = { Desc, lt.FromKey };
                            ImGui::EndDragDropSource();
                        }
                        if (Ignore || Illegal || HasBtnCol)ImGui::PopStyleColor(1);

                        ImGui::SetCursorPos(NewLineCursor);
                    }
                    Last = lt.FromKey;
                }
                for (const auto& k : sub.Lines_ByName)
                {
                    //const auto& l = sub.Lines.at(k);
                    if (Used.count(k) > 0)continue;
                    auto _F = Bsec->OnShow.find(k);
                    if (_F != Bsec->OnShow.end() && _F->second.empty())continue;
                    bool HasInput{ OnLineEdit(k, false) };
                    if (!HasInput)
                    {
                        auto NewLineCursor = ImGui::GetCursorPos();
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - FontHeight * 2.0f);
                        if (Ignore)ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_WorkSpace::TempWbg);
                        else
                        {
                            if (IsInheritSubSec) ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_Color::FocusLineColor.Value);
                            else ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_Color::IllegalLineColor.Value);
                        }
                        if (IsInheritSubSec)ImGui::SetCursorPosY(FinalY);
                        ImGui::RadioButton(("##" + k + "\n" + ModuleStrID).c_str(), true,
                            IsInheritSubSec ? ImGuiRadioButtonFlags_RoundedSquare : GlobalNodeStyle);
                        if (ImGui::BeginDragDropSource())
                        {
                            if (k == InheritKeyName)
                            {
                                auto w = UTF8toUnicode((Desc.Ini + " -> " + DisplayName));
                                ImGui::Text(UnicodetoUTF8(std::vformat(locw("GUI_InheritTo"), std::make_wformat_args(w))).c_str());
                            }
                            else ImGui::Text((Desc.Ini + " -> " + DisplayName + " : " + k).c_str());
                            DrawDragPreviewIcon();
                            auto s = RandStr(16);
                            ImGui::SetDragDropPayload("IBR_LineDrag", s.c_str(), 17);
                            IBR_Inst_Project.IBR_LineDragMap[s] = { Desc, k };
                            ImGui::EndDragDropSource();
                        }
                        ImGui::PopStyleColor(1);
                        ImGui::SetCursorPos(NewLineCursor);
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
                //auto K2 = k + " = " + V;
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
                            auto& ed = IBR_EditFrame::EditLines[k].Edit;
                            if (ed.Input && ed.Input->Form)
                                ed.Input->Form->ParseFromString(V);
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
                        Line.Edit.Input.reset(new IBR_InputManager(l, "##" + RandStr(8), [desc = Desc, Bsec, Str = k](const std::string& S)
                            {
                                IBG_Undo.SomethingShouldBeHere();
                                Bsec->UnknownLines.Value[Str] = S;
                                if (IBR_Inst_Project.IBR_Rev_SectionMap[desc] == IBR_EditFrame::CurSection.ID)
                                {
                                    auto& ed = IBR_EditFrame::EditLines[Str].Edit;
                                    if (ed.Input && ed.Input->Form)
                                        ed.Input->Form->ParseFromString(S);
                                }
                            },
                            IBB_DefaultRegType::GetDefaultInputType().WorkSpace->Duplicate()));
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
    

    if (IsVirtualBlock())
    {
        int HiddenCount = 0;
        FinalY += std::ranges::fold_left(IncludingModules, 0.0F, [&HiddenCount](float f, auto id) {
            if (auto Data = IBR_Inst_Project.GetSectionFromID(id).GetSectionData(); Data)
            {
                auto PosUL = ImGui::GetCursorScreenPos();
                PosUL.x -= ImGui::GetCurrentContext()->Style.ItemSpacing.x;
                PosUL.y -= ImGui::GetCurrentContext()->Style.ItemSpacing.y;
                Data->RenderUI();
                if (Data->First)Data->First = false;
                auto PosDR = ImVec2{ PosUL.x + ImGui::GetWindowWidth(), PosUL.y + Data->FinalY };
                ImRect rc{ PosUL,PosDR };
                if (Data->Hidden)++HiddenCount;
                //For debug
                /*
                if (rc.Contains(ImGui::GetMousePos()))
                    ImGui::GetForegroundDrawList()->AddRectFilled(rc.Min, rc.Max,
                        ImGui::GetColorU32(IBR_WorkSpace::CurOnRender_Clicked ? ImGuiCol_ButtonActive : ImGuiCol_Border, 0.7F), 0.0F, 0);
                else
                    ImGui::GetForegroundDrawList()->AddRect(rc.Min, rc.Max, ImGui::
                        GetColorU32(ImGuiCol_Border, 0.7F), 0.0F, 0, 3.0F);
                */
                if (Data->Frozen && !Data->Hidden)
                {
                    auto FL = ImGui::GetWindowDrawList();
                    FL->AddRectFilled(PosUL, PosDR, IBR_Color::FrozenMaskColor, 5.0F);
                }
                if (!ImGui::GetCurrentContext()->OpenPopupStack.Size)
                {
                    if (id == IBR_EditFrame::CurSection.ID)
                    {
                        auto FL = ImGui::GetForegroundDrawList();
                        FL->PushClipRect(IBR_RealCenter::WorkSpaceUL, IBR_RealCenter::WorkSpaceDR, true);
                        //FL->AddRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), IBR_Color::FocusWindowColor, 5.0F, 0, 5.0F);
                        FL->AddRect(PosUL, PosDR, IBR_Color::FocusWindowColor, 5.0F, 0, 5.0F);
                        FL->PopClipRect();
                    }
                }
                
                if (IBR_WorkSpace::CurOnRender_Clicked && rc.Contains(ImGui::GetMousePos()))
                {
                    IBR_EditFrame::ActivateAndEdit(id, false);
                    IBR_HintManager::SetHint(Data->DisplayName, HintStayTimeMillis);
                }
                return f + Data->FinalY;
            }
            else return f;
            });

        if (HiddenCount > 0)
        {
            auto BtnText = UnicodetoUTF8(std::vformat(locw("GUI_ShowAllIncludingBlocks"), std::make_wformat_args(HiddenCount)));
            if (ImGui::SmallButton(BtnText.c_str()))
            {
                std::ranges::for_each(IncludingModules, [](auto id) {
                    if (auto Data = IBR_Inst_Project.GetSectionFromID(id).GetSectionData(); Data)Data->Hidden = false;
                });
            }
        }
    }
    else if(IBR_WorkSpace::CurOnRender_Clicked && !Included)
    {
        IBR_EditFrame::ActivateAndEdit(IBR_WorkSpace::CurOnRender_ID, false);
    }
}
