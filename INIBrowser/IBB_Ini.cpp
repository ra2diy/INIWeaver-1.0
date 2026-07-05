
#include "FromEngine/Include.h"
#include "FromEngine/global_tool_func.h"
#include "IBB_Index.h"
#include "Global.h"
#include "IBB_RegType.h"
#include "IBR_LinkNode.h"
#include "IBSave.h"
#include <ranges>

const char* Internal_IniName = "_LINKGROUP_INI_FILE";
extern const char* LinkAltPropType;
extern const char* DefaultSubSecName;

namespace ExportContext
{
    extern StrPoolID Key;
    extern size_t LineMult;
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
IBB_SectionID IBB_Project::GetSecID(const std::string& Name, const std::string& PriorIni) const
{
    auto Idx = GetSecIndex(Name, PriorIni);
    if (Idx.Empty())return {};
    else return IBB_SectionID(Idx.Ini.Name, Idx.Section.Name);
}

IBB_LineLocation IBB_Project::GetSecAndLineID(const std::string& KeyName, const std::string& PriorIni) const
{
    //Acceptor Node : SecName$$KeyID@@LineMult
    auto pos = KeyName.find("$$");
    if (pos == KeyName.npos)
        return { GetSecID(KeyName, PriorIni), EmptyPoolStr, 0 };
    else
    {
        auto SecName = KeyName.substr(0, pos);
        auto KeyIDStr = KeyName.substr(pos + 2);

        auto pos2 = KeyIDStr.find("@@");
        size_t LineMult = 0;
        if (pos2 != KeyIDStr.npos)
        {
            auto LMS = KeyIDStr.substr(pos2 + 2);
            LineMult = LMS.empty() ? 0 : (size_t)std::stoull(LMS.c_str());
            KeyIDStr = KeyIDStr.substr(0, pos2);
        }

        StrPoolID KeyID = NewPoolStr(KeyIDStr);
        return { GetSecID(SecName, PriorIni), KeyID, LineMult };
    }
}








void MergePresetOrder(std::vector<std::string>& tg, const std::vector<std::string>& order)
{
    tg.reserve(tg.size() + order.size());
    auto Pre = tg | std::ranges::to<std::unordered_set<std::string_view>>();
    tg.insert_range(tg.end(), order | std::views::filter([&](const auto& s) {return !Pre.contains(s); }));
}

void IBB_RegisterList::AddPresetOrder(const std::vector<std::string>& order)
{
    MergePresetOrder(PresetOrder, order);
}

void IBB_RegisterList::Load(const IBS_RegisterList& L)
{
    Type = L.Type;
    IniType = L.IniType;
    PresetOrder = L.PresetOrder;
}

void IBB_RegisterList::Save(IBS_RegisterList& L) const
{
    L.Type = Type;
    L.IniType = IniType;
    L.PresetOrder = PresetOrder;
}

bool IBB_RegisterList::Merge(const IBB_RegisterList& Another)
{
    if (&Another == this)return false;
    List.insert(List.end(), Another.List.begin(), Another.List.end());
    AddPresetOrder(Another.PresetOrder);
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


StrPoolID MixKeyAndReg(StrPoolID Key, StrPoolID Reg)
{
    // 使用黄金比例和位移旋转，让两个 8 字节充分混合
    uint64_t x = Key;
    uint64_t y = Reg;
    x += 0x9e3779b97f4a7c15ULL; // 黄金比例常数
    y += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x = x ^ (x >> 31);
    y = (y ^ (y >> 30)) * 0xbf58476d1ce4e5b9ULL;
    y = (y ^ (y >> 27)) * 0x94d049bb133111ebULL;
    y = y ^ (y >> 31);
    return x ^ (y + 0x9e3779b97f4a7c15ULL + (x << 6) + (x >> 2));
}


StrPoolID SelectDefaultInput(StrPoolID LinkType);
const char* SelectDefaultInput(const std::string& LinkType);

void IBB_DefaultTypeList::CreateUnknownType(StrPoolID KeyName)
{
    IBB_DefaultTypeAlt Alt;

    Alt.Name = KeyName;
    Alt.DescShort = NewPoolDesc(PoolStr(KeyName));
    Alt.DescLong = EmptyPoolStr;

    const auto& link = IBB_DefaultRegType::GetDefaultLinkNodeSetting();
    Alt.LinkType = link.LinkType;
    Alt.LinkLimit = link.LinkLimit;
    Alt.Color = link.LinkCol;
    Alt.SecType = EmptyPoolStr;

    Alt.Input = SelectDefaultInput(Alt.LinkType);

    EnsureType(Alt);

    auto Ptr = KeyBelongToLine_NoNew(KeyName, Alt.SecType);
    if (!Ptr) throw std::runtime_error("CreateUnknownType failed to create line for key: " + PoolStr(KeyName));
    auto& Sub = SubSec_Default[DefaultSubSecName];
    Ptr->Known = false;
    Ptr->InSubSec = &Sub;
}

IBB_IniLine_Default* IBB_DefaultTypeList::KeyBelongToLine(const std::string& KeyName, StrPoolID RegType)
{
    return KeyBelongToLine(NewPoolStr(KeyName), RegType);
}

IBB_SubSec_Default* IBB_DefaultTypeList::KeyBelongToSubSec(const std::string& KeyName, StrPoolID RegType)
{
    auto ptr = KeyBelongToLine(KeyName, RegType);
    return ptr ? ptr->InSubSec : nullptr;
}

IBB_IniLine_Default* IBB_DefaultTypeList::KeyBelongToLine_NoNew(StrPoolID KeyName, StrPoolID RegType)
{
    IM_UNUSED(RegType);
    //MyType AnyType Empty -> Default
    if (RegType != AnyTypeID() && RegType != MyTypeID() && RegType != EmptyPoolStr)
    {
        auto MixedID = MixKeyAndReg(KeyName, RegType);
        auto it = IniLine_MixedDefault.find(MixedID);
        if (it != IniLine_MixedDefault.end())return &it->second;
    }
    auto it = IniLine_FirstDefault.find(KeyName);
    if (it == IniLine_FirstDefault.end())return nullptr;
    return it->second;
}

IBB_IniLine_Default* IBB_DefaultTypeList::KeyBelongToLine(StrPoolID KeyName, StrPoolID RegType)
{
    if (auto ptr = KeyBelongToLine_NoNew(KeyName, RegType)) return ptr;
    CreateUnknownType(KeyName);
    return KeyBelongToLine_NoNew(KeyName, RegType);
}

IBB_SubSec_Default* IBB_DefaultTypeList::KeyBelongToSubSec(StrPoolID KeyName, StrPoolID RegType)
{
    auto ptr = KeyBelongToLine(KeyName, RegType);
    return ptr ? ptr->InSubSec : nullptr;
}
