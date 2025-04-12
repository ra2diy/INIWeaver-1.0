#pragma once
#include "FromEngine/global_tool_func.h"
#include "IBG_Ini.h"


namespace IBR_L10n
{
    bool LoadFromINI(const std::wstring& FileName);
    const std::string& _TEXT_UTF8 GetString(const std::string& Key);
    std::string _TEXT_UTF8 GetStringAligned(const std::string& Key, int AlignMax);
    void SetLanguage(const std::string& Language);
    bool RenderUI(std::string_view Title);
}

#define loc(x) IBR_L10n::GetString(x)
#define aloc(x,n) IBR_L10n::GetStringAligned(x,n)
#define locw(x) UTF8toUnicode(IBR_L10n::GetString(x))
#define alocw(x,n) UTF8toUnicode(IBR_L10n::GetStringAligned(x,n))
#define locc(x) IBR_L10n::GetString(x).c_str()
#define alocc(x,n) IBR_L10n::GetStringAligned(x,n).c_str()
#define locwc(x) UTF8toUnicode(IBR_L10n::GetString(x)).c_str()
#define alocwc(x,n) UTF8toUnicode(IBR_L10n::GetStringAligned(x,n)).c_str()

#define _AppName locc("AppName")
#define _AppNameW locwc("AppName")
