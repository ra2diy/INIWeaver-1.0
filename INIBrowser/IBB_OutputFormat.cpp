#include "IBB_OutputFormat.h"
#include <ranges>
#include <string_view>
#include "IBB_Project.h"
#include "Global.h"

namespace KVFormatter
{
    void AddUniqueTmpLine(std::vector<std::string>* TmpLineOrder, const std::string& Key)
    {
        if (!TmpLineOrder)return;
        if (std::find(TmpLineOrder->begin(), TmpLineOrder->end(), Key) == TmpLineOrder->end())
            TmpLineOrder->push_back(Key);
    }

    KVFormatter_t Default()
    {
        return [](IBB_VariableList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder)
            {
                IM_UNUSED(TmpLineOrder);
                Dest.Value[Key] = Value;
            };
    }

    const auto ToSVs = [](auto delim) {
            return
                std::views::split(delim) |
                std::views::transform([](const auto& subrange) -> std::string_view {
                if (subrange.empty()) {
                    return std::string_view();
                }
                return std::string_view(&*subrange.begin(),
                    std::ranges::distance(subrange));
                });
        };

    KVFormatter_t SplitValue(const std::string& delim )
    {
        return [=](IBB_VariableList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder)
            {
                IM_UNUSED(Key);
                auto List = Value | ToSVs(delim);
                auto Iter = List.begin();
                if (Iter == List.end())return;
                std::string K{ *Iter };
                ++Iter;
                if (Iter == List.end())return;
                Dest.Value[K] = *Iter;
                AddUniqueTmpLine(TmpLineOrder, K);
            };
    }

    KVFormatter_t ImportAllModules(const std::string& delim , const std::string& INIType)
    {
        return [=](IBB_VariableList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder)
            {
                IM_UNUSED(Key);
                for (auto sv : Value | ToSVs(delim))
                {
                    IBB_Project_Index idx(INIType, std::string(sv));
                    auto pSec = idx.GetSec(IBF_Inst_Project.Project);
                    auto Lines = pSec->GetLineList(false, true, TmpLineOrder);
                    Dest.Merge(Lines, true);
                }
            };
    }
}

namespace KVFormatterFactory
{
    KVFormatter_t LoadFromJson(JsonObject j)
    {
        using namespace KVFormatter;
        if (j.IsTypeString())
        {
            auto str = j.GetString();
            if (str == "Default")return Default();
            else return Default();
        }
        else if (j.IsTypeObject())
        {
            auto Type = j.ItemStringOr("Type", "Default");
            if(Type == "Default")return Default();
            else if (Type == "SplitValue")
            {
                auto Delim = j.ItemStringOr("Delim", ",");
                return SplitValue(Delim);
            }
            else if (Type == "Import")
            {
                auto Delim = j.ItemStringOr("Delim", ",");
                auto IniType = j.ItemStringOr("IniType", DefaultIniName);
                return ImportAllModules(Delim, IniType);
            }
            else return Default();
        }
        else return Default();
    }
}
