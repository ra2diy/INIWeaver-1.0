#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include<imgui_internal.h>
#include <shlwapi.h>
#include "IBR_HotKey.h"



bool IsExistingDir(const wchar_t* Path)
{
    DWORD dwAttr = GetFileAttributesW(Path);
    return (dwAttr != INVALID_FILE_ATTRIBUTES) && (dwAttr & FILE_ATTRIBUTE_DIRECTORY);
}

namespace ImGui
{
    ImVec2 GetLineEndPos();
    ImVec2 GetLineBeginPos();
    bool IsWindowClicked(ImGuiMouseButton Button);
    void PushOrderFront(ImGuiWindow* Window);
}
extern const char* LinkGroup_IniName;
extern wchar_t CurrentDirW[];
extern int RFontHeight;

bool InRectangle(ImVec2 P, ImVec2 UL, ImVec2 DR);
std::tuple<bool, ImVec2, ImVec2> RectangleCross(ImVec2 UL1, ImVec2 DR1, ImVec2 UL2, ImVec2 DR2);

void DrawCross(ImVec2 pos, float size, ImU32 col);

dImVec2 operator+(const dImVec2 a, const dImVec2 b);
dImVec2 operator-(const dImVec2 a, const dImVec2 b);
ImVec4 operator+(const ImVec4 a, const ImVec4 b);
dImVec2 operator/(const dImVec2 a, const double b);
dImVec2 operator*(const dImVec2 a, const double b);
dImVec2& operator+=(dImVec2& a, const dImVec2 b);
dImVec2& operator-=(dImVec2& a, const dImVec2 b);
dImVec2& operator/=(dImVec2& a, const double b);
dImVec2& operator*=(dImVec2& a, const double b);


namespace IBR_RealCenter
{
    ImRect GetWorkSpaceRect();
}


namespace SearchModuleAlt
{
    std::vector<IBB_ModuleAlt*> Arr;
    bool ConsiderRegName, ConsiderDescName, ConsiderDesc;
    BufString InputBuf;
    std::vector<IBB_ModuleAlt*>& Get() { return Arr; };
    void Update(std::vector<IBB_ModuleAlt*>&& Mod);
    void RenderUI();


    void RenderModuleAltSelect(IBB_ModuleAlt* pModule)
    {
        ImRect R{ ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2{ ImGui::GetWindowWidth(), ImGui::GetTextLineHeightWithSpacing() } };
        ImGui::Text(pModule->DescShort.c_str());
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text(pModule->DescLong.c_str());
            ImGui::EndTooltip();
        }
        //ImGui::SameLine();
        //ImGui::SetCursorPosX(FontHeight * 8.0f);
        /*
        if (ImGui::SmallButton((u8"属性##" + pModule->Name).c_str()))
        {
            IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}.CreateModal(pModule->DescShort, true).SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize).PushMsgBack([pModule]()
                {
                    ImGui::SetWindowSize({ RFontHeight * 20.0f,RFontHeight * 15.0f });
                    ImGui::Text(pModule->DescLong.c_str());
                    ImGui::NewLine();
                    if (!pModule->ParamDescLong.empty() || !pModule->ParamDescShort.empty())ImGui::Text(u8"参数：%s", pModule->ParamDescShort.c_str());
                    if(!pModule->ParamDescLong.empty())ImGui::Text(pModule->ParamDescLong.c_str());

                    ImGui::Text(u8"文件路径：%s", UnicodetoUTF8(pModule->Path).c_str());
                    if (ImGui::SmallButton(u8"复制路径"))
                    {
                        ImGui::SetClipboardText(UnicodetoUTF8(pModule->Path).c_str());
                        IBR_HintManager::SetHint(u8"路径复制成功", HintStayTimeMillis);
                    }

                })));
        }
        */
        //ImGui::SameLine();
        //ImGui::SmallButton((u8"添加##" + pModule->Name).c_str());
        if(R.Contains(ImGui::GetMousePos()) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            IBR_Inst_Project.AddModule(*pModule, GenerateModuleTag());
    }

        IBR_ListMenu<IBB_ModuleAlt*> DoubleClickTable{ SearchModuleAlt::Get(), u8"DoubleClick_Module",
        [](IBB_ModuleAlt*& pModule,int,int)
        {
            RenderModuleAltSelect(pModule);
        } };

        void Update(std::vector<IBB_ModuleAlt*> && Mod)
        {
            SearchModuleAlt::Arr = std::move(Mod);
            DoubleClickTable.Rewind();
        }

        void RenderUI()
        {
            ImGui::Text(locc("GUI_Search_Title"));
            bool ToUpdate = false;
            if (!Arr.size())
            {
                ConsiderRegName = true;
                ConsiderDescName = true;
                ConsiderDesc = true;
                ToUpdate = true;
            }
            if (ImGui::Checkbox(locc("GUI_Search_Name"), &ConsiderDescName))ToUpdate = true;
            ImGui::SameLine();
            if (ImGui::Checkbox(locc("GUI_Search_RegName"), &ConsiderRegName))ToUpdate = true;
            ImGui::SameLine();
            if (ImGui::Checkbox(locc("GUI_Search_Desc"), &ConsiderDesc))ToUpdate = true;
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            if (ImGui::InputText(u8"##SEARCH", InputBuf, sizeof(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                ToUpdate = true;
            }
            if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
            ImGui::Text(locc("GUI_Search_Tip"));
            /*
            auto P = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2{ float(FontHeight),float(FontHeight) });
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                InputBuf[0] = 0;
            DrawCross(P, FontHeight, IBR_Color::ErrorTextColor);
            */
            if (ToUpdate)Update(IBB_ModuleAltDefault::Search(InputBuf, ConsiderRegName, ConsiderDescName, ConsiderDesc));
            void DDCC();
            DDCC();
        }
    }


extern std::atomic_bool LoadDatabaseComplete;

namespace IBR_WorkSpace
{
    ImVec2 EqCenterPrev, ReCenterPrev;
    float RatioPrev;
    bool IsBgDragging;
    ImVec2 DragStartMouse, DragStartEqCenter, DragStartReCenter, InitialMassCenter;
    ImVec2 ExtraMove, MousePosExt, DragStartEqMouse, DragCurMouse, DragCurEqMouse;
    int UpdatePrevResult;
    bool LastClickable{ false }, LastOnWindow{ true }, LastCont{ false }, Cont{ false };
    bool ShowRegName{ false };
    float NewRatio;
    bool NeedChangeRatio;
    bool HoldingModules{ false };
    bool InitHolding{ false };
    bool IsMassSelecting{ false };
    bool IsMassAfter{ false };
    bool HasRightDownToWait{ false };
    bool HasLefttDownToWait{ false };
    bool MoveAfterMass{ false };
    std::vector<IBR_Project::id_t> MassTarget;
    bool LastOperateOnText{ false };
    bool OperateOnText{ false };

    
    void RenderRightClickTable()
    {
        if (!LoadDatabaseComplete)
        {
            ImGui::TextDisabled(locc("GUI_ModuleDataLoading"));
        }
        else
        {
            ImGui::BeginChild("##RightClick_Module", { FontHeight * 10.0F,FontHeight * 14.0F }, false);
            IBB_ModuleAltDefault::Tree_RenderUI();
            ImGui::EndChild();
        }
    }

    bool IsResizingWindow()
    {
        return false;
    }

    void UpdateEdge(const ImVec2& ExpectedEqCenter)
    {
        auto XR = IBR_FullView::EqXRange(ExpectedEqCenter), YR = IBR_FullView::EqYRange(ExpectedEqCenter);
        auto DList = ImGui::GetForegroundDrawList();
        auto DT = IBR_RealCenter::GetWorkSpaceRect();
        ImU32 UX = IBR_Color::TouchEdgeColor;
        UX &= 0xFFFFFF;
        if (XR == 1)for (int i = 0; i < 64; i++)DList->AddLine({ DT.Max.x - i,DT.Min.y }, { DT.Max.x - i,DT.Max.y }, UX | ((64 - i) << 26));
        if (XR == -1)for (int i = 0; i < 64; i++)DList->AddLine({ DT.Min.x + i,DT.Min.y }, { DT.Min.x + i,DT.Max.y }, UX | ((64 - i) << 26));
        if (YR == 1)for (int i = 0; i < 64; i++)DList->AddLine({ DT.Min.x,DT.Max.y - i}, { DT.Max.x,DT.Max.y - i }, UX | ((64 - i) << 26));
        if (YR == -1)for (int i = 0; i < 64; i++)DList->AddLine({ DT.Min.x,DT.Min.y + i }, { DT.Max.x,DT.Min.y + i }, UX | ((64 - i) << 26));
    }
    void UpdateScroll(const ImVec2& MousePos)
    {
        auto ScrollRate = IBF_Inst_Setting.ScrollRate();
        auto V = (MousePos - DragStartMouse) * ScrollRate - ExtraMove;
        auto F = (float)FontHeight;
        ImVec2 ExpectedEqCenter = DragStartEqCenter - V / IBR_FullView::Ratio;
        if (IBR_FullView::EqPosInRange(ExpectedEqCenter))
        {
            ExtraMove.x += ScrollRate * std::min((float)std::max(FontHeight - MousePos.x + IBR_RealCenter::WorkSpaceUL.x, 0.0), F);
            ExtraMove.x -= ScrollRate * std::min((float)std::max(FontHeight + MousePos.x - IBR_RealCenter::WorkSpaceDR.x, 0.0), F);
            ExtraMove.y += ScrollRate * std::min((float)std::max(FontHeight - MousePos.y + IBR_RealCenter::WorkSpaceUL.y, 0.0), F);
            ExtraMove.y -= ScrollRate * std::min((float)std::max(FontHeight + MousePos.y - IBR_RealCenter::WorkSpaceDR.y, 0.0), F);
        }
        UpdateEdge(ExpectedEqCenter);
        IBR_FullView::EqPosFixRange(ExpectedEqCenter);
        IBR_FullView::EqCenter = ExpectedEqCenter;
        IBR_RealCenter::Center = DragStartReCenter - V;
    }
    ImVec2 UpdateScrollGeneral(const ImVec2& MousePos)
    {
        auto ScrollRate = IBF_Inst_Setting.ScrollRate();
        auto F = (float)FontHeight;
        ImVec2 ET{};
        if (IBR_FullView::EqPosInRange(IBR_FullView::EqCenter))
        {
            ET.x += ScrollRate * std::min((float)std::max(FontHeight - MousePos.x + IBR_RealCenter::WorkSpaceUL.x, 0.0), F);
            ET.x -= ScrollRate * std::min((float)std::max(FontHeight + MousePos.x - IBR_RealCenter::WorkSpaceDR.x, 0.0), F);
            ET.y += ScrollRate * std::min((float)std::max(FontHeight - MousePos.y + IBR_RealCenter::WorkSpaceUL.y, 0.0), F);
            ET.y -= ScrollRate * std::min((float)std::max(FontHeight + MousePos.y - IBR_RealCenter::WorkSpaceDR.y, 0.0), F);
        }
        auto ExpectedEqCenter = IBR_FullView::EqCenter - ET / IBR_FullView::Ratio;
        UpdateEdge(ExpectedEqCenter);
        IBR_RealCenter::Center = IBR_RealCenter::Center - ET;
        IBR_FullView::EqCenter = ExpectedEqCenter;
        IBR_FullView::EqPosFixRange(IBR_FullView::EqCenter);
        return ET;
    }
    void UpdateScrollAlt(const ImVec2& MousePos)
    {
        auto ET = UpdateScrollGeneral(MousePos);
        auto MouseEq = RePosToEqPos(MousePos);
        auto DeltaEq = ET / -IBR_FullView::Ratio;
        for (auto& [ID, Data] : IBR_Inst_Project.IBR_SectionMap)
            if (Data.Dragging)
                Data.EqPos = MouseEq + Data.EqDelta;
    }
    void UpdateScrollMassSelect(const ImVec2& MousePos)
    {
        auto ET = UpdateScrollGeneral(MousePos);
        auto MouseEq = RePosToEqPos(MousePos);
        DragCurEqMouse = MouseEq;
        DragCurMouse = MousePos;
    }

    void Close()
    {
        EqCenterPrev = IBR_FullView::EqCenter = { 0.0f,0.0f };
        RatioPrev = IBR_FullView::Ratio = 1.0F;
        LastClickable = false;
        LastOnWindow = true;
        LastCont = false;
        Cont = false;
        NeedChangeRatio = false;
        HoldingModules = false;
        InitHolding = false;
        IsMassSelecting = false;
        IsMassAfter = false;
        HasRightDownToWait = false;
        HasLefttDownToWait = false;
        MoveAfterMass = false;
        IsBgDragging = false;
    }

    void UpdateNewRatio()
    {
        if (NeedChangeRatio)
        {
            if (IBR_FullView::Ratio < NewRatio)//放大
            {
                auto OrigSize = IBR_RealCenter::WorkSpaceDR - IBR_RealCenter::WorkSpaceUL;
                auto NewSize = OrigSize * IBR_FullView::Ratio / NewRatio;
                ImVec2 NewCenter;
                NewCenter.y = std::max((float)(IBR_RealCenter::WorkSpaceUL.y + NewSize.y * 0.5f), MousePosExt.y);
                NewCenter.y = std::min((float)(IBR_RealCenter::WorkSpaceDR.y - NewSize.y * 0.5f), NewCenter.y);
                NewCenter.x = std::max((float)(IBR_RealCenter::WorkSpaceUL.x + NewSize.x * 0.5f), MousePosExt.x);
                NewCenter.x = std::min((float)(IBR_RealCenter::WorkSpaceDR.x - NewSize.x * 0.5f), NewCenter.x);
                IBR_RealCenter::Center = NewCenter;
            }
            IBR_FullView::Ratio = NewRatio;
        }
    }

    bool SelectedAllIgnored()
    {
        for (auto& v : MassTarget)
        {
            auto sc = IBR_Inst_Project.GetSectionFromID(v);
            auto Data = sc.GetSectionData();
            if (Data && !Data->Ignore)
            {
                return false;
            }
        }
        return true;
    }
    void IgnoreSelected()
    {
        for (auto& v : MassTarget)
        {
            auto sc = IBR_Inst_Project.GetSectionFromID(v);
            auto Data = sc.GetSectionData();
            if (Data)
            {
                Data->Ignore = true;
            }
        }
    }
    void NoIgnoreSelected()
    {
        for (auto& v : MassTarget)
        {
            auto sc = IBR_Inst_Project.GetSectionFromID(v);
            auto Data = sc.GetSectionData();
            if (Data)
            {
                Data->Ignore = false;
            }
        }
    }
    void DeleteSelected()
    {
        IBRF_CoreBump.SendToR({ [=]() {
            IBG_Undo.SomethingShouldBeHere();
            IBR_Inst_Project.DeleteSection(MassTarget);
            },nullptr });
    }
    void GenerateClipDataFromMassSelect(IBB_ClipBoardData& ClipData)
    {
        std::vector<IBB_Section_Desc> Sel;
        dImVec2 TSum{};
        double Sum{};
        for (auto& v : MassTarget)
        {
            auto sc = IBR_Inst_Project.GetSectionFromID(v);
            auto Data = sc.GetSectionData();
            if (Data)
            {
                TSum += Data->EqPos * Data->EqSize.x * Data->EqSize.y;
                Sum += Data->EqSize.x * Data->EqSize.y;
                Sel.push_back(Data->Desc);
            }
        }
        TSum /= Sum;
        for (auto& v : MassTarget)
        {
            auto sc = IBR_Inst_Project.GetSectionFromID(v);
            auto Data = sc.GetSectionData();
            if (Data)
            {
                Data->EqDelta = Data->EqPos - TSum;
            }
        }
        ClipData.Generate(Sel);
    }
    void CopySelected()
    {
        IBB_ClipBoardData ClipData;
        GenerateClipDataFromMassSelect(ClipData);
        ImGui::SetClipboardText(ClipData.GetString().c_str());
        auto c = ClipData.Modules.size();
        IBR_HintManager::SetHint(UnicodetoUTF8(std::vformat(locw("GUI_CopySuccess"), std::make_wformat_args(c))), HintStayTimeMillis);
    }
    void CutSelected()
    {
        CopySelected();
        IsMassAfter = false;
        IsBgDragging = false;
        DeleteSelected();
    }
    void Paste()
    {
        auto Str = ImGui::GetClipboardText();
        IBB_ClipBoardData ClipData;
        if (ClipData.SetString(Str))
        {
            //Check ClipData.ProjectRID
            auto X = IBR_Inst_Project.AddModule(ClipData.Modules);
            if (X)
            {
                auto c = X.value().size();
                IBR_HintManager::SetHint(UnicodetoUTF8(std::vformat(locw("GUI_PasteSuccess"), std::make_wformat_args(c))), HintStayTimeMillis);
                MassSelect(X.value());
            }
            else IBR_HintManager::SetHint(loc("GUI_PasteFailed"), HintStayTimeMillis);
        }
        else IBR_HintManager::SetHint(loc("GUI_PasteFailed"), HintStayTimeMillis);
    }
    void MassSelect(const std::vector<IBR_Project::id_t>& Target)
    {
        IsBgDragging = true;
        IsMassSelecting = false;
        IsMassAfter = true;
        DragStartEqMouse = IBR_FullView::GetEqMin();
        DragCurEqMouse = IBR_FullView::GetEqMax();
        MassTarget = Target;
        IBR_PopupManager::ClearRightClickMenu();
    }
    void SelectAll()
    {
        IsBgDragging = true;
        IsMassSelecting = false;
        IsMassAfter = true;
        DragStartEqMouse = IBR_FullView::GetEqMin();
        DragCurEqMouse = IBR_FullView::GetEqMax();
        MassTarget.clear();
        for (auto& [D, I] : IBR_Inst_Project.IBR_Rev_SectionMap)
            MassTarget.push_back(I);
        IBR_PopupManager::ClearRightClickMenu();
    }

    void RightClickTextHelper(const char* ss)
    {
        auto Pos = ImGui::GetCursorPos();
        ImGui::Text(ss);
        ImGui::SetCursorPos(Pos);
        ImGui::Dummy(ImVec2{ FontHeight * 9.7f, ImGui::GetTextLineHeight() });
    }
    void OpenRightClick()
    {
        IBB_ModuleAltDefault::Tree_ResetHover();
        IBR_PopupManager::SetRightClickMenu(std::move(
            IBR_PopupManager::Popup{}.Create(RandStr(8))
            .SetFlag(ImGuiWindowFlags_NoFocusOnAppearing)
            .EnableInstantClose()
            .PushMsgBack([]() {

                //为什么这里检测Drag，我不知道，我也不敢动，虽然说这段代码写下的时间距离这行注释写下的时间只有1小时
                /*
                if (abs((DragCurMouse - DragStartMouse).max()) > 2.0f)
                {
                    MassTarget = IBR_SelectMode::GetMassSelected();
                }
                */
                if (IBR_Inst_Project.IBR_SectionMap.empty())
                {
                    ImGui::TextDisabled(locc("GUI_SelectAll"));
                }
                else
                {
                    RightClickTextHelper(locc("GUI_SelectAll"));
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        SelectAll();
                        IBR_PopupManager::ClearRightClickMenu();
                    }
                }
                RightClickTextHelper(locc("GUI_Paste"));
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    Paste();
                    IBR_PopupManager::ClearRightClickMenu();
                }
                ImGui::Text(locc("GUI_CreateCommentBlock"));
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    auto EqMouse = RePosToEqPos(ImGui::GetMousePos());
                    IBRF_CoreBump.SendToR({ [=]() {IBR_Inst_Project.CreateCommentBlock(EqMouse); } });
                    IBR_PopupManager::ClearRightClickMenu();
                }
                ImGui::PopStyleColor();
                RenderRightClickTable();
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_PopupBg));
                })), ImGui::GetMousePos());
    }
    void OutputSelectedImpl(const char* IPath, const char* IDescShort, const char* IDescLong)
    {
        IBB_ModuleAlt Alt;
        IBB_ClipBoardData ClipData;
        /*
        std::vector<IBB_Section_Desc> Sel;
        for (auto& v : MassTarget)
        {
            auto Data = IBR_Inst_Project.GetSectionFromID(v).GetSectionData();
            if (Data && !Data->Ignore)Sel.push_back(Data->Desc);
        }
        ClipData.Generate(Sel);
        */
        GenerateClipDataFromMassSelect(ClipData);
        Alt.Available = true;
        Alt.Name = IDescShort;
        Alt.DescShort = IDescShort;
        Alt.DescLong = IDescLong;
        Alt.ParamDescShort = Alt.ParamDescLong = "";
        Alt.Parameter = "****";
        Alt.Path = UTF8toUnicode(IPath);
        Alt.Modules = std::move(ClipData.Modules);
        Alt.SaveToFile();
        IBB_ModuleAltDefault::NewModule(std::move(Alt));
    }
    void OutputSelected()
    {
        char* IDescShort = new BufString{};
        char* IDescLong = new BufString{};
        char* IPath = new BufString;
        //auto PPath = UnicodetoUTF8(IBB_ModuleAltDefault::GenerateModulePath_NoName());
        auto WPath = IBB_ModuleAltDefault::GenerateModulePath();
        auto WD = WPath; PathRemoveFileSpecW(WD.data());
        bool OK1 = IsExistingDir(WD.c_str()) ,OK2 = !PathFileExistsW(WPath.c_str());
        strcpy(IPath, UnicodetoUTF8(WPath).c_str());
        auto PF = [=]
            {
                delete[]IDescShort;
                delete[]IDescLong;
                delete[]IPath;
            };
        IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}.CreateModal(locc("GUI_OutputModule_Title"), true, PF)
            .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize).PushMsgBack([=]() mutable
                {
                    ImGui::SetWindowSize({ FontHeight * 20.0f,FontHeight * 10.0f });
                    ImGui::InputText(locc("GUI_OutputModule_Name"), IDescShort, sizeof(BufString));
                    if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
                    ImGui::InputText(locc("GUI_OutputModule_Desc"), IDescLong, sizeof(BufString));
                    if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
                    if (ImGui::InputText(locc("GUI_OutputModule_OutputPath"), IPath, sizeof(BufString)))
                    {
                        WPath = UTF8toUnicode(IPath);
                        WD = WPath; PathRemoveFileSpecW(WD.data());
                        OK1 = IsExistingDir(WD.c_str());
                        OK2 = !PathFileExistsW(WPath.c_str());
                    }
                    if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
                    ImGui::SameLine();
                    if (ImGui::SmallButton("..."))
                    {
                        auto T1 = locw("GUI_OutputModule_Type1");
                        auto T2 = locw("GUI_OutputModule_Type2");
                        auto L1 = wcslen(L"*.ini");
                        auto L2 = wcslen(L"*.*");
                        std::wstring wbuf;
                        wbuf.resize(T1.size() + T2.size() + 32, 0);
                        memcpy(wbuf.data(), T1.c_str(), T1.size() * sizeof(wchar_t));
                        memcpy(wbuf.data() + (T1.size() + 1), L"*.ini", L1 * sizeof(wchar_t));
                        memcpy(wbuf.data() + (T1.size() + L1 + 2), T2.c_str(), T2.size() * sizeof(wchar_t));
                        memcpy(wbuf.data() + (T1.size() + L1 + T2.size() + 3), L"*.*", L2 * sizeof(wchar_t));
                        auto Ret = InsertLoad::SelectFileName(MainWindowHandle,
                            InsertLoad::SelectFileType{ WD.c_str() ,locw("GUI_OutputModule_Title"),
                            PathFindFileNameW(WPath.c_str()), wbuf.c_str() }, ::GetSaveFileNameW, false);
                        if (Ret.Success)
                        {
                            WPath = Ret.RetBuf;
                            strcpy(IPath, UnicodetoUTF8(WPath).c_str());
                            WD = WPath; PathRemoveFileSpecW(WD.data());
                            OK1 = IsExistingDir(WD.c_str());
                            OK2 = !PathFileExistsW(WPath.c_str());
                        }
                    }
                    auto len = strlen(IDescShort);
                    if (OK1 && OK2 && len)
                    {
                        if (ImGui::Button(locc("GUI_OK")))
                        {
                            OutputSelectedImpl(IPath, IDescShort, IDescLong);
                            delete[]IDescShort;
                            delete[]IDescLong;
                            delete[]IPath;
                            IBR_PopupManager::ClearPopupDelayed();
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled(locc("GUI_OK"));
                        ImGui::SameLine();
                        if (!len)
                        {
                            ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_OutputModule_Error1"));
                            ImGui::SameLine();
                        }
                        if (!OK2)
                        {
                            ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_OutputModule_Error2"));
                            ImGui::SameLine();
                        }
                        if (!OK1)
                        {
                            ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_OutputModule_Error3"));
                            ImGui::SameLine();
                        }
                    }
                })));
    }

    void MoveToCenter()
    {
        //move repos
        IBR_FullView::EqCenter = { 0.0f, 0.0f };
    }

    /*
     状态机:
     Normal -> BgDragging MassSelecting HoldingModules
     BgDragging -> Normal
     HoldingModules -> Normal
     MassSelecting -> MassAfter Normal
     MassAfter -> HoldingModules Normal
    */
    //画布上的键鼠动作
    void ProcessBackgroundOpr()
    {
        if (IsResizingWindow())return;
        auto MousePos = ImGui::GetMousePos();
        bool OnWindow = false;
        Cont = IBR_RealCenter::GetWorkSpaceRect().Contains(MousePos);
        for (auto w : ImGui::GetCurrentContext()->Windows)
        {
            if((w->Flags & ImGuiWindowFlags_Tooltip) ||
                (w->Flags & ImGuiWindowFlags_ChildWindow) ||
                w->Hidden ||
                (w->Flags & ImGuiWindowFlags_Popup) ||
                w->IsFallbackWindow ||
                !w->LastFrameRendered
                )continue;


            //ImGui::GetForegroundDrawList()->AddText(w->Rect().Min, IBR_Color::IllegalLineColor, w->Name);
            //ImGui::GetForegroundDrawList()->AddRect(w->Rect().Min, w->Rect().Max, IBR_Color::FocusWindowColor, 2.0F, 0, 3.0F);
            if (w->Rect().Contains(MousePos))
            {
                OnWindow = true;
                break;
            }
        }
        if (Cont && LastCont && !IBR_PopupManager::IsMouseOnPopup() && !IsEnumHovered)
        {
            auto DeltaWheel = ImGui::GetIO().MouseWheel;
            if (abs(DeltaWheel) > 1e-6f)
            {
                NewRatio = floor(IBR_FullView::Ratio * exp(DeltaWheel / 9.0f) * 20.0f) / 20.0f;
                if (DeltaWheel < 0)NewRatio += 0.05f;
                if (abs(NewRatio - IBR_FullView::Ratio) < 1e-6f)
                    NewRatio += DeltaWheel < 0 ? -0.05f : 0.05f;
                NewRatio = std::min(NewRatio, IBR_FullView::RatioMax / 100.0f);
                NewRatio = std::max(NewRatio, IBR_FullView::RatioMin / 100.0f);
                MousePosExt = ImGui::GetMousePos();
                NeedChangeRatio = true;
            }
            else NeedChangeRatio = false;
        }


        if (IsHotKeyPressed(Paste))
        {
            Paste();
        }
        else if (IsHotKeyPressed(Center))
        {
            MoveToCenter();
        }
        else if (IsHotKeyPressed(SelectAll))
        {
            SelectAll();
        }
        else if (IsHotKeyPressed(Refresh))
        {
            IBR_Inst_Project.UpdateAll();
        }
        else if (IsHotKeyPressed(DeleteAll))
        {
            IBRF_CoreBump.SendToR({ [=]() {
                IBG_Undo.SomethingShouldBeHere();
                std::vector<IBB_Section_Desc>All;
                All.reserve(IBR_Inst_Project.IBR_SectionMap.size());
                for (auto& [K, V] : IBR_Inst_Project.IBR_SectionMap)
                {
                    if (V.Desc.Ini.empty() || V.Desc.Sec.empty())continue;
                    All.push_back(V.Desc);
                }
                IBR_Inst_Project.DeleteSection(All);
            },nullptr });
        }
        else if (IsHotKeyPressed(SwitchDisplayMode))ShowRegName ^= 1;
        else if (!OnWindow && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && Cont)
        {
            //IsBgDragging = false;
            IBR_PopupManager::SetRightClickMenu(std::move(
                IBR_PopupManager::Popup{}.Create(RandStr(8)).UseMyStyle().PushMsgBack([]() {
                    SearchModuleAlt::RenderUI();
                    //ControlPanel_ModuleAlt();
                    })), ImGui::GetMousePos());
        }
        else if ((LastClickable && !IsBgDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)))
        {
            
            if (!OnWindow && MousePos.x != -FLT_MAX &&
                ImGui::GetCurrentContext()->MouseCursor != ImGuiMouseCursor_ResizeEW/*//TODO: WHAT THE FUCK ?!?!?!?!*/)
            {
                IsBgDragging = true;
                DragStartMouse = MousePos;
                DragStartEqCenter = IBR_FullView::EqCenter;
                DragStartReCenter = IBR_RealCenter::Center;
                ExtraMove = { 0.0f,0.0f };
            }
        }
        else if ((LastClickable && !IsBgDragging && ImGui::IsMouseDown(ImGuiMouseButton_Right)))
        {
            if (/*!OnWindow && */MousePos.x != -FLT_MAX)
            {
                IsBgDragging = true;
                IsMassSelecting = true;
                DragCurMouse = DragStartMouse = MousePos;
                DragCurEqMouse = DragStartEqMouse = RePosToEqPos(MousePos);
            }
        }
        else if (Cont && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            OpenRightClick();
        }
        else if (HoldingModules && !InitHolding)
        {
            IsBgDragging = true;
            dImVec2 TSum{};
            double Sum{};
            for (auto& [ID, Data] : IBR_Inst_Project.IBR_SectionMap)
                if (Data.Dragging)
                {
                    TSum += Data.EqPos * Data.EqSize.x * Data.EqSize.y;
                    Sum += Data.EqSize.x * Data.EqSize.y;
                }
            TSum /= Sum;
            InitialMassCenter = TSum;
            for (auto& [ID, Data] : IBR_Inst_Project.IBR_SectionMap)
                if (Data.Dragging)
                    Data.EqDelta = Data.EqPos - TSum;
            InitHolding = true;
        }
        else if (IsBgDragging && MousePos.x != -FLT_MAX)
        {
            if (HoldingModules)
            {
                if (Cont)
                {
                    if (!HasLefttDownToWait && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        IsBgDragging = false;
                        HoldingModules = false;
                        InitHolding = false;
                        MoveAfterMass = false;
                        for (auto& [ID, Data] : IBR_Inst_Project.IBR_SectionMap)
                            Data.Dragging = false;
                    }
                    else if (!HasRightDownToWait && ImGui::IsMouseDown(ImGuiMouseButton_Right))
                    {
                        IsBgDragging = false;
                        HoldingModules = false;
                        InitHolding = false;
                        if (MoveAfterMass)
                        {
                            MoveAfterMass = false;
                            for (auto& p : IBR_Inst_Project.IBR_SectionMap)
                            {
                                if (p.second.Dragging)
                                {
                                    //p.second.EqPos = InitialMassCenter + p.second.EqDelta;
                                    p.second.Dragging = false;
                                }
                            }
                        }
                        else
                        {
                            IBRF_CoreBump.SendToR({ []() {
                            std::vector<IBB_Section_Desc> Descs;
                            for (auto& p : IBR_Inst_Project.IBR_SectionMap)
                                if (p.second.Dragging)Descs.push_back(p.second.Desc);
                            IBR_Inst_Project.DeleteSection(Descs);
                            IBF_Inst_Project.UpdateAll();
                                }, nullptr });
                        }
                    }
                    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                    {
                        IBR_EditFrame::Clear();
                        if(IBR_Inst_Menu.GetMenuItem()==MenuItemID_EDIT)
                            IBR_Inst_Menu.ChooseMenu(MenuItemID_FILE);

                        HasLefttDownToWait = false;
                        if (MoveAfterMass)
                        {
                            IsBgDragging = false;
                            HoldingModules = false;
                            InitHolding = false;
                            MoveAfterMass = false;
                            for (auto& [ID, Data] : IBR_Inst_Project.IBR_SectionMap)
                                Data.Dragging = false;
                        }
                    }
                    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    {
                        HasRightDownToWait = false;
                    }
                    else UpdateScrollAlt(MousePos);
                }
            }
            else if (IsMassSelecting)
            {
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                {
                    IsMassSelecting = false;
                    if (abs((DragCurMouse - DragStartMouse).max()) > 2.0f)
                    {
                        MassTarget = IBR_SelectMode::GetMassSelected();
                        if (MassTarget.empty())
                        {
                            IsMassAfter = false;
                            IsBgDragging = false;
                        }
                        else IsMassAfter = true;
                    }
                    else
                    {
                        IsBgDragging = false;
                        OpenRightClick();
                    }
                }
                else UpdateScrollMassSelect(MousePos);
            }
            else if (IsMassAfter)
            {
                if (IsHotKeyPressed(Delete))
                {
                    IsMassAfter = false;
                    IsBgDragging = false;
                    DeleteSelected();
                }
                else if (IsHotKeyPressed(Copy))
                {
                    CopySelected();
                }
                else if (IsHotKeyPressed(Cut))
                {
                    CutSelected();
                }

                if (!OnWindow && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !IBR_PopupManager::HasRightClickMenu)
                {
                    IsMassAfter = false;
                    IsBgDragging = false;
                }
                else if (!OnWindow && ImGui::IsMouseDown(ImGuiMouseButton_Right))
                {
                    IBR_PopupManager::SetRightClickMenu(std::move(
                        IBR_PopupManager::Popup{}.Create(RandStr(8)).PushMsgBack([]() {
                            if (ImGui::SmallButtonAlignLeft(locc("GUI_Copy"), ImVec2{FontHeight * 7.0f, ImGui::GetTextLineHeight()}))
                            {
                                CopySelected();
                                IBR_PopupManager::ClearRightClickMenu();
                            }
                            if (ImGui::SmallButtonAlignLeft(locc("GUI_Cut"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                            {
                                CutSelected();
                                IBR_PopupManager::ClearRightClickMenu();
                            }
                            if (ImGui::SmallButtonAlignLeft(locc("GUI_Paste"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                            {
                                IsMassAfter = false;
                                IsBgDragging = false;
                                Paste();
                                IBR_PopupManager::ClearRightClickMenu();
                            }
                            if (SelectedAllIgnored())
                            {
                                if (ImGui::SmallButtonAlignLeft(locc("GUI_IgnoreNone"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                                {
                                    NoIgnoreSelected();
                                    IBR_PopupManager::ClearRightClickMenu();
                                }
                            }
                            else
                            {
                                if (ImGui::SmallButtonAlignLeft(locc("GUI_IgnoreAll"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                                {
                                    IgnoreSelected();
                                    IBR_PopupManager::ClearRightClickMenu();
                                }
                            }
                            
                            if (ImGui::SmallButtonAlignLeft(locc("GUI_ExportModule"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                            {
                                OutputSelected();
                                IBR_PopupManager::ClearRightClickMenu();
                            }

                            if (ImGui::SmallButtonAlignLeft(locc("GUI_Delete"), ImVec2{ FontHeight * 7.0f, ImGui::GetTextLineHeight() }))
                            {
                                IsMassAfter = false;
                                IsBgDragging = false;
                                DeleteSelected();
                                IBR_PopupManager::ClearRightClickMenu();
                            }
                            })), ImGui::GetMousePos());
                }
                else if (OnWindow && Cont)
                {
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
                    {
                        IsMassAfter = false;
                        HasRightDownToWait = true;
                        IBR_Inst_Project.CopyTransform.clear();
                        //MessageBoxA(NULL, std::to_string(MassTarget.size()).c_str(), "!", MB_OK);
                        std::set<IBB_Section_Desc>DS;
                        for (auto& v : MassTarget)
                        {
                            auto sc = IBR_Inst_Project.GetSectionFromID(v);
                            auto pd = sc.GetDesc();
                            if (pd)DS.insert(*pd);
                        }
                        for (auto& ini : IBF_Inst_Project.Project.Inis)
                        {
                            if (ini.Secs_ByName.empty())continue;
                            for (auto& sec : ini.Secs)
                            {
                                IBB_Section_Desc DescOrig = { ini.Name,sec.second.Name };
                                if (DS.count(DescOrig))
                                    IBR_Inst_Project.CopyTransform[sec.second.Name] = GenerateModuleTag();
                            }
                            for (auto& sec : ini.Secs)
                            {
                                IBB_Section_Desc DescOrig = { ini.Name,sec.second.Name };
                                if (DS.count(DescOrig))
                                    IBRF_CoreBump.SendToR({ [=]()
                                        {
                                            IBB_Section_Desc desc = { ini.Name,IBR_Inst_Project.CopyTransform[sec.second.Name]};
                                            IBR_Inst_Project.GetSection(DescOrig).DuplicateSection(desc);
                                            auto rsc = IBR_Inst_Project.GetSection(desc);
                                            auto rsc_orig = IBR_Inst_Project.GetSection(DescOrig);
                                            auto& rsd = *rsc.GetSectionData();
                                            auto& rsd_orig = *rsc_orig.GetSectionData();
                                            rsd.RenameDisplayImpl(rsd_orig.DisplayName);
                                            rsd.EqPos = rsd_orig.EqPos + dImVec2{2.0 * FontHeight, 2.0 * FontHeight};
                                            rsd.EqSize = rsd_orig.EqSize;
                                            rsd.Ignore = rsd_orig.Ignore;
                                            rsd.IsComment = rsd_orig.IsComment;
                                            if (rsd.IsComment)
                                            {
                                                rsd.CommentEdit = std::make_shared<BufString>();
                                                strcpy(rsd.CommentEdit.get(), rsd_orig.CommentEdit.get());
                                            }
                                            rsd.Dragging = true;
                                        },nullptr });
                                //见V0.2.0任务清单（四）第75条“涉及字段数目变化的指令应借由IBF_SendToR等提至主循环开头”
                            }
                        }
                        IBRF_CoreBump.SendToR({ [=]() {IBF_Inst_Project.UpdateAll(); IBR_WorkSpace::HoldingModules = true;  },nullptr });
                    }
                    else if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        for (auto& v : MassTarget)
                        {
                            auto sc = IBR_Inst_Project.GetSectionFromID(v);
                            auto pd = sc.GetSectionData();
                            if (pd)pd->Dragging = true;
                        }
                        HoldingModules = true;
                        MoveAfterMass = true;
                        HasLefttDownToWait = true;
                        IsMassAfter = false;
                        //MessageBoxA(NULL, "!!", "!!!", MB_OK);
                    }
                }
                else UpdateScrollGeneral(MousePos);
            }
            else//BgDragging
            {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    UpdateScroll(MousePos);
                }
                else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    IBR_EditFrame::Clear();
                    if (IBR_Inst_Menu.GetMenuItem() == MenuItemID_EDIT)
                        IBR_Inst_Menu.ChooseMenu(MenuItemID_FILE);

                    IsBgDragging = false;
                }
            }
        }

        LastOnWindow = OnWindow;
        LastClickable = !OnWindow && !ImGui::IsMouseDown(ImGuiMouseButton_Left);
        LastCont = Cont;
    }

    bool InWorkSpace(ImVec2 RePos)
    {
        return InRectangle(RePos, IBR_RealCenter::WorkSpaceUL, IBR_RealCenter::WorkSpaceDR);
    }
    void UpdatePrev()
    {
        RatioPrev = IBR_FullView::Ratio;
        EqCenterPrev = IBR_FullView::EqCenter;
        ReCenterPrev = IBR_RealCenter::Center;
    }
    void UpdatePrevII()
    {
        UpdatePrevResult = _UpdatePrev_None;
        if (abs(IBR_FullView::EqCenter - EqCenterPrev).max() >= 1)
        {
            auto EqCCopy = IBR_FullView::EqCenter;
            auto EqCPCopy = EqCenterPrev;
            UpdatePrevResult |= _UpdatePrev_EqCenter;
            IBG_Undo.Release();
            auto top = IBG_Undo.Top();
            if (top != nullptr && top->Id == "IBR_FullView::EqCenter")//MERGE
            {
                EqCPCopy = std::any_cast<std::pair<ImVec2, ImVec2>>(top->Extra()).first;
                top->UndoAction = [=]() {EqCenterPrev = IBR_FullView::EqCenter = EqCPCopy; };
                top->RedoAction = [=]() {EqCenterPrev = IBR_FullView::EqCenter = EqCCopy; };
                top->Extra = [=]()->std::any {return std::any{ std::make_pair(EqCPCopy,EqCCopy) }; };
            }
            else
            {
                IBG_UndoStack::_Item it;
                it.Id = "IBR_FullView::EqCenter";
                it.UndoAction = [=]() {EqCenterPrev = IBR_FullView::EqCenter = EqCPCopy; };
                it.RedoAction = [=]() {EqCenterPrev = IBR_FullView::EqCenter = EqCCopy; };
                it.Extra = [=]()->std::any {return std::any{ std::make_pair(EqCPCopy,EqCCopy) }; };
                IBG_Undo.Push(it);
            }
        }
        if (abs(IBR_FullView::Ratio - RatioPrev) >= 0.01)
        {
            auto EqCCopy = IBR_FullView::Ratio;
            auto EqCPCopy = RatioPrev;
            UpdatePrevResult |= _UpdatePrev_Ratio;
            IBG_Undo.Release();
            auto top = IBG_Undo.Top();
            if (top != nullptr && top->Id == "IBR_FullView::Ratio")//MERGE
            {
                EqCPCopy = std::any_cast<std::pair<float, float>>(top->Extra()).first;
                top->UndoAction = [=]() {RatioPrev = IBR_FullView::Ratio = EqCPCopy; };
                top->RedoAction = [=]() {RatioPrev = IBR_FullView::Ratio = EqCCopy; };
                top->Extra = [=]()->std::any {return std::any{ std::make_pair(EqCPCopy,EqCCopy) }; };
            }
            else
            {
                IBG_UndoStack::_Item it;
                it.Id = "IBR_FullView::Ratio";
                it.UndoAction = [=]() {RatioPrev = IBR_FullView::Ratio = EqCPCopy; };
                it.RedoAction = [=]() {RatioPrev = IBR_FullView::Ratio = EqCCopy; };
                it.Extra = [=]()->std::any {return std::any{ std::make_pair(EqCPCopy,EqCCopy) }; };
                IBG_Undo.Push(it);
            }

        }
    }

    InfoStack<StdMessage> ExtSetPos;
    IBR_SectionData* CurOnRender;
    ImVec4 TempWbg;
    struct PosHelper { ImVec2 eq, re; };
    void RenderUI()
    {
        IBB_Section_Desc _RenderUI_OnRender;
        if (!IBR_ProjectManager::IsOpen())return;
        auto vu = ExtSetPos.Release();

        IBR_Inst_Project.LinkList.clear();

        bool HasFocusedModule{ false };

        for (auto& sp : IBR_Inst_Project.IBR_SectionMap)
        {
            auto RSec = IBR_Inst_Project.GetSectionFromID(sp.first);
            if (!RSec.HasBack())continue;

            //IBR_Inst_Debug.AddMsgCycle([=]() {ImGui::TextWrapped("Render Section %s", sp.second.Desc.GetText().c_str());});

            auto& sd = sp.second;
            CurOnRender = &sd;
            _RenderUI_OnRender = sd.Desc;
            auto TA = EqPosToRePos(sd.EqPos);

            if (sd.First)
            {
                ImGui::SetNextWindowPos(TA);
                ImGui::SetNextWindowSize(sd.EqSize * IBR_FullView::Ratio);
                if (sp.first == IBR_EditFrame::CurSection.ID)ImGui::SetNextWindowFocus();
                //sd.First = false;//延迟清理
            }

            for (auto& p : vu)p();//for Undo & Redo

            if (sd.Dragging || sd.Ignore)
            {
                TempWbg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                if (sd.Dragging)TempWbg.w *= IBF_Inst_Setting.TransparencyBase() * 0.82f;
                if (sd.Ignore)
                {
                    if (IBR_SelectMode::IsWindowMassSelected(sd.Desc))
                    {
                        TempWbg.w *= IBF_Inst_Setting.TransparencyBase() * 0.667f;
                    }
                    else
                    {
                        if (IBF_Inst_Setting.IsDarkMode())
                        {
                            TempWbg.x += 0.3f;
                            TempWbg.y += 0.3f;
                            TempWbg.z += 0.3f;
                        }
                        else
                        {
                            TempWbg.x *= 0.8f;
                            TempWbg.y *= 0.8f;
                            TempWbg.z *= 0.8f;
                        }
                    }
                }
                ImGui::PushStyleColor(ImGuiCol_WindowBg, TempWbg);
                TempWbg.x = 1.0f;
                TempWbg.y = 1.0f;
                TempWbg.z = 1.0f;
            }
            else
            {
                TempWbg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                TempWbg.w *= IBF_Inst_Setting.TransparencyBase();
                ImGui::PushStyleColor(ImGuiCol_WindowBg, TempWbg);
            }


            bool NoMouseInput = IBR_SelectMode::IsWindowMassSelected(sd.Desc) || HoldingModules;
            if (NoMouseInput)ImGui::CaptureMouseFromApp(false);

            //ImGuiWindowFlags_NoClamping 是非标的私货，小朋友们不要学坏哦~
            ImGui::Begin((sd.Desc.Ini + u8" - " + sd.Desc.Sec).c_str(), &sd.IsOpen,
                ImGuiWindowFlags_NoClamping |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoScrollWithMouse |
                (NoMouseInput ? ImGuiWindowFlags_NoMove : 0));
            ImGui::SetWindowFontScale(IBR_FullView::Ratio);
            ImGui::PushClipRect(IBR_RealCenter::WorkSpaceUL, IBR_RealCenter::WorkSpaceDR, true);

            if (sd.Dragging && !IBR_PopupManager::HasPopup)ImGui::PushOrderFront(ImGui::GetCurrentWindow());

            if (ImGui::IsWindowFocused() && !sd.IsComment)
            {
                IBR_EditFrame::SetActive(sp.first);
                IBR_Inst_Menu.ChooseMenu(MenuItemID_EDIT);
            }
            auto PCopy = ImGui::GetWindowPos();
            auto SCopy = ImGui::GetWindowSize();
            sd.Hovered = ImGui::IsWindowHovered();

            if (UpdatePrevResult & _UpdatePrev_EqCenter)
            {
                ImGui::SetWindowPos(TA);
                //RePos
            }
            else if (sd.Dragging)
            {
                ImGui::SetWindowPos(TA);
                //RePos
            }
            else if (UpdatePrevResult & _UpdatePrev_Ratio)
            {
                ImGui::SetWindowSize(sd.EqSize * IBR_FullView::Ratio);
                ImGui::SetWindowPos(TA);
                //ReSize|RePos
            }
            else if (abs(IBR_WorkSpace::ReCenterPrev - IBR_RealCenter::Center).max() >= 1.0)
            {
                ImGui::SetWindowPos(TA);
                //RePos
            }
            else if (abs(PCopy - TA).max() >= 1.0)
            {
                ImVec2 NP = sd.EqPos + (PCopy - TA) / IBR_FullView::Ratio;
                PosHelper un = { sd.EqPos,TA }, re = { NP,PCopy };
                sd.EqPos = NP;

                IBG_Undo.Release();
                auto top = IBG_Undo.Top();
                auto sn = sd.Desc.Ini + "[" + sd.Desc.Sec + "].EqPos";
                if (top != nullptr && top->Id == sn)//MERGE
                {
                    un = std::any_cast<std::pair<PosHelper, PosHelper>>(top->Extra()).first;
                    top->UndoAction = [=]() {IBRF_CoreBump.SendToR({ [=]() {
                        if (_RenderUI_OnRender == CurOnRender->Desc) { CurOnRender->EqPos = un.eq; ImGui::SetNextWindowPos(un.re); }},
                        &ExtSetPos }); };
                    top->RedoAction = [=]() {IBRF_CoreBump.SendToR({ [=]() {
                        if (_RenderUI_OnRender == CurOnRender->Desc) { CurOnRender->EqPos = re.eq; ImGui::SetNextWindowPos(re.re); }},
                        &ExtSetPos }); };
                    top->Extra = [=]()->std::any {return std::any{ std::make_pair(un,re) }; };
                }
                else
                {
                    IBG_UndoStack::_Item it;
                    it.Id = sn;
                    it.UndoAction = [=]() {IBRF_CoreBump.SendToR({ [=]() {
                        if (_RenderUI_OnRender == CurOnRender->Desc) { CurOnRender->EqPos = un.eq; ImGui::SetNextWindowPos(un.re); }},
                        &ExtSetPos }); };
                    it.RedoAction = [=]() {IBRF_CoreBump.SendToR({ [=]() {
                        if (_RenderUI_OnRender == CurOnRender->Desc) { CurOnRender->EqPos = re.eq; ImGui::SetNextWindowPos(re.re); }},
                        &ExtSetPos }); };
                    it.Extra = [=]()->std::any {return std::any{ std::make_pair(un,re) }; };
                    IBG_Undo.Push(it);
                }
                //EqPos
            }
            else if (abs(SCopy - sd.EqSize * IBR_FullView::Ratio).max() >= 1.0)
            {
                ImVec2 NP = SCopy / IBR_FullView::Ratio;
                PosHelper un = { sd.EqSize,sd.EqSize * IBR_FullView::Ratio }, re = { NP,SCopy };
                sd.EqSize = NP;

                IBG_Undo.Release();
                auto top = IBG_Undo.Top();
                auto sn = sd.Desc.Ini + "[" + sd.Desc.Sec + "].EqSize";
                if (top != nullptr && top->Id == sn)//MERGE
                {
                    un = std::any_cast<std::pair<PosHelper, PosHelper>>(top->Extra()).first;
                    top->UndoAction = [=]() {IBRF_CoreBump.SendToR({ [=]() {
                        if (_RenderUI_OnRender == CurOnRender->Desc) { CurOnRender->EqSize = un.eq; ImGui::SetNextWindowSize(un.re); }},
                        &ExtSetPos }); };
                    top->RedoAction = [=]() {IBRF_CoreBump.SendToR({ [=]() {
                        if (_RenderUI_OnRender == CurOnRender->Desc) { CurOnRender->EqSize = re.eq; ImGui::SetNextWindowSize(re.re); }},
                        &ExtSetPos }); };
                    top->Extra = [=]()->std::any {return std::any{ std::make_pair(un,re) }; };
                }
                else
                {
                    IBG_UndoStack::_Item it;
                    it.Id = sn;
                    it.UndoAction = [=]() {IBRF_CoreBump.SendToR({ [=]() {
                        if (_RenderUI_OnRender == CurOnRender->Desc) { CurOnRender->EqSize = un.eq; ImGui::SetNextWindowSize(un.re); }},
                        &ExtSetPos }); };
                    it.RedoAction = [=]() {IBRF_CoreBump.SendToR({ [=]() {
                        if (_RenderUI_OnRender == CurOnRender->Desc) { CurOnRender->EqSize = re.eq; ImGui::SetNextWindowSize(re.re); }},
                        &ExtSetPos }); };
                    it.Extra = [=]()->std::any {return std::any{ std::make_pair(un,re) }; };
                    IBG_Undo.Push(it);
                }
                //EqSize
            }
            {
                auto sz = ImGui::GetWindowSize();
                if (sd.FinalY < 1.0F)sd.FinalY = FontHeight * 8.0F;
                if (sd.WidthFix > FontHeight * 15.0f)ImGui::SetWindowSize({ sd.WidthFix, sd.FinalY + FontHeight * 2.0F });
                else ImGui::SetWindowSize({ FontHeight * 15.0f, sd.FinalY + FontHeight * 2.0F });
            }

            sd.RenderUI();

            if (sd.First)sd.First = false;
            if (sp.first == IBR_EditFrame::CurSection.ID)
            {
                if (!ImGui::GetCurrentContext()->OpenPopupStack.Size)
                {
                    auto FL = ImGui::GetForegroundDrawList();
                    FL->PushClipRect(IBR_RealCenter::WorkSpaceUL, IBR_RealCenter::WorkSpaceDR, true);
                    FL->AddRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), IBR_Color::FocusWindowColor, 5.0F, 0, 5.0F);
                    FL->PopClipRect();
                }
            }
            if (sp.first == IBR_EditFrame::CurSection.ID && !IsMassSelecting && !IsMassAfter && LastCont)
            {
                HasFocusedModule = true;
                if (IsHotKeyPressed(Delete))
                    IBRF_CoreBump.SendToR({ [desc = sd.Desc]() { IBG_Undo.SomethingShouldBeHere(); IBR_Inst_Project.DeleteSection(desc); } });
                else if (IsHotKeyPressed(RenameModule))
                {
                    sd.RenameDisplay();
                    //IBR_PopupManager::ClearRightClickMenu();
                }
                else if (IsHotKeyPressed(RenameRegister))
                {
                    sd.RenameRegister();
                    //IBR_PopupManager::ClearRightClickMenu();
                }
                else if (IsHotKeyPressed(Copy))sd.CopyToClipBoard();
            }

            ImGui::PopClipRect();

            ImGui::End();
            if (NoMouseInput)ImGui::CaptureMouseFromApp(true);
            ImGui::PopStyleColor();

            if (!sd.IsOpen)IBRF_CoreBump.SendToR({ [=]() {IBR_Inst_Project.DeleteSection(sd.Desc); },nullptr });
        }

        /*
        if (!HasFocusedModule && IBR_Inst_Project.IBR_SectionMap.size())
        {
            //IBR_EditFrame::Clear();
            if (IBR_Inst_Menu.GetMenuItem() == MenuItemID_EDIT)
            {
                IBR_UICondition::MenuCollapse = true;
            }
        }
        */
        for (auto& Link : IBR_Inst_Project.LinkList)
        {

            auto Rsec = IBR_Inst_Project.GetSection(Link.Dest);
            auto RSD = Rsec.GetSectionData();
            if(RSD != nullptr && Rsec.HasBack())
            {
                //auto KList = ImGui::GetForegroundDrawList();
                auto KList = (!RSD->Dragging || IBR_PopupManager::HasPopup) ? ImGui::GetBackgroundDrawList() : ImGui::GetForegroundDrawList();
                //auto KList = IBR_PopupManager::HasPopup ? ImGui::GetBackgroundDrawList() : ImGui::GetForegroundDrawList();
                KList->PushClipRect(IBR_RealCenter::WorkSpaceUL, IBR_RealCenter::WorkSpaceDR, true);
                ImColor Col;
                if (IBR_EditFrame::CurSection.ID == Rsec.ID
                    && !ImGui::GetCurrentContext()->OpenPopupStack.Size)
                {
                    Col = IBR_Color::FocusLineColor;
                }
                else
                {
                    ImColor LinkCol = Link.Color;
                    Col = LinkCol.Value.w > 0.0001f ? LinkCol : IBR_Color::LegalLineColor;
                }
                if (RSD->Dragging || Link.IsSrcDragging)
                    Col.Value.w *= IBF_Inst_Setting.TransparencyBase() * 0.625f;
                {
                    ImVec2 pa = Link.BeginR;
                    ImVec2 pb = EqPosToRePos(RSD->EqPos) + RSD->ReOffset;
                    float LineWidth = FontHeight / 5.0f;
                    ImVec2 Mid = (pa + pb) / 2.0F;
                    bool Straight = (pb.x - pa.x >= FontHeight * 5.0F);

                    if(Straight)KList->AddBezierCubic(
                        pa,
                        { (pa.x + 4 * pb.x) / 5,pa.y },
                        { (4 * pa.x + pb.x) / 5,pb.y },
                        pb,
                        Col,
                        LineWidth);
                    else
                    if (Link.IsSelfLinked)
                    {
                        KList->AddBezierCubic(
                            pa,
                            { (7 * pa.x - 2 * pb.x) / 5,(3 * pb.y - pa.y) / 2 },
                            { pa.x ,(3 * pb.y - pa.y) / 2 } ,
                            Mid,
                            Col,
                            LineWidth);
                        KList->AddBezierCubic(
                            Mid,
                            { pb.x ,(3 * pa.y - pb.y) / 2 },
                            { (7 * pb.x - 2 * pa.x) / 5,(3 * pa.y - pb.y) / 2 },
                            pb,
                            Col,
                            LineWidth);
                        //KList->AddCircleFilled({ (7 * pa.x - 2 * pb.x) / 5,(5 * pb.y - pa.y) / 4 }, LineWidth * 2.0F, Col);
                        //KList->AddCircleFilled({ pb.x ,(5 * pa.y - pb.y) / 4 }, LineWidth * 2.0F, Col);
                        //KList->AddCircleFilled({ Mid }, LineWidth * 2.0F, Col);
                        //KList->AddCircleFilled({ pa.x ,(5 * pb.y - pa.y) / 4 }, LineWidth * 2.0F, Col);
                        //KList->AddCircleFilled({ (7 * pb.x - 2 * pa.x) / 5,(5 * pa.y - pb.y) / 4 }, LineWidth * 2.0F, Col);
                    }
                    else
                    {
                        //auto V = pa.x - pb.x;
                        //auto ExOfs = V < FontHeight * 12.5F;
                        KList->AddBezierCubic(
                            pa,
                            { pa.x + FontHeight * 5.0F ,pa.y },
                            { pa.x + FontHeight * 5.0F ,(3 * pa.y + pb.y) / 4 },
                            Mid,
                            Col,
                            LineWidth);
                        KList->AddBezierCubic(
                            Mid,
                            { pb.x - FontHeight * 5.0F ,(3 * pb.y + pa.y) / 4 },
                            { pb.x - FontHeight * 5.0F ,pb.y },
                            pb,
                            Col,
                            LineWidth);
                        /*
                        if (ExOfs)
                        {
                            //KList->AddCircleFilled({ pa.x + FontHeight * 5.0F ,pa.y }, LineWidth * 2.0F, Col);
                            //KList->AddCircleFilled({ pb.x - FontHeight * 5.0F ,pb.y }, LineWidth * 2.0F, Col);
                            //KList->AddCircleFilled(Mid, LineWidth * 2.0F, Col);
                            //KList->AddCircleFilled({ pb.x - FontHeight * 5.0F ,(3 * pb.y + pa.y) / 4 }, LineWidth * 2.0F, Col);
                            //KList->AddCircleFilled({ pa.x + FontHeight * 5.0F ,(3 * pa.y + pb.y) / 4 }, LineWidth * 2.0F, Col);
                        }
                        else {
                            KList->AddBezierCubic(
                                pa,
                                { (7 * pa.x - 2 * pb.x) / 5 ,pa.y },
                                { (7 * pa.x - 2 * pb.x) / 5 ,(3 * pa.y + pb.y) / 4 },
                                Mid,
                                Col,
                                LineWidth);
                            KList->AddBezierCubic(
                                Mid,
                                { (7 * pb.x - 2 * pa.x) / 5,(3 * pb.y + pa.y) / 4 },
                                { (7 * pb.x - 2 * pa.x) / 5,pb.y },
                                pb,
                                Col,
                                LineWidth);
                            //KList->AddCircleFilled({ (7 * pa.x - 2 * pb.x) / 5 ,pa.y }, LineWidth * 1.0F, Col);
                            //KList->AddCircleFilled({ (7 * pb.x - 2 * pa.x) / 5 ,pb.y }, LineWidth * 1.0F, Col);
                            //KList->AddCircleFilled({ (7 * pa.x - 2 * pb.x) / 5 ,(3 * pa.y + pb.y) / 4 }, LineWidth * 1.0F, Col);
                            //KList->AddCircleFilled({ (7 * pb.x - 2 * pa.x) / 5,(3 * pb.y + pa.y) / 4 }, LineWidth * 1.0F, Col);
                            //KList->AddCircleFilled(Mid, LineWidth * 2.0F, Col);
                        }
                        */
                    }

                }
                KList->PopClipRect();
            }
        }
    }
}
