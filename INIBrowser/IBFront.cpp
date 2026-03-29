
#include "FromEngine/Include.h"
#include "IBFront.h"
#include "FromEngine/RFBump.h"
#include "Global.h"
#include "IBSave.h"
#include "IBB_ModuleAlt.h"
#include "IBB_FileChecker.h"

extern int ScrX;//extern everything in Browser.h

void IBF_Thr_FrontLoop()
{
    BackThreadID = GetCurrentThreadId();
    uint64_t TimeWait = GetSysTimeMicros();
    if (EnableLog)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog(locc("Log_LaunchFrontThread"));
    }
    while (1)
    {
        int FrameRateLim = 50;
        if (SettingLoadComplete.load())FrameRateLim = IBG_GetSetting().FrameRateLimit;
        if (FrameRateLim != -1)
        {
            int Uax = 1000000 / FrameRateLim;//单位：微秒
            while (GetSysTimeMicros() < TimeWait)Sleep(Uax / 1000);
            if (Uax > 3000 && abs((int64_t)GetSysTimeMicros() - (int64_t)ShellLoopLastTime) < 1000ull)Sleep(Uax / 2000);
            TimeWait += Uax;
        }
        
        IBRF_CoreBump.IBF_AutoProc();
    }
}

bool IBF_Setting::ReadSetting(const wchar_t* Name)
{
    if (EnableLog)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog(locc("Log_FReadSetting"));
    }
    IBB_SettingRegisterRW(SettingFile);
    List.PackSetDefault();
    bool Ret = SettingFile.Read(Name);
    IBB_SetGlobalSetting(List.Pack);
    SettingLoadComplete.store(true);
    return Ret;
}

bool IBF_Setting::SaveSetting(const wchar_t* Name)
{
    bool Ret=SettingFile.Write(Name);
    SettingSaveComplete.store(true);
    return Ret;
}


const std::unordered_map<IBB_SettingType::_Type, std::function<IBF_SettingType(IBB_SettingType&)>> UploadSettingMap =
{
    {IBB_SettingType::None,[](IBB_SettingType& BSet)->IBF_SettingType
    {
        IBF_SettingType Ret;
        Ret.DescLong = BSet.DescLong;
        Ret.Action = [=]() -> bool
        {
            ImGui::Separator();
            ImGui::TextDisabled(BSet.DescShort.c_str());
            return ImGui::IsItemHovered();
        };
        return Ret;
    }},
    {IBB_SettingType::IntA,[](IBB_SettingType& BSet)->IBF_SettingType
    {
        IBF_SettingType Ret;
        Ret.DescLong = BSet.DescLong;
        Ret.Action = [=]() -> bool
        {
            int TempSet = *((int*)BSet.Data), MedInt;
            int Ori = TempSet;
            ImGui::InputInt(BSet.DescShort.c_str(), &TempSet, 1, 200);
            bool Drt = ImGui::IsItemHovered();
            MedInt = TempSet;
            bool a = false;
            for (int i = 1; i < (int)BSet.Limit.size(); i += 2)
            {
                if (*((const int*)BSet.Limit.at(i)) <= MedInt && MedInt <= *((const int*)BSet.Limit.at(i + 1)))
                {
                    a = true; break;
                }
            }
            if (!a)MedInt = *((const int*)BSet.Limit.at(0));
            if (MedInt != Ori)*BSet.Changing = true;
            else *BSet.Changing = false;
            *((int*)BSet.Data) = MedInt;
            TempSet = MedInt;
            return Drt;
        };
        return Ret;
    }},
    {IBB_SettingType::IntB,[](IBB_SettingType& BSet)->IBF_SettingType
    {
        IBF_SettingType Ret;
        Ret.DescLong = BSet.DescLong;
        Ret.Action = [=]() -> bool
        {
            int TempSet = *((int*)BSet.Data);
            int Ori = TempSet;
            ImGui::SliderInt(BSet.DescShort.c_str(), &TempSet, *(int*)BSet.Limit[0], *(int*)BSet.Limit[1], (const char*)BSet.Limit[2]);
            bool Drt = ImGui::IsItemHovered();
            if (TempSet != Ori)*BSet.Changing = true;
            else *BSet.Changing = false;
            *((int*)BSet.Data) = TempSet;
            return Drt;
        };
        return Ret;
    }},
    {IBB_SettingType::Bool,[](IBB_SettingType& BSet)->IBF_SettingType
    {
        IBF_SettingType Ret;
        Ret.DescLong = BSet.DescLong;
        Ret.Action = [=]() -> bool
        {
            *BSet.Changing = ImGui::Checkbox(BSet.DescShort.c_str(), (bool*)BSet.Data);
            if (*BSet.Changing && BSet.Limit.size() >= 2 && BSet.Limit[0] && BSet.Limit[1])
            {
                if (*(bool*)BSet.Data)((void(*)())BSet.Limit[1])();
                else ((void(*)())BSet.Limit[0])();
            }
            return ImGui::IsItemHovered();
        };
        return Ret;
    }},
    {IBB_SettingType::Lang,[](IBB_SettingType& BSet)->IBF_SettingType
    {
        IBF_SettingType Ret;
        Ret.DescLong = BSet.DescLong;
        Ret.Action = [=]() -> bool
        {
            return IBR_L10n::RenderUI(BSet.DescShort);
        };
        return Ret;
    }},
};

void IBF_Setting::UploadSettingBoard(std::function<void(const std::vector<IBF_SettingType>&)> Callback)
{
    std::vector<IBF_SettingType> Ret;

    for (auto Ty : List.Types)
    {
        Ret.push_back(UploadSettingMap.at(Ty.Type)(Ty));
    }

    
    IBRF_CoreBump.SendToR({ [=]()
        {
            auto CRet = Ret;
            Callback(CRet);
        }
    ,nullptr });
}







bool IBF_DefaultTypeList::ReadAltSetting(const wchar_t* Name)
{
    std::vector<std::wstring> FindFileVec(const std::wstring & pattern);
    using namespace std::string_literals;

    if (EnableLog)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog((u8"IBF_DefaultTypeList::ReadAltSetting ： " + loc("Log_ReadTypeAlt")).c_str());
    }
    bool Ret = true;

    for (auto& CSV : FindFileVec(Name + L".csv"s))
    {
        IBB_FileCheck(CSV, false, true, false);
        if (!List.LoadFromCSVFile(CSV.c_str()))Ret = false;
    }
    for (auto& JSON : FindFileVec(Name + L".json"s))
    {
        IBB_FileCheck(JSON, false, true, false);
        if (!List.LoadFromJsonFile(JSON.c_str()))Ret = false;
    }

    if (!List.LoadFromAlt())Ret = false;
    return Ret;
}

_TEXT_UTF8 std::string IBF_Project::GetText(bool PrintExtraData) const
{
    return Project.GetText(PrintExtraData);
}

bool IBF_Project::UpdateAll()
{
    return Project.UpdateAll();
}

void IBF_Project::RegenLinkedBy(const IBB_Section_Desc& Desc)
{
    auto& Arr = LinkedBy[Desc];
    Arr.clear();
    auto S = [&](IBB_NewLink& L) {
            if(L.ToLoc.Sec.ToDesc() == Desc)
                Arr.push_back(L);
        };
    for(auto& Ini:Project.Inis)
        for (auto& [K, Sec] : Ini.Secs)
        {
            if (Sec.IsLinkGroup)
                for (auto& L : Sec.LinkGroup_NewLinkTo)
                    S(L);
            else
                for (auto& Sub : Sec.SubSecs)
                    for (auto& L : Sub.NewLinkTo)
                        S(L);
        }
}
std::vector<IBB_NewLink>& IBF_Project::GetLinkedBy_Cached(const IBB_Section_Desc& Desc)
{
    return LinkedBy[Desc];
}
std::vector<IBB_NewLink>& IBF_Project::GetLinkedBy_NoCached(const IBB_Section_Desc& Desc)
{
    RegenLinkedBy(Desc);
    return LinkedBy[Desc];
}

bool IBF_Project::UpdateCreateSection(const IBB_Section_Desc& Desc)
{
    IBB_Section* pSec = const_cast<IBB_Section*>(Project.GetSec(Desc));
    if (pSec == nullptr)return false;
    return true;
}

void IBF_Project::Load(const IBS_Project& Proj)
{
    Project.CreateVersionMajor = Proj.CreateVersionMajor;
    Project.CreateVersionMinor = Proj.CreateVersionMinor;
    Project.CreateVersionRelease = Proj.CreateVersionRelease;
    Project.CreateTime = Proj.CreateTime;
    Project.LastUpdate = GetSysTimeMicros();
    Project.ProjName = Proj.ProjName;
    Project.Path = Proj.Path;
    Project.LastOutputDir = Proj.LastOutputDir;
    Project.LastOutputIniName.clear();
    for(auto& l: Proj.LastOutputIniName)
        Project.LastOutputIniName.insert(l);
    PersistentID = Proj.PersistentID;
    Project.IsNewlyCreated = false;
    Project.ChangeAfterSave = false;
}

void IBF_Project::Save(IBS_Project& Proj)
{
    Proj.CreateVersionMajor = Project.CreateVersionMajor;
    Proj.CreateVersionMinor = Project.CreateVersionMinor;
    Proj.CreateVersionRelease = Project.CreateVersionRelease;
    Proj.CreateTime = Project.CreateTime;
    Project.LastUpdate = GetSysTimeMicros();
    Proj.ProjName = Project.ProjName;
    Proj.Path = Project.Path;
    Proj.LastOutputDir = Project.LastOutputDir;
    Proj.LastOutputIniName.clear();
    Proj.LastOutputIniName.reserve(Project.LastOutputIniName.size());
    for (auto& l : Project.LastOutputIniName)
        Proj.LastOutputIniName.push_back(l);
    Proj.PersistentID = PersistentID;
    Project.IsNewlyCreated = false;
    Project.ChangeAfterSave = false;
}

namespace IBR_NodeSession
{
    void ClearSession();
}

void IBF_Project::Clear()
{
    CurrentProjectRID = 0;
    PersistentID = 0;
    Project.Clear();
    this->DisplayNames.clear();
    IBR_NodeSession::ClearSession();
    IBB_SectionID::ClearCache();
}
