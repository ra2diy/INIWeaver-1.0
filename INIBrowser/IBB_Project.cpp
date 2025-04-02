#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include<imgui_internal.h>

bool IBB_Project::CreateIni(const std::string& Name)
{
    if (EnableLogEx)
    {
        sprintf_s(LogBufB, "IBB_Project::CreateIni : <- Name=%s", Name.c_str());
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog(LogBufB);
    }
    IBB_Project_Index IniIndex(Name, "");
    auto Ini = GetIni(IniIndex);
    if (Ini == nullptr)
    {
        Inis.emplace_back();
        Inis.back().Name = Name;
        Inis.back().Root = this;
        return true;
    }
    else return false;
}
bool IBB_Project::AddIni(const IBB_Ini& Ini, bool IsDuplicate)
{
    IBB_Project_Index IniIndex(Ini.Name, "");
    auto IniF = GetIni(IniIndex);
    if (IniF == nullptr)
    {
        Inis.emplace_back(Ini);
        Inis.back().Root = this;
        return true;
    }
    else return IniF->Merge(Ini, IsDuplicate);
}

const char* __WTF__ = "_KENOSIS_SB_";

bool IBB_Project::CreateRegisterList(const std::string& Name, const std::string& IniName)
{
    IBB_DIndex Index(Name + __WTF__ + IniName);
    auto It = Index.Search<IBB_RegisterList>(RegisterLists, true, [](const IBB_RegisterList& L) {return L.Type + __WTF__ + L.IniType; });
    if (It == RegisterLists.end())
    {
        RegisterLists.emplace_back();
        RegisterLists.back().Type = Name;
        RegisterLists.back().IniType = IniName;
        RegisterLists.back().Root = this;
        return true;
    }
    else return false;
}
bool IBB_Project::AddRegisterList(const IBB_RegisterList& List)
{
    IBB_DIndex Index(List.Type + __WTF__ + List.IniType);
    auto It = Index.Search<IBB_RegisterList>(RegisterLists, true, [](const IBB_RegisterList& L) {return L.Type + __WTF__ + L.IniType; });
    if (It == RegisterLists.end())
    {
        RegisterLists.emplace_back(List);
        RegisterLists.back().Root = this;
        return true;
    }
    else return It->Merge(List);
}
IBB_RegisterList& IBB_Project::GetRegisterList(const std::string& Name, const std::string& IniName)
{
    IBB_DIndex Index(Name + __WTF__ + IniName);
    auto It = Index.Search<IBB_RegisterList>(RegisterLists, true, [](const IBB_RegisterList& L) {return L.Type + __WTF__ + L.IniType; });
    if (It == RegisterLists.end())
    {
        RegisterLists.emplace_back();
        RegisterLists.back().Type = Name;
        RegisterLists.back().IniType = IniName;
        RegisterLists.back().Root = this;
        return RegisterLists.back();
    }
    else return *It;
}
bool IBB_Project::RegisterSection(const std::string& Name, const std::string& IniName, IBB_Section& Section)
{
    IBB_RegisterList& Li = GetRegisterList(Name, IniName);
    Li.List.push_back(const_cast<IBB_Section*>(&Section));
    Section.Register = Li.Type;
    return true;
}
bool IBB_Project::RegisterSection(size_t RegListID, IBB_Section& Section)
{
    if (RegListID >= RegisterLists.size())return false;
    RegisterLists.at(RegListID).List.push_back(const_cast<IBB_Section*>(&Section));
    Section.Register = RegisterLists.at(RegListID).Type;
    return true;
}
IBB_Section* IBB_Project::AddNewSection(const IBB_Section_NameType& Paragraph)
{
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_Project::AddNewSection <- IBB_Section_NameType Paragraph=%p(Name=%s)", &Paragraph, Paragraph.Name.c_str()); GlobalLogB.AddLog(LogBufB);
        auto Ret = AddNewSectionEx(Paragraph);
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_Project::AddNewSection -> IBB_Section* Ret=%p", Ret); GlobalLogB.AddLog(LogBufB);
        return Ret;
    }
    else return AddNewSectionEx(Paragraph);
}
IBB_Section* IBB_Project::AddNewSectionEx(const IBB_Section_NameType& Paragraph)
{
    IBB_Project_Index Tg(Paragraph.IniType, Paragraph.Name);
    auto Sc = Tg.GetSec(*this);
    if (Sc != nullptr)return nullptr;//this is AddNewSection plz don't give me an existing paragraph plz
    auto SIni = Tg.GetIni(*this);
    if (SIni == nullptr)
    {
        CreateIni(Paragraph.IniType);
        SIni = Tg.GetIni(*this);
        if (SIni == nullptr)
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_Project::AddNewSection ：无法添加INI。");
            }
            return nullptr;
        }
    }
    if (!SIni->CreateSection(Paragraph.Name))
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog("IBB_Project::AddNewSection ：CreateSection创建失败。");
        }
        return nullptr;
    }
    Sc = Tg.GetSec(*this);

    if (Sc == nullptr)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog("IBB_Project::AddNewSection ：无法添加字段。");
        }
        return nullptr;
    }
    if (Sc->Generate(Paragraph))return Sc;
    else return nullptr;
}
IBB_Section* IBB_Project::CreateNewSection(const IBB_Section_Desc& Desc)
{
    IBB_Project_Index Tg(Desc.Ini, Desc.Sec);
    auto Sc = Tg.GetSec(*this);
    if (Sc != nullptr)return nullptr;//this is AddNewSection plz don't give me an existing paragraph plz
    auto SIni = Tg.GetIni(*this);
    if (SIni == nullptr)
    {
        CreateIni(Desc.Ini);
        SIni = Tg.GetIni(*this);
        if (SIni == nullptr)
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_Project::CreateNewSection ：无法添加INI。");
            }
            return nullptr;
        }
    }
    if (!SIni->CreateSection(Desc.Sec))
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog("IBB_Project::CreateNewSection ：CreateSection创建失败。");
        }
        return nullptr;
    }
    Sc = Tg.GetSec(*this);
    if (Sc == nullptr)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog("IBB_Project::CreateNewSection ：无法添加字段。");
        }
        return nullptr;
    }
    return Sc;
}

bool IBB_Project::AddNewLinkToLinkGroup(const IBB_Section_Desc& From, const IBB_Section_Desc& To)
{
    IBB_Project_Index FIn(From), TIn(To);
    auto FromPtr = FIn.GetSec(*this);
    if (FromPtr == nullptr)
    {
        FromPtr = CreateNewSection(From);
        if (FromPtr == nullptr)
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_Project::AddNewLinkToLinkGroup ：无法添加From字段。");
            }
            return false;
        }
    }
    if (!FromPtr->IsLinkGroup)return false;
    auto ToPtr = TIn.GetSec(*this);
    if (ToPtr == nullptr)
    {
        ToPtr = CreateNewSection(To);
        if (ToPtr == nullptr)
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_Project::AddNewLinkToLinkGroup ：无法添加From字段。");
            }
            return false;
        }
    }
    FromPtr->LinkGroup_LinkTo.push_back({ nullptr,FIn,TIn });
    //ToPtr->LinkedBy.push_back({ nullptr,FIn,TIn });
    //auto& a = FromPtr->LinkGroup_LinkTo.back(), b = ToPtr->LinkedBy.back();
    //a.FillData(&b, "");
    //b.FillData(&a, "");
    return true;
}

bool IBB_Project::AddModule(const IBB_ModuleAlt& Module)
{
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_Project::AddModule -> IBB_ModuleAlt Module=%p(Name=%s)", &Module, Module.Name.c_str());
        GlobalLogB.AddLog(LogBufB);
    }

    bool Ret = true;

    for (auto& R : Module.Modules)
    {
        if (R.IsLinkGroup)continue;
        if (!AddModule(R))Ret = false;
    }

    for (auto& R : Module.Modules)
    {
        if (!R.IsLinkGroup)continue;
        if (!AddModule(R))Ret = false;
    }

    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_Project::AddModule -> bool Ret=%s", IBD_BoolStr(Ret)); GlobalLogB.AddLog(LogBufB);
    }

    return Ret;
}

bool IBB_Project::AddModule(const ModuleClipData& Module)
{
    IBB_Project_Index Tg(Module.Desc.A, Module.Desc.B);
    auto Sc = Tg.GetSec(*this);
    if (Sc == nullptr)
    {
        auto SIni = Tg.GetIni(*this);
        if (SIni == nullptr)
        {
            CreateIni(Module.Desc.A);
            SIni = Tg.GetIni(*this);
            if (SIni == nullptr)
            {
                if (EnableLog)
                {
                    GlobalLogB.AddLog_CurTime(false);
                    GlobalLogB.AddLog("IBB_Project::AddNewSection ：无法添加INI。");
                }
                return false;
            }
        }
        if (!SIni->CreateSection(Module.Desc.B))
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_Project::AddNewSection ：CreateSection创建失败。");
            }
            return false;
        }
        Sc = Tg.GetSec(*this);
    }
    if (Sc == nullptr)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog("IBB_Project::AddNewSection ：无法添加字段。");
        }
        return false;
    }
    auto& Li = GetRegisterList(Module.Register, IBB_DefaultRegType::GetRegType(Module.Register).IniType);
    Li.List.push_back(Sc);
    return Sc->Generate(Module);
}

//我也不太确定就这吗
bool IBB_Project::UpdateAll()
{
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_Project::UpdateAll <- void"); GlobalLogB.AddLog(LogBufB);
    }
    bool Ret = true;
    for (auto& Ini : Inis)for (auto& sp : Ini.Secs)
    {
        sp.second.LinkedBy.clear();
        for (auto& ss : sp.second.SubSecs)ss.LinkTo.clear();
    }
    for (auto& Ini : Inis)if (!Ini.UpdateAll())Ret = false;
    //GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("——IBB_Project::UpdateAll——");//BREAKPOINT
    //GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("——测试：触发Update——");//BREAKPOINT
    for (auto& Ini : Inis)for (auto& sp : Ini.Secs)
    {
        if (sp.second.IsLinkGroup)
        {
            int iOrd = 0;
            for (auto& L : sp.second.LinkGroup_LinkTo)
            {
                L.DynamicCheck_Legal(*this);
                L.DynamicCheck_UpdateNewLink(*this);
                L.Order = iOrd;
                L.OrderEx = INT_MAX;
                ++iOrd;
            }
        }
        else
        {
            int iOrdEx = 0;
            for (auto& ss : sp.second.SubSecs)
            {
                int iOrd = 0;
                for (auto& L : ss.LinkTo)
                {
                    L.DynamicCheck_Legal(*this);
                    L.DynamicCheck_UpdateNewLink(*this);
                    L.Order = iOrd;
                    L.OrderEx = iOrdEx;
                    ++iOrd;
                    if (EnableLogEx)
                    {
                        GlobalLogB.AddLog_CurTime(false);
                        GlobalLogB.AddLog(L.GetText(*this).c_str(), false);//BREAKPOINT
                    }
                }
                ++iOrdEx;
            }
        }
    }
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_Project::UpdateAll -> bool Ret=%s", IBD_BoolStr(Ret)); GlobalLogB.AddLog(LogBufB);
    }
    return Ret;
}

_TEXT_UTF8 std::string IBB_Project::GetText(bool PrintExtraData) const
{
    std::string Text{ ";Project::GetText\n\n" };
    {
        Text = "[ProjectBasicInfo]\n";
        Text += "ProjName=" + UnicodetoMBCS(ProjName); Text.push_back('\n');
        Text += "CreateTime=" + std::to_string(CreateTime); Text.push_back('\n');
        Text += "LastUpdate=" + std::to_string(LastUpdate); Text.push_back('\n');
        Text += "CreateVersion=" + std::to_string(CreateVersionMajor) + "." +
            std::to_string(CreateVersionMinor) + "." + std::to_string(CreateVersionRelease);
        Text.push_back('\n');
    }
    Text += "\n;INI Files\n";
    for (const auto& ini : Inis)
    {
        Text += ini.GetText(PrintExtraData);
        Text.push_back('\n');
    }
    Text += "\n;Register Lists\n";
    for (const auto& list : RegisterLists)
    {
        Text += list.GetText(PrintExtraData);
        Text.push_back('\n');
    }
    return Text;
}

void IBB_Project::Clear()
{
    ProjName.clear();
    Path.clear();
    LastOutputDir.clear();
    IsNewlyCreated = true;
    ChangeAfterSave = false;
    RegisterLists.clear();
    Inis.clear();
    LastUpdate = GetSysTimeMicros();
    CreateTime = GetSysTimeMicros();
    CreateVersionMajor = VersionMajor;
    CreateVersionMinor = VersionMinor;
    CreateVersionRelease = VersionRelease;
}

