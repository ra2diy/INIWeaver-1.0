
#include "FromEngine/Include.h"
#include "FromEngine/global_tool_func.h"
#include "IBB_Index.h"
#include "Global.h"
#include "IBB_RegType.h"
#include "IBR_LinkNode.h"
#include <ranges>

const char* Internal_IniName = "_LINKGROUP_INI_FILE";
extern const char* LinkAltPropType;

namespace ExportContext
{
    extern StrPoolID Key;
    extern size_t SameKeyIdx;//用于当Key重复时区分不同的Key
    extern bool OnExport;
}

/*
bool IBB_IniLine_Default::Load(JsonObject FromJson)
{
    bool Ret = true;

    Platform = FromJson.GetObjectItem(u8"Platform").GetArrayString();
    Name = FromJson.GetObjectItem(u8"Name").GetString();
    DescShort = FromJson.GetObjectItem(u8"DescShort").GetString();
    DescLong = FromJson.GetObjectItem(u8"DescLong").GetString();

    auto JLimit = FromJson.GetObjectItem(u8"Limit");
    Limit.Type = JLimit.GetObjectItem(u8"Type").GetString();
    Limit.Lim = JLimit.GetObjectItem(u8"Data").GetString();

    auto JProp = FromJson.GetObjectItem(u8"Prop");
    Property.Type = JProp.GetObjectItem(u8"Type").GetString();
    auto It = IBB_IniLine_ProcessMap.find(Property.Type);
    if (It == IBB_IniLine_ProcessMap.end())
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            sprintf_s(LogBufB, "IBB_IniLine_Default::Load ： IniLine \"%s\" 的Property.Type使用了不存在的类型 \"%s\"。"
                , Name.c_str(), Property.Type.c_str());
            GlobalLogB.AddLog(LogBufB);
        }
        It = IBB_IniLine_ProcessMap.find("String");//string does exist
        Ret = false;
        Property.Proc = nullptr;
    }
    else Property.Proc = (IBB_IniLine_Process*)std::addressof(It->second);
    Property.Lim= JProp.GetObjectItem(u8"Restriction");

    auto JRequire = FromJson.GetObjectItem(u8"Require");
    Require.RequiredNames = JRequire.GetObjectItem(u8"RequiredNames").GetArrayString();
    Require.ForbiddenNames = JRequire.GetObjectItem(u8"ForbiddenNames").GetArrayString();
    Require.RequiredValues = JRequire.GetObjectItem(u8"RequiredValues").GetArrayObject();
    Require.ForbiddenValues = JRequire.GetObjectItem(u8"ForbiddenValues").GetArrayObject();

    return Ret;
}*/

IBB_SubSec_Default::IBB_SubSec_Default() :
    Type(IBB_SubSec_Default::Default)
{}



const IBB_Ini* IBB_Project::GetIni(const IBB_Project_Index& Index) const
{
    return Index.GetIni(*this);
}
const IBB_Section* IBB_Project::GetSec(const IBB_Project_Index& Index)const
{
    return Index.GetSec(*this);
}
IBB_Ini* IBB_Project::GetIni(IBB_Project_Index& Index) const
{
    return Index.GetIni(*(IBB_Project*)this);
}
IBB_Section* IBB_Project::GetSec(IBB_Project_Index& Index)const
{
    return Index.GetSec(*(IBB_Project*)this);
}
IBB_Project_Index IBB_Project::GetSecIndex(const std::string& Name, const std::string& PriorIni) const
{
    if(!PriorIni.empty())
        for (auto& I : Inis)
    {
        if (I.Name != PriorIni)continue;
        auto it = I.Secs.find(Name);
        if (it != I.Secs.end())return { I.Name, it->second.Name };
    }

    for (auto& I : Inis)
    {
        auto it = I.Secs.find(Name);
        if (it != I.Secs.end())return { I.Name, it->second.Name };
    }

    return { "","" };
}













bool IBB_RegisterList::Merge(const IBB_RegisterList& Another)
{
    List.insert(List.end(), Another.List.begin(), Another.List.end());
    return true;//return !Another.List.empty();
}
std::string IBB_RegisterList::GetText(bool PrintExtraData) const
{
    (void)PrintExtraData;
    std::string Text;
    Text += "[" + Type + "]\n";
    Text += "Type=" + Type; Text.push_back('\n');
    Text += "IniType=" + IniType; Text.push_back('\n');
    int i = 1;
    for (auto ptr : List)
    {
        //if(ptr==nullptr) 暂不需检查
        Text += std::to_string(i) + "=" + ptr->Name; Text.push_back('\n');
        i++;
    }
    return Text;
}



IBB_Ini::IBB_Ini(const IBB_Ini& rhs)
    : Root(rhs.Root), Name(rhs.Name), Secs_ByName(rhs.Secs_ByName), Secs(rhs.Secs)
{
    for (auto& [k, s] : Secs)
        s.ChangeRoot(this);
}
IBB_Ini::IBB_Ini(IBB_Ini&& rhs) noexcept
    : Root(rhs.Root), Name(std::move(rhs.Name)), Secs_ByName(std::move(rhs.Secs_ByName)), Secs(std::move(rhs.Secs))
{
    for (auto& [k, s] : Secs)
        s.ChangeRoot(this);
}

bool IBB_Ini::CreateSection(const std::string& _Name)
{
    if (EnableLogEx)
    {
        sprintf_s(LogBufB, "IBB_Project::CreateSection : <- Name=%s Sec=%s", Name.c_str(), _Name.c_str());
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog(LogBufB);
    }
    if (_Name.empty())
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog((u8"IBB_Ini::CreateSection ：" + loc("Log_EmptySectionName")).c_str());
        }
        return false;
    }
    auto Is = Secs.find(_Name);
    if (Is == Secs.end())
    {
        Secs_ByName.push_back(_Name);
        Secs.insert({ _Name,IBB_Section(_Name, this) });
        return true;
    }
    else return false;
}


bool IBB_Ini::DeleteSection(const std::string& Tg)
{
    if (EnableLogEx)
    {
        sprintf_s(LogBufB, "IBB_Ini::DeleteSection : <- Ini=%s Name=%s", Name.c_str(), Tg.c_str());
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog(LogBufB);
    }
    bool Ret = true;
    auto it = Secs.find(Tg);
    if (it == Secs.end())return false;
    else
    {
        for (auto ij = Secs_ByName.begin(); ij != Secs_ByName.end(); ++ij)
            if (*ij == Tg){ Secs_ByName.erase(ij); break; }
        if(!it->second.Isolate())Ret = false;
        auto psc = &it->second;
        for (auto& rl : Root->RegisterLists)
        {
            std::vector<IBB_Section*> pv; pv.reserve(rl.List.size());
            for (auto ps : rl.List)
            {
                if (ps != psc)pv.push_back(ps);
            }
            rl.List = pv;
        }
        Secs.erase(it);
    }
    return Ret;
}

std::string IBB_Ini::GetText(bool PrintExtraData) const
{
    std::string Text;
    Text.push_back(';'); Text += Name; Text.push_back('\n');
    for (const auto& sn : Secs_ByName)
    {
        auto It = Secs.find(sn);
        if (It == Secs.end())
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                auto gt = UTF8toUnicode(Name + "->" + sn);
                GlobalLogB.AddLog(std::vformat(L"IBB_Ini::GetText ：" + locw("Log_SectionNotExist"), std::make_wformat_args(gt)));
            }
        auto& Sec = It->second;
        Text += "[" + Sec.Name + "]\n" + Sec.GetText(PrintExtraData);
        Text.push_back('\n');
    }
    return Text;
}

bool IBB_Ini::UpdateAll()
{
    bool Ret = true;
    for (auto& P : Secs)
    {
        P.second.Root = this;
        if (!P.second.UpdateAll())Ret = false;
    }
    return Ret;
}




StrPoolID SelectDefaultInput(StrPoolID LinkType);
const char* SelectDefaultInput(const std::string& LinkType);

void IBB_DefaultTypeList::CreateUnknownType(StrPoolID KeyName)
{
    IBB_DefaultTypeAlt Alt;

    Alt.Name = KeyName;
    Alt.DescShort = NewPoolDesc(PoolStr(KeyName));
    Alt.DescLong = NewPoolDesc("");

    const auto& link = IBB_DefaultRegType::GetDefaultLinkNodeSetting();
    Alt.LinkType = NewPoolStr(link.LinkType);
    Alt.LinkLimit = link.LinkLimit;
    Alt.Color = link.LinkCol;

    Alt.Input = SelectDefaultInput(Alt.LinkType);

    EnsureType(Alt);

    auto& Def = IniLine_Default[KeyName];
    Def.Known = false;

    extern const char* DefaultSubSecName;
    auto& Sub = SubSec_Default[DefaultSubSecName];
    Def.InSubSec = &Sub;
}

IBB_IniLine_Default* IBB_DefaultTypeList::KeyBelongToLine(const std::string& KeyName)
{
    return KeyBelongToLine(NewPoolStr(KeyName));
}

IBB_SubSec_Default* IBB_DefaultTypeList::KeyBelongToSubSec(const std::string& KeyName)
{
    auto ptr = KeyBelongToLine(KeyName);
    return ptr ? ptr->InSubSec : nullptr;
}

IBB_IniLine_Default* IBB_DefaultTypeList::KeyBelongToLine(StrPoolID KeyName)
{
    auto it = IniLine_Default.find(KeyName);
    if (it == IniLine_Default.end())CreateUnknownType(KeyName);
    it = IniLine_Default.find(KeyName);
    if (it == IniLine_Default.end())return nullptr;
    return &it->second;
}

IBB_SubSec_Default* IBB_DefaultTypeList::KeyBelongToSubSec(StrPoolID KeyName)
{
    auto ptr = KeyBelongToLine(KeyName);
    return ptr ? ptr->InSubSec : nullptr;
}
