// Copyright (C) 2019 Nomango

#pragma once


#include"FromEngine/Include.h"
#include"FromEngine/global_timer.h"
#include"Global.h"
#include"IBFront.h"
#include"IBRender.h"
#include<ShlObj.h>
#include<imgui_internal.h>

std::string FontPath = ".\\Resources\\";//全过程不变
std::wstring FontPathW = L".\\Resources\\";//全过程不变
std::wstring Defaultpath{ L"C:\\" };
std::string Pathbuf, Desktop, TextEditAboutU8;
std::wstring PathbufW;

int FPSLimit = -1;
int RFontHeight;

wchar_t CurrentDirW[5000];//调用当前目录时必须调用这两个，不要临时求！
char CurrentDirA[5000];//因为OpenFileDialog会改变当前目录，所以必须提前加载

int RecentFileLimit = 10;
std::vector<std::string>RecentFilePath;//UTF-8


int IBG_ExitProcess();

int DynamicDataXDelta, DynamicDataYDelta;

bool Await{ true };
bool ProjectLoad_Final{ false };

void ControlPanel();
void ControlPanel_Debug();


std::vector<std::function<void(int)>>DelayWindow;

RECT FinalWP;

int CurrentINIPage = 0;

namespace ImGui
{
    ImVec2 GetLineEndPos()
    {
        return { ImGui::GetWindowPos().x + ImGui::GetWindowWidth(),ImGui::GetCursorScreenPos().y };
    }
    ImVec2 GetLineBeginPos()
    {
        return { ImGui::GetWindowPos().x ,ImGui::GetCursorScreenPos().y };
    }
    bool IsWindowClicked(ImGuiMouseButton Button)
    {
        return ImGui::IsWindowHovered() && ImGui::IsMouseClicked(Button);
    }
    void PushOrderFront(ImGuiWindow* Window)
    {
        //change WindowFocusOrder and Windows array
        ImGuiContext& g = *GImGui;
        //push Window in g.WindowFocusOrder[Window->FocusOrder] to the back of the array (topmost)
        for (int i = Window->FocusOrder; i < g.WindowsFocusOrder.Size - 1; i++)
        {
            g.WindowsFocusOrder[i] = g.WindowsFocusOrder[i + 1];
        }
        //Change all focusorder
        for (short i = 0; i < g.WindowsFocusOrder.Size; i++)
        {
            g.WindowsFocusOrder[i]->FocusOrder = i;
        }
        //change Window->FocusOrder to the last index
        Window->FocusOrder = (short)g.WindowsFocusOrder.Size - 1;
        //change g.Windows array
        for (int i = 0; i < g.Windows.Size; i++)
        {
            if (g.Windows[i] == Window)
            {
                //move Window to the back of the array
                ImGuiWindow* Temp = g.Windows[i];
                for (int j = i; j < g.Windows.Size - 1; j++)
                {
                    g.Windows[j] = g.Windows[j + 1];
                }
                g.Windows[g.Windows.Size - 1] = Temp;
                break;
            }
        }
    }
}




int PrevSet = 0;
bool WaitingInsertLoad{ false };
int A_INI = -2, A_Sec = -2;
float MWWidth{0.0f}, MWWidth2{0.0f};
bool MWWidthSetSuccess{ false };
BufString TempTargetPath;

bool ChangeWithFontSize;//EVENT : FONT_SCALE_CHANGE

bool PanelOnHold = false;


void ControlPanel()
{
    static bool First = true;

    GetWindowRect(MainWindowHandle, &FinalWP);
    IBR_UICondition::UpdateWindowTitle();
    IBR_WorkSpace::UpdatePrev();
    if (First)
    {
        IBR_DynamicData::Open();
        DynamicDataXDelta = ScrX - (FinalWP.right - FinalWP.left);
        DynamicDataYDelta = ScrY - (FinalWP.bottom - FinalWP.top);
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(locc("Log_MainLoopStart"));
        }
        First = false;
    }
    IBR_UICondition::CurrentScreenWidth = FinalWP.right - FinalWP.left + DynamicDataXDelta;
    IBR_UICondition::CurrentScreenHeight = FinalWP.bottom - FinalWP.top + DynamicDataYDelta;

    //use glfw to check if the window is maximized
    if (!glfwGetWindowAttrib(PreLink::window, GLFW_MAXIMIZED))
        IBR_DynamicData::Save();
    IBRF_CoreBump.IBR_AutoProc();


    ImGui::Begin("INIBrowserMainMenu", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoCollapse |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove );
    ImGui::SetWindowPos({ 0,-FontHeight * 0.0f });
    ImGui::SetWindowSize({ (float)(FinalWP.right - FinalWP.left - WindowSizeAdjustX),FontHeight * 2.0f });

    IBG_Undo.RenderUI();
    ImGui::SameLine();
    IBR_Inst_Menu.RenderUIBar();

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - loc("GUI_TopRightHint").size() * FontHeight * 0.7f);
    ImGui::Text(locc("GUI_TopRightHint"), _AppName, Version.c_str(), ImGui::GetIO().Framerate);

    ImGui::End();
    if (!(IBR_PopupManager::HasPopup && IBR_PopupManager::CurrentPopup.Modal))
    {
        ImGui::PushOrderFront(ImGui::FindWindowByName("INIBrowserMainMenu"));
        ImGui::GetCurrentContext()->ExtraTopMostWindow = ImGui::FindWindowByName("INIBrowserMainMenu");
    }

    if (IBR_UICondition::MenuCollapse)
    {
        ImGui::SetNextWindowCollapsed(true);
        IBR_UICondition::MenuCollapse = false;
    }
    if (IBR_UICondition::MenuChangeShow)
    {
        ImGui::SetNextWindowCollapsed(false);
        IBR_UICondition::MenuChangeShow = false;
    }
    if (IBR_Inst_Menu.GetMenuItem() == MenuItemID_EDIT && IBR_EditFrame::CurSection.GetSectionData())
        ImGui::Begin((IBR_EditFrame::CurSection.GetSectionData()->DisplayName + u8"###" + loc("GUI_MainMenu")).c_str()
            , nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
    else ImGui::Begin(locc("GUI_MainMenu"), nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
    ImGui::SetWindowPos({ 0,FontHeight * 2.0f - WindowSizeAdjustY });
    auto M = std::min(((float)(FinalWP.right - FinalWP.left - WindowSizeAdjustX)) / 4, 400.0f);
    ImGui::SetWindowSize({ std::max(ImGui::GetWindowWidth(),M),
       (float)(FinalWP.bottom - FinalWP.top) - FontHeight * 3.5f + WindowSizeAdjustY});
    if (!MWWidthSetSuccess && MWWidth2 > 0.0f)
        ImGui::SetWindowSize({ MWWidth2 , ImGui::GetWindowHeight() });
    if (ImGui::GetWindowWidth() == MWWidth2)
        MWWidthSetSuccess = true;
    IBR_RealCenter::Update();

    auto MainMenuWindow = ImGui::GetCurrentWindow();
    if (!(IBR_PopupManager::HasPopup && IBR_PopupManager::CurrentPopup.Modal))
    {
        ImGui::PushOrderFront(MainMenuWindow);
        //ImGui::GetCurrentContext()->ExtraTopMostWindow2 = MainMenuWindow;
    }

    IBR_Inst_Menu.RenderUIMenu();
    MWWidth = ImGui::GetWindowWidth();
    ImGui::NewLine(); ImGui::NewLine();
	ImGui::End();
   
    if (!IBR_PopupManager::HasPopup && IBR_ProjectManager::IsOpen())IBR_WorkSpace::ProcessBackgroundOpr();
    if (!IBR_PopupManager::HasPopup)IBR_ProjectManager::ProjActionByKey();
    IBR_WorkSpace::UpdateNewRatio();

    ChangeWithFontSize = (fabs(IBR_WorkSpace::RatioPrev - IBR_FullView::Ratio) > 0.001f);
    IBR_Inst_Debug.ClearMsgCycle();
    IBR_WorkSpace::UpdatePrevII();
    
    auto DelayWindowB = DelayWindow;
    DelayWindow.clear();
    RFontHeight = FontHeight;
    FontHeight = (int)(FontHeight * IBR_FullView::Ratio);
    dImVec2 PrevClipZoneCenter = IBR_RealCenter::Center;
    IBR_RealCenter::Center = (IBR_RealCenter::WorkSpaceUL + IBR_RealCenter::WorkSpaceDR) / 2.0;
    

    IBR_Inst_Debug.AddMsgCycle([=]() {ImGui::Text("PrevSet = %s", (PrevSet?"true":"false")); });
    IBR_Inst_Project.DataCheck();

    IBR_WorkSpace::RenderUI();
    IBR_Inst_Debug.RenderOnWorkSpace();
    
    //IBR_WorkSpace::EqCenterPrev = IBR_FullView::EqCenter;
    FontHeight = RFontHeight;

    IBR_SelectMode::RenderUI();
    IBR_HintManager::RenderUI();
    IBR_PopupManager::RenderUI();

    IBR_WorkSpace::LastOperateOnText = IBR_WorkSpace::OperateOnText;
    IBR_WorkSpace::OperateOnText = false;
}



