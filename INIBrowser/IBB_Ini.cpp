
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
    extern std::string Key;
    extern size_t SameKeyIdx;//用于当Key重复时区分不同的Key
    extern bool OnExport;
}

/*
struct IBB_IniLine_Data_Int final : public IBB_IniLine_Data_Base
{
    static constexpr const char* TypeName{ "Int" };
    int Value{};

    IBB_IniLine_Data_Int() {}

    virtual bool SetValue(const std::string& Val)
    {
        if (Val.empty())
        {
            _Empty = true;
            return true;
        }
        try
        {
            Value = std::stoi(Val);
            _Empty = false;
            return true;
        }
        catch (std::invalid_argument const& e)
        {
            (void)e;
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_IniLine_Data_Int::SetValue ： std::stoi : invalid_argument");
            }
            _Empty = true;
            return false;
        }
        catch(std::out_of_range const& e)
        {
            (void)e;
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_IniLine_Data_Int::SetValue ： std::stoi : out_of_range");
            }
            _Empty = true;
            return false;
        }
    }
    virtual LineData Duplicate() const
    {
        std::shared_ptr<IBB_IniLine_Data_Base> R{ new IBB_IniLine_Data_Int };
        R->MergeData(this);
        return R;
    }
    virtual void UpdateAsDuplicate() {}
    virtual bool MergeValue(const std::string& Val){ return SetValue(Val); }
    virtual bool MergeData(const IBB_IniLine_Data_Base* Data)
    {
        if (Data == nullptr)return false;
        if (Data->_Empty)return true;
        auto D = dynamic_cast<const IBB_IniLine_Data_Int*>(Data);
        if (D == nullptr)return false;
        Value = D->Value;
        return true;
    }
    virtual bool Clear()
    {
        _Empty = true;
        return true;
    }

    virtual std::string GetString() const { return _Empty ? "" : std::to_string(Value); }
    virtual std::string GetStringForExport() const { return GetString(); }

    virtual const char* GetName() const { return TypeName; }


    virtual ~IBB_IniLine_Data_Int() {}
};
*/

bool IBB_IniLine_Data_String::SetValue(const std::string& Val)
{
    Value = Val;
    _Empty = Val.empty();
    return true;
}
bool IBB_IniLine_Data_String::MergeValue(const std::string& Val) { return SetValue(Val); }
bool IBB_IniLine_Data_String::MergeData(const IBB_IniLine_Data_Base* data)
{
    auto Data = dynamic_cast<const IBB_IniLine_Data_String*>(data);
    if (Data == nullptr)return false;
    if (Data->_Empty)return true;
    Value = Data->Value;
    return true;
}
bool IBB_IniLine_Data_String::Clear()
{
    _Empty = true;
    Value.clear();
    return true;
}
void IBB_IniLine_Data_String::UpdateAsDuplicate()
{
    auto it = IBR_Inst_Project.CopyTransform.find(Value);
    if (it != IBR_Inst_Project.CopyTransform.end())
        Value = it->second;
}
LineData IBB_IniLine_Data_String::Duplicate() const
{
    LineData R{ new IBB_IniLine_Data_String };
    R->MergeData(this);
    return R;
}


std::string DecodeListForExport(const std::string& Val)
{
    if (Val.empty())return "";
    IBB_Section_Desc Desc{ Internal_IniName, Val };
    auto pSec = IBF_Inst_Project.Project.GetSec(Desc);
    if (pSec && pSec->IsLinkGroup)
    {
        std::string R;
        for (auto& V : pSec->LinkGroup_NewLinkTo)
        {
            auto pp = V.To.GetSec(IBF_Inst_Project.Project);
            if (pp)
            {
                R += pp->Name;
                R += ',';
            }
        }
        if (!R.empty())R.pop_back();
        return R;
    }
    if (pSec && pSec->SingleVal)
    {
        auto pLine = pSec->GetLineFromSubSecs(SingleValName);
        if (pLine)return pLine->Data->GetStringForExport();
        else return Val;
    }
    else
    {
        return Val;
    }
}

std::string_view TrimView(std::string_view Line);

std::string IBB_IniLine_Data_String::GetStringForExport() const
{

    const std::string& Delim = ",";
    return GetString() |
        std::views::split(Delim) |
        std::views::transform([](const auto& subrange) -> std::string {
            if (subrange.empty()) return "";
            return
                DecodeListForExport(
                    std::string(
                        TrimView(
                            std::string_view(
                                &*subrange.begin(), std::ranges::distance(subrange)
                            )
                        )
                    )
                );
        }) |
        std::views::join_with(Delim) |
        std::ranges::to<std::string>();
}


/*
struct IBB_IniLine_Data_Double : public IBB_IniLine_Data_Base
{
    double Value{};

    IBB_IniLine_Data_Double() {}

    bool SetValue(const std::string& Val)
    {
        if (Val.empty())
        {
            _Empty = true;
            return true;
        }
        try
        {
            Value = std::stod(Val);
            _Empty = false;
            return true;
        }
        catch (std::invalid_argument const& e)
        {
            (void)e;
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_IniLine_Data_Double::SetValue ： std::stod : invalid_argument");
            }
            _Empty = true;
            return false;
        }
        catch (std::out_of_range const& e)
        {
            (void)e;
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_IniLine_Data_Double::SetValue ： std::stod : out_of_range");
            }
            _Empty = true;
            return false;
        }
    }
    void UpdateAsDuplicate() {};
    bool MergeValue(const std::string& Val) { return SetValue(Val); }
    bool MergeData(const IBB_IniLine_Data_Double* Data)
    {
        if (Data == nullptr)return false;
        if (Data->_Empty)return true;
        Value = Data->Value;
        return true;
    }
    bool Clear()
    {
        _Empty = true;
        return true;
    }

    std::string GetString() { return _Empty ? "" : std::to_string(Value); }
    std::string GetStringForExport() { return GetString(); }

    typedef double type;
    typedef double alt_type;
    type GetValue() { return _Empty ? 0.0 : Value; }
    alt_type GetAltValue() { return _Empty ? 0.0 : Value; }
    const char* GetName() { return "Double"; }

    ~IBB_IniLine_Data_Double() {}
};



*/


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
    DescShort(""), DescLong(""), Platform({ "" })
{
    Require.RequiredValues.clear();
    Require.ForbiddenValues.clear();
}

bool IBB_SubSec_Default::Load(JsonObject FromJson, const std::unordered_map<std::string, IBB_IniLine_Default>& LineMap)
{
    Platform = FromJson.GetObjectItem(u8"Platform").GetArrayString();
    Name = FromJson.GetObjectItem(u8"Name").GetString();
    DescShort = FromJson.GetObjectItem(u8"DescShort").GetString();
    DescLong = FromJson.GetObjectItem(u8"DescLong").GetString();

    Require.RequiredValues = FromJson.GetObjectItem(u8"RequiredValues").GetArrayObject();
    Require.ForbiddenValues = FromJson.GetObjectItem(u8"ForbiddenValues").GetArrayObject();
    Lines_ByName = FromJson.GetObjectItem(u8"Lines").GetArrayString();
    for (const auto& s : Lines_ByName)Lines.insert({ s,LineMap.at(s) });

    return true;
}


LineData IBB_IniLine_Default::Create() const
{
    /*if (Property.Type == IBB_IniLine_Data_Int::TypeName)
        return LineData(new IBB_IniLine_Data_Int);
    else if (Property.Type == IBB_IniLine_Data_String::TypeName)
        return LineData(new IBB_IniLine_Data_String);
    else if (IsLinkAlt())
        return LineData(new IBB_IniLine_DataList);
    return LineData(new IBB_IniLine_Data_String);*/
    return std::make_shared<IBB_IniLine_Data_String>();
}

const IBB_RegType& IBB_IniLine_Default::GetRegType() const
{
    return IBB_DefaultRegType::GetRegType(Property.TypeAlt);
}

const std::string& IBB_IniLine_Default::GetIniType() const
{
    return IBB_DefaultRegType::GetIniTypeOfReg(Property.TypeAlt);
}

const IBG_InputType& IBB_IniLine_Default::GetInputType() const
{
    return *Input;
}

int IBB_IniLine_Default::GetLinkLimit() const
{
    return *reinterpret_cast<const int*>(&Property.Lim);
}

LinkNodeSetting IBB_IniLine_Default::GetNodeSetting() const
{
    return LinkNodeSetting{
        Property.TypeAlt,
        GetLinkLimit(),
        Color
    };
}


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
IBB_RegisterList_NameType IBB_RegisterList::GetNameType() const
{
    IBB_RegisterList_NameType Ret;
    Ret.UseTargetIniTypeList = true;
    Ret.List.reserve(List.size());
    Ret.TargetIniTypeList.reserve(List.size());
    Ret.IniType = IniType;
    Ret.Type = Type;
    for (const auto& Sec : List)
    {
        if (Sec == nullptr)continue;
        Ret.List.push_back(Sec->Name);
        Ret.TargetIniTypeList.push_back(Sec->Root->Name);
    }
    return Ret;
}



bool IBB_Ini::Merge(const IBB_Ini& Another, bool IsDuplicate)
{
    bool Ret = true;
    for (const auto& fs : Another.Secs_ByName)
    {
        auto It = Another.Secs.find(fs);
        if (It == Another.Secs.end())
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                auto gt = UTF8toUnicode(Another.Name + "->" + fs);
                GlobalLogB.AddLog(std::vformat(L"IBB_Ini::Merge ：" + locw("Log_SectionNotExist"), std::make_wformat_args(gt)));
            }
            Ret = false;
        }
        else
        {
            if (!AddSection(It->second, IsDuplicate))Ret = false;
        }
    }
    return Ret;
}

/*
ValidateResult IBB_IniLine::ValidateValue() const
{
    if (!Default)return ValidateResult::Unknown;

    auto& RegType = Default->GetRegType();
    if (RegType.Options.contains(Data->GetString()))
        return ValidateResult::Normal;
    else if (RegType.ValidateOptions)
        return ValidateResult::Refused;
    else
        return ValidateResult::Abnormal;
}
ValidateResult IBB_IniLine::ValidateAndSet(const std::string& Value)
{
    //Backup -> Set -> Validate -> Accept or Recover

    if (!Default)return ValidateResult::Unknown;

    auto Backup = Default->Create();

    if (Data)
    {
        if(!Backup->MergeData(Data.get()))
            return ValidateResult::Unknown;
    }

    Data->SetValue(Value);

    auto VR = ValidateValue();
    if (VR == ValidateResult::Unknown || VR == ValidateResult::Refused)
    {
        std::swap(Data, Backup);
    }

    return VR;
}
ValidateResult IBB_IniLine::ValidateAndMerge(const std::string& Another, IBB_IniMergeMode Mode)
{
    //Backup -> Set -> Validate -> Accept or Recover

    if (!Default)return ValidateResult::Unknown;

    auto Backup = Default->Create();

    if (Data)
    {
        if (!Backup->MergeData(Data.get()))
            return ValidateResult::Unknown;
    }

    Merge(Another, Mode);

    auto VR = ValidateValue();
    if (VR == ValidateResult::Unknown || VR == ValidateResult::Refused)
    {
        std::swap(Data, Backup);
    }

    return VR;
}
ValidateResult IBB_IniLine::ValidateAndMerge(const IBB_IniLine& Another, IBB_IniMergeMode Mode)
{
    //Backup -> Set -> Validate -> Accept or Recover

    if (!Default)return ValidateResult::Unknown;

    auto Backup = Default->Create();

    if (Data)
    {
        if (!Backup->MergeData(Data.get()))
            return ValidateResult::Unknown;
    }

    Merge(Another, Mode);

    auto VR = ValidateValue();
    if (VR == ValidateResult::Unknown || VR == ValidateResult::Refused)
    {
        std::swap(Data, Backup);
    }

    return VR;
}
*/

bool IBB_IniLine::Merge(const IBB_IniLine& Another, IBB_IniMergeMode Mode)
{
    if (Another.Default == nullptr)return false;
    if (!Another.Data)return false;
    return Merge(Another.Data->GetString(), Mode);
}
bool IBB_IniLine::Merge(const std::string& Another, IBB_IniMergeMode Mode)
{
    if (Default == nullptr)return false;
    if (!Data)
    {
        Data = Default->Create();
        if (!Data)
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                auto K = UTF8toUnicode(Default->Name);
                auto LT = UTF8toUnicode(Default->Property.Type);
                GlobalLogB.AddLog(std::vformat(L"IBB_DefaultTypeList::Merge ： " + locw("Error_DataTypeNotExist"),
                    std::make_wformat_args(K, LT)).c_str());
            }
            return false;
        }
        return Data->SetValue(Another);
    }
    if (Mode == IBB_IniMergeMode::Reserve)
    {
        if (Data->Empty())return Data->SetValue(Another);
        else return true;
    }
    else if (Mode == IBB_IniMergeMode::Replace)return Data->SetValue(Another);
    else if (Mode == IBB_IniMergeMode::Merge)
    {
        if (Data->Empty())return Data->SetValue(Another);
        else return Data->MergeValue(Another);
    }
    else
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            auto K = UTF8toUnicode(Default->Name);
            auto LT = std::to_wstring(static_cast<int>(Mode));
            GlobalLogB.AddLog(std::vformat(L"IBB_IniLine::Merge ： " + locw("Error_MergeTypeNotExist"),
                std::make_wformat_args(K, LT)).c_str());
        }
        return false;
    }
    return true;
}
bool IBB_IniLine::Generate(const std::string& Value, IBB_IniLine_Default* Def)
{
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_IniLine::Generate <- std::string Value=%s, IBB_IniLine_Default* Def=%p(Name=%s)",
            Value.c_str(), Def, (Def == nullptr) ? "_ERROR" : Def->Name.c_str()); GlobalLogB.AddLog(LogBufB);
    }
    if (Def != nullptr)Default = Def;
    if (Default == nullptr)return false;
    Data = Default->Create();
    if (!Data)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            auto K = UTF8toUnicode(Default->Name);
            auto LT = UTF8toUnicode(Default->Property.Type);
            GlobalLogB.AddLog(std::vformat(L"IBB_IniLine::Generate ： " + locw("Error_DataTypeNotExist"),
                std::make_wformat_args(K, LT)).c_str());
        }
        return false;
    }
    bool Ret = Data->SetValue(Value);
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_IniLine::Generate -> std::string Default->Name=%s, std::string Default->Property.Type=%s",
            Default->Name.c_str(), Default->Property.Type.c_str()); GlobalLogB.AddLog(LogBufB);
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_IniLine::Generate -> DataType=%s bool Ret=%s",
            Data->GetName(), IBD_BoolStr(Ret)); GlobalLogB.AddLog(LogBufB);
    }
    return Ret;
}

void IBB_IniLine::MakeKVForExport(IBB_VariableList& vl, IBB_Section* AtSec, std::vector<std::string>* TmpLineOrder) const
{
    auto& input = Default->GetInputType();
    auto& key = Default->Name;

    auto IIF = Default->GetInputType().Sidebar->Duplicate();
    auto Str = Data->GetString();
    IIF->ParseFromString(Str);
    ExportContext::OnExport = true;
    Data->SetValue(IIF->RegenFormattedString());
    ExportContext::OnExport = false;
    auto value = Data->GetStringForExport();
    Data->SetValue(Str);
    input.KVFmt(vl, key, value, TmpLineOrder, AtSec);
}


//IBB_IniLine(IBB_IniLine&& F) { Default = F.Default; Data = F.Data; if (F.ShouldDestroy) { F.ShouldDestroy = false; ShouldDestroy = true; } }
IBB_IniLine::IBB_IniLine(IBB_IniLine&& F) noexcept
{
    if (EnableLogEx) {GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_IniLine : Move Ctor");}
    Default = F.Default; Data = F.Data;
}

IBB_IniLine IBB_IniLine::Duplicate() const
{
    IBB_IniLine L;
    L.Default = Default;
    if (Data != nullptr)L.Data = Data->Duplicate();
    return L;
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
bool IBB_Ini::AddSection(const IBB_Section& Section, bool IsDuplicate)
{
    auto Is = Secs.find(Section.Name);
    if (Is == Secs.end())
    {
        Secs_ByName.push_back(Section.Name);
        auto It = Secs.insert({ Section.Name,Section });
        It.first->second.ChangeRootAndBack(this);
        return true;
    }
    else
    {
        return Is->second.Merge(Section, IBB_IniMergeMode::Merge, IsDuplicate);
    }
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






















void IBB_Section_NameType::Read(const ExtFileClass& File)
{
    File.ReadData(Name);
    File.ReadData(IniType);
    File.ReadData(IsLinkGroup);
    VarList.Read(File);
    Lines.Read(File);
}
void IBB_Section_NameType::Write(const ExtFileClass& File)const
{
    File.WriteData(Name);
    File.WriteData(IniType);
    File.WriteData(IsLinkGroup);
    VarList.Write(File);
    Lines.Write(File);
}


void IBB_VariableList::Read(const ExtFileClass& File)
{
    std::vector<std::string> sv;
    File.ReadVector(sv);
    for (size_t i = 0; i + 1 < sv.size(); i += 2)
        Value.insert({ std::move(sv.at(i)),std::move(sv.at(i + 1)) });
}
void IBB_VariableList::Write(const ExtFileClass& File)const
{
    std::vector<std::string> sv;
    for (const auto& p : Value)
    {
        sv.push_back(p.first);
        sv.push_back(p.second);
    }
    File.WriteVector(sv);
}

void IBB_RegisterList_NameType::Read(const ExtFileClass& File)
{
    File.ReadData(Type);
    File.ReadData(IniType);
    File.ReadData(UseTargetIniTypeList);
    File.ReadVector(List);
    if (UseTargetIniTypeList)File.ReadVector(TargetIniTypeList);
    else File.ReadData(TargetIniType);
}
void IBB_RegisterList_NameType::Write(const ExtFileClass& File)const
{
    File.WriteData(Type);
    File.WriteData(IniType);
    File.WriteData(UseTargetIniTypeList);
    File.WriteVector(List);
    if (UseTargetIniTypeList)File.WriteVector(TargetIniTypeList);
    else File.WriteData(TargetIniType);
}

















IBB_DefaultTypeList::_Query::_Query()
{
    IniLine_Default_Special_FunctionList = {};//有什么Special类型再添加。
}

bool IBB_DefaultTypeList::BuildQuery()
{
    bool Ret = true;
    for (const auto& p : IniLine_Default)
    {
        if (p.second.Limit.Type == u8"String") { Query.IniLine_Default_Full.insert({ p.second.Limit.Lim,(IBB_IniLine_Default*)std::addressof(p.second) }); }
        else if (p.second.Limit.Type == u8"Special") { Query.IniLine_Default_Special.push_back({ p.second.Limit.Lim,(IBB_IniLine_Default*)std::addressof(p.second) }); }
        else if (p.second.Limit.Type == u8"RegexFull") { Query.IniLine_Default_RegexFull.push_back({ p.second.Limit.Lim,(IBB_IniLine_Default*)std::addressof(p.second) }); }
        else if (p.second.Limit.Type == u8"RegexNone") { Query.IniLine_Default_RegexNone.push_back({ p.second.Limit.Lim,(IBB_IniLine_Default*)std::addressof(p.second) }); }
        else if (p.second.Limit.Type == u8"RegexNotFull") { Query.IniLine_Default_RegexNotFull.push_back({ p.second.Limit.Lim,(IBB_IniLine_Default*)std::addressof(p.second) }); }
        else if (p.second.Limit.Type == u8"RegexNotNone") { Query.IniLine_Default_RegexNotNone.push_back({ p.second.Limit.Lim,(IBB_IniLine_Default*)std::addressof(p.second) }); }
        else
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                auto K = UTF8toUnicode(p.second.Name);
                auto LT = UTF8toUnicode(p.second.Limit.Type);
                GlobalLogB.AddLog(std::vformat(L"IBB_DefaultTypeList::EnsureType ： " + locw("Error_LimitTypeNotExist"),
                    std::make_wformat_args(K, LT)).c_str());

            }
            Ret = false;
        }
    }
    for (const auto& p : SubSec_Default)
    {
        for (const auto& s : p.second.Lines_ByName)
        {
            Query.SubSec_Default_FromLineID.insert({ s,(IBB_SubSec_Default*)std::addressof(p.second) });
        }
    }
    //InputTextOptions
    for (const auto& p : IniLine_Default)
    {
        if (p.second.Name.empty())continue;
        //Text, Desc, Hint
        auto Pattern = p.second.Name + "\n" + p.second.DescShort;
        for (auto& c : Pattern)if(isupper(c))c = (char)tolower(c);
        Query.InputTextOptions.push_back({
            p.second.Name,
            p.second.DescShort,
            p.second.DescLong,
            Pattern
        });

    }
    std::sort(Query.InputTextOptions.begin(), Query.InputTextOptions.end(),
        [](const auto& a, const auto& b) { return a.Text < b.Text; });
    return Ret;
}

IBB_IniLine_Default* IBB_DefaultTypeList::KeyBelongToLine(const std::string& KeyName) const
{
    {
        auto It = Query.IniLine_Default_Full.find(KeyName);
        if (It != Query.IniLine_Default_Full.end())return It->second;
    }
    {
        for (const auto& p : Query.IniLine_Default_Special)
        {
            auto It = Query.IniLine_Default_Special_FunctionList.find(p.first);
            if (It != Query.IniLine_Default_Special_FunctionList.end())
                if (It->second(KeyName))
                    return p.second;
        }
    }
    {
        //for (const auto& p : Query.IniLine_Default_RegexFull)if (RegexFull_Nothrow(KeyName, p.first))return p.second;
        //for (const auto& p : Query.IniLine_Default_RegexNone)if (RegexNone_Nothrow(KeyName, p.first))return p.second;
        //for (const auto& p : Query.IniLine_Default_RegexNotFull)if (RegexNotFull_Nothrow(KeyName, p.first))return p.second;
        //for (const auto& p : Query.IniLine_Default_RegexNotNone)if (RegexNotNone_Nothrow(KeyName, p.first))return p.second;
    }
    return nullptr;
}

IBB_SubSec_Default* IBB_DefaultTypeList::KeyBelongToSubSec(const std::string& KeyName) const
{
    auto ptr = KeyBelongToLine(KeyName);
    if (ptr == nullptr)return nullptr;
    auto it = Query.SubSec_Default_FromLineID.find(ptr->Name);
    if (it == Query.SubSec_Default_FromLineID.end())return nullptr;
    return it->second;
}
