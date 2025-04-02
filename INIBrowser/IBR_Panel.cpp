#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include<imgui_internal.h>


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
                GlobalLog.AddLog("切换了主菜单");
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
    ImGui::Text(u8"调试信息：");

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
    ImGui::TextDisabled(u8"请等待打开项目");
}

void ControlPanel_File()
{
    if (ImGui::Button(u8"保存"))IBR_ProjectManager::SaveOptAction();//
    if (ImGui::Button(u8"另存为"))IBR_ProjectManager::SaveAsAction();//
    if (IBR_Inst_Project.IBR_SectionMap.empty())ImGui::TextDisabled(u8"导出");
    else if (ImGui::Button(u8"导出"))IBR_ProjectManager::OutputAction();//

    ImGui::NewLine();
    if (ImGui::Button(u8"关闭项目"))IBR_ProjectManager::ProjOpen_CreateAction(); //王德发你猜为什么是CreateAction
    if (ImGui::Button(u8"打开项目"))IBR_ProjectManager::ProjOpen_OpenAction();//
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
    {
        int SelectN = 0;
        bool FullSelected = true;

        IBD_RInterruptF(x);

        for (auto& ini : IBF_Inst_Project.Project.Inis)
        {
            if (ini.Secs_ByName.empty())continue;
            for (auto& sec : ini.Secs)
            {
                if (sec.second.Dynamic.Selected)++SelectN;
                else FullSelected = false;
            }
        }

        bool SelectAll{ false }, SelectNone{ false }, Delete{ false }, Duplicate{ false };
        if (FullSelected)
        {
            if (SelectN == 0)ImGui::TextDisabled(u8"全选");
            else if (ImGui::Button(u8"全不选"))SelectNone = true;
        }
        else if (ImGui::Button(u8"全选"))SelectAll = true;
        ImGui::SameLine();
        if (SelectN == 0)
        {
            ImGui::TextDisabled(u8"删除"); ImGui::SameLine();
            ImGui::TextDisabled(u8"复制");
        }
        else
        {
            if (ImGui::Button(u8"删除"))Delete = true; ImGui::SameLine();
            if (ImGui::Button(u8"复制"))Duplicate = true;
        }

        if (SelectAll || SelectNone || Delete || Duplicate)
        {
            if (Duplicate)IBR_Inst_Project.CopyTransform.clear();
            for (auto& ini : IBF_Inst_Project.Project.Inis)
            {
                if (ini.Secs_ByName.empty())continue;
                if (Duplicate)
                {
                    for (auto& sec : ini.Secs)
                    {
                        if (sec.second.Dynamic.Selected && Duplicate)
                            IBR_Inst_Project.CopyTransform[sec.second.Name] = GenerateModuleTag();
                    }
                }
                for (auto& sec : ini.Secs)
                {
                    if (SelectAll)sec.second.Dynamic.Selected = true;
                    if (SelectNone)sec.second.Dynamic.Selected = false;
                    if (sec.second.Dynamic.Selected && Delete)IBRF_CoreBump.SendToR({ [=]() {IBR_Inst_Project.DeleteSection({ ini.Name,sec.second.Name }); },nullptr });
                    if (sec.second.Dynamic.Selected && Duplicate)
                        IBRF_CoreBump.SendToR({ [=]()
                            {
                                IBB_Section_Desc desc = { ini.Name,IBR_Inst_Project.CopyTransform[sec.second.Name]};
                                IBR_Inst_Project.GetSection({ ini.Name,sec.second.Name }).DuplicateSection(desc);
                                auto rsc = IBR_Inst_Project.GetSection(desc);
                                auto rsc_orig = IBR_Inst_Project.GetSection({ ini.Name,sec.second.Name });
                                auto& rsd = *rsc.GetSectionData();
                                auto& rsd_orig = *rsc_orig.GetSectionData();
                                rsd.RenameDisplayImpl(rsd_orig.DisplayName);
                                rsd.EqPos = rsd_orig.EqPos + dImVec2{2.0 * FontHeight, 2.0 * FontHeight};
                                rsd.EqSize = rsd_orig.EqSize;
                                rsd.Dragging = true;
                                rsd.IsComment = rsd_orig.IsComment;
                                if (rsd.IsComment)
                                {
                                    rsd.CommentEdit = std::make_shared<BufString>();
                                    strcpy(rsd.CommentEdit.get(), rsd_orig.CommentEdit.get());
                                }
                            },nullptr });
                    //见V0.2.0任务清单（四）第75条“涉及字段数目变化的指令应借由IBF_SendToR等提至主循环开头”
                }
            }
            if (Duplicate)
                IBRF_CoreBump.SendToR({ [=]() {IBF_Inst_Project.UpdateAll(); IBR_WorkSpace::HoldingModules = true;  },nullptr });
            else IBRF_CoreBump.SendToR({ [=]() {IBF_Inst_Project.UpdateAll(); },nullptr });
        }

        for (auto& ini : IBF_Inst_Project.Project.Inis)
        {
            if (ini.Secs_ByName.empty())continue;//屏蔽空INI
            //if (ImGui::TreeNode(ini.Name == LinkGroup_IniName ? u8"链接组" : (MBCStoUTF8(ini.Name).c_str())))
            {
                for (auto& sec : ini.Secs)
                {
                    auto rsc = IBR_Inst_Project.GetSection({ ini.Name,sec.second.Name });
                    ImGui::Checkbox((u8"##" + sec.second.Name).c_str(), &sec.second.Dynamic.Selected);
                    ImGui::SameLine();
                    ImGui::Text(rsc.GetSectionData()->DisplayName.c_str());
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - FontHeight * 2.0f));//4.5个字符是右侧内容的预留空间
                    if (ImGui::ArrowButton((sec.second.Name + "_ub_arr").c_str(), ImGuiDir_Right))
                    {
                        //auto rsc = IBR_Inst_Project.GetSection(IBB_Section_Desc{ ini.Name,sec.second.Name });
                        auto dat = rsc.GetSectionData();
                        if (dat != nullptr)
                        {
                            IBR_EditFrame::SetActive(rsc.ID);
                            IBR_FullView::EqCenter = dat->EqPos + (dat->EqSize / 2.0);
                        }
                    }
                }
                //ImGui::TreePop();
            }
        }
    }
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
    {[]() {return ImGui::SmallButton(u8"文件"); },ControlPanel_File},
    {[]() {return SmallButton_Disabled_Helper(IBR_ProjectManager::IsOpen(), u8"视图"); },ControlPanel_View},
    {[]() {return SmallButton_Disabled_Helper(IBR_ProjectManager::IsOpen(), u8"列表"); },ControlPanel_ListView},
    //{[]() {return false;/*SmallButton_Disabled_Helper(IsProjectOpen, u8"预设");*/ },ControlPanel_Module},
    {[]() {return SmallButton_Disabled_Helper(IBR_ProjectManager::IsOpen(), u8"编辑"); },ControlPanel_Edit},
    {[]() {return ImGui::SmallButton(u8"设置"); },[]() {IBR_Inst_Setting.RenderUI(); }},
    {[]() {return ImGui::SmallButton(u8"关于"); },ControlPanel_About},
    {[]() {return ImGui::SmallButton(u8"调试"); },ControlPanel_Debug},
}
};


void ControlPanel_About()
{
    ImGui::TextWrapped(u8"INI浏览器 V%s", Version.c_str());
    ImGui::TextWrapped(u8"最近更新于 2022年11月");
    ImGui::Separator();
    ImGui::TextWrapped(u8"作者：std::iron_hammer");
    ImGui::TextWrapped(u8"QQ：钢铁之锤（2482911962）");
    ImGui::TextWrapped(u8"贴吧：笨030504");
    ImGui::TextWrapped(u8"邮箱：2482911962@qq.com");
    ImGui::Separator();
    ImGui::TextWrapped(u8"更新链接：");
    ImGui::TextWrapped(u8"更新帖（红色警戒吧）"); //ImGui::SameLine();
    if (ImGui::Button(u8"复制链接##A"))
    {
        ImGui::LogToClipboard();
        ImGui::LogText("https://tieba.baidu.com/p/8005920823");
        ImGui::LogFinish();
    }ImGui::SameLine();
    if (ImGui::Button(u8"打开链接##A"))
    {
        ::ShellExecuteA(nullptr, "open", "https://tieba.baidu.com/p/8005920823", NULL, NULL, SW_SHOWNORMAL);
    }
    ImGui::TextWrapped(u8"更新帖（心灵终结吧）"); //ImGui::SameLine();
    if (ImGui::Button(u8"复制链接##B"))
    {
        ImGui::LogToClipboard();
        ImGui::LogText("https://tieba.baidu.com/p/8005924464");
        ImGui::LogFinish();
    }ImGui::SameLine();
    if (ImGui::Button(u8"打开链接##B"))
    {
        ::ShellExecuteA(nullptr, "open", "https://tieba.baidu.com/p/8005924464", NULL, NULL, SW_SHOWNORMAL);
    }
    ImGui::TextWrapped(u8"更新帖（心灵终结3ini吧）"); //ImGui::SameLine();
    if (ImGui::Button(u8"复制链接##C"))
    {
        ImGui::LogToClipboard();
        ImGui::LogText("https://tieba.baidu.com/p/8133473361");
        ImGui::LogFinish();
    }ImGui::SameLine();
    if (ImGui::Button(u8"打开链接##C"))
    {
        ::ShellExecuteA(nullptr, "open", "https://tieba.baidu.com/p/8133473361", NULL, NULL, SW_SHOWNORMAL);
    }
    ImGui::TextWrapped(u8"注：三个更新帖同步更新");
    ImGui::TextWrapped(u8"全部版本下载（百度网盘）"); //ImGui::SameLine();
    if (ImGui::Button(u8"复制链接##D"))
    {
        ImGui::LogToClipboard();
        ImGui::LogText("https://pan.baidu.com/s/1EpzAuIQfbU1-7sjb2YJocg?pwd=EASB");
        ImGui::LogFinish();
    }ImGui::SameLine();
    if (ImGui::Button(u8"打开链接##D"))
    {
        ::ShellExecuteA(nullptr, "open", "https://pan.baidu.com/s/1EpzAuIQfbU1-7sjb2YJocg?pwd=EASB", NULL, NULL, SW_SHOWNORMAL);
    }
    ImGui::TextWrapped(u8"提取码：EASB");
}
