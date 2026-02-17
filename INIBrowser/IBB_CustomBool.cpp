
#include "IBB_CustomBool.h"
#include "IBR_Components.h"
#include <map>

struct BoolStrings
{
    std::string T;
    std::string F;
};

std::unordered_map<std::string, StrBoolType> CustomNames;
std::map<StrBoolType, BoolStrings> CustomStrBoolTypes;

StrBoolType CurrentCount = StrBoolType::Str_Custom_Start;

std::optional<StrBoolType> GetOrCreateCustomStrBoolType(const std::string& Name, const std::string& True, const std::string& False)
{
    if (auto Type = GetCustomStrBoolType(Name); Type)
        return Type;
    if (!IsTrueString(True))
        return std::nullopt;
    if (IsTrueString(False))
        return std::nullopt;
    auto Cur = CurrentCount;
    CustomStrBoolTypes[Cur] = { True, False };
    CustomNames[Name] = Cur;
    CurrentCount = static_cast<StrBoolType>(static_cast<int>(Cur) + 1);
    return Cur;
}

std::optional<StrBoolType> GetCustomStrBoolType(const std::string& Name)
{
    auto it = CustomNames.find(Name);
    if (it == CustomNames.end())
        return std::nullopt;
    return it->second;
}

bool IsCustomStrBoolType(StrBoolType S)
{
    return CustomStrBoolTypes.contains(S);
}

bool AcceptAsCustomStrBoolType(const std::string& Str, StrBoolType S)
{
    auto it = CustomStrBoolTypes.find(S);
    if (it == CustomStrBoolTypes.end())return false;
    auto& B = it->second;
    if (!_strcmpi(Str.c_str(), B.T.c_str()))return true;
    if (!_strcmpi(Str.c_str(), B.F.c_str()))return true;
    return false;
}

const char* CustomStrBoolType(StrBoolType S, bool Value)
{
    auto& B = CustomStrBoolTypes[S];
    return (Value ? B.T : B.F).c_str();
}

StrBoolType StrBoolTypeFromString(const std::string& str, StrBoolType Default)
{
    /*
    Str_true_false,
    Str_True_False,
    Str_TRUE_FALSE,
    Str_yes_no,
    Str_Yes_No,
    Str_YES_NO,
    Str_t_f,
    Str_T_F,
    Str_y_n,
    Str_Y_N,
    Str_1_0,
    Str_yeah_fuck
    */

    if (str == "true_false")return StrBoolType::Str_true_false;
    else if (str == "True_False")return StrBoolType::Str_True_False;
    else if (str == "TRUE_FALSE")return StrBoolType::Str_TRUE_FALSE;
    else if (str == "yes_no")return StrBoolType::Str_yes_no;
    else if (str == "Yes_No")return StrBoolType::Str_Yes_No;
    else if (str == "YES_NO")return StrBoolType::Str_YES_NO;
    else if (str == "t_f")return StrBoolType::Str_t_f;
    else if (str == "T_F")return StrBoolType::Str_T_F;
    else if (str == "y_n")return StrBoolType::Str_y_n;
    else if (str == "Y_N")return StrBoolType::Str_Y_N;
    else if (str == "1_0")return StrBoolType::Str_1_0;
    else if (str == "yeah_fuck")return StrBoolType::Str_yeah_fuck;
    else return Default;
}

extern const char* EmptyBoolStrDesc;
StrBoolType CustomStrBoolTypeFromString(const std::string& T, const std::string& F, StrBoolType Default)
{
    auto Name = T + EmptyBoolStrDesc + F;
    auto opt = GetOrCreateCustomStrBoolType(Name, T, F);
    if (!opt)
    {
        static std::unordered_set<std::string> reported;
        if (!reported.contains(Name))
        {
            reported.insert(Name);
            auto TT = UTF8toUnicode(T), FF = UTF8toUnicode(F);
            auto ErrorStr = UnicodetoUTF8(std::vformat(locw("Error_InvalidBool"), std::make_wformat_args(TT, FF)));
            IBR_PopupManager::AddLoadConfigErrorPopup(std::move(ErrorStr), loc("Error_InvalidBoolInfo"));
        }
    }
    return opt ? *opt : Default;
}

StrBoolType StrBoolTypeFromJSON(JsonObject obj, StrBoolType Default)
{
    if (!obj)
        return Default;
    if (obj.IsTypeString())
        return StrBoolTypeFromString(obj.GetString(), Default);
    if (obj.IsTypeArray())
    {
        auto Arr = obj.GetArrayString();
        if (Arr.size() >= 2)
            return CustomStrBoolTypeFromString(Arr[0], Arr[1], Default);
    }
    return Default;
}
