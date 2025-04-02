
#include "FromEngine/Include.h"
#include "IBFront.h"
#include "FromEngine/RFBump.h"
#include "Global.h"
#include "IBSave.h"
#include "IBB_ModuleAlt.h"

extern int ScrX;//extern everything in Browser.h

void IBF_Thr_FrontLoop()
{
    BackThreadID = GetCurrentThreadId();
    uint64_t TimeWait = GetSysTimeMicros();
    if (EnableLog)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog("启动了IBF_Thr_FrontLoop。");
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
        GlobalLogB.AddLog("调用了IBF_Setting::ReadSetting。");
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







void IBF_DefaultTypeList::EnsureType(const std::string& Key, const std::string& LinkType)
{
    List.EnsureType(Key, LinkType);
}

const IBB_IniLine_Default* IBF_DefaultTypeList::GetDefault(const std::string& Key) const
{
    auto it = List.IniLine_Default.find(Key);
    if (it == List.IniLine_Default.end())return nullptr;
    else return std::addressof(it->second);
}

bool IBF_DefaultTypeList::ReadAltSetting(const char* Name)
{
    using namespace std::string_literals;
    IBB_DefaultTypeAltList Alt;
    if (EnableLog)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog("IBF_DefaultTypeList::ReadAltSetting ： 开始读取模块配置。");
    }
    bool Ret = true;
    //CSV then fall back to json
    if (!Alt.LoadFromCSVFile((Name + ".csv"s).c_str())
        && !Alt.LoadFromJsonFile((Name + ".json"s).c_str()))Ret = false;
    if (!List.LoadFromAlt(Alt))Ret = false;
    if (!List.BuildQuery())Ret = false;
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

bool IBF_Project::UpdateCreateSection(const IBB_Section_Desc& Desc)
{
    IBB_Section* pSec = const_cast<IBB_Section*>(Project.GetSec(Desc));
    if (pSec == nullptr)return false;
    bool Ret = true;
    for (auto& Ini : Project.Inis)for (auto& sp : Ini.Secs)
    {
        if (sp.second.IsLinkGroup)
        {
            for(auto& l:sp.second.LinkGroup_LinkTo)//Index不可能用于指向一个尚未存在的Section
                if ((!l.To.Ini.UseIndex && l.To.Ini.Name == Desc.Ini) && (!l.To.Section.UseIndex && l.To.Section.Name == Desc.Sec))
                {
                    l.DynamicCheck_Legal(Project);
                    pSec->LinkedBy.push_back(l);
                    auto& b = pSec->LinkedBy.back();
                    b.FillData(&l, l.FromKey);
                    l.Another = &b;
                }
        }
        else
        {
            for (auto& ss : sp.second.SubSecs)
            {
                for (auto& l : ss.LinkTo)
                {
                    if ((!l.To.Ini.UseIndex && l.To.Ini.Name == Desc.Ini) && (!l.To.Section.UseIndex && l.To.Section.Name == Desc.Sec))
                    {
                        l.DynamicCheck_Legal(Project);
                        pSec->LinkedBy.push_back(l);
                        auto& b = pSec->LinkedBy.back();
                        b.FillData(&l, l.FromKey);
                        l.Another = &b;
                    }
                }
                for (auto& [k, l] : ss.Lines)
                {
                    pSec->OnShow[k] = l.Default->IsLinkAlt() ? EmptyOnShowDesc : "";
                }
            }
        }
    }
    return Ret;
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
    Project.IsNewlyCreated = false;
    Project.ChangeAfterSave = false;
}
