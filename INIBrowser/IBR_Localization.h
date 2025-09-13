#pragma once
#include "FromEngine/global_tool_func.h"
#include "IBG_Ini.h"


namespace IBR_L10n
{
    bool LoadFromINI(const std::wstring& FileName);
    const std::string& _TEXT_UTF8 GetString(const std::string& Key);
    const std::wstring& _TEXT_WIDE GetWString(const std::string& Key);
    std::string _TEXT_UTF8 GetStringAligned(const std::string& Key, int AlignMax);
    std::wstring _TEXT_WIDE GetWStringAligned(const std::string& Key, int AlignMax);
    void SetLanguage(const std::string& Language);
    bool RenderUI(std::string_view Title);
}

#define loc(x) IBR_L10n::GetString(x)
#define aloc(x,n) IBR_L10n::GetStringAligned(x,n)
#define locw(x) IBR_L10n::GetWString(x)
#define alocw(x,n) IBR_L10n::GetWStringAligned(x,n)
#define locc(x) IBR_L10n::GetString(x).c_str()
#define alocc(x,n) IBR_L10n::GetStringAligned(x,n).c_str()
#define locwc(x) IBR_L10n::GetWString(x).c_str()
#define alocwc(x,n) IBR_L10n::GetWStringAligned(x,n).c_str()

#define _AppName locc("AppName")
#define _AppNameW locwc("AppName")
