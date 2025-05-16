#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include "IBB_Index.h"
#include<imgui_internal.h>

const char* DefaultSubSecName = "_DEFAULT_SUBSEC";
const char* DefaultAltPropType = "\"D\"";
const char* LinkAltPropType = "\"L\"";

void IBB_DefaultTypeList::EnsureType(const IBB_DefaultTypeAlt& D, std::set<std::string>* UsedStrings)
{
    auto& L = IniLine_Default[D.Name];
    L.Name = D.Name;
    L.Platform = { "" };
    L.Limit.Type = "String";
    L.Limit.Lim = D.Name;
    L.DescShort = D.DescShort;
    L.DescLong = D.DescLong;
    L.Color = D.Color;
    if (D.LinkType == "enum")
    {
        L.Property.Type = DefaultAltPropType;
        L.Property.Lim = JsonObject(reinterpret_cast<cJSON*>(D.LinkLimit));
        L.Property.TypeAlt = D.LinkType;
        L.Property.Enum = D.EnumVector;
        L.Property.EnumValue = D.EnumValue;
    }
    else
    {
        if (D.LinkType.empty() || D.LinkLimit == 0 || D.LinkType == "bool")
        {
            L.Property.Type = DefaultAltPropType;
            L.Property.Lim = JsonObject(nullptr);
            L.Property.TypeAlt = D.LinkType;
        }
        else
        {
            L.Property.Type = LinkAltPropType;
            L.Property.Lim = JsonObject(reinterpret_cast<cJSON*>(D.LinkLimit));
            L.Property.TypeAlt = D.LinkType;
            if (UsedStrings)
                UsedStrings->insert(D.LinkType);
            else
            {
                auto& TheOnlySubSec = SubSec_Default[DefaultSubSecName];
                TheOnlySubSec.Lines_ByName.push_back(D.Name);
                TheOnlySubSec.Lines[D.Name] = L;

                auto& K = Link_Default[D.LinkType];
                K.Name = D.LinkType;
                K.NameOnlyAsRegister = true;
                IBB_DefaultRegType::EnsureRegType(D.LinkType);

            }
        
        }
    }
}

void IBB_DefaultTypeList::EnsureType(const std::string& Key, const std::string& LinkType)
{
    auto it = IniLine_Default.find(Key);
    if (it != IniLine_Default.end())return;//Exists
    {
        auto K = UTF8toUnicode(Key);
        auto LT = UTF8toUnicode(LinkType);
        MessageBoxW(NULL, std::vformat(locw("Error_LinkTypeNotExist"), std::make_wformat_args(K, LT)).c_str(),
            L"IBB_DefaultTypeList::EnsureType", MB_OK);
    }
    IBB_DefaultTypeAlt Alt;
    Alt.LinkType = LinkType;
    Alt.Name = Key;
    Alt.LinkLimit = -1;
    Alt.DescShort = Alt.DescLong = Key;
    EnsureType(Alt);
}

bool IBB_DefaultTypeList::LoadFromAlt(const IBB_DefaultTypeAltList& AltList)
{
    Require_Default.clear();

    std::set<std::string> UsedStrings;
    for (const auto& D : AltList.List)EnsureType(D, &UsedStrings);

    auto& TheOnlySubSec = SubSec_Default[DefaultSubSecName];
    TheOnlySubSec.Name = DefaultSubSecName;
    TheOnlySubSec.DescShort = "";
    TheOnlySubSec.DescLong = "";
    TheOnlySubSec.Platform = { "" };
    TheOnlySubSec.Lines_ByName.reserve(IniLine_Default.size());
    for (auto& [k, v] : IniLine_Default)
        TheOnlySubSec.Lines_ByName.push_back(k);
    TheOnlySubSec.Lines = IniLine_Default;
    TheOnlySubSec.Require.RequiredValues.clear();
    TheOnlySubSec.Require.ForbiddenValues.clear();

    IBB_DefaultTypeAlt b;
    b.Name = "__INHERIT__";
    b.LinkLimit = 1;
    b.LinkType = "_AnyType";
    b.DescShort = "__INHERIT_SHORT__";
    b.DescLong = "";
    b.Color = ImColor(0, 0, 255);
    EnsureType(b, &UsedStrings);
    auto& InheritSubSec = SubSec_Default["  INHERIT_SUBSEC__"];
    InheritSubSec.Name = "  INHERIT_SUBSEC__";
    InheritSubSec.DescShort = "";
    InheritSubSec.DescLong = "";
    InheritSubSec.Platform = { "" };
    InheritSubSec.Lines_ByName = { "__INHERIT__" };
    InheritSubSec.Lines["__INHERIT__"] = IniLine_Default["__INHERIT__"];
    InheritSubSec.Require.RequiredValues.clear();
    InheritSubSec.Require.ForbiddenValues.clear();

    for (const auto& s : UsedStrings)
    {
        auto& L = Link_Default[s];
        L.Name = s;
        L.NameOnlyAsRegister = true;
        IBB_DefaultRegType::EnsureRegType(s);
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

std::vector<std::string> IBB_DefaultTypeAlt::EnumSplit(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

bool IBB_DefaultTypeAlt::Load(JsonObject FromJson)
{
    Name = FromJson.ItemStringOr("Name");
    DescLong = FromJson.ItemStringOr("DescLong");
    DescShort = FromJson.ItemStringOr("DescShort");
    LinkType = FromJson.ItemStringOr("LinkType");
    LinkLimit = FromJson.ItemIntOr("LinkLimit", 1);
    Color = StrToCol(FromJson.ItemStringOr("LineColor", "00000000").c_str());
    return true;
}

bool IBB_DefaultTypeAlt::Load(const std::vector<std::string>& FromCSV)
{
    if (FromCSV.size() < 5)return false;
    // Name LinkType LinkLimit DescShort DescLong;
    Name = FromCSV[0];
    LinkType = FromCSV[1];
    LinkLimit = atoi(FromCSV[2].c_str());
    DescShort = FromCSV[3];
    DescLong = FromCSV[4];
    Color = StrToCol(FromCSV.size() > 5 ? FromCSV[5].c_str() : "00000000");
    EnumVector = FromCSV.size() > 6 ? EnumSplit(FromCSV[6], ',') : std::vector<std::string>{};
    EnumValue = FromCSV.size() > 7 ? EnumSplit(FromCSV[7], ',') : std::vector<std::string>{};
    return true;
}

bool IBB_DefaultTypeAltList::Load(JsonObject FromJson)
{
    auto V = FromJson.ItemArrayObjectOr("IniLine");
    List.resize(V.size());
    for (size_t i = 0; i < V.size(); i++)
        List[i].Load(V[i]);
    return true;
}

bool IBB_DefaultTypeAltList::LoadFromJsonFile(const char* Name)
{
    std::string FileName(const std::string & ss);
    JsonFile F;
    auto V = UTF8toUnicode(FileName(Name));
    IBR_PopupManager::AddJsonParseErrorPopup(F.ParseFromFileChecked(Name, loc("Error_JsonParseErrorPos"), nullptr),
        UnicodetoUTF8(std::vformat(locw("Error_JsonSyntaxError"), std::make_wformat_args(V))));
    if (!F.Available())return false;
    Load(F);

    ExtFileClass Et;
    Et.Open(".\\Global\\TypeAlt.csv", "w");
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

    return true;
}

bool IBB_DefaultTypeAltList::LoadFromCSVFile(const char* Name)
{
    CSVReader Reader;
    Reader.ReadFromFile(Name);
    auto& D = Reader.GetData();
    if (D.size() <= 1)return false;
    List.resize(D.size());
    bool Ret = true;
    for (size_t i = 1; i < D.size(); i++)
        Ret &= List[i].Load(D[i]);
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
    if (Paragraph.IniType.empty() || Paragraph.Name.empty())
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog((u8"IBB_Project::AddNewSection ：" + loc("Log_NoEmptyArgument")).c_str());
        }
        return nullptr;
    }
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
                GlobalLogB.AddLog((u8"IBB_Project::AddNewSection ：" + loc("Log_CannotCreateINI")).c_str());
            }
            return nullptr;
        }
    }
    if (!SIni->CreateSection(Paragraph.Name))
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog((u8"IBB_Project::AddNewSection ：" + loc("Log_CreateSectionFailed")).c_str());
        }
        return nullptr;
    }
    Sc = Tg.GetSec(*this);

    if (Sc == nullptr)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog((u8"IBB_Project::AddNewSection ：" + loc("Log_CannotCreateSection")).c_str());
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
                GlobalLogB.AddLog((u8"IBB_Project::AddNewLinkToLinkGroup ：" + loc("Log_CannotCreateSection")).c_str());
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
                GlobalLogB.AddLog((u8"IBB_Project::AddNewLinkToLinkGroup ：" + loc("Log_CannotCreateSection")).c_str());
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

