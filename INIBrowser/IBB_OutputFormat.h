#pragma once
#include "FromEngine/Include.h"
#include "IBB_Components.h"

using KVFormatter_t = std::function <void(IBB_VariableList& Dest, const std::string& Key, const std::string& Value, std::vector<std::string>* TmpLineOrder)>;

namespace KVFormatter
{
    KVFormatter_t Default();
    KVFormatter_t SplitValue(const std::string& delim);
    KVFormatter_t ImportAllModules(const std::string& delim, const std::string& INIType);
}

namespace KVFormatterFactory
{
    KVFormatter_t LoadFromJson(JsonObject j);
}
