#include "IBB_IniLine.h"
#include "Global.h"
#include <ranges>
#include "IBB_RegType.h"
#include "IBG_InputType_Derived.h"

namespace ExportContext
{
    extern StrPoolID Key;
    extern size_t SameKeyIdx;//用于当Key重复时区分不同的Key
    extern std::set<IBB_Section_Desc> MergedDescs;//被Import而合并的Section列表
    extern bool OnExport;
}

extern const char* Internal_IniName;

std::string_view TrimView(std::string_view Line);

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
        auto pLine = pSec->GetLineFromSubSecs(SingleValID());
        if (pLine)return pLine->Data->GetStringForExport();
        else return Val;
    }
    else
    {
        return Val;
    }
}

std::string GetExportString(const std::string& StrValue)
{
    const std::string& Delim = ",";
    return StrValue |
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

// ---------------------------------------------------------------
// ------------------- IBB_IniLine_Data_String -------------------
// ---------------------------------------------------------------

bool IBB_IniLine_Data_String::SetValue(const std::string& Val)
{
    Value = Val;
    _Empty = Val.empty();
    return true;
}
bool IBB_IniLine_Data_String::MergeValue(const std::string& Val) { return SetValue(Val); }
bool IBB_IniLine_Data_String::Clear()
{
    _Empty = true;
    Value.clear();
    return true;
}

bool IBB_IniLine_Data_String::FirstIsLink() const
{
    return Status_Workspace.InputMethod == IICStatus::Link;
}
std::string IBB_IniLine_Data_String::GetStringForExport() const
{
    return GetExportString(GetString());
}
void IBB_IniLine_Data_String::RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace)
{
    auto& Form = Default->GetInputType().Form;
    Status_Sidebar.InputMethod = IICStatus::Input;
    auto& Status = IsWorkspace ? Status_Workspace : Status_Sidebar;
    //Form->SetInWorkSpace(IsWorkspace);
    //IIC_InputText* test;
    //test->RenderUI()
}

// ---------------------------------------------------------------
// -------------------- IBB_IniLine_Data_Bool --------------------
// ---------------------------------------------------------------

void RenderIICBool(IIC_Bool* pBool, bool& Val);

bool IBB_IniLine_Data_Bool::SetValue(const std::string& Val)
{
    Value = IsTrueString(Val);
}
bool IBB_IniLine_Data_Bool::MergeValue(const std::string& Val) { return SetValue(Val); }
bool IBB_IniLine_Data_Bool::Clear()
{
    _Empty = true;
    Value = false;
    return true;
}
void IBB_IniLine_Data_Bool::RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace)
{
    auto& Form = Default->GetInputType().Form;
    Form->SetInWorkSpace(IsWorkspace);
    auto pBool = dynamic_cast<IIC_Bool*>(Form->InputComponents->at(0).get());
    Type = pBool->FmtType;
    RenderIICBool(pBool, Value);
}
bool IBB_IniLine_Data_Bool::FirstIsLink() const
{
    return false;
}
std::string IBB_IniLine_Data_Bool::GetString() const
{
    return StrBoolImpl(Value, Type);
}
std::string IBB_IniLine_Data_Bool::GetStringForExport() const
{
    return GetString();
}

// ---------------------------------------------------------------
// -------------------- IBB_IniLine_Data_IIF ---------------------
// ---------------------------------------------------------------

IBB_IniLine_Data_IIF::IBB_IniLine_Data_IIF(const IBB_IniLine_Default* Default)
    : Value(Default->GetInputType().Form->Duplicate())
{}


bool IBB_IniLine_Data_IIF::SetValue(const std::string& Val)
{
    Value->ParseFromString(Val);
    return true;
}
bool IBB_IniLine_Data_IIF::MergeValue(const std::string& Val) { return SetValue(Val); }
bool IBB_IniLine_Data_IIF::Clear()
{
    Value->ResetState();
    return true;
}
void IBB_IniLine_Data_IIF::RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace)
{
    Value->SetInWorkSpace(IsWorkspace);
    Value->RenderUI(LinkNode);
}

bool IBB_IniLine_Data_IIF::FirstIsLink() const
{
    auto& status = Value->GetComponentStatus();
    if (status.empty())return true;
    return status.front().InputMethod == IICStatus::Link;
}
std::string IBB_IniLine_Data_IIF::GetString() const
{
    return Value->GetFormattedString();
}
std::string IBB_IniLine_Data_IIF::GetStringForExport() const
{
    return GetExportString(GetString());
}

// ---------------------------------------------------------------
// --------------------- IBB_IniLine_Default ---------------------
// ---------------------------------------------------------------

LineData IBB_IniLine_Default::Create() const
{
    //return std::make_shared<IBB_IniLine_Data_String>();
    return std::make_shared<IBB_IniLine_Data_IIF>(this);
}

const IBB_RegType& IBB_IniLine_Default::GetRegType() const
{
    return IBB_DefaultRegType::GetRegType(PoolStr(TypeAlt));
}

const std::string& IBB_IniLine_Default::GetIniType() const
{
    return IBB_DefaultRegType::GetIniTypeOfReg(PoolStr(TypeAlt));
}

const IBG_InputType* IBB_IniLine_Default::GetInputTypeByValue(const std::string& Value) const
{
    return Known ?
        Input :
        &IBB_DefaultRegType::SelectInputTypeByValue(Value);
}

const IBG_InputType& IBB_IniLine_Default::GetInputType() const
{
    return *Input;
}

int IBB_IniLine_Default::GetLinkLimit() const
{
    return LinkLimit;
}

LinkNodeSetting IBB_IniLine_Default::GetNodeSetting() const
{
    return LinkNodeSetting{
        PoolStr(TypeAlt),
        LinkLimit,
        Color
    };
}

// ---------------------------------------------------------------
// ------------------------- IBB_IniLine -------------------------
// ---------------------------------------------------------------

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
                auto K = UTF8toUnicode(PoolStr(Default->Name));
                auto LT = L"";
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
            auto K = UTF8toUnicode(PoolStr(Default->Name));
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
    if (Def != nullptr)Default = Def;
    if (Default == nullptr)return false;
    Data = Default->Create();
    if (!Data)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            auto K = UTF8toUnicode(PoolStr(Default->Name));
            auto LT = L"";
            GlobalLogB.AddLog(std::vformat(L"IBB_IniLine::Generate ： " + locw("Error_DataTypeNotExist"),
                std::make_wformat_args(K, LT)).c_str());
        }
        return false;
    }
    bool Ret = Data->SetValue(Value);
    return Ret;
}

void IBB_IniLine::MakeKVForExport(IBB_VariableList& vl, IBB_Section* AtSec, std::vector<std::string>* TmpLineOrder) const
{
    auto& input = Default->GetInputType();
    auto key = PoolStr(Default->Name);

    auto IIF = Default->GetInputType().Form->Duplicate();
    auto Str = Data->GetString();
    IIF->ParseFromString(Str);
    ExportContext::OnExport = true;
    Data->SetValue(IIF->RegenFormattedString());
    ExportContext::OnExport = false;
    auto value = Data->GetStringForExport();
    Data->SetValue(Str);
    input.KVFmt(vl, key, value, TmpLineOrder, AtSec);
}

IBB_IniLine::IBB_IniLine(IBB_IniLine&& F) noexcept
{
    if (EnableLogEx) { GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog("IBB_IniLine : Move Ctor"); }
    Default = F.Default; Data = F.Data;
}

void IBB_IniLine::RenderUI(const LinkNodeSetting& LinkNode, bool IsWorkspace)
{
    if (!Default)
    {
        ImGui::TextColored(IBR_Color::IllegalLineColor, "%s", locc("GUI_MissingLineDefault"));
        return;
    }
    Data->RenderUI(Default, LinkNode, IsWorkspace);
}
void IBB_IniLine::RenderUI(bool IsWorkspace)
{
    if (!Default)RenderUI(LinkNodeSetting{}, IsWorkspace);
    else RenderUI(Default->GetNodeSetting(), IsWorkspace);
}
const void* IBB_IniLine::GetComponentID()
{
    return Data.get();
}
