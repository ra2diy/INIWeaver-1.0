#include "IBR_Components.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBR_ListView.h"
#include "IBR_Debug.h"
#include <imgui_internal.h>
#include "IBR_Misc.h"
#include "IBG_UndoTree.h"

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
    sprintf(LogBuf, "%zu", Choice);
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

    if (ImGui::TreeNode(u8"撤销栈信息："))
    {
        int ii = 0;
        for (auto& s : IBG_Undo.Stack)
        {
            ImGui::Text(s.Id.c_str());
            if (ii == IBG_Undo.Cursor)ImGui::Separator();
            ++ii;
        }
        ImGui::TreePop();
    }
}

void ControlPanel_About();

void ControlPanel_WaitOpen()
{
    ImGui::TextDisabled(locc("GUI_WaitOpen"));
}

void ControlPanel_File()
{
    auto lw = ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x * 2.0F;
    auto lw1 = ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x * 1.0F;
    auto lh = FontHeight * 2.0F;

    if (ImGui::Button(locc("GUI_Save"), { lw / 3.0f, lh }))IBR_ProjectManager::SaveOptAction();//
    ImGui::SameLine();
    if (ImGui::Button(locc("GUI_SaveAs"), { lw / 3.0f, lh }))IBR_ProjectManager::SaveAsAction();//
    ImGui::SameLine();
    if (IBR_Inst_Project.IBR_SectionMap.empty())
    {
        ImGui::BeginDisabled();
        ImGui::Button(locc("GUI_Output"), { lw / 3.0f, lh });
        ImGui::EndDisabled();
    }
    else if (ImGui::Button(locc("GUI_Output"), { lw / 3.0f, lh }))IBR_ProjectManager::OutputAction();//
    ImGui::NewLine();
    if (ImGui::Button(locc("GUI_CloseProject"), { lw1 / 2.0f, lh }))IBR_ProjectManager::ProjOpen_CreateAction(); //王德发你猜为什么是CreateAction
    ImGui::SameLine();
    if (ImGui::Button(locc("GUI_OpenProject"), { lw1 / 2.0f, lh }))IBR_ProjectManager::ProjOpen_OpenAction();//

    ImGui::NewLine();
    if (ImGui::Button(locc("GUI_SetFileAssoc"), { ImGui::GetWindowContentRegionWidth() , FontHeight * 1.5F }))
    {
        bool SetFileAssociation();
        bool GuideUserToSetDefaultProgram();
        bool IsDefaultForExtension();
        if (SetFileAssociation())
        {
            
            if (IsDefaultForExtension())
            {
                IBR_HintManager::SetHint(locc("GUI_AlreadySetFileAssoc"), HintStayTimeMillis);
            }
            else
            {
                IBR_HintManager::SetHint(locc("GUI_SetFileAssocSuccess"), HintStayTimeMillis);
                GuideUserToSetDefaultProgram();
            }
        }
        else
            IBR_HintManager::SetHint(locc("GUI_SetFileAssocFailure"), HintStayTimeMillis);
        
    }
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
    {[]() {if (EnableDebugList) { ImGui::NewLine(); ImGui::SameLine(); return ImGui::SmallButton(locc("GUI_MenuItem_Debug")); } else { ImGui::NewLine(); return false; } },ControlPanel_Debug},
}
};

const char* Document_CN = "https://inibrowser-02-chinese.readthedocs.io/zh-cn/latest/Info.html";
const char* Document_EN = "https://inibrowser-02-chinese.readthedocs.io/en/latest/Info.html";
const char* BugReportURL = "https://docs.qq.com/form/page/DWXdKYUFRV1dHSnNE";
const char* RepositoryURL = "https://github.com/ra2diy/INIWeaver-1.0";
const char* LicenseURL = "https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html";

void ControlPanel_About()
{
    const auto URLOpr = [](const char* URL)
        {
            ImGui::PushID(URL);
            if (ImGui::Button(locc("GUI_CopyLinkToClipboard")))
            {
                ImGui::LogToClipboard();
                ImGui::LogText(URL);
                ImGui::LogFinish();
                IBR_HintManager::SetHint(loc("GUI_CopyLinkToClipboard_Success"), HintStayTimeMillis);
            }
            ImGui::SameLine();
            if (ImGui::Button(locc("GUI_OpenLink")))
            {
                ::ShellExecuteA(nullptr, "open", URL, NULL, NULL, SW_SHOWNORMAL);
            }
            ImGui::PopID();
        };

    ImGui::TextWrapped(locc("GUI_About1"), _AppName, Version.c_str());
    ImGui::TextWrapped(locc("GUI_About_Copyright"));
    ImGui::Text(u8"GLFW/Dear ImGui/CJSON/FmtLib");
    ImGui::Separator();
    ImGui::TextWrapped((loc("GUI_About5")).c_str());
    ImGui::TextWrapped(u8"      钢铁之锤");
    ImGui::TextWrapped((loc("GUI_About6")).c_str());
    ImGui::TextWrapped(u8"      Kenosis");
    ImGui::TextWrapped((loc("GUI_About8")).c_str());
    ImGui::TextWrapped(u8"      九千天华");
    ImGui::TextWrapped(locc("GUI_About2"));
    ImGui::Separator();
    if (ImGui::Button(locc("GUI_About_License"), { ImGui::GetWindowContentRegionWidth(), FontHeight * 1.5f }))
    {
        IBR_PopupManager::SetCurrentPopup(std::move(
            IBR_PopupManager::Popup{}
            .CreateModal(loc("GUI_About_License"), true)
            .SetFlag(ImGuiWindowFlags_NoResize)
            .SetSize({FontHeight * 22.0f, FontHeight * 30.0f})
            .PushMsgBack([=]() {
                ImGui::TextWrapped(locc("GUI_About_LicenseInfo_1"));
                ImGui::TextWrapped(LicenseURL);
                URLOpr(LicenseURL);
                ImGui::TextWrapped(locc("GUI_About_LicenseInfo_2"));
                ImGui::TextWrapped(RepositoryURL);
                URLOpr(RepositoryURL);
                ImGui::TextWrapped(locc("GUI_About_LicenseInfo_3"));
                ImGui::Separator();
                ImGui::TextWrapped(locc("GUI_About_LicenseInfo_4"));
            })
        ));
    }

    if (ImGui::Button(locc("GUI_About_Acknowledgments"), { ImGui::GetWindowContentRegionWidth(), FontHeight * 1.5f }))
    {
        IBR_PopupManager::SetCurrentPopup(std::move(
            IBR_PopupManager::Popup{}
            .CreateModal(loc("GUI_About_Acknowledgments"), true)
            .SetFlag(ImGuiWindowFlags_NoResize)
            .SetSize({ FontHeight * 22.0f, FontHeight * 30.0f })
            .PushTextBack(loc("GUI_About_AcknowledgmentsInfo"))
        ));
    }

    auto DocLang = loc("DocLang");
    //目前可用 ：ZH-CN EN
    const char* DocumentURL;
    if (DocLang == "ZH-CN")DocumentURL = Document_CN;
    else if(DocLang == "EN")DocumentURL = Document_EN;
    else DocumentURL = Document_CN;

    ImGui::TextWrapped(locc("GUI_About7")); //ImGui::SameLine();
    URLOpr(DocumentURL);

    ImGui::TextWrapped(locc("GUI_About3")); //ImGui::SameLine();
    URLOpr(BugReportURL);

    ImGui::TextWrapped(locc("GUI_About4"));
    URLOpr(RepositoryURL);
}
