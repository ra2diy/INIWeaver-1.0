#pragma once
#include "FromEngine/Include.h"
#define NOMINMAX
#include <Windows.h>

struct IniToken
{
    bool IsSection{ false }, Empty{ false }, HasDesc{ false };
    std::string Key, Value, Desc;

    void Tokenize(std::string_view, bool UseDesc = true);
    IniToken() = default;
    IniToken(std::string_view Line, bool UseDesc = true)
    {
        Tokenize(Line, UseDesc);
    }
};

std::vector<std::string_view> GetLines(char* Text, bool SkipEmptyLine = true);
std::vector<std::string_view> GetLines(BytePointerArray Text, size_t ExtBytes, bool SkipEmptyLine = true);//ExtBytes > 0
std::vector<std::string_view> GetLines(std::string&& Text, bool SkipEmptyLine = true);
std::vector<IniToken> GetTokens(const std::vector<std::string_view>& Lines, bool UseDesc = true);
std::vector<IniToken> GetTokensFromFile(ExtFileClass& Loader);
std::vector<std::vector<IniToken>> SplitTokens(std::vector<IniToken>&& Tok);
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> IniToMap(std::vector<std::vector<IniToken>>&& Secs);
std::string DataToBase64(const std::vector<BYTE>& Data);
std::vector<BYTE> Base64ToData(const std::string_view Str);
