#include "IBR_Components.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include "IBB_Index.h"
#include <imgui_internal.h>

const char* InheritSubSecName = "A__INHERIT_SUBSEC";
const char* DefaultSubSecName = "B__DEFAULT_SUBSEC";
const char* ImportSubSecName  = "D__IMPORT_SUBSEC";

namespace IBB_DefaultRegType
{
    extern std::unordered_set<StrPoolID> RingCheckKeys;
    extern std::unordered_map<StrPoolID, IBB_SubSec_Default::_Type> InSubSecKeys;
}


void IBB_DefaultTypeList::EnsureType(const IBB_DefaultTypeAlt& D)
{
    auto& L = IniLine_Default[D.Name];
    L.Name = D.Name;
    L.DescShort = D.DescShort;
    L.DescLong = D.DescLong;
    L.LinkNode.LinkCol = D.Color;
    L.Known = true;
    L.Input = &IBB_DefaultRegType::GetInputType(PoolStr(D.Input));
    L.InputName = D.Input;
    L.LinkNode.LinkType = D.LinkType;
    L.LinkNode.LinkLimit = D.LinkLimit;
    IBB_DefaultRegType::EnsureRegType(PoolStr(D.LinkType));
}

bool IBB_DefaultTypeList::LoadFromAlt()
{
    //Subsec在模块上按照字典序排列，不能随便起名字，否则可能会影响显示顺序
    auto& DefaultSubSec = SubSec_Default[DefaultSubSecName];
    DefaultSubSec.Name = DefaultSubSecName;
    DefaultSubSec.Type = IBB_SubSec_Default::Default;

    auto& InheritSubSec = SubSec_Default[InheritSubSecName];
    InheritSubSec.Name = InheritSubSecName;
    InheritSubSec.Type = IBB_SubSec_Default::Inherit;

    auto& ImportSubSec = SubSec_Default[ImportSubSecName];
    ImportSubSec.Name = ImportSubSecName;
    ImportSubSec.Type = IBB_SubSec_Default::Import;

    //默认都在DefaultSubSec里，然后再分出去
    for (auto& [k, v] : IniLine_Default)v.InSubSec = &DefaultSubSec;

    for (auto& [k, v] : IBB_DefaultRegType::InSubSecKeys)
    {
        if (v == IBB_SubSec_Default::Inherit)IniLine_Default[k].InSubSec = &InheritSubSec;
        else if (v == IBB_SubSec_Default::Import)IniLine_Default[k].InSubSec = &ImportSubSec;
        else IniLine_Default[k].InSubSec = &DefaultSubSec;
    }

    return true;
}

ImU32 StrToCol(const std::string& Str)
{
    ImU32 V = strtol(Str.c_str(), nullptr, 16);
    if (V > 0x1000000 || Str.length() > 6)
    {
        //ABGR
        return V;
    }
    else
    {
        return V >> 16 | (V & 0xFF) << 16 | (V & 0xFF00) | 0xFF000000;
    }
}

const char* SelectDefaultInput(const std::string& LinkType)
{
    if (!_strcmpi(LinkType.c_str(), "bool"))return "Bool";
    if (!_strcmpi(LinkType.c_str(), "string"))return "String";
    return "Link";
}

StrPoolID SelectDefaultInput(StrPoolID LinkType)
{
    auto Str = PoolStr(LinkType);
    if (!_strcmpi(Str.c_str(), "bool"))return NewPoolStr("Bool");
    if (!_strcmpi(Str.c_str(), "string"))return NewPoolStr("String");
    return NewPoolStr("Link");
}

void IBB_DefaultTypeAlt::Clear()
{
    Name = EmptyPoolStr;
    DescShort = EmptyPoolStr;
    DescLong = EmptyPoolStr;
    LinkType = EmptyPoolStr;
    Input = EmptyPoolStr;
    LinkLimit = 1;
    Color = 0xFF000000;
}

bool IBB_DefaultTypeAlt::Load(JsonObject FromJson)
{
    Name = NewPoolStr(FromJson.ItemStringOr("Name"));
    DescLong = NewPoolDesc(FromJson.ItemStringOr("DescLong"));
    DescShort = NewPoolDesc(FromJson.ItemStringOr("DescShort"));
    LinkType = NewPoolStr(FromJson.ItemStringOr("LinkType"));
    LinkLimit = FromJson.ItemIntOr("LinkLimit", 1);
    Color = StrToCol(FromJson.ItemStringOr("LineColor", "00000000").c_str());
    auto InStr = FromJson.ItemStringOr("InputType", "");
    Input = InStr.empty() ? SelectDefaultInput(LinkType) : NewPoolStr(InStr);
    return true;
}

bool IBB_DefaultTypeAlt::Load(const std::vector<std::string>& FromCSV)
{
    if (FromCSV.size() < 5)return false;
    // Name LinkType LinkLimit DescShort DescLong;
    Name = NewPoolStr(FromCSV[0]);
    LinkType = NewPoolStr(FromCSV[1]);
    LinkLimit = atoi(FromCSV[2].c_str());
    DescShort = NewPoolDesc(IBR_L10n::ProcessEscape(FromCSV[3]));
    DescLong = NewPoolDesc(IBR_L10n::ProcessEscape(FromCSV[4]));
    Color = StrToCol((FromCSV.size() > 5 && !FromCSV[5].empty()) ? FromCSV[5].c_str() : "00000000");
    Input = (FromCSV.size() > 6 && !FromCSV[6].empty()) ? NewPoolStr(FromCSV[6]) : SelectDefaultInput(LinkType);
    return true;
}

bool IBB_DefaultTypeList::LoadFromJsonObject(JsonObject FromJson)
{
    auto V = FromJson.ItemArrayObjectOr("IniLine");

    IBB_DefaultTypeAlt Alt;
    for (size_t i = 0; i < V.size(); i++)
    {
        Alt.Clear();
        Alt.Load(V[i]);
        EnsureType(Alt);
    }

    return true;
}

bool IBB_DefaultTypeList::LoadFromJsonFile(const wchar_t* Name)
{
    std::wstring FileName(const std::wstring & ss);
    JsonFile F;
    auto V = FileName(Name);
    IBR_PopupManager::AddJsonParseErrorPopup(F.ParseFromFileChecked(UnicodetoUTF8(Name).c_str(), loc("Error_JsonParseErrorPos"), nullptr),
        UnicodetoUTF8(std::vformat(locw("Error_JsonSyntaxError"), std::make_wformat_args(V))));
    if (!F.Available())return false;
    LoadFromJsonObject(F);

    //Should no longer auto convert to CSV after 1.0
    /*
    ExtFileClass Et;
    Et.Open((Name + std::wstring(L".csv")).c_str(), L"w");
    for (auto& L : List)
    {
        Et.PutStr(L.Name); Et.PutChr(',');
        Et.PutStr(L.LinkType); Et.PutChr(',');
        Et.PutStr(std::to_string(L.LinkLimit)); Et.PutChr(',');
        Et.PutStr(L.DescShort); Et.PutChr(',');
        Et.PutStr(L.DescLong); Et.PutChr('\n');
    }
    Et.Close();
    MessageBoxW(NULL, locwc("GUI_TypeAltConverted"), _AppNameW, MB_OK);
    */

    return true;
}

bool IBB_DefaultTypeList::LoadFromCSVFile(const wchar_t* Name)
{
    CSVReader Reader;
    Reader.ReadFromFile(Name);
    auto& D = Reader.GetData();
    if (D.size() <= 1)return false;
    bool Ret = true;

    IBB_DefaultTypeAlt Alt;
    for (size_t i = 1; i < D.size(); i++)
    {
        Alt.Clear();
        Alt.Load(D[i]);
        EnsureType(Alt);
    }

    return Ret;
}


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
    Section.Register = NewPoolStr(Li.Type);
    return true;
}
bool IBB_Project::RegisterSection(size_t RegListID, IBB_Section& Section)
{
    if (RegListID >= RegisterLists.size())return false;
    RegisterLists.at(RegListID).List.push_back(const_cast<IBB_Section*>(&Section));
    Section.Register = NewPoolStr(RegisterLists.at(RegListID).Type);
    return true;
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
                GlobalLogB.AddLog((u8"IBB_Project::CreateNewSection ：" + loc("Log_CannotCreateINI")).c_str());
            }
            return nullptr;
        }
    }
    if (!SIni->CreateSection(Desc.Sec))
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog((u8"IBB_Project::CreateNewSection ：" + loc("Log_CreateSectionFailed")).c_str());
        }
        return nullptr;
    }
    Sc = Tg.GetSec(*this);
    if (Sc == nullptr)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog((u8"IBB_Project::CreateNewSection ：" + loc("Log_CannotCreateSection")).c_str());
        }
        return nullptr;
    }
    return Sc;
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
                    GlobalLogB.AddLog((u8"IBB_Project::AddModule ：" + loc("Log_CannotCreateINI")).c_str());
                }
                return false;
            }
        }
        if (!SIni->CreateSection(Module.Desc.B))
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog((u8"IBB_Project::AddModule ：" + loc("Log_CreateSectionFailed")).c_str());
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
            GlobalLogB.AddLog((u8"IBB_Project::AddModule ：" + loc("Log_CreateSectionFailed")).c_str());
        }
        return false;
    }
    auto& Li = GetRegisterList(Module.Register, IBB_DefaultRegType::GetRegType(Module.Register).IniType);
    Li.List.push_back(Sc);
    return Sc->Generate(Module);
}

size_t& SPCacheSize();
void RecalcSPCacheSize(IBB_Project&, size_t&);

//我也不太确定就这吗
bool IBB_Project::UpdateAll()
{
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_Project::UpdateAll <- void"); GlobalLogB.AddLog(LogBufB);
    }
    bool Ret = true;
    for (auto& Ini : Inis)if (!Ini.UpdateAll())Ret = false;
    
    RecalcSPCacheSize(*this, SPCacheSize());
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
        Text += "ProjName=" + UnicodetoUTF8(ProjName); Text.push_back('\n');
        Text += "Path=" + UnicodetoUTF8(Path); Text.push_back('\n');
        Text += "CreateTime=" + std::to_string(CreateTime); Text.push_back('\n');
        Text += "LastUpdate=" + std::to_string(LastUpdate); Text.push_back('\n');
        Text += "CreateVersion=" + GetVersionStr(CreateVersionMajor * 10000 + CreateVersionMinor * 100 + CreateVersionRelease); Text.push_back('\n');
        Text += "ChangeAfterSave=" + std::string(IBD_BoolStr(ChangeAfterSave)); Text.push_back('\n');
        Text += "LastOutputDir=" + UnicodetoUTF8(LastOutputDir); Text.push_back('\n');
        for (auto& [k, v] : LastOutputIniName)
        {
            Text += "LastOutputIniName[" + k + "]=" + UnicodetoUTF8(v); Text.push_back('\n');
        }
        Text += "IsNewlyCreated=" + std::string(IBD_BoolStr(IsNewlyCreated)); Text.push_back('\n');

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

void RecalcSPCacheSize(IBB_Project& Proj, size_t& Sz)
{
    size_t NewSz = 0;
    for (auto& ini : Proj.Inis)
        for (auto& [secname, sec] : ini.Secs)
            NewSz += sec.LineOrder.size();
    Sz = NewSz;
}

void IBB_Project::Clear()
{
    ProjName.clear();
    Path.clear();
    LastOutputDir.clear();
    LastOutputIniName.clear();
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

bool IBB_Project::IsEmpty() const
{
    for (const auto& ini : Inis)
    {
        if (!ini.Secs.empty())
        {
            return false;
        }
    }
    return true;
}

