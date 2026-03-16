#pragma once
#include "FromEngine/Include.h"
#include "IBB_Components.h"
#include "IBG_InputType_Defines.h"

using KVFormatter_t = std::function <void(IBB_VariableMultiList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder, IBB_Section* AtSec)>;

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
    extern StrPoolID Key;
    extern std::set<IBB_Section_Desc> MergedDescs;
}
