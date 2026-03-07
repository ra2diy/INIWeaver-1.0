#pragma once
#include "FromEngine/Include.h"
#include "IBB_Components.h"
#include "IBG_InputType_Defines.h"

using KVFormatter_t = std::function <void(IBB_VariableList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder, IBB_Section* AtSec)>;

namespace KVFormatter
{
    KVFormatter_t Default();
    KVFormatter_t SplitValue(const std::string& delim);
    KVFormatter_t ImportAllModules(const std::string& delim, const std::string& INIType, bool MergeTargetOnExport);
    KVFormatter_t Recompose(IICVPtr SaveInput, IFCVPtr SaveFormat, ILFVPtr ExportLines, IBB_ValueContainer&& Values);
}

namespace KVFormatterFactory
{
    KVFormatter_t LoadFromJson(JsonObject j, IBG_InputType& AtType);
}

namespace ExportContext
{
    extern std::string Key;
    extern size_t SameKeyIdx;//用于当Key重复时区分不同的Key
    extern std::set<IBB_Section_Desc> MergedDescs;
}
