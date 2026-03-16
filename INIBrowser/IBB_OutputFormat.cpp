#include "IBB_OutputFormat.h"
#include <ranges>
#include <string_view>
#include "IBB_Project.h"
#include "Global.h"

std::string_view TrimView(std::string_view Line);

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
        return [](IBB_VariableMultiList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder, IBB_Section* AtSec)
            {
                IM_UNUSED(TmpLineOrder);
                IM_UNUSED(AtSec);
                Dest.Push(Key, Value);
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
                }) |
                std::views::transform([](std::string_view sv) {
                    return TrimView(sv);
                });
        };

    const auto ConcatSVs = [](auto delim) {
        return
            std::views::join_with(delim) |
            std::ranges::to<std::string>();
        };

    KVFormatter_t SplitValue(const std::string& delim)
    {
        return [=](IBB_VariableMultiList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder, IBB_Section* AtSec)
            {
                IM_UNUSED(Key);
                IM_UNUSED(AtSec);
                auto List = Value | ToSVs(delim);
                auto Iter = List.begin();
                if (Iter == List.end())return;
                std::string K{ *Iter };
                ++Iter;
                if (Iter == List.end())return;
                Dest.Push(
                    K,
                    std::ranges::subrange(Iter, List.end()) |
                    ConcatSVs(delim));
                AddUniqueTmpLine(TmpLineOrder, K);
            };
    }

    KVFormatter_t ImportAllModules(const std::string& delim , const std::string& INIType, bool MergeTargetOnExport)
    {
        bool IsMyType = (INIType == "_MyType");
        bool IsLinkType = (INIType == "_LinkType");
        return [=](IBB_VariableMultiList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder, IBB_Section* AtSec)
            {
                IM_UNUSED(Key);
                if (Value.empty())return;
                for (auto sv : Value | ToSVs(delim))
                {
                    const auto& INI = [&]() -> const auto& {
                        if (IsMyType) return AtSec->Root->Name;
                        if (IsLinkType) {
                            auto Line = AtSec->GetLineFromSubSecs(NewPoolStr(Key));
                            if(!Line || !Line->Default) return INIType;
                            return Line->Default->GetIniType();
                        }
                        return INIType;
                    }();
                        
                    IBB_Project_Index idx(INI, std::string(sv));
                    auto pSec = idx.GetSec(IBF_Inst_Project.Project);
                    if (!pSec)continue;
                    if (MergeTargetOnExport)ExportContext::MergedDescs.insert(idx);
                    auto Lines = pSec->GetLineList(false, true, TmpLineOrder);
                    Dest.Merge(Lines);
                }
            };
    }

    KVFormatter_t Recompose(IICVPtr SaveInput, IFCVPtr SaveFormat, ILFVPtr ExportLines, IBB_ValueContainer&& Values)
    {
        return[=, Vals = std::move(Values)](IBB_VariableMultiList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder, IBB_Section* AtSec)
            {
                IM_UNUSED(AtSec);

                //Set Context
                ExportContext::Key = NewPoolStr(Key);

                //Initialize InputForm
                IBG_InputForm Form;
                Form.InputComponents = SaveInput;
                Form.ResetState();
                Form.SetValues(Vals);

                //Parse and recompose
                Form.FormatComponents = SaveFormat;
                Form.ParseFromString(Value);

                for (auto&& [ExportKey, ExportValue] : *ExportLines)
                {
                    Form.FormatComponents = ExportKey;
                    auto K = Form.RegenFormattedString();
                    Form.FormatComponents = ExportValue;
                    auto V = Form.RegenFormattedString();
                    Dest.Push(K, V);
                    AddUniqueTmpLine(TmpLineOrder, K);
                }
                //Reset Context
                ExportContext::Key = EmptyPoolStr;
            };
    }
}

namespace KVFormatterFactory
{
    KVFormatter_t LoadFromJson(JsonObject j, IBG_InputType& AtType)
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
                auto MergeTargetOnExport = j.ItemBoolOr("MergeTarget", false);
                return ImportAllModules(Delim, IniType, MergeTargetOnExport);
            }
            else if (Type == "Recompose")
            {
                auto& SaveFormat = AtType.Form->FormatComponents;
                auto& SaveInput = AtType.Form->InputComponents;
                auto oExportLines = j.GetObjectItem("ExportLines");
                if(!oExportLines)return Default();
                IBB_ValueContainer TmpCont;
                bool HasError;
                auto ExportLines = InputTypeFactory::CreateLineFormatVector(TmpCont, oExportLines, HasError);
                if(HasError)return Default();
                return Recompose(SaveInput, SaveFormat, ExportLines, std::move(TmpCont));
            }
            else return Default();
        }
        else return Default();
    }
}
