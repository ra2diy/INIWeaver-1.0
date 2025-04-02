
#include "FromEngine/Include.h"
#include "FromEngine/global_tool_func.h"
#include "IBBack.h"
#include "Global.h"
#include "IBB_RegType.h"

const char* LinkGroup_IniName = "_LINKGROUP_INI_FILE";

std::string IBB_DIndex::GetText() const
{
    return UseIndex ? ("<IDX:" + std::to_string(Index) + ">") : Name;
}






struct IBB_IniLine_Data_Int : public IBB_IniLine_Data_Base
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

    typedef int type;
    typedef int alt_type;
    type GetValue() { return _Empty ? 0 : Value; }
    alt_type GetAltValue() { return _Empty ? 0 : Value; }
    virtual const char* GetName() const { return TypeName; }
    

    virtual ~IBB_IniLine_Data_Int() {}
};
struct IBB_IniLine_Data_String : public IBB_IniLine_Data_Base
{
    static constexpr const char* TypeName{ "String" };
    std::string Value{};

    IBB_IniLine_Data_String() {}

    virtual bool SetValue(const std::string& Val)
    {
        Value = Val;
        _Empty = Val.empty();
        return true;
    }
    virtual bool MergeValue(const std::string& Val) { return SetValue(Val); }
    virtual bool MergeData(const IBB_IniLine_Data_Base* data)
    {
        auto Data = dynamic_cast<const IBB_IniLine_Data_String*>(data);
        if (Data == nullptr)return false;
        if (Data->_Empty)return true;
        Value = Data->Value;
        return true;
    }
    virtual bool Clear()
    {
        _Empty = true;
        Value.clear();
        return true;
    }
    virtual void UpdateAsDuplicate()
    {
        auto it = IBR_Inst_Project.CopyTransform.find(Value);
        if (it != IBR_Inst_Project.CopyTransform.end())
            Value = it->second;
    }
    virtual LineData Duplicate() const
    {
        std::shared_ptr<IBB_IniLine_Data_Base> R{ new IBB_IniLine_Data_String };
        R->MergeData(this);
        return R;
    }
    virtual std::string GetString() const { return Value; }
    virtual std::string GetStringForExport() const { return GetString(); }

    typedef std::string type;
    typedef std::string alt_type;
    type GetValue() { return Value; }
    alt_type GetAltValue() { return Value; }
    virtual const char* GetName() const { return TypeName; }

    virtual ~IBB_IniLine_Data_String() {}
};
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

bool IBB_Link_Default::Load(JsonObject FromJson)
{
    LinkFromRequired = FromJson.GetObjectItem(u8"LinkFromRequired").GetArrayObject();
    LinkFromForbidden = FromJson.GetObjectItem(u8"LinkFromForbidden").GetArrayObject();
    LinkToRequired = FromJson.GetObjectItem(u8"LinkToRequired").GetArrayObject();
    LinkToForbidden = FromJson.GetObjectItem(u8"LinkToForbidden").GetArrayObject();
    Name = FromJson.GetObjectItem(u8"Name").GetString();
    NameOnlyAsRegister = false;
    return true;
}



const char* DefaultAltPropType = "\"D\"";
const char* LinkAltPropType = "\"L\"";

bool IBB_IniLine_Default::IsLinkAlt() const
{
    return Property.Type == LinkAltPropType
        || Property.Type == IBB_IniLine_DataList::TypeName;
}

LineData IBB_IniLine_Default::Create() const
{
         if (Property.Type == IBB_IniLine_Data_Int::TypeName)
        return LineData(new IBB_IniLine_Data_Int);
    else if (Property.Type == IBB_IniLine_Data_String::TypeName)
        return LineData(new IBB_IniLine_Data_String);
    else if (IsLinkAlt())
        return LineData(new IBB_IniLine_DataList);
    return LineData(new IBB_IniLine_Data_String);
}

const char* DefaultSubSecName = "_DEFAULT_SUBSEC";

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
        if(UsedStrings)
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

void IBB_DefaultTypeList::EnsureType(const std::string& Key, const std::string& LinkType)
{
    auto it = IniLine_Default.find(Key);
    if (it != IniLine_Default.end())return;//Exists
    IBB_DefaultTypeAlt Alt;
    MessageBoxA(NULL, ("ERROR Key : " + Key).c_str(), (LinkType + " link type does not exist").c_str(), MB_OK);
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
    JsonFile F;
    F.ParseFromFile(Name);
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
    MessageBoxA(NULL, "已将TypeAlt注册表从json转换为CSV格式，下次将从TypeAlt.csv加载。", AppNameA, MB_OK);

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

bool IBB_VariableList::Load(JsonObject FromJson)
{
    Value = FromJson.GetMapString();
    return true;
}


bool operator<(const IBB_Section_Desc& A, const IBB_Section_Desc& B) { return (A.Ini < B.Ini) ? true : (A.Sec < B.Sec); }

IBB_Ini* IBB_Project_Index::GetIni(IBB_Project& Proj)
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetIni ：Func I Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, true, [](const IBB_Ini& F) {return F.Name; });
    return (Iter == Proj.Inis.end()) ? nullptr : std::addressof(*Iter);
}
IBB_Section* IBB_Project_Index::GetSec(IBB_Project& Proj)
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetSec ：Func I Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, true, [](const IBB_Ini& F) {return F.Name; });
    if (Iter == Proj.Inis.end())return nullptr;
    auto Iter1 = Section.Search(Iter->Secs, true, false);
    return (Iter1 == Iter->Secs.end()) ? nullptr : std::addressof(Iter1->second);
}
IBB_Ini* IBB_Project_Index::GetIni(IBB_Project& Proj) const
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetIni ：Func II Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, [](const IBB_Ini& F) {return F.Name; });
    return (Iter == Proj.Inis.end()) ? nullptr : std::addressof(*Iter);
}
IBB_Section* IBB_Project_Index::GetSec(IBB_Project& Proj) const
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetSec ：Func II Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, [](const IBB_Ini& F) {return F.Name; });
    if (Iter == Proj.Inis.end())return nullptr;
    auto Iter1 = Section.Search(Iter->Secs, true);
    return (Iter1 == Iter->Secs.end()) ? nullptr : std::addressof(Iter1->second);
}
const IBB_Ini* IBB_Project_Index::GetIni(const IBB_Project& Proj)
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetIni ：Func III Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, true, [](const IBB_Ini& F) {return F.Name; });
    return (Iter == Proj.Inis.end()) ? nullptr : std::addressof(*Iter);
}
const IBB_Section* IBB_Project_Index::GetSec(const IBB_Project& Proj)
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetSec ：Func III Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, true, [](const IBB_Ini& F) {return F.Name; });
    if (Iter == Proj.Inis.end())return nullptr;
    auto Iter1 = Section.Search(Iter->Secs, true, false);
    return (Iter1 == Iter->Secs.end()) ? nullptr : std::addressof(Iter1->second);
}
const IBB_Ini* IBB_Project_Index::GetIni(const IBB_Project& Proj) const
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetIni ：Func IV Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, [](const IBB_Ini& F) {return F.Name; });
    return (Iter == Proj.Inis.end()) ? nullptr : std::addressof(*Iter);
}
const IBB_Section* IBB_Project_Index::GetSec(const IBB_Project& Proj) const
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_Project_Index::GetSec ：Func IV Idx=", false); GlobalLogB.AddLog(GetText().c_str()); }
    auto Iter = Ini.Search<IBB_Ini>(Proj.Inis, true, [](const IBB_Ini& F) {return F.Name; });
    if (Iter == Proj.Inis.end())return nullptr;
    auto Iter1 = Section.Search(Iter->Secs, true);
    return (Iter1 == Iter->Secs.end()) ? nullptr : std::addressof(Iter1->second);
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
IBB_Project_Index IBB_Project::GetSecIndex(const std::string& Name) const
{
    for (auto& I : Inis)
    {
        auto it = I.Secs.find(Name);
        if (it != I.Secs.end())return { I.Name, it->second.Name };
    }
    return { "","" };
}

bool IBB_Project_Index::SameTarget(const IBB_Project& Proj, const IBB_Project_Index& A) const
{
    return GetSec(Proj) == A.GetSec(Proj);
}
std::string IBB_Project_Index::GetText() const
{
    return Ini.GetText() + "->" + Section.GetText();
}

std::string IBB_Section_Desc::GetText() const
{
    return Ini + "->" + Sec;
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
                GlobalLogB.AddLog("IBB_Ini::Merge ：有名字的字段理应存在。");
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




bool IBB_IniLine::Merge(const IBB_IniLine& Another, const std::string& Mode)
{
    if (Another.Default == nullptr)return false;
    if (!Another.Data)return false;
    return Merge(Another.Data->GetString(), Mode);
}
bool IBB_IniLine::Merge(const std::string& Another, const std::string& Mode)
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
                GlobalLogB.AddLog(("IBB_IniLine::Merge ：" + Default->Property.Type + " 类型的 Data 不能被分配。").c_str());
            }
            return false;
        }
        return Data->SetValue(Another);
    }
    if (Mode == u8"Reserve")
    {
        if (Data->Empty())return Data->SetValue(Another);
        else return true;
    }
    else if (Mode == u8"Replace")return Data->SetValue(Another);
    else if (Mode == u8"Merge")
    {
        if (Data->Empty())return Data->SetValue(Another);
        else return Data->MergeValue(Another);
    }
    else
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            sprintf_s(LogBufB, "IBB_IniLine::Merge ：Line \"%s\"合并模式\"%s\"不存在。", Default->Name.c_str(), Mode.c_str());
            GlobalLogB.AddLog(LogBufB);
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
            GlobalLogB.AddLog(("IBB_IniLine::Generate ：" + Default->Property.Type + " 类型的 Data 不能被分配。").c_str());
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
//IBB_IniLine(IBB_IniLine&& F) { Default = F.Default; Data = F.Data; if (F.ShouldDestroy) { F.ShouldDestroy = false; ShouldDestroy = true; } }
IBB_IniLine::IBB_IniLine(IBB_IniLine&& F)
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
        return Is->second.Merge(Section, "Merge", IsDuplicate);
    }
}
//TODO:刷新Link，不过不着急，毕竟AddSection实际只会在AddModule时调用，而不会导致原有Link的改动

/*
bool IBB_Ini::DeleteSection(const std::string& Tg)
{
    bool Ret = true;
    auto it = Secs.find(Tg);
    if (it == Secs.end())return false;
    else
    {
        for (auto ij = Secs_ByName.begin(); ij != Secs_ByName.end(); ++ij)
            if (*ij == Tg){ Secs_ByName.erase(ij); break; }
        if(it->second.Isolate())Ret = false;
        Secs.erase(it);
    }
    return Ret;
}
*/
//TODO

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
        //if(it->second.Isolate())Ret = false;
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
                GlobalLogB.AddLog(("IBB_Ini::GetText ：字段"+ sn +"有名字而无对应的内容。").c_str());
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
    for (auto& P : Secs)if (!P.second.UpdateAll())Ret = false;
    return Ret;
}



void IBB_VariableList::FillKeys(const std::vector<std::string>& List, const std::string& Val)
{
    for (const auto& s : List)Value[s] = Val;
}
void IBB_VariableList::Merge(const IBB_VariableList& Another, bool MergeUpValue)
{
    for (const auto& p : Another.Value)
        Value[p.first] = p.second;
    if (MergeUpValue && UpValue != nullptr && Another.UpValue != nullptr && UpValue != Another.UpValue)
        UpValue->Merge(*Another.UpValue, true);
}
const std::string& IBB_VariableList::GetVariable(const std::string& Name) const
{
    static std::string Null = "";
    auto It = Value.find(Name);
    if (It != Value.end())return It->second;
    else if (UpValue == nullptr)return Null;
    else return UpValue->GetVariable(Name);
}
bool IBB_VariableList::HasValue(const std::string& Name) const
{
    if ((Value.find(Name) != Value.end()))return true;
    else if (UpValue == nullptr)return false;
    else return UpValue->HasValue(Name);
}
bool IBB_VariableList::CoverUpValue(const std::string& Name) const
{
    if (UpValue == nullptr)return false;
    else return (Value.find(Name) != Value.end()) && UpValue->HasValue(Name);
}
std::string IBB_VariableList::GetText(bool ConsiderUpValue) const
{
    if (!ConsiderUpValue)
    {
        std::string Ret;
        for (const auto& p : Value)
        {
            Ret += p.first;
            Ret.push_back('=');
            Ret += p.second;
            Ret.push_back('\n');
        }
        return Ret;
    }
    else
    {
        IBB_VariableList List;
        Flatten(List);
        return List.GetText(false);
    }
}
void IBB_VariableList::Flatten(IBB_VariableList& Target) const
{
    if (UpValue != nullptr)UpValue->Flatten(Target);
    for (const auto& p : Value)
        Target.Value[p.first]=p.second;
}



void IBB_Link::DynamicCheck_Legal(const IBB_Project& Proj)
{
    auto pf = Proj.GetSec(From), pt = Proj.GetSec(To);
    if (pf == nullptr || pt == nullptr){ Dynamic.Legal = IBB_Link::_Dynamic::Incomplete; return; }
    else { Dynamic.Legal = IBB_Link::_Dynamic::Correct; return; }
    /*
    std::vector<IBB_Link*> ToChange;
    while(pt->IsLinkGroup)
    {
        if(pt->LinkGroup_LinkTo.empty()) { Dynamic.Legal = IBB_Link::_Dynamic::Incomplete; return; }
        pt = nullptr; auto it = pt->LinkGroup_LinkTo.begin();
        IBB_Link* pl;
        while (pt == nullptr && it != pt->LinkGroup_LinkTo.end()) { pl = &(*it); pt = Proj.GetSec(it->To); ++it; }
        if(pt == nullptr) { Dynamic.Legal = IBB_Link::_Dynamic::Incomplete; return; }
        //暂且这么干
        ToChange.push_back(pl);
        for (auto l : pt->LinkedBy)if (l == *pl)ToChange.push_back(&l);
    }
    while (pf->IsLinkGroup)
    {
        if(pf->LinkedBy.empty()) { Dynamic.Legal = IBB_Link::_Dynamic::Incomplete; return; }
        pf = nullptr; auto it = pf->LinkedBy.begin();
        IBB_Link* pl;
        while (pf == nullptr && it != pf->LinkedBy.end()) { pl = &(*it); pf = Proj.GetSec(it->From); ++it; }
        if (pf == nullptr) { Dynamic.Legal = IBB_Link::_Dynamic::Incomplete; return; }
        //暂且这么干
        ToChange.push_back(pl);
        for (auto l : pt->GetLinkTo())if (l == *pl)ToChange.push_back(&l);
    }
    //TODO: Correct or Wrong
    //TODO: Mixed Cond Check
    */
}
void IBB_Link::DynamicCheck_UpdateNewLink(const IBB_Project& Proj)
{
    auto sp = Proj.GetSec(To);
    if (sp != nullptr)
    {
        //sp->Dynamic.NewLinkedBy.push_back(*this);
        //auto& b = sp->Dynamic.NewLinkedBy.back();
        //b.FillData(this, FromKey);
        //Another = &b;
        sp->LinkedBy.push_back(*this);
        auto& b = sp->LinkedBy.back();
        b.Order = sp->LinkedBy.size() - 1;
        b.Another = this;
        Another = &b;
    }
}
bool IBB_Link::ChangeAddress()
{
    if (Another != nullptr)
    {
        Another->Another = this;
    }
    return true;
}



std::string IBB_Link::GetText(const IBB_Project& Proj) const
{
    std::string Text;
    auto pf = From.GetSec(Proj), pt = To.GetSec(Proj);

    Text += ((pf) ? ("_LINK_FROM:" + pf->Root->Name + "->" + pf->Name) : ("_MISSING:" + From.GetText()));
    Text.push_back('=');
    Text += ((pt && pf)? ("_LINK_TO:" + pf->Root->Name + "->" + pt->Name) : ("_MISSING:" + To.GetText()));
    Text.push_back('\n');

    Text += "_ADDRESS=" + std::to_string(this); Text.push_back('\n');
    Text += "_ANO_ADDR=" + std::to_string(Another); Text.push_back('\n');
    Text += "_FROM_KEY=" + FromKey; Text.push_back('\n');
    return Text;
}

IBB_Link_NameType IBB_Link::GetNameType() const
{
    return IBB_Link_NameType{ From.Ini.GetText(),From.Section.GetText(),To.Ini.GetText(),To.Section.GetText() };
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
    //File.WriteLabel("<NameType>");
    File.WriteData(Type);
    File.WriteData(IniType);
    File.WriteData(UseTargetIniTypeList);
    //::MessageBoxA(NULL, "91", "Title", MB_OK);
    //File.WriteLabel("<NameType::List>");
    File.WriteVector(List);
    //File.WriteLabel("</NameType::List>");
    //File.WriteLabel("<NameType::TargetIniType>");
    //::MessageBoxA(NULL, "92", "Title", MB_OK);
    if (UseTargetIniTypeList)File.WriteVector(TargetIniTypeList);
    else File.WriteData(TargetIniType);
    //File.WriteLabel("</NameType::TargetIniType>");
    //::MessageBoxA(NULL, "93", "Title", MB_OK);
    //File.WriteLabel("</NameType>");
}

void IBB_Link_NameType::Read(const ExtFileClass& File)
{
    File.ReadData(FromIni);
    File.ReadData(FromSec);
    File.ReadData(ToIni);
    File.ReadData(ToSec);
}
void IBB_Link_NameType::Write(const ExtFileClass& File)const
{
    File.WriteData(FromIni);
    File.WriteData(FromSec);
    File.WriteData(ToIni);
    File.WriteData(ToSec);
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
                sprintf_s(LogBufB, "IBB_DefaultTypeList::BuildQuery ：Ini行\"%s\"的Limit.Type=\"%s\"不属于任何已知类型。",
                    p.second.Name.c_str(), p.second.Limit.Type.c_str());
                GlobalLogB.AddLog(LogBufB);
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
        for (const auto& p : Query.IniLine_Default_RegexFull)if (RegexFull_Nothrow(KeyName, p.first))return p.second;
        for (const auto& p : Query.IniLine_Default_RegexNone)if (RegexNone_Nothrow(KeyName, p.first))return p.second;
        for (const auto& p : Query.IniLine_Default_RegexNotFull)if (RegexNotFull_Nothrow(KeyName, p.first))return p.second;
        for (const auto& p : Query.IniLine_Default_RegexNotNone)if (RegexNotNone_Nothrow(KeyName, p.first))return p.second;
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
