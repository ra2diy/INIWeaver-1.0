#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include "IBR_Components.h"
#include "IBR_LinkNode.h"
#include "IBR_Combo.h"
#include <imgui_internal.h>
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

    void Dummy(const ImVec2& size, bool AffectsLayout)
    {
        if (!AffectsLayout)return Dummy(size);
        else
        {
            auto w = GetCurrentWindow();
            auto mx = w->DC.CursorMaxPos;
            Dummy(size);
            w->DC.CursorMaxPos = mx;
        }
    }
}

extern wchar_t CurrentDirW[];
extern const char* DefaultAltPropType;
extern const char* LinkAltPropType;
extern int RFontHeight;
extern bool EnableDebugList;

void DrawDragPreviewIcon_LinkLim0()
{
    if (ImGui::IsDragDropPayloadBeingAccepted())
    {
        auto P = ImGui::GetCursorScreenPos();
        DrawCross(P, float(RFontHeight), IBR_Color::ErrorTextColor);
        ImGui::Dummy(ImVec2{ float(RFontHeight),float(RFontHeight) });
        ImGui::SameLine();
        ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_Preview_InvalidLink"));
    }
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
        if (T.empty())ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_Preview_InvalidLink"));
        else if (T != "HOLY_SHIT\nWHAT_HAD_JUST_HAPPENED\nONE_MINUTE_AGO")
        {
            auto T2 = UTF8toUnicode(T);
            ImGui::Text(UnicodetoUTF8(std::vformat(locw("GUI_Preview_LinkTo"), std::make_wformat_args(T2))).c_str());
        }
        else ImGui::Text(IBR_Inst_Project.DragConditionTextAlt.c_str());
    }
    //else ImGui::Text(u8"NOT ACCEPTED");
}

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

IBB_ClipBoardData IBR_SectionData::GetClipBoardData(int& Copied)
{
    IBB_ClipBoardData ClipData;
    if (IsVirtualBlock())
    {
        auto IDs = IncludingModules;
        if (auto ido = IBR_Inst_Project.GetSectionID(Desc); ido)
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

    return ClipData;
}

void IBR_SectionData::CopyToClipBoard()
{
    int Copied;
    auto ClipData = GetClipBoardData(Copied);

    ImGui::SetClipboardText(ClipData.GetString().c_str());
    IBR_HintManager::SetHint(UnicodetoUTF8(std::vformat(locw("GUI_CopySuccess"), std::make_wformat_args(Copied))), HintStayTimeMillis);
}

void IBR_SectionData::CutToClipBoard()
{
    int Copied;
    auto ClipData = GetClipBoardData(Copied);

    IBRF_CoreBump.SendToR({ [desc = Desc]() { IBG_Undo.SomethingShouldBeHere(); IBR_Inst_Project.DeleteSection(desc); } });

    ImGui::SetClipboardText(ClipData.GetString().c_str());
    IBR_HintManager::SetHint(UnicodetoUTF8(std::vformat(locw("GUI_CopySuccess"), std::make_wformat_args(Copied))), HintStayTimeMillis);
}

void IBR_SectionData::Duplicate()
{
    int Copied;
    auto ClipData = GetClipBoardData(Copied);
    ClipData.Mangle(true);

    auto [Success, X] = IBR_Inst_Project.AddModule(ClipData.Modules);
    if (Success)
    {
        IBR_HintManager::SetHint(UnicodetoUTF8(std::vformat(locw("GUI_DuplicateSuccess"), std::make_wformat_args(Copied))), HintStayTimeMillis);
        IBR_WorkSpace::MassSelect(X);
    }
    else IBR_HintManager::SetHint(loc("GUI_DuplicateFailed"), HintStayTimeMillis);
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

bool IBR_SectionData::IsComposedAllFold() const
{
    return std::ranges::all_of(IncludingModules, [this](auto id) {
        auto pData = IBR_Inst_Project.GetSectionFromID(id).GetSectionData();
        if (pData) return pData->CollapsedInComposed;
        else return false;
    });
}

void IBR_SectionData::FoldComposed()
{
    std::ranges::for_each(IncludingModules, [this](auto id) {
        auto pData = IBR_Inst_Project.GetSectionFromID(id).GetSectionData();
        if (pData) pData->CollapsedInComposed = true;
    });
}

void IBR_SectionData::UnfoldComposed()
{
    std::ranges::for_each(IncludingModules, [this](auto id) {
        auto pData = IBR_Inst_Project.GetSectionFromID(id).GetSectionData();
        if (pData) pData->CollapsedInComposed = false;
    });
}

// ---------- RENDER UI ----------

bool IBR_SectionData::RenderUI_Line(const std::string& OnShow, StrPoolID Name)
{
    if (OnShow.empty())return true;

    auto back = GetBack_Inl();
    auto [line, idx] = back->GetLineFromSubSecsEx(Name);
    if (!line)
    {
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            auto W = UTF8toUnicode(Desc.GetText() + u8" : " + PoolStr(Name));
            GlobalLog.AddLog(std::vformat(L"IBR_SectionData::RenderUI_KnownLine ： " + locw("Log_INILineNotExist"),
                std::make_wformat_args(W)));
        }
        return true;
    }
    LinkNodeContext::LineIndex = idx;

    const auto ShowInherit = [&]() {
        return line->Data->FirstIsLink();
    };

    const auto InheritStr = [&]() {
        auto& Q = back->Inherit;
        auto w = IBR_WorkSpace::ShowRegName ? UTF8toUnicode(Q) :
            (IBR_Inst_Project.HasSection({ Desc.Ini, Q }) ? UTF8toUnicode(IBR_Inst_Project.GetSection(IBB_Section_Desc{ Desc.Ini, Q }).GetDisplayName()) : UTF8toUnicode(Q));
        std::wstring Nul;
        if (w.empty()) return loc("GUI_NoInherit");
        else return UnicodetoUTF8(std::vformat(locw("GUI_InheritFrom"), std::make_wformat_args(ShowInherit() ? w : Nul)));
        };

    auto DescLong = PoolDesc(line->Default->DescLong);

    auto& wline = ActiveLines[Name];

    ExportContext::Key = Name;
    if (Name == InheritKeyID())
    {
        auto DescShort = InheritStr();
        wline.RenderUI(DescShort.c_str(), DescLong, *line);
    }
    else if (!IBR_WorkSpace::ShowRegName)
    {
        if (OnShow == EmptyOnShowDesc)
            wline.RenderUI(PoolDesc(line->Default->DescShort), DescLong, *line);
        else wline.RenderUI(OnShow.c_str(), DescLong, *line);
    }
    else wline.RenderUI(PoolCStr(Name), DescLong, *line);
    ExportContext::Key = EmptyPoolStr;

    return true;
}

bool Acceptor_CheckLinkType(StrPoolID SourceReg, StrPoolID TargetReg, StrPoolID LinkType)
{
    bool Check = true;
    if (SourceReg != EmptyPoolStr)
    {
        auto& typealt = LinkType;
        if (typealt != EmptyPoolStr)
        {
            if (typealt == MyTypeID())Check = IBB_DefaultRegType::MatchType(SourceReg, TargetReg);
            else if (typealt == AnyTypeID())Check = true;
            else Check = IBB_DefaultRegType::MatchType(typealt, TargetReg);

        }
    }
    return Check;
}
void Acceptor_RefusePreview(StrPoolID SourceReg, StrPoolID TargetReg, StrPoolID LinkType)
{
    IBR_Inst_Project.DragConditionText = "HOLY_SHIT\nWHAT_HAD_JUST_HAPPENED\nONE_MINUTE_AGO";
    auto& alt = LinkType;
    std::wstring W1;
    if (alt == MyTypeID())W1 = UTF8toUnicode(PoolStr(SourceReg));
    else if (alt == AnyTypeID())W1 = UTF8toUnicode(PoolStr(SourceReg));
    else W1 = UTF8toUnicode(PoolStr(alt));
    auto W2 = UTF8toUnicode(PoolStr(TargetReg));
    IBR_Inst_Project.DragConditionTextAlt = UnicodetoUTF8(std::vformat(locw("GUI_Preview_WrongType"),
        std::make_wformat_args(W1, W2)));
}

void IBR_SectionData::RenderUI_Acceptor(float LastFinalY)
{
    auto Pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({ 0.0f, FinalY });
    auto Virtual = IsVirtualBlock();
    auto Included = IsIncluded();

    ImGui::Dummy({ ImGui::GetWindowWidth(), Included ? LastFinalY : ImGui::GetWindowHeight() }, true);
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
                    if (auto DLK = srcback->GetDLK(back->Register); DLK != EmptyPoolStr)
                    {
                        if (payload->IsPreview())
                            IBR_Inst_Project.DragConditionText = Desc.Ini + " -> " + DisplayName + " : " + PoolStr(DLK);
                        if (payload->IsDelivery())
                        {
                            IBG_Undo.SomethingShouldBeHere();
                            back->MergeLine(DLK, desc.Sec, IBB_IniMergeMode::Merge);
                            back->SetOnShow(DLK);
                        }
                    }
                    else if (payload->IsPreview())
                    {
                        IBR_Inst_Project.DragConditionText = "HOLY_SHIT\nWHAT_HAD_JUST_HAPPENED\nONE_MINUTE_AGO";
                        auto W = UTF8toUnicode(PoolStr(back->Register));
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
                const auto& lin = **(LineDragData**)(payload->Data);
                auto sec = IBR_Inst_Project.GetSection(lin.Desc);
                auto back = sec.GetBack();
                auto tgback = GetBack_Inl();

                if (back && tgback)
                {
                    bool Check = Acceptor_CheckLinkType(back->Register, tgback->Register, lin.TypeAlt);
                    if (Check)
                    {
                        if (payload->IsPreview())
                            IBR_Inst_Project.DragConditionText = Desc.Ini + " -> " + DisplayName;
                        if (payload->IsDelivery())
                        {
                            IBG_Undo.SomethingShouldBeHere();
                            lin.pSession->ValueToMerge = Desc.Sec;
                            lin.pSession->NotifyValueToMerge = true;
                        }
                    }
                    else if (payload->IsPreview())
                        Acceptor_RefusePreview(back->Register, tgback->Register, lin.TypeAlt);

                }
                else if (payload->IsPreview())
                    IBR_Inst_Project.DragConditionText.clear();
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::SetCursorPos(Pos);
}

void IBR_SectionData::RenderUI_TitleBar(IBR_Section Rsec, IBB_Section* Bsec, bool &TriggeredRightMenu, float LastFinalY)
{
    IM_UNUSED(LastFinalY);
    auto HalfLine = ImGui::GetTextLineHeightWithSpacing() * 0.5F;
    auto Pos = ImGui::GetCursorPos();
    auto Virtual = IsVirtualBlock();

    auto NotAsImported = Bsec->Dynamic.ImportCount == 0;
    
    ImVec2 CurL{ Pos.x,Pos.y - 0.2f * FontHeight };
    ImGui::SetCursorPos({ 0.0f,CurL.y });
    ImGui::Dummy({ ImGui::GetWindowWidth(), ImGui::GetTextLineHeightWithSpacing()}, true);
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
                if (Frozen)
                {
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_UnfreezeSec"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        Frozen = false;
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                }
                else
                {
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_FreezeSec"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        Frozen = true;
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                }
                if (Hidden)
                {
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_ShowSec"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        Hidden = false;
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                }
                else
                {
                    if (ImGui::SmallButtonAlignLeft(locc("GUI_HideSec"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                    {
                        Hidden = true;
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                }
                if (IsVirtualBlock())
                {
                    if (IsComposedAllFold())
                    {
                        if (ImGui::SmallButtonAlignLeft(locc("GUI_UnfoldComposed"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                        {
                            UnfoldComposed();
                            IBR_PopupManager::ClearRightClickMenu();
                        }
                    }
                    else
                    {
                        if (ImGui::SmallButtonAlignLeft(locc("GUI_FoldComposed"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                        {
                            FoldComposed();
                            IBR_PopupManager::ClearRightClickMenu();
                        }
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

    if (!TitleCol_Cached) TitleCol_Cached = Rsec.GetRegTypeColor();
    ImColor& Col = *TitleCol_Cached;

    auto PPos = ImGui::GetWindowPos();
    ImU32 UCol = Col.Value.w < 1e-6 ? ImGui::GetColorU32(ImGuiCol_TitleBg) : (ImU32)Col;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2{ PPos.x, PPos.y + CurL.y },
        ImVec2{ PPos.x + ImGui::GetWindowWidth(), PPos.y + CurL.y + ImGui::GetTextLineHeightWithSpacing() * 1.1f }, UCol);
    ImGui::SetCursorPos(CurL);

    if (!IsComment && !Virtual)
    {
        if (Ignore)ImGui::PushStyleColor(ImGuiCol_CheckMark, IBR_WorkSpace::TempWbg);
        if (!NotAsImported)
        {
            auto X = ImGui::GetWindowContentRegionWidth() * 0.5f - FontHeight * 0.5f;
            ImGui::SetCursorPosX(X);
            ReOffset = { X, HalfLine };
        }
        else
        {
            ReOffset = { FontHeight * 0.7f, HalfLine };
        }
        auto CPos = ImGui::GetCursorPos();
        auto wpp = ImGui::GetWindowPos();
        ImGui::RadioButton(("##MODULE" + ModuleStrID).c_str(), true, GlobalNodeStyle);
        if (Ignore)ImGui::PopStyleColor();
        if (ImGui::BeginDragDropSource())
        {
            LinkNodeContext::CurDragStart = { wpp.x + CPos.x + HalfLine, wpp.y + CPos.y + HalfLine };
            LinkNodeContext::CurDragCol = UCol;
            LinkNodeContext::HasDragNow = true;

            ImGui::Text((Desc.Ini + " -> " + DisplayName).c_str());
            DrawDragPreviewIcon();
            auto s = Desc.GetText();
            //IBR_HintManager::SetHint(s, 1000);
            ImGui::SetDragDropPayload("IBR_SecDrag", s.c_str(), s.size() + 1);
            IBR_Inst_Project.IBR_SecDragMap[s] = { Desc };
            ImGui::EndDragDropSource();
        }

        if (NotAsImported)
        {
            ImGui::SameLine();
            if (IBR_WorkSpace::ShowRegName)
            {
                IBD_RInterruptF(x);
                if (Bsec)ImGui::Text(Bsec->Name.c_str());
                else
                {
                    ImGui::TextColored(IBR_Color::ErrorTextColor, locc("Back_GunMu"));
                    BackPtr_Cached = nullptr;
                }
            }
            else ImGui::Text(DisplayName.c_str());
        }

        auto UPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos({ 0.0f,CurL.y });
        ImGui::Dummy({ ImGui::GetWindowWidth(), ImGui::GetTextLineHeightWithSpacing() }, true);
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

void IBR_SectionData::RenderUI_Error()
{
    ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_MissingSectionLink"));
}

void IBR_SectionData::RenderUI_Comment(IBB_Section* Bsec)
{
    if (!CommentEdit)ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_MissingCommentBuffer"));
    else
    {
        auto TSize = ImGui::CalcTextSize(CommentEdit.get());
        TSize.x = std::max(TSize.x + FontHeight * 1.2f, FontHeight * 14.6f);
        TSize.y = std::max(TSize.y + FontHeight, FontHeight * 8.0f);
        //IBR_HintManager::SetHint(std::to_string(TSize.x) + ","+ std::to_string(TSize.y),HintStayTimeMillis);
        if (ImGui::InputTextMultiline(("##COMMENT" + ModuleStrID).c_str(), CommentEdit.get(), MAX_STRING_LENGTH,
            TSize,
            ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_WrappedText))
        {
            Bsec->Comment = CommentEdit.get();
        }
        if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
        if (TSize.x > FontHeight * 14.6f)WidthFix = TSize.x;
    }
}

void IBR_SectionData::RenderUI_Collapsed(IBB_Section* Bsec, ImVec2 HeadLineRN, IBR_Section Rsec)
{
    IM_UNUSED(Rsec);
    for (auto& [k, v] : ActiveLines)
        v.Collapsed = true;
    for (auto i : Bsec->SubSecOrder)
    {
        const auto& sub = Bsec->SubSecs[i];
        for (const auto& lt : sub.NewLinkTo)
        {
            if (IBR_Inst_Project.RefreshLinkList)
            {
                bool FromImport = (lt.FromKey == ImportKeyID());
                bool IsLinkingToSelf = (lt.From == lt.To);
                IBR_LinkNode::PushLinkForDraw(HeadLineRN, lt.To, lt.ToKey, lt.SessionID, lt.DefaultColor, FromImport, IsLinkingToSelf, true);
            }
            else
            {
                IBR_NodeSession::SetSessionStatus(lt.SessionID, HeadLineRN, true);
            }
        }
    }
    ImGui::SetCursorPos({ ImGui::GetWindowWidth() - FontHeight * 4.0f, FinalY - FontHeight * 0.15f });
    if (ImGui::SmallButton(("+##ExpandBtn" + ModuleStrID).c_str(), { FontHeight * 1.5f,FontHeight * 1.2f }))
    {
        CollapsedInComposed = false;
    }
    if (ImGui::IsItemHovered())
        IBR_ToolTip(loc("GUI_UnfoldModule"));
}

void IBR_SectionData::RenderUI_Virtual()
{
    int HiddenCount = 0;
    FinalY += std::ranges::fold_left(IncludingModules, 0.0F, [&HiddenCount](float f, auto id) {
        if (auto Data = IBR_Inst_Project.GetSectionFromID(id).GetSectionData(); Data)
        {
            auto PosUL = ImGui::GetCursorScreenPos();
            PosUL.x -= ImGui::GetCurrentContext()->Style.ItemSpacing.x;
            PosUL.y -= ImGui::GetCurrentContext()->Style.ItemSpacing.y;
            Data->RenderUI();

            if (!Data->CollapsedInComposed)
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetTextLineHeight() * 0.5f);
                Data->FinalY += ImGui::GetTextLineHeight() * 0.5f;
            }

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
                    IBR_TopMost::CommitPushClipRect(IBR_RealCenter::WorkSpaceUL, IBR_RealCenter::WorkSpaceDR, true);
                    //FL->AddRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), IBR_Color::FocusWindowColor, 5.0F, 0, 5.0F);
                    IBR_TopMost::CommitRect(PosUL, PosDR, IBR_Color::FocusWindowColor, 5.0F, 0, 5.0F);
                    IBR_TopMost::CommitPopClipRect();
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

void IBR_SectionData::RenderUI_Composed()
{
    ImGui::SetCursorPos({ ImGui::GetWindowWidth() - FontHeight * 4.0f, FinalY - FontHeight * 0.15f });
    if (ImGui::SmallButton(("-##ExpandBtn" + ModuleStrID).c_str(), { FontHeight * 1.5f,FontHeight * 1.2f }))
    {
        CollapsedInComposed = true;
    }
    if (ImGui::IsItemHovered())
        IBR_ToolTip(loc("GUI_FoldModule"));
}

void IBR_SectionData::RenderUI_Lines(IBB_Section* Bsec)
{
    Bsec->CheckSubsecOrder();

    for (auto i : Bsec->SubSecOrder)
    {
        auto& sub = Bsec->SubSecs[i];
        LinkNodeContext::CurSub = &sub;

        if (sub.Default->Type == IBB_SubSec_Default::Inherit && Bsec->Inherit.empty())continue;
        else
        {
            for (const auto& k : Bsec->LineOrder)
            {
                if (!sub.CanOwnKey(k))continue;
                auto coll = !Bsec->IsOnShow(k);
                ActiveLines[k].Collapsed = coll;
                if (coll)continue;
                RenderUI_Line(Bsec->GetOnShow(k), k);
            }
        }

        IBR_LinkNode::PushInactiveLines(sub, LinkNodeContext::CollapsedCenter);
    }
    LinkNodeContext::CurSub = nullptr; 
}

namespace IBR_LinkNode
{
    ImU32 AdjustLineCol(ImU32 Color)
    {
        bool HighLight = (IBR_EditFrame::CurSection.ID == IBR_WorkSpace::CurOnRender_ID
            && !ImGui::GetCurrentContext()->OpenPopupStack.Size);
        return HighLight ? (ImU32)IBR_Color::FocusLineColor : Color;
    }

    ImColor AdjustNodeCol(ImU32 Color, bool Empty, bool Inherit)
    {
        ImU32 BtnColorW = (Color >> IM_COL32_A_SHIFT) & 0xFF;
        bool HasBtnCol = BtnColorW > 0;

        bool Illegal = false;
        if (!IBR_WorkSpace::CurOnRender)Illegal = true;
        else
        {
            auto R = IBR_Inst_Project.GetSectionFromID(IBR_WorkSpace::CurOnRender_ID);
            if(!R.GetSectionData())Illegal = true;//Reduce 3.5% CPU Time than R.HasBack()
        }
        if (Illegal)
        {
            if(Inherit)return Color;
            else return IBR_Color::IllegalLineColor;
        }

        if (IBR_WorkSpace::CurOnRender->Ignore)
            return IBR_WorkSpace::TempWbg;

        if (Empty)
        {
            if (Inherit) return IBR_Color::FocusLineColor;
            else return IBR_Color::IllegalLineColor;
        }

        if (HasBtnCol)
            return Color;

        return ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
    }

    void PushLinkForDraw(ImVec2 Center, IBB_SectionID Dest, StrPoolID DestKey, uint64_t SessionID, ImU32 LineCol, bool FromImport, bool SelfLink, bool Collapsed, bool SrcDragging)
    {
        IBR_NodeSession::SetSessionStatus(SessionID, Center, Collapsed);
        IBR_Inst_Project.LinkList.push_back({ Dest, DestKey, SessionID, IBR_WorkSpace::CurOnRender_ID, LineCol, FromImport, SelfLink, SrcDragging });
    }
}

void IBR_SectionData::RenderUI()
{
    if (!First && Hidden)
    {
        FinalY = 0;
        return;
    }
    auto Bsec = GetBack_Inl();
    if (Bsec == nullptr)
    {
        RenderUI_Error();
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

    RenderUI_Acceptor(LastFinalY);

    if(!Bsec->SingleVal)RenderUI_TitleBar(Rsec, Bsec, TriggeredRightMenu, LastFinalY);
    
    ImVec2 HeadLineRN = ImGui::GetLineEndPos() - ImVec2{ FontHeight * 1.5f, HalfLine };
    {
        IBD_RInterruptF(x);
        if (IsComment)
            RenderUI_Comment(Bsec);
        else if (Included && CollapsedInComposed && !First)
            RenderUI_Collapsed(Bsec, HeadLineRN, Rsec);
        else
        {
            if (IsIncluded())RenderUI_Composed();
            LinkNodeContext::CollapsedCenter = { HeadLineRN.x , FinalY + HalfLine + ImGui::GetWindowPos().y };
            RenderUI_Lines(Bsec);
        }
    }

    FinalY = ImGui::GetCursorPosY() - FinalY;
    
    if (IsVirtualBlock())
        RenderUI_Virtual();
    else if(IBR_WorkSpace::CurOnRender_Clicked && !Included)
        IBR_EditFrame::ActivateAndEdit(IBR_WorkSpace::CurOnRender_ID, false);
}
