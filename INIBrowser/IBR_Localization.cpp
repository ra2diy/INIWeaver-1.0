#include "IBR_Localization.h"
#include "IBRender.h"
#include "FromEngine/Include.h"
#include "Global.h"
#include <imgui_internal.h>
#include <format>

extern wchar_t CurrentDirW[];
void subreplace(std::string& dst_str, const std::string& sub_str, const std::string& new_str);
void RefreshSettingTypes();
namespace ImGui
{
    void PushOrderFront(ImGuiWindow* Window);
}
bool RefreshLangBuffer1 = false;
bool RefreshLangBuffer2 = false;
bool RefreshLangBuffer3 = false;
bool RefreshLangBuffer4 = false;
bool RefreshLangBuffer5 = false;

namespace IBR_L10n
{
    std::string CurrentLanguage;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> LocalizationMap;
    std::unordered_map<std::string, std::string> CurrentMap;
    std::wstring LanguageININame;

    bool RenderUI(std::string_view Title)
    {
        auto it = CurrentMap.find("LangName");
        if (ImGui::BeginCombo(Title.data(), (it == CurrentMap.end() ? CurrentLanguage : it->second).c_str()))
        {
            auto h = ImGui::IsItemHovered();
            for (auto& [k, v] : LocalizationMap)
            {
                ImGui::PushOrderFront(ImGui::GetCurrentWindow());
                if (k == "Basic")continue;
                auto i2 = v.find("LangName");
                if (ImGui::Selectable((i2 == v.end() ? k : i2->second).c_str(), k == CurrentLanguage))
                {
                    SetLanguage(k);
                }
            }
            ImGui::EndCombo();
            return h;
        }
        return ImGui::IsItemHovered();
    }

    void ConvertLocMap()
    {
        for (auto& [k0, v0] : LocalizationMap)
        {
            if (k0 == "Basic")continue;
            else
            {
                for (auto& [k, v] : v0)
                {
                    //handle \n \t \r \\ etc
                    if (v.empty())continue;
                    std::string NewStr;
                    NewStr.reserve(v.size());
                    for (size_t i = 0; i < v.size(); i++)
                    {
                        if (v[i] == '\\')
                        {
                            i++;
                            if (i >= v.size())break;
                            switch (v[i])
                            {
                            case 'n':NewStr.push_back('\n'); break;
                            case 't':NewStr.push_back('\t'); break;
                            case 'r':NewStr.push_back('\r'); break;
                            case '\\':NewStr.push_back('\\'); break;
                            case '\"':NewStr.push_back('\"'); break;
                            case '0':NewStr.push_back('\0'); break;
                            case 'a':NewStr.push_back('\a'); break;
                            case 'b':NewStr.push_back('\b'); break;
                            case 'f':NewStr.push_back('\f'); break;
                            case 'v':NewStr.push_back('\v'); break;
                            case 'x':
                            {
                                i++;
                                if (i >= v.size())break;
                                int Hex = 0;
                                int j = 0;
                                while (i < v.size() && isxdigit(v[i]) && j < 2)
                                {
                                    Hex <<= 4;
                                    if (isdigit(v[i]))Hex += v[i] - '0';
                                    else if (isupper(v[i]))Hex += v[i] - 'A' + 10;
                                    else Hex += v[i] - 'a' + 10;
                                    i++; j++;
                                }
                                NewStr.push_back((char)Hex);
                                i--;
                            }
                            default:break;
                            }
                        }
                        else
                        {
                            NewStr.push_back(v[i]);
                        }
                    }
                    v = NewStr;
                }
            }
        }
    }
    bool LoadFromINI(const std::wstring& FileName)
    {
        LanguageININame = CurrentDirW + FileName;
        LocalizationMap = IniToMap(SplitTokens(GetTokens(GetLines(GetStringFromFile(LanguageININame.c_str())), false)));
        ConvertLocMap();
        CurrentLanguage = LocalizationMap["Basic"]["CurrentLanguage"];
        CurrentMap = LocalizationMap[CurrentLanguage];
        if (CurrentLanguage.empty() || CurrentMap.empty())
        {
            const wchar_t* Msg = L"***FATAL*** String Manager failed to initilaized properly";
            MessageBoxW(NULL, Msg, Msg/*L"INIBrowser.exe"*/, MB_ICONWARNING);
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog("***FATAL***String Manager failed to initilaized properly");
            }
            return false;
        }
        return true;
    }
    const std::string& _TEXT_UTF8 GetString(const std::string& Key)
    {
        auto& W = CurrentMap[Key];
        if (W.empty())
        {
            W = "MISSING:" + Key;
            if (EnableLog)
            {
                auto W1 = UTF8toUnicode(CurrentLanguage);
                auto W2 = UTF8toUnicode(Key);
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(std::vformat(locw("Error_LanguageKeyNotFound"),
                    std::make_wformat_args(W1, W2)));
            }
        }
        return W;
    }
    std::string _TEXT_UTF8 GetStringAligned(const std::string& Key, int AlignMax)
    {
        auto W = GetString(Key);
        if ((int)W.size() < AlignMax)
            W.resize(AlignMax, ' ');
        return W;
    }
    void SetLanguage(const std::string& Language)
    {
        if (CurrentLanguage == Language)return;
        if (LocalizationMap[Language].empty())
        {
            auto WL = UTF8toUnicode(Language);
            MessageBoxW(NULL, std::vformat(locw("Error_LanguageNotFound"), std::make_wformat_args(WL)).c_str(), locwc("AppName"), MB_ICONWARNING);
            if (EnableLog)
            {
                auto W1 = UTF8toUnicode(Language);
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(std::vformat(locw("Error_LanguageNotFound"), std::make_wformat_args(W1)));
            }
            return;
        }

        if (!LanguageININame.empty())
        {
            auto S = GetStringFromFile(LanguageININame.c_str());
            auto L = GetLines(std::move(S), false);
            ExtFileClass E;
            E.Open(LanguageININame.c_str(), L"w");
            for (auto& l : L)
            {
                auto t = IniToken(l, false);
                if (!t.IsSection && t.Key == "CurrentLanguage")
                {
                    std::string dst(l);
                    subreplace(dst, CurrentLanguage, Language);
                    E.PutStr(dst);
                    E.Ln();
                }
                else
                {
                    E.PutStr(l);
                    E.Ln();
                }
            }
        }

        CurrentLanguage = Language;
        CurrentMap = LocalizationMap[CurrentLanguage];

        RefreshSettingTypes();
        IBR_Inst_Setting.RefreshSetting();
        RefreshLangBuffer1 = true;
        RefreshLangBuffer2 = true;
        RefreshLangBuffer3 = true;
        RefreshLangBuffer4 = true;
        RefreshLangBuffer5 = true;
    }
}
