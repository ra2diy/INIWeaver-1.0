#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include<imgui_internal.h>


bool IBR_Setting::IsReadSettingComplete()
{
    return SettingLoadComplete.load();
}

bool IBR_Setting::IsSaveSettingComplete()
{
    return SettingSaveComplete.load();
}

void IBR_Setting::SetSettingName(const wchar_t* Name)
{
    SettingName = Name;
}

void IBR_Setting::CallReadSetting()
{
    SettingLoadComplete.store(false);
    IBRF_CoreBump.SendToF({ [=]() {IBF_Inst_Setting.ReadSetting(SettingName); } });
}
void IBR_Setting::CallSaveSetting()
{
    SettingSaveComplete.store(false);
    IBRF_CoreBump.SendToF({ [=]() {IBF_Inst_Setting.SaveSetting(SettingName); } });
}

std::vector<IBF_SettingType> IBR_Inst_Setting_UIList;

void IBR_Setting::RefreshSetting()
{
    IBRF_CoreBump.SendToF({ [this]()
            {
                IBF_Inst_Setting.UploadSettingBoard([](const std::vector<IBF_SettingType>& Vec)
                    {
                        IBR_Inst_Setting_UIList = Vec;
                    });
            } });
}

void IBR_Setting::RenderUI()
{
    static bool Called = false;
    static bool First = true;
    static std::string DescLong = "";
    static ETimer::TimerClass Timer;

    if (Timer.GetClock() <= ETimer::__DurationZero || First)
    {
        First = false;
        CallSaveSetting();
        Timer.Set(ETimer::SecondToDuration(5));
    }

    if (IBR_Inst_Setting_UIList.empty())
    {
        if (!Called)
        {
            IBRF_CoreBump.SendToF({ [this]()
            {
                IBF_Inst_Setting.UploadSettingBoard([](const std::vector<IBF_SettingType>& Vec)
                    {
                        IBR_Inst_Setting_UIList = Vec;
                    });
            } });
            Called = true;
        }
        ImGui::TextDisabled(locc("GUI_SettingListLoading"));
    }
    else
    {
        auto CRgMax = ImGui::GetWindowContentRegionMax(), CRgMin = ImGui::GetWindowContentRegionMin();

        //ImGui::Text(u8"所有修改将在重启后生效");

        ImGui::BeginChild(113007, { CRgMax.x - CRgMin.x , CRgMax.y - CRgMin.y - FontHeight * 10.0f },
            false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
        bool Appear = false;
        for (auto& Li : IBR_Inst_Setting_UIList)
        {
            if (Li.Action())
            {
                DescLong = Li.DescLong;
                Appear = true;
            }
        }

        /*

        ImGui::NewLine();
        ImGui::NewLine();
        if (ImGui::Button(u8"打开config.ini"))
            ::ShellExecuteA(nullptr, "open", ".\\Resources\\Config.ini", NULL, NULL, SW_SHOWNORMAL);
        if (ImGui::IsItemHovered())
        {
            DescLong = u8"一部分设置错误会导致无法启动程序\n不建议随意修改\n这部分设置放在Config.ini中\n改动将在重启后生效";
            Appear = true;
        }
        */

        if (!Appear)DescLong = "";

        ImGui::EndChild();

        ImGui::BeginChildFrame(113003, { CRgMax.x - CRgMin.x , FontHeight * 6.0f });
        ImGui::TextWrapped(DescLong.c_str());
        ImGui::EndChildFrame();

        bool Changing = false;
        for (auto& M : IBF_Inst_Setting.List.Types)
        {
            if (*M.Changing)
            {
                *M.Changing = false;
                Changing = true;
            }
        }
        if (Changing)CallSaveSetting();
    }
}
