
#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "IBB_RegType.h"
#include <imgui_internal.h>
#include <string.h>
#include <Windows.h>
#include <algorithm>
#include <climits>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <imgui.h>
#include "IBR_Components.h"
#include "FromEngine/global_tool_func.h"
#include "IBR_Combo.h"
#include "IBR_Debug.h"

bool ImGui_TextDisabled_Helper(const char* Text);
bool SmallButton_Disabled_Helper(bool cond, const char* Text);
bool InputTextStdString(const char* label, std::string& str,
    ImGuiInputTextFlags flags);

int HintStayTimeMillis = 3000;

namespace ImGui
{
    ImVec2 GetLineEndPos();
    ImVec2 GetLineBeginPos();
    bool IsWindowClicked(ImGuiMouseButton Button);
    void Dummy(const ImVec2& size, bool AffectsLayout);
}

void SidebarLine::RenderUI(const char* Line, const char* _Hint, IBB_IniLine& Back)
{
    Edit.RenderUI(Line, _Hint, Back, false);
}

bool Acceptor_CheckLinkType(StrPoolID SourceReg, StrPoolID TargetReg, StrPoolID LinkType);
void Acceptor_RefusePreview(StrPoolID SourceReg, StrPoolID TargetReg, StrPoolID LinkType);

void WorkSpaceLine::RenderUI(const char* Line, const char* Hint, IBB_IniLine& Back)
{
    auto Cursor = ImGui::GetCursorScreenPos();
    auto Y1 = ImGui::GetCursorPos().y;
    Edit.RenderUI(Line, Hint, Back, true);
    auto LH = ImGui::GetTextLineHeight();
    AcceptCenter = { Cursor.x - LH * 0.7f, Cursor.y + LH * 0.5f };
    if (auto& acc = Back.Default->GetInputType().AcceptorSetting; acc || AcceptCount > 0)
    {
        auto Y2 = ImGui::GetCursorPos().y;
        auto Cursor2 = ImGui::GetCursorPos();
        auto KeyName = Back.Default->Name;
        auto tgback = LinkNodeContext::CurSub->Root;
        auto tgreg = acc ? acc->AcceptRegType : tgback->Register;
        ImVec4 Col = IBB_DefaultRegType::GetRegType(tgreg).FrameColor;
        ImGui::SetCursorPos({ 0.0f, Y1 });
        ImGui::Dummy({ ImGui::GetWindowWidth(), Y2 - Y1 }, true);
        //IBR_TopMost::CommitRect(Cursor, { Cursor.x + ImGui::GetWindowWidth(), Cursor.y + Y2 - Y1 }, IBR_Color::CheckMarkColor);
        if(!ImGui::GetTopMostPopupModal())
            IBR_TopMost::CommitDrawOpr({ Cursor.x - LH * 1.2f, Cursor.y }, [=]() {
                    ImGui::PushID(KeyName);
                    ImGui::PushStyleColor(ImGuiCol_CheckMark, Col);
                    ImGui::RadioButton("", true, ImGuiRadioButtonFlags_RoundedSquare);
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                });

        if (ImGui::BeginDragDropTarget())
        {
            auto payload = ImGui::AcceptDragDropPayload("IBR_LineDrag", ImGuiDragDropFlags_AcceptBeforeDelivery);
            if (payload && (payload->IsPreview() || payload->IsDelivery()))
            {
                const auto& lin = **(LineDragData**)(payload->Data);
                auto sec = IBR_Inst_Project.GetSection(lin.Desc);
                auto back = sec.GetBack();
                auto& rsd = *IBR_Inst_Project.GetSection(tgback->GetThisID()).GetSectionData();
                if (back && tgback)
                {
                    bool Check = Acceptor_CheckLinkType(back->Register, tgreg, lin.TypeAlt);
                    if (Check)
                    {
                        if (payload->IsPreview())
                            IBR_Inst_Project.DragConditionText =
                                rsd.Desc.Ini + " -> " + rsd.DisplayName + " : " + PoolCStr(KeyName);
                        if (payload->IsDelivery())
                        {
                            IBG_Undo.SomethingShouldBeHere();
                            lin.pSession->ValueToMerge = rsd.Desc.Sec + "$$" + PoolCStr(KeyName);
                            lin.pSession->NotifyValueToMerge = true;
                        }
                    }
                    else if (payload->IsPreview())
                        Acceptor_RefusePreview(back->Register, tgreg, lin.TypeAlt);
                }
                else if (payload->IsPreview())
                    IBR_Inst_Project.DragConditionText.clear();
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SetCursorPos(Cursor2);
    }
    
}

void IBR_IniLine::RenderUI(const char* Line, const char* Hint, IBB_IniLine& Back, bool IsWorkSpace)
{
    LinkNodeContext::CurLineChangeCompStatus = false;

    if (IBR_Inst_Debug.LinkDebugMode)
    {
        for (auto& [k, l] : LinkNodeContext::CurSub->LinkSrc)
        {
            if ((k >> 32) ^ LinkNodeContext::LineIndex)continue;
            ImGui::TextWrappedEx(LinkNodeContext::CurSub->NewLinkTo[l].GetText().c_str());
        }
        if (Back.Data)
        {
            ImGui::TextWrapped("Value = %s", Back.Data->GetString().c_str());
        }
    }


    ImGui::TextWrappedEx(Line);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        LinkNodeContext::CurLineChangeCompStatus = true;
    //ImGui::TextEx(Line, nullptr, ImGuiTextFlags_NoWidthForLargeClippedText);
    if (ImGui::IsItemHovered())
    {
        IBR_ToolTip(Hint);
    }

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::PushID(Back.GetComponentID());
    Back.RenderUI(IsWorkSpace);
    ImGui::PopID();
    ImGui::EndGroup();
}





extern const char* Internal_IniName;
bool InRectangle(ImVec2 P, ImVec2 UL, ImVec2 DR)
{
    std::tie(UL.x, DR.x) = std::minmax(UL.x, DR.x);
    std::tie(UL.y, DR.y) = std::minmax(UL.y, DR.y);
    return UL.x <= P.x && UL.y <= P.y && DR.x >= P.x && DR.y >= P.y;
}
std::tuple<bool, ImVec2, ImVec2> RectangleCross(ImVec2 UL1, ImVec2 DR1, ImVec2 UL2, ImVec2 DR2)//TODO:Test
{
    std::tie(UL1.x, DR1.x) = std::minmax(UL1.x, DR1.x);
    std::tie(UL1.y, DR1.y) = std::minmax(UL1.y, DR1.y);
    std::tie(UL2.x, DR2.x) = std::minmax(UL2.x, DR2.x);
    std::tie(UL2.y, DR2.y) = std::minmax(UL2.y, DR2.y);
    ImVec2 NU{ std::max(UL1.x, UL2.x), std::max(UL1.y, UL2.y) }, ND{ std::min(DR1.x, DR2.x),std::min(DR1.y, DR2.y) };
    return { NU.x < ND.x&& NU.y < ND.y, NU, ND };
}





dImVec2 operator+(const dImVec2 a, const dImVec2 b) { return { a.x + b.x,a.y + b.y }; }
dImVec2 operator-(const dImVec2 a, const dImVec2 b) { return { a.x - b.x,a.y - b.y }; }
ImVec4 operator+(const ImVec4 a, const ImVec4 b) { return { a.x + b.x,a.y + b.y,a.z + b.z,a.w + b.w }; }
dImVec2 operator/(const dImVec2 a, const double b) { return { a.x / b,a.y / b }; }
dImVec2 operator*(const dImVec2 a, const double b) { return { a.x * b,a.y * b }; }
dImVec2& operator+=(dImVec2& a, const dImVec2 b) { a.x += b.x; a.y += b.y; return a; }
dImVec2& operator-=(dImVec2& a, const dImVec2 b) { a.x -= b.x; a.y -= b.y; return a; }
dImVec2& operator/=(dImVec2& a, const double b) { a.x /= b; a.y /= b; return a; }
dImVec2& operator*=(dImVec2& a, const double b) { a.x *= b; a.y *= b; return a; }


IBR_SectionData::~IBR_SectionData()
{
    IncludingModules.clear();
}

IBR_SectionData::IBR_SectionData() :
    EqPos(IBR_FullView::EqCenter),
    EqSize({ (float)IBG_GetSetting().FontSize * 15,(float)IBG_GetSetting().FontSize * 8 }),
    BackPtr_Cached(nullptr),
    ReWindowUL(IBR_RealCenter::Center)
{}
IBR_SectionData::IBR_SectionData(const IBB_Section_Desc& D) :
    Desc(D),
    EqPos(IBR_FullView::EqCenter),
    EqSize({ (float)IBG_GetSetting().FontSize * 15,(float)IBG_GetSetting().FontSize * 8 }),
    BackPtr_Cached(nullptr),
    ReWindowUL(IBR_RealCenter::Center)
{}
IBR_SectionData::IBR_SectionData(const IBB_Section_Desc& D, std::string&& Name) :
    DisplayName(std::move(Name)),
    Desc(D),
    EqPos(IBR_FullView::EqCenter),
    EqSize({ (float)IBG_GetSetting().FontSize * 15,(float)IBG_GetSetting().FontSize * 8 }),
    BackPtr_Cached(nullptr),
    ReWindowUL(IBR_RealCenter::Center)
{
    
}

IBR_SectionData::IBR_SectionData(IBR_SectionData&& rhs)
noexcept :
    DisplayName(std::move(rhs.DisplayName)),
    Desc(std::move(rhs.Desc)),
    BackPtr_Cached(nullptr),
    EqPos(rhs.EqPos),
    EqSize(rhs.EqSize),
    ReWindowUL(rhs.ReWindowUL),
    IncludingModules(std::move(rhs.IncludingModules)),
    IncludedByModule(rhs.IncludedByModule),
    IncludedByModule_TmpDesc(std::move(rhs.IncludedByModule_TmpDesc)),
    IncludingModules_TmpDesc(std::move(rhs.IncludingModules_TmpDesc))
{

}

namespace IBR_RealCenter
{
    ImVec2 Center;
    dImVec2 FixedUL, WorkSpaceUL, WorkSpaceDR;
    bool Update()
    {
        double LWidth = ImGui::IsWindowCollapsed() ? 0.0f : ImGui::GetWindowWidth();
        FixedUL = dImVec2{ 0.0, FontHeight * 2.0 - WindowSizeAdjustY };
        WorkSpaceUL = dImVec2{ LWidth,FontHeight * 2.0 - WindowSizeAdjustY };
        WorkSpaceDR = dImVec2{ (double)IBR_UICondition::CurrentScreenWidth ,(double)IBR_UICondition::CurrentScreenHeight - FontHeight * 1.5 };
        Center = ImVec2((FixedUL + WorkSpaceDR) / 2.0);

        return true;
    }
    ImRect GetWorkSpaceRect()
    {
        return ImRect{ WorkSpaceUL,WorkSpaceDR };
    }
}

namespace IBR_UICondition
{
    int CurrentScreenWidth, CurrentScreenHeight;
    bool MenuChangeShow = true;
    bool MenuCollapse = false;
    std::wstring WindowTitle;
    bool UpdateWindowTitle()
    {
        std::wstring nt = _AppNameW;
        auto& proj = IBF_Inst_Project.Project;
        if (IBR_ProjectManager::IsOpen())nt += L" - " + proj.ProjName;
        if (proj.ChangeAfterSave)nt += L"[*]";
        if (nt == WindowTitle)return true;
        WindowTitle = nt;
        return (bool)SetWindowTextW(MainWindowHandle, nt.c_str());
    }
}



bool IBG_UndoStack::Undo()
{
    if (Cursor == -1)return false;
    Stack[Cursor].UndoAction();
    --Cursor;
    IBF_Inst_Project.Project.ChangeAfterSave = true;
    return true;
}
bool IBG_UndoStack::Redo()
{
    if (Cursor == (int)Stack.size() - 1)return false;
    ++Cursor;
    Stack[Cursor].RedoAction();
    IBF_Inst_Project.Project.ChangeAfterSave = true;
    return true;
}
bool IBG_UndoStack::CanUndo() const
{
    return Cursor > -1;
}
bool IBG_UndoStack::CanRedo() const
{
    return Cursor < (int)Stack.size() - 1;
}
void IBG_UndoStack::Release()
{
    while (Cursor < (int)Stack.size() - 1)
        Stack.pop_back();
}
void IBG_UndoStack::SomethingShouldBeHere()
{
    if (IBF_Inst_Project.Project.IsEmpty())return;
    Release();
    IBF_Inst_Project.Project.ChangeAfterSave = true;
}
void IBG_UndoStack::Push(const _Item& a)
{
    if (IBF_Inst_Project.Project.IsEmpty())return;
    Release();
    Stack.push_back(a);
    ++Cursor;
    IBF_Inst_Project.Project.ChangeAfterSave = true;
}
void IBG_UndoStack::RenderUI()
{
    //ImGui::TextDisabled(locc("GUI_Undo"));
    //if (SmallButton_Disabled_Helper(CanUndo(), locc("GUI_Undo")))IBRF_CoreBump.SendToR({[this]() {IBR_Inst_Debug.AddMsgOnce([=]() {ImGui::Text("Undo"); }); Undo(); }});
    //ImGui::SameLine();
    //ImGui::TextDisabled(locc("GUI_Redo"));
    //if (SmallButton_Disabled_Helper(CanRedo(), locc("GUI_Redo")))IBRF_CoreBump.SendToR({ [this]() {IBR_Inst_Debug.AddMsgOnce([=]() {ImGui::Text("Redo"); }); Redo(); } });
}
void IBG_UndoStack::Clear()
{
    Stack.clear();
    Cursor = -1;
}
IBG_UndoStack::_Item* IBG_UndoStack::Top()
{
    if (Cursor <= -1)return nullptr;
    else return &Stack.at(Cursor);
}
IBG_UndoStack IBG_Undo;


#define AFTER_INTERRUPT_F

namespace IBR_EditFrame
{
    IBR_Section CurSection{ &IBR_Inst_Project,UINT64_MAX };
    ModuleID_t PrevId{ UINT64_MAX };
    bool Empty{ true };
    char EditBuf[100000];
    bool TextEditError{ false }, OnTextEdit{ false }, TextEditReset{ false };
    std::unordered_map<StrPoolID, SidebarLine> EditLines;
    std::string NewLineKey, NewLineValue;
    
    void AFTER_INTERRUPT_F ResetEdit(IBB_Section* rsc)
    {
        EditLines.clear();
        rsc->RecheckLineOrder();
        for (auto& Sub : rsc->SubSecs)
        {
            for (auto& [K, V] : Sub.Lines)
            {
                auto& Line = EditLines[K];
                Line.Buffer = V.Data->GetString();
                Line.Hint = V.Default->DescLong;
                Line.LinkNode = V.Default->GetNodeSetting();
                Line.OnShowBuf = rsc->GetOnShow(K);
                Line.InputOnShow = false;
                Line.InputType = V.Default->GetInputTypeByValue(Line.Buffer);
            }
        }
    }

    void SetActive(ModuleID_t id)
    {
        if (Empty)
        {
            PrevId = id;
            Empty = false;
        }
        if (CurSection.ID == id)return;
        //IBR_HintManager::SetHint(std::to_string(id),1000);
        CurSection = IBR_Inst_Project.GetSectionFromID(id);
        IBR_UICondition::MenuChangeShow = true;
        OnTextEdit = false;
        TextEditError = false;
        TextEditReset = false;
        NewLineKey.clear();
        NewLineValue.clear();
        for (auto& ini : IBF_Inst_Project.Project.Inis)
            for (auto& sec : ini.Secs)
                sec.second.Dynamic.Selected = false;
        {
            auto rsc = CurSection.GetBack();
            IBD_RInterruptF(x);
            ResetEdit(rsc);
            rsc->Dynamic.Selected = true;
        }
    }

    void ActivateAndEdit(ModuleID_t id, bool TextMode)
    {
        SetActive(id);
        if (TextMode)SwitchToText();
        if(!IBR_Inst_Debug.DontGoToEdit)IBR_Inst_Menu.ChooseMenu(MenuItemID_EDIT);
    }

    void SwitchToText()
    {
        auto rsc = CurSection.GetBack();
        if (rsc)
        {
            strcpy(EditBuf, rsc->GetTextForEdit().c_str());
            TextEditError = false;
        }
        else TextEditError = true;

        OnTextEdit = true;
        TextEditReset = false;
    }

    void ExitTextEdit()
    {
        auto pbk = CurSection.GetBack();
        auto rsd = CurSection.GetSectionData();
        if (!pbk || !rsd)
        {
            Empty = true;
            return;
        }
        OnTextEdit = false;

        IBG_Undo.SomethingShouldBeHere();

        pbk->SetText(EditBuf);
        rsd->ActiveLines.clear();
        ResetEdit(pbk);

        NewLineKey.clear();
        NewLineValue.clear();
        IBR_Inst_Project.UpdateAll();
    }

    void AFTER_INTERRUPT_F Modify(StrPoolID s, SidebarLine& L)
    {
        auto back = CurSection.GetBack();
        back->MergeLine(s, L.Buffer, IBB_IniMergeMode::Replace, false);
    }

    void RenderUI_NewLine(IBB_Section* pbk)
    {
        auto FrameWidth = (ImGui::GetWindowWidth() - FontHeight * 4.5f) * 0.5f;
        ImGui::SetNextItemWidth(FrameWidth);

        //InputTextStdStringWithOption("##NewLineKey", NewLineKey, 0, IBF_Inst_DefaultTypeList.List.Query.InputTextOptions);
        InputTextStdString("##NewLineKey", NewLineKey);
        bool Active = ImGui::IsItemActive();
        ImGui::SameLine();

        ImGui::Text("="); ImGui::SameLine();
        ImGui::SetNextItemWidth(FrameWidth);
        InputTextStdString("##NewLineValue", NewLineValue); ImGui::SameLine();
        
        if (ImGui::Button(" + "))
        {
            auto NewKeyID = NewPoolStr(NewLineKey);

            IBG_Undo.SomethingShouldBeHere();
            {
                IBD_RInterruptF(x);

                if (NewLineValue.empty())
                {
                    auto pLine = IBF_Inst_DefaultTypeList.List.KeyBelongToLine(NewKeyID);
                    if (pLine)
                    {
                        auto& Input = pLine->GetInputType();
                        NewLineValue = Input.Form->GetFormattedString();
                    }
                }

                pbk->OnShow[NewKeyID] = EmptyOnShowDesc;//默认在画布上显示
                pbk->MergeLine(NewKeyID, NewLineValue, IBB_IniMergeMode::Replace);//添加或替换
                pbk->OrderKey(NewKeyID, 0);//调整到最前面，方便用户看到
                IBF_Inst_Project.UpdateAll();
            }
            ResetEdit(pbk);
            NewLineKey.clear();
            NewLineValue.clear();
        }

        if (NewLineValue.empty() && !NewLineKey.empty())
        {
            auto pLine = IBF_Inst_DefaultTypeList.List.KeyBelongToLine(NewLineKey);
            if (pLine)
            {
                auto& Input = pLine->GetInputType();
                auto& Str = Input.Form->GetFormattedString();
                if (!Str.empty())
                {
                    ImGui::TextDisabled(locc("GUI_UseInitialValue"), Str.c_str());
                }
            }
        }

        EditStringWithOptions(Active, NewLineKey);
    }

    void RenderUI_SwitchToText()
    {
        if (ImGui::Button(locc("GUI_SwitchToTextEdit")))
        {
            IBR_EditFrame::SwitchToText();
            return;
        }
    }

    //此处关联了IBR_Project::RenameAll() & ModuleClipData::NeedtoMangle()
    //记得同时修改
    bool NeedtoMangle(IBB_Section* pbk)
    {
        bool P = false;
        std::string W{}, Q{};
        W = pbk->VarList.GetVariable("_InitialSecName");
        Q = pbk->VarList.GetVariable("UseOwnName");
        if (W == pbk->Name && !IsTrueString(Q))
        {
            P = true;
        }
        if (W.empty())P = true;
        return P;
    }

    void RenderUI_UseOwnName(IBB_Section* pbk)
    {
        bool N = NeedtoMangle(pbk);
        bool P = N;
        ImGui::Checkbox(locc("GUI_RefreshRegisterOnPaste"), &P);
        if (P != N)
        {
            if (P)
            {
                //Set 1
                pbk->VarList.Value["_OldInitSecName"] = pbk->VarList.Value["_InitialSecName"];
                pbk->VarList.Value.erase("_InitialSecName");
            }
            else
            {
                //Set 0
                auto& Str = pbk->VarList.Value["_OldInitSecName"];
                //生成一个随机的、不可能是ModuleTag的初始名字
                if(Str.empty())Str = GenerateModuleTag() + RandStr(4);
                pbk->VarList.Value["_InitialSecName"] = GenerateModuleTag() + RandStr(4); 
            }
        }
    }

    void RenderUI_TextEdit()
    {

        ImGui::Text(locc("GUI_TextEditModeTitle"));

        ImGui::SameLine();
        if (ImGui::Button(locc("GUI_Exit")))
        {
            ExitTextEdit();
            return;
        }

        ImGui::InputTextMultiline(u8"", EditBuf, sizeof(EditBuf),
            ImVec2{ ImGui::GetWindowWidth() - FontHeight * 1.2f ,ImGui::GetWindowHeight() - FontHeight * 5.4f });
        if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
        if (!ImGui::IsItemActive())
        {
            if (TextEditReset)
                ExitTextEdit();
        }
        else TextEditReset = true;
    }

    void RenderUI_OnShow(StrPoolID K, SidebarLine& V, IBB_Section* pbk)
    {
        ImGui::PushID(K);
        if (ImGui::RadioButton("", pbk->IsOnShow(K), GlobalNodeStyle))
        {
            IBG_Undo.SomethingShouldBeHere();
            if (pbk->OnShow[K].empty())pbk->OnShow[K] = EmptyOnShowDesc;
            else pbk->OnShow[K].clear();
        }
        ImGui::PopID();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            V.InputOnShow = !V.InputOnShow;
        }

        ImGui::SameLine();

        if (V.InputOnShow)
        {
            ImGui::PushID(K);
            bool Show = pbk->IsOnShow(K);
            if (V.OnShowBuf == EmptyOnShowDesc)V.OnShowBuf = "";
            auto Changed = InputTextStdString("", V.OnShowBuf);
            if (Changed)
            {
                IBG_Undo.SomethingShouldBeHere();
                if (Show && V.OnShowBuf.empty())pbk->OnShow[K] = EmptyOnShowDesc;
                else pbk->OnShow[K] = V.OnShowBuf;
            }
            ImGui::PopID();
        }
    }

    void RenderUI_Lines(IBB_Section* pbk)
    {
        for (auto& K : pbk->LineOrder)
        {
            //like Waiting for SYNC
            if (!EditLines.contains(K))continue;

            auto& V = EditLines.at(K);
            RenderUI_OnShow(K, V, pbk);

            auto [pLine, pSub] = pbk->GetLineFromSubSecsEx2(K);
            if (pLine)
            {
                LinkNodeContext::CurSub = pSub;
                V.RenderUI(PoolCStr(K), PoolDesc(V.Hint), *pLine);
                LinkNodeContext::CurSub = nullptr;
            }
            else ImGui::TextColored(IBR_Color::IllegalLineColor, "%s", locc("GUI_MissingLineData"));
           
        }
    }

    void RenderUI()
    {
        if (Empty)
        {
            IBR_Inst_Menu.ChooseMenu(MenuItemID_FILE);
            return;
        }

        if (OnTextEdit)
        {
            RenderUI_TextEdit();
            return;
        }
        else
        {
            IBD_RInterruptF(x);

            auto pbk = CurSection.GetBack();
            if (!pbk)
            {
                //tmd section jb die why cnmd edit fuck it
                Empty = true;
                return;
            }

            RenderUI_SwitchToText();
            ImGui::SameLine();
            RenderUI_UseOwnName(pbk);

            RenderUI_NewLine(pbk);

            ImGui::Separator();

            RenderUI_Lines(pbk);
        }
    }
    void Clear()
    {
        Empty = true;
        CurSection.ID = UINT_MAX;
    }
};

namespace IBR_Color
{
    ImColor BackgroundColor;
    ImColor ViewFocusWindowColor;
    ImColor FocusWindowColor;
    ImColor TouchEdgeColor;
    ImColor CheckMarkColor;
    ImColor ClipFrameLineColor;
    ImColor CenterCrossColor;
    ImColor ForegroundCoverColor;
    ImColor ForegroundAltColor;
    ImColor ForegroundMarkColor;
    ImColor LegalLineColor;
    ImColor LinkingLineColor;
    ImColor IllegalLineColor;
    ImColor ErrorTextColor;
    ImColor FocusLineColor;
    ImColor FrozenSecColor;
    ImColor HiddenSecColor;
    ImColor FrozenMaskColor;

    namespace Light
    {
        ImColor BackgroundColor(247, 236, 153, 255);//背景色

        ImColor FocusWindowColor(0, 100, 255, 255);

        ImColor ViewFocusWindowColor(170, 204, 244, 255);
        ImColor TouchEdgeColor(170, 204, 244, 255);

        ImColor CheckMarkColor(40, 255, 5, 255);

        ImColor ClipFrameLineColor(108, 255, 45, 255);

        ImColor ForegroundCoverColor(0, 145, 255, 35);
        ImColor ForegroundAltColor(0, 145, 255, 19);

        ImColor ForegroundMarkColor(0, 100, 255, 255);

        ImColor LegalLineColor(255, 138, 5, 255);

        ImColor LinkingLineColor(255, 168, 21, 255);
        ImColor IllegalLineColor(255, 45, 45, 255);
        ImColor ErrorTextColor(255, 45, 45, 255);
        ImColor CenterCrossColor(255, 5, 5, 255);

        ImColor FocusLineColor(255, 255, 255, 255);

        ImColor FrozenSecColor(37, 127, 224, 255);
        ImColor HiddenSecColor(255, 255, 224, 255);
        ImColor FrozenMaskColor(37, 127, 224, 60);
    }

    namespace Dark
    {
        ImColor BackgroundColor(66, 89, 92, 255);//背景色

        ImColor ViewFocusWindowColor(255, 240, 0, 255);

        ImColor FocusWindowColor(255, 240, 0, 255);
        ImColor TouchEdgeColor(255, 240, 0, 255);

        ImColor CheckMarkColor(40, 255, 5, 255);

        ImColor ClipFrameLineColor(108, 255, 45, 255);

        ImColor ForegroundCoverColor(255, 240, 0, 36);

        ImColor ForegroundAltColor(115, 194, 255, 38);

        ImColor ForegroundMarkColor(0, 100, 255, 255);

        ImColor LegalLineColor(133, 175, 181, 255);

        ImColor LinkingLineColor(153, 165, 171, 255);

        ImColor IllegalLineColor(255, 45, 45, 255);
        ImColor ErrorTextColor(255, 45, 45, 255);

        ImColor CenterCrossColor(255, 5, 5, 255);

        ImColor FocusLineColor(255, 255, 255, 255);

        ImColor FrozenSecColor(37, 127, 224, 255);
        ImColor HiddenSecColor(254, 180, 100, 255);
        ImColor FrozenMaskColor(37, 127, 224, 60);
    }

    void LoadCol(JsonObject Obj, const char* ColName,ImColor& I)
    {
        auto P = Obj.ItemArrayIntOr(ColName);
        if (P.size() == 3)I = ImColor(P[0], P[1], P[2]);
        else if (P.size() >= 4)I = ImColor(P[0], P[1], P[2], P[3]);
    }
#define LoadLightCol(ColName) LoadCol(Obj,#ColName,Light::ColName)
#define LoadDarkCol(ColName) LoadCol(Obj,#ColName,Dark::ColName)
    void LoadLight(JsonObject Obj)
    {
        LoadLightCol(BackgroundColor);
        LoadLightCol(FocusWindowColor);
        LoadLightCol(ViewFocusWindowColor);
        LoadLightCol(TouchEdgeColor);
        LoadLightCol(CheckMarkColor);
        LoadLightCol(ClipFrameLineColor);
        LoadLightCol(CenterCrossColor);
        LoadLightCol(ForegroundCoverColor);
        LoadLightCol(ForegroundAltColor);
        LoadLightCol(ForegroundMarkColor);
        LoadLightCol(LegalLineColor);
        LoadLightCol(LinkingLineColor);
        LoadLightCol(IllegalLineColor);
        LoadLightCol(ErrorTextColor);
        LoadLightCol(FocusLineColor);
        LoadLightCol(FrozenSecColor);
        LoadLightCol(HiddenSecColor);
        LoadLightCol(FrozenMaskColor);
    }
    void LoadDark(JsonObject Obj)
    {
        LoadDarkCol(BackgroundColor);
        LoadDarkCol(FocusWindowColor);
        LoadDarkCol(ViewFocusWindowColor);
        LoadDarkCol(TouchEdgeColor);
        LoadDarkCol(CheckMarkColor);
        LoadDarkCol(ClipFrameLineColor);
        LoadDarkCol(CenterCrossColor);
        LoadDarkCol(ForegroundCoverColor);
        LoadDarkCol(ForegroundAltColor);
        LoadDarkCol(ForegroundMarkColor);
        LoadDarkCol(LegalLineColor);
        LoadDarkCol(LinkingLineColor);
        LoadDarkCol(IllegalLineColor);
        LoadDarkCol(ErrorTextColor);
        LoadDarkCol(FocusLineColor);
        LoadDarkCol(FrozenSecColor);
        LoadDarkCol(HiddenSecColor);
        LoadDarkCol(FrozenMaskColor);
    }

    void StyleLight()
    {
        //MessageBoxA(NULL, "Light", "!", MB_OK);
        ImGui::StyleColorsLight();
        BackgroundColor = Light::BackgroundColor;
        FocusWindowColor = Light::FocusWindowColor;
        ViewFocusWindowColor = Light::ViewFocusWindowColor;
        TouchEdgeColor = Light::TouchEdgeColor;
        CheckMarkColor = Light::CheckMarkColor;
        ClipFrameLineColor = Light::ClipFrameLineColor;
        CenterCrossColor = Light::CenterCrossColor;
        ForegroundCoverColor = Light::ForegroundCoverColor;
        ForegroundAltColor = Light::ForegroundAltColor;
        ForegroundMarkColor = Light::ForegroundMarkColor;
        LegalLineColor = Light::LegalLineColor;
        LinkingLineColor = Light::LinkingLineColor;
        IllegalLineColor = Light::IllegalLineColor;
        ErrorTextColor = Light::ErrorTextColor;
        FocusLineColor = Light::FocusLineColor;
        FrozenSecColor = Light::FrozenSecColor;
        HiddenSecColor = Light::HiddenSecColor;
        FrozenMaskColor = Light::FrozenMaskColor;

        IBB_DefaultRegType::SwitchLightColor();
    }
    void StyleDark()
    {
        //MessageBoxA(NULL, "Dark", "!", MB_OK);
        ImGui::StyleColorsDark();
        BackgroundColor = Dark::BackgroundColor;
        FocusWindowColor = Dark::FocusWindowColor;
        ViewFocusWindowColor = Dark::ViewFocusWindowColor;
        TouchEdgeColor = Dark::TouchEdgeColor;
        CheckMarkColor = Dark::CheckMarkColor;
        ClipFrameLineColor = Dark::ClipFrameLineColor;
        CenterCrossColor = Dark::CenterCrossColor;
        ForegroundCoverColor = Dark::ForegroundCoverColor;
        ForegroundAltColor = Dark::ForegroundAltColor;
        ForegroundMarkColor = Dark::ForegroundMarkColor;
        LegalLineColor = Dark::LegalLineColor;
        LinkingLineColor = Dark::LinkingLineColor;
        IllegalLineColor = Dark::IllegalLineColor;
        ErrorTextColor = Dark::ErrorTextColor;
        FocusLineColor = Dark::FocusLineColor;
        FrozenSecColor = Dark::FrozenSecColor;
        HiddenSecColor = Dark::HiddenSecColor;
        FrozenMaskColor = Dark::FrozenMaskColor;

        IBB_DefaultRegType::SwitchDarkColor();
    }
}
