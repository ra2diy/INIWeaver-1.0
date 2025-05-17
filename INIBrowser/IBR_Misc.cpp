
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
#include "FromEngine/global_tool_func.h"

bool ImGui_TextDisabled_Helper(const char* Text);
bool SmallButton_Disabled_Helper(bool cond, const char* Text);
namespace ImGui
{
    void PushOrderFront(ImGuiWindow* Window);
}

int HintStayTimeMillis = 3000;






std::shared_ptr<IBR_InputManager> NewInput(const std::string& InitialText, const std::string& id, const std::function<void(char*)>& Fn)
{
    return std::make_shared<IBR_InputManager>(InitialText, id, Fn);
}
IBR_InputManager::IBR_InputManager(const std::string& InitialText, const std::string& id, const std::function<void(char*)>& Fn) :ID(id), AfterInput(Fn)
{
    Input.reset(new char[InputSize]); 
    strcpy_s(Input.get(), InputSize, InitialText.c_str());
}
bool IBR_InputManager::RenderUI()
{
    if (ImGui::InputText(ID.c_str(), Input.get(), InputSize))
    {
        AfterInput(Input.get());
    }
    if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
    return ImGui::IsItemActive();
}

void IBR_IniLine::RenderUI(const std::string& Line, const std::string& Hint, const InitType* Init)
{
    ImGui::TextWrappedEx(Line.c_str());
    //ImGui::TextEx(Line.c_str(), nullptr, ImGuiTextFlags_NoWidthForLargeClippedText);
    if (!Hint.empty() && ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextEx(Hint.c_str());
        ImGui::EndTooltip();
    }
    if (!HasInput && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        HasInput = true;
        UseInput = false;
    }
    else if (HasInput)
    {
        if (Input)
        {
            ImGui::SameLine();
            if (Input->RenderUI())
            {
                UseInput = true;
            }
            else if (UseInput)
            {
                Input.reset();
                HasInput = false;
            }
        }
        else
        {
            Input = NewInput(Init->InitText , Init->ID, Init->AfterInput);
        }
    }
}



void IBR_IniLine::CloseInput()
{
    if(Input)Input->AfterInput(Input->Input.get());
    Input.reset();
    HasInput = false;
    UseInput = false;
}


extern const char* LinkGroup_IniName;
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




IBR_SectionData::IBR_SectionData() :
    EqPos(IBR_FullView::EqCenter), EqSize({ (float)IBG_GetSetting().FontSize * 15,(float)IBG_GetSetting().FontSize * 8 }) {}
IBR_SectionData::IBR_SectionData(const IBB_Section_Desc& D) :
    Desc(D), EqPos(IBR_FullView::EqCenter), EqSize({ (float)IBG_GetSetting().FontSize * 15,(float)IBG_GetSetting().FontSize * 8 }) {}
IBR_SectionData::IBR_SectionData(const IBB_Section_Desc& D, std::string&& Name) :
    DisplayName(std::move(Name)), Desc(D), EqPos(IBR_FullView::EqCenter), EqSize({ (float)IBG_GetSetting().FontSize * 15,(float)IBG_GetSetting().FontSize * 8 })
{
    
}



namespace IBR_RealCenter
{
    ImVec2 Center;
    dImVec2 WorkSpaceUL, WorkSpaceDR;
    bool Update()
    {
        double LWidth = ImGui::IsWindowCollapsed() ? 0.0f : ImGui::GetWindowWidth();
        WorkSpaceUL = dImVec2{ LWidth,FontHeight * 2.0 - WindowSizeAdjustY };
        WorkSpaceDR = dImVec2{ (double)IBR_UICondition::CurrentScreenWidth ,(double)IBR_UICondition::CurrentScreenHeight - FontHeight * 1.5 };
        Center = ImVec2((WorkSpaceUL + WorkSpaceDR) / 2.0);
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
    IBR_Project::id_t PrevId{ UINT64_MAX };
    bool Empty{ true };
    char EditBuf[100000];
    bool TextEditError{ false }, OnTextEdit{ false }, TextEditReset{ false };
    std::string EditingLine;
    std::unordered_map<std::string, BufferedLine> EditLines;
    
    void AFTER_INTERRUPT_F ResetEdit(IBB_Section* rsc)
    {
        EditLines.clear();
        for (auto& Sub : rsc->SubSecs)
        {
            for (auto& [K, V] : Sub.Lines)
            {
                auto& Line = EditLines[K];
                Line.Buffer = V.Data->GetString();
                Line.Known = true;
                Line.IsAltBool = V.Default->Property.TypeAlt == "bool";
                if (V.Default->Property.TypeAlt == "enum")
                {
                    Line.IsAltEnum = true;
                    Line.Enum = V.Default->Property.Enum;
                    Line.EnumValue = V.Default->Property.EnumValue;
                }
                Line.Hint = V.Default->DescLong;
            }
        }
        for (auto& [K, V] : rsc->UnknownLines.Value)
        {
            auto& Line = EditLines[K];
            Line.Buffer = V;
            Line.Known = false;
            Line.IsAltBool = false;
            Line.Hint = "";
        }
    }

    void SetActive(IBR_Project::id_t id)
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
        EditingLine.clear();
        {
            auto rsc = CurSection.GetBack();
            IBD_RInterruptF(x);
            ResetEdit(rsc);
        }
    }

    void SwitchToText()
    {
        auto rsc = CurSection.GetBack();
        if (rsc)
        {
            strcpy(EditBuf, rsc->GetText(false).c_str());
            TextEditError = false;
        }
        else TextEditError = true;

        OnTextEdit = true;
        TextEditReset = false;
    }

    void ExitTextEdit()
    {
        auto pbk = CurSection.GetBack();
        if (!pbk)return;
        OnTextEdit = false;
        IBG_Undo.SomethingShouldBeHere();
        pbk->SetText(EditBuf);
        ResetEdit(pbk);
        IBR_Inst_Project.UpdateAll();
    }

    void UpdateSection()
    {
        //TODO
    }
    void AFTER_INTERRUPT_F Modify(const std::string& s, BufferedLine& L)
    {
        auto back = CurSection.GetBack();
        auto data = CurSection.GetSectionData();
        if (L.Known)
        {
            auto Line = back->GetLineFromSubSecs(s);
            if (Line)
            {
                IBG_Undo.SomethingShouldBeHere();
                Line->Data->SetValue(L.Buffer);
                IBF_Inst_Project.UpdateAll();
            }
            else
            {
                MessageBoxW(NULL, L"back->GetLineFromSubSecs(s) == nullptr", L"IBR_EditFrame::RenderUI", MB_OK);
            }
        }
        else
        {
            back->UnknownLines.Value[s] = L.Buffer;
        }
        {
            auto it = data->ActiveLines.find(s);
            if (it != data->ActiveLines.end())
            {
                it->second.Buffer = L.Buffer;
                if(it->second.Edit.Input && it->second.Edit.Input->Input)
                    strcpy(it->second.Edit.Input->Input.get(), L.Buffer.c_str());
            }
        }
    }
    const char* a[] = { "1","2","3","4" };
    const char* x = a[0];
    void RenderUI()
    {
        if (Empty)
        {
            //TODO
            return;
        }

        IBD_RInterruptF(x);

        auto pbk = CurSection.GetBack();
        if (!pbk)
        {
            //tmd section jb die why cnmd edit fuck it
            Empty = true;
            return;
        }
        
        if (OnTextEdit)
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
            return;
        }

        if (ImGui::Button(locc("GUI_SwitchToTextEdit")))
        {
            IBR_EditFrame::SwitchToText();
            return;
        }

        for (auto& [K, V] : EditLines)
        {
            if (K == "__INHERIT__")continue;
            if (ImGui::RadioButton(("##" + K).c_str(), !pbk->OnShow[K].empty()))
            {
                IBG_Undo.SomethingShouldBeHere();
                if (pbk->OnShow[K].empty())pbk->OnShow[K] = EmptyOnShowDesc;
                else pbk->OnShow[K].clear();
            }
            ImGui::SameLine();
            if (V.Known && V.IsAltEnum)
            {
                bool Redefine = V.EnumValue.size() > 0;
                int X = -1;
                std::vector<std::string> EnumVector;
                if (Redefine)
                {
                    EnumVector = V.EnumValue;
                    for (int i = 0; i < V.EnumValue.size(); i++)
                    {
                        if (V.Buffer == V.EnumValue[i]) { X = i; break; }
                    }
                }
                else
                {
                    EnumVector = V.Enum;
                    for (int i = 0; i < V.Enum.size(); i++)
                    {
                        if (V.Buffer == V.Enum[i]) { X = i; break; }
                    }
                }
                bool EnumExist = !(X < 0);
                ImGui::TextWrapped(K.c_str());
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FontHeight);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::PushID(("##" + K).c_str());
                if (ImGui::BeginCombo(("##" + K).c_str(), EnumExist ? EnumVector[X].c_str() : V.Buffer.c_str()))
                {
                    for (int i = 0; i < EnumVector.size(); i++)
                    {
                        ImGui::PushOrderFront(ImGui::GetCurrentWindow());
                        if (ImGui::Selectable(EnumVector[i].c_str(), i == X))
                        {
                            X = i;
                            //IBG_Undo.SomethingShouldBeHere();
                            V.Buffer = EnumVector[X].c_str();
                            Modify(K, V);
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopID();
            }
            else
            {
                if ((V.Known && V.IsAltBool) || (!V.Known && (V.Buffer == "yes" || V.Buffer == "no" || V.Buffer == "true" || V.Buffer == "false")))
                {
                    V.AltRes = (V.Buffer == "yes" || V.Buffer == "true");
                    ImGui::TextWrapped(K.c_str());
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FontHeight);
                    ImGui::SameLine();
                    ImGui::Checkbox(("##" + K).c_str(), &V.AltRes);
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        IBG_Undo.SomethingShouldBeHere();
                        V.Buffer = !V.AltRes ? "yes" : "no";
                        IBR_HintManager::SetHint(V.Buffer, 1000);
                        Modify(K, V);
                    }
                }
                else
                {
                    if (V.Edit.NeedInit())
                    {
                        if (!EditingLine.empty() && EditingLine != K)
                        {
                            auto it = EditLines.find(EditingLine);
                            if (it != EditLines.end())
                            {
                                it->second.Edit.CloseInput();
                            }
                        }
                        EditingLine = K;
                        IBR_IniLine::InitType It{ V.Buffer ,"##" + RandStr(8),[Str = K](char* S)
                                 {
                                     IBG_Undo.SomethingShouldBeHere();
                                     EditLines[Str].Buffer = S;
                                     Modify(Str, EditLines[Str]);
                                 } };
                        V.Edit.RenderUI(K, V.Hint, &It);
                    }
                    else
                    {
                        if (V.Edit.HasInput)V.Edit.RenderUI(K, V.Hint);
                        else V.Edit.RenderUI(K + " = " + V.Buffer, V.Hint);
                    }
                }
            }
        }
        //TODO
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

    namespace Light
    {
        ImColor BackgroundColor(247, 236, 153, 255);//±³¾°É«

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
    }

    namespace Dark
    {
        ImColor BackgroundColor(66, 89, 92, 255);//±³¾°É«

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

        IBB_DefaultRegType::SwitchDarkColor();
    }
}
