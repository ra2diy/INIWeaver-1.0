#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBR_ListView.h"
#include <imgui_internal.h>

extern bool EnableDebugList;

void IBR_MainMenu::RenderUIBar()
{
    ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
    for (size_t i = 0; i < ItemList.size(); i++)
    {
        if (ItemList[i].Button())
        {
            Choice = i;
            IBR_UICondition::MenuChangeShow = true;
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(locc("Log_SwitchMainMenu"));
            }
        }
        ImGui::SameLine();
    }
    ImGui::PopStyleColor(1);
}
void IBR_MainMenu::RenderUIMenu()
{
    sprintf(LogBuf, "%u", Choice);
    if (Choice < ItemList.size())ItemList.at(Choice).Menu();
}
void IBR_MainMenu::ChooseMenu(size_t S)
{
    if (S >= 0 && S < ItemList.size())Choice = S;
}

void ControlPanel_Debug()
{
    ImGui::Text(locc("GUI_DebugTitle"));

    IBR_Inst_Debug.RenderUI();

    int ii = 0;
    for (auto& s : IBG_Undo.Stack)
    {
        ImGui::Text(s.Id.c_str());
        if (ii == IBG_Undo.Cursor)ImGui::Separator();
        ++ii;
    }

}

void ControlPanel_About();

void ControlPanel_WaitOpen()
{
    ImGui::TextDisabled(locc("GUI_WaitOpen"));
}

void ControlPanel_File()
{
    if (ImGui::Button(locc("GUI_Save")))IBR_ProjectManager::SaveOptAction();//
    if (ImGui::Button(locc("GUI_SaveAs")))IBR_ProjectManager::SaveAsAction();//
    if (IBR_Inst_Project.IBR_SectionMap.empty())ImGui::TextDisabled(locc("GUI_Output"));
    else if (ImGui::Button(locc("GUI_Output")))IBR_ProjectManager::OutputAction();//

    ImGui::NewLine();
    if (ImGui::Button(locc("GUI_CloseProject")))IBR_ProjectManager::ProjOpen_CreateAction(); //王德发你猜为什么是CreateAction
    if (ImGui::Button(locc("GUI_OpenProject")))IBR_ProjectManager::ProjOpen_OpenAction();//
    ImGui::NewLine();
    IBR_RecentManager::RenderUI();//
    /*
    if (!IsProjectOpen)
    {
        if (ImGui::Button(u8"新建"))IBR_ProjectManager::CreateAction();//
        if (ImGui::Button(u8"打开"))IBR_ProjectManager::OpenAction();//
        ImGui::NewLine();
        IBR_RecentManager::RenderUI();//
    }
    else
    {
        if (ImGui::Button(u8"保存"))IBR_ProjectManager::SaveOptAction();//
        if (ImGui::Button(u8"另存为"))IBR_ProjectManager::SaveAsAction();//
        if (IBR_Inst_Project.IBR_SectionMap.empty())ImGui::TextDisabled(u8"导出");
        else if (ImGui::Button(u8"导出"))IBR_ProjectManager::OutputAction();//

        ImGui::NewLine();
        if (ImGui::Button(u8"关闭"))IBR_ProjectManager::ProjOpen_CreateAction(); //王德发你猜为什么是CreateAction
        //IBR_ProjectManager::CloseAction();//

        ImGui::NewLine();
        //if (ImGui::Button(u8"新建"))IBR_ProjectManager::ProjOpen_CreateAction();//
        if (ImGui::Button(u8"打开"))IBR_ProjectManager::ProjOpen_OpenAction();//
        ImGui::NewLine();
        IBR_RecentManager::RenderUI();//
    }
    */
}

void ControlPanel_View()
{
    if (!IBR_ProjectManager::IsOpen())
    {
        ControlPanel_WaitOpen();
        return;
    }
    IBR_FullView::RenderUI();
}



void ControlPanel_ListView()
{
    if (!IBR_ProjectManager::IsOpen())
    {
        ControlPanel_WaitOpen();
        return;
    }
    IBR_ListView::RenderUI();
}



void ControlPanel_Edit()
{
    if (!IBR_ProjectManager::IsOpen())
    {
        ControlPanel_WaitOpen();
        return;
    }
    if (IBR_EditFrame::CurSection.ID != IBR_EditFrame::PrevId)
    {
        IBR_EditFrame::PrevId = IBR_EditFrame::CurSection.ID;
        IBR_EditFrame::UpdateSection();
    }
    if (!IBR_EditFrame::Empty)if (!IBR_EditFrame::CurSection.HasBack())
    {
        IBR_EditFrame::Empty = true;
    }
    IBR_EditFrame::RenderUI();
}

bool ImGui_TextDisabled_Helper(const char* Text)
{
    ImGui::TextDisabled(Text); return false;
}
bool SmallButton_Disabled_Helper(bool cond, const char* Text)
{
    return cond ? ImGui::SmallButton(Text) : ImGui_TextDisabled_Helper(Text);
}

/*
#define MenuItemID_FILE 0
#define MenuItemID_VIEW 1
#define MenuItemID_LIST 2
//#define MenuItemID_MODULE 3
#define MenuItemID_EDIT 4
#define MenuItemID_SETTING 5
#define MenuItemID_ABOUT 6
#define MenuItemID_DEBUG 7
*/
IBR_MainMenu IBR_Inst_Menu
{
{
    {[]() {return ImGui::SmallButton(locc("GUI_MenuItem_File")); },ControlPanel_File},
    {[]() {ImGui::NewLine(); ImGui::SameLine(); return SmallButton_Disabled_Helper(IBR_ProjectManager::IsOpen(), locc("GUI_MenuItem_View")); },ControlPanel_View},
    {[]() {ImGui::NewLine(); ImGui::SameLine(); return SmallButton_Disabled_Helper(IBR_ProjectManager::IsOpen(), locc("GUI_MenuItem_List")); },ControlPanel_ListView},
    //{[]() {return false;/*SmallButton_Disabled_Helper(IsProjectOpen, u8"预设");*/ },ControlPanel_Module},

    //{[]() {return SmallButton_Disabled_Helper(IBR_ProjectManager::IsOpen(), locc("GUI_MenuItem_Edit")); },ControlPanel_Edit},
    {[]() {ImGui::NewLine(); return false; },ControlPanel_Edit},

    {[]() {return ImGui::SmallButton(locc("GUI_MenuItem_Setting")); },[]() {IBR_Inst_Setting.RenderUI(); }},
    {[]() {ImGui::NewLine(); ImGui::SameLine(); return ImGui::SmallButton(locc("GUI_MenuItem_About")); },ControlPanel_About},
    {[]() {if (EnableDebugList) return ImGui::SmallButton(locc("GUI_MenuItem_Debug")); else { ImGui::NewLine(); return false; } },ControlPanel_Debug},
}
};


void ControlPanel_About()
{
    ImGui::TextWrapped(locc("GUI_About1"), _AppName, Version.c_str());
    ImGui::TextWrapped(u8"GLFW/Dear ImGui", Version.c_str());
    ImGui::Separator();
    ImGui::TextWrapped((loc("GUI_About5") + u8"：钢铁之锤").c_str());
    ImGui::TextWrapped(u8"QQ：2482911962");
    ImGui::TextWrapped(u8"贴吧：笨030504");
    ImGui::TextWrapped(u8"GitHub：Ironhammer Std");
    ImGui::NewLine();
    ImGui::TextWrapped((loc("GUI_About6") + u8"：").c_str());
    ImGui::TextWrapped(u8"      Kenosis");
    ImGui::Separator();
    ImGui::TextWrapped(locc("GUI_About2"));

    const char* Document_CN = "https://inibrowser-02-chinese.readthedocs.io/zh-cn/latest/Info.html";
    const char* Document_EN = "https://inibrowser-02-chinese.readthedocs.io/en/latest/Info.html";
    const char* BugReportURL = "https://docs.qq.com/form/page/DWXdKYUFRV1dHSnNE";
    const char* RepositoryURL = "https://github.com/ra2diy/INIWeaver-1.0";

    auto DocLang = loc("DocLang");
    //目前可用 ：ZH-CN EN
    const char* DocumentURL;
    if (DocLang == "ZH-CN")DocumentURL = Document_CN;
    else if(DocLang == "EN")DocumentURL = Document_EN;
    else DocumentURL = Document_CN;

    ImGui::TextWrapped(locc("GUI_About7")); //ImGui::SameLine();
    if (ImGui::Button((loc("GUI_CopyLinkToClipboard") + u8"##C").c_str()))
    {
        ImGui::LogToClipboard();
        ImGui::LogText(Document_CN);
        ImGui::LogFinish();
    }
    ImGui::SameLine();
    if (ImGui::Button((loc("GUI_OpenLink") + u8"##C").c_str()))
    {
        ::ShellExecuteA(nullptr, "open", Document_CN, NULL, NULL, SW_SHOWNORMAL);
    }

    ImGui::TextWrapped(locc("GUI_About3")); //ImGui::SameLine();
    if (ImGui::Button((loc("GUI_CopyLinkToClipboard") + u8"##A").c_str()))
    {
        ImGui::LogToClipboard();
        ImGui::LogText(BugReportURL);
        ImGui::LogFinish();
    }
    ImGui::SameLine();
    if (ImGui::Button((loc("GUI_OpenLink") + u8"##A").c_str()))
    {
        ::ShellExecuteA(nullptr, "open", BugReportURL, NULL, NULL, SW_SHOWNORMAL);
    }

    ImGui::TextWrapped(locc("GUI_About4"));
    if (ImGui::Button((loc("GUI_CopyLinkToClipboard") + u8"##B").c_str()))
    {
        ImGui::LogToClipboard();
        ImGui::LogText(RepositoryURL);
        ImGui::LogFinish();
    }
    ImGui::SameLine();
    if (ImGui::Button((loc("GUI_OpenLink") + u8"##B").c_str()))
    {
        ::ShellExecuteA(nullptr, "open", RepositoryURL, NULL, NULL, SW_SHOWNORMAL);
    }
}
