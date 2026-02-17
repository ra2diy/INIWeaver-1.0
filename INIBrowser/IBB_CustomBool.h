#pragma once
#include "FromEngine/global_tool_func.h"

std::optional<StrBoolType> GetOrCreateCustomStrBoolType(const std::string& Name, const std::string& True, const std::string& False);
std::optional<StrBoolType> GetCustomStrBoolType(const std::string& Name);
bool IsCustomStrBoolType(StrBoolType S);
bool AcceptAsCustomStrBoolType(const std::string& Str, StrBoolType S);
const char* CustomStrBoolType(StrBoolType S, bool Value);
StrBoolType StrBoolTypeFromString(const std::string& str, StrBoolType Default);
StrBoolType CustomStrBoolTypeFromString(const std::string& T, const std::string& F, StrBoolType Default);
StrBoolType StrBoolTypeFromJSON(JsonObject obj, StrBoolType Default);
