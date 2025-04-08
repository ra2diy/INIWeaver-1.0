#include "FromEngine/Include.h"
#include <ranges>
#include <filesystem>

extern std::wstring FontPathW;
std::vector<std::wstring> FindFileVec(const std::wstring& pattern);

std::wstring_view WTrimView(std::wstring_view Line)
{
    while (!Line.empty())
    {
        if (isspace(Line.front()))Line = Line.substr(1, Line.size() - 1);
        else break;
    }
    while (!Line.empty())
    {
        if (isspace(Line.back()))
        {
            if (Line.size() >= 2 && Line[Line.size() - 2] < 0)break;
            else Line = Line.substr(0, Line.size() - 1);
        }
        else break;
    }
    return Line;
}

void ToUpper(std::wstring& w)
{
    for (auto& l : w)
        if (iswalpha(l))
            l = towupper(l);
    w = w.c_str();
    //wprintf(L"%s %zu\n", w.c_str(), w.size());
}

namespace IBR_Font
{
    struct FontEntry
    {
        std::wstring Name;
        std::wstring Path;
    };

    struct FontLink
    {
        std::wstring Name;
        std::vector<FontEntry> Linked;
    };

    struct FontFallback
    {
        std::wstring Name;
        std::wstring Fallback;
    };

    std::vector<FontEntry> SystemFonts;
    std::vector<FontLink> FontLinks;
    std::vector<FontFallback> FontSubstitutes;
    std::vector<FontFallback> FontMapperFamilyFallback;
    std::wstring FontDir;

    std::wstring SearchFontBase(std::wstring_view FontName)
    {
        for (const auto& font : SystemFonts)
        {
            if (font.Name.find(FontName) != std::wstring::npos)
            {
                return font.Path;
            }
            auto p = FontName.find(L'.');
            auto S = p != std::wstring::npos ? FontName.substr(0, p) : FontName;
            if (font.Path.find(S) != std::wstring::npos)
                return font.Path;
        }
        return L"";
    }

    std::wstring SearchFontName(std::wstring FontName)
    {
        auto P = std::filesystem::path(FontPathW + FontName);
        if (std::filesystem::exists(P))
            return FontPathW + FontName;

        ToUpper(FontName);
        auto W = SearchFontBase(FontName);
        if (!W.empty())return W;
        for (const auto& font : FontSubstitutes)
        {
            if (font.Name.find(FontName) != std::wstring::npos)
            {
                W = SearchFontBase(font.Fallback);
                if (!W.empty())return W;
            }
        }
        for (const auto& font : FontMapperFamilyFallback)
        {
            if (font.Name.find(FontName) != std::wstring::npos)
            {
                W = SearchFontBase(font.Fallback);
                if (!W.empty())return W;
            }
        }
        for (const auto& font : FontLinks)
        {
            if (font.Name.find(FontName) != std::wstring::npos)
            {
                for (const auto& linkedFont : font.Linked)
                {
                    W = SearchFontBase(linkedFont.Name);
                    if (!W.empty())return W;
                }
            }
        }
        return L"";
    }

    std::wstring SearchFont(std::wstring_view FontName)
    {
        auto V = SearchFontName(std::wstring(FontName));
        return V.empty() ? L"" : FontDir + V;
    }

#define BaseDir L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
    static LPCWSTR SystemFonts_RegistryPath = BaseDir L"Fonts";
    static LPCWSTR FontLinks_RegistryPath = BaseDir L"FontLink\\SystemLink";
    static LPCWSTR FontSubstitutes_RegistryPath = BaseDir L"FontSubstitutes";
    static LPCWSTR FontMapperFamilyFallback_RegistryPath = BaseDir  L"FontMapperFamilyFallback";
#undef BaseDir

    void BuildFontQuery1()
    {
        HKEY hKey;
        LONG result;
        // Open the registry key for system fonts
        result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, SystemFonts_RegistryPath, 0, KEY_READ, &hKey);
        if (result != ERROR_SUCCESS)return;
        DWORD maxValueNameSize, maxValueDataSize;
        result = RegQueryInfoKeyW(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
        if (result != ERROR_SUCCESS)return;
        DWORD valueIndex = 0;
        LPWSTR valueName = new WCHAR[maxValueNameSize]{};
        LPBYTE valueData = new BYTE[maxValueDataSize]{};
        DWORD valueNameSize, valueDataSize, valueType;
        do {
            memset(valueName, 0, maxValueNameSize * sizeof(WCHAR));
            memset(valueData, 0, maxValueDataSize * sizeof(BYTE));
            valueDataSize = maxValueDataSize;
            valueNameSize = maxValueNameSize;
            result = RegEnumValueW(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);
            valueIndex++;
            if (result != ERROR_SUCCESS || valueType != REG_SZ)continue;
            std::wstring wsValueName(valueName, valueNameSize);
            std::wstring wsFontFile((LPWSTR)valueData, valueDataSize);
            ToUpper(wsFontFile);
            ToUpper(wsValueName);
            if (wsFontFile.ends_with(L".TTF") || wsFontFile.ends_with(L".TTC"))
            {
                //wprintf(L"%s -> %s\n", wsValueName.c_str(), wsFontFile.c_str());
                SystemFonts.push_back({ wsValueName, wsFontFile });
            }
        } while (result != ERROR_NO_MORE_ITEMS);

        delete[] valueName;
        delete[] valueData;
        RegCloseKey(hKey);
    }
    void BuildFontQuery2()
    {
        HKEY hKey;
        LONG result;
        // Open the registry key for system fonts
        result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, FontMapperFamilyFallback_RegistryPath, 0, KEY_READ, &hKey);
        if (result != ERROR_SUCCESS)return;
        DWORD maxValueNameSize, maxValueDataSize;
        result = RegQueryInfoKeyW(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
        if (result != ERROR_SUCCESS)return;
        DWORD valueIndex = 0;
        LPWSTR valueName = new WCHAR[maxValueNameSize]{};
        LPBYTE valueData = new BYTE[maxValueDataSize]{};
        DWORD valueNameSize, valueDataSize, valueType;
        do {
            memset(valueName, 0, maxValueNameSize * sizeof(WCHAR));
            memset(valueData, 0, maxValueDataSize * sizeof(BYTE));
            valueDataSize = maxValueDataSize;
            valueNameSize = maxValueNameSize;
            result = RegEnumValueW(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);
            valueIndex++;
            if (result != ERROR_SUCCESS || valueType != REG_SZ)continue;
            std::wstring wsValueName(valueName, valueNameSize);
            std::wstring wsFontFile((LPWSTR)valueData, valueDataSize);
            ToUpper(wsFontFile);
            ToUpper(wsValueName);
            //wprintf(L"%s -> %s\n", wsValueName.c_str(), wsFontFile.c_str());
            FontMapperFamilyFallback.push_back({ wsValueName, wsFontFile });
        } while (result != ERROR_NO_MORE_ITEMS);

        delete[] valueName;
        delete[] valueData;
        RegCloseKey(hKey);
    }
    void BuildFontQuery3()
    {
        HKEY hKey;
        LONG result;
        // Open the registry key for system fonts
        result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, FontSubstitutes_RegistryPath, 0, KEY_READ, &hKey);
        if (result != ERROR_SUCCESS)return;
        DWORD maxValueNameSize, maxValueDataSize;
        result = RegQueryInfoKeyW(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
        if (result != ERROR_SUCCESS)return;
        DWORD valueIndex = 0;
        LPWSTR valueName = new WCHAR[maxValueNameSize]{};
        LPBYTE valueData = new BYTE[maxValueDataSize]{};
        DWORD valueNameSize, valueDataSize, valueType;
        do {
            memset(valueName, 0, maxValueNameSize * sizeof(WCHAR));
            memset(valueData, 0, maxValueDataSize * sizeof(BYTE));
            valueDataSize = maxValueDataSize;
            valueNameSize = maxValueNameSize;
            result = RegEnumValueW(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);
            valueIndex++;
            if (result != ERROR_SUCCESS || valueType != REG_SZ)continue;
            std::wstring wsValueName(valueName, valueNameSize);
            std::wstring wsFontFile((LPWSTR)valueData, valueDataSize);
            ToUpper(wsFontFile);
            ToUpper(wsValueName);
            //remove , and after:
            auto p = wsFontFile.find_first_of(L",");
            if (p != std::string_view::npos)wsFontFile = wsFontFile.substr(0, p);
            p = wsValueName.find_first_of(L",");
            if (p != std::string_view::npos)wsValueName = wsValueName.substr(0, p);
            //wprintf(L"%s -> %s\n", wsValueName.c_str(), wsFontFile.c_str());
            FontMapperFamilyFallback.push_back({ wsValueName, wsFontFile });
        } while (result != ERROR_NO_MORE_ITEMS);

        delete[] valueName;
        delete[] valueData;
        RegCloseKey(hKey);
    }
    void BuildFontQuery4()
    {
        HKEY hKey;
        LONG result;
        // Open the registry key for system fonts
        result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, FontLinks_RegistryPath, 0, KEY_READ, &hKey);
        if (result != ERROR_SUCCESS)return;
        DWORD maxValueNameSize, maxValueDataSize;
        result = RegQueryInfoKeyW(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
        if (result != ERROR_SUCCESS)return;
        DWORD valueIndex = 0;
        LPWSTR valueName = new WCHAR[maxValueNameSize]{};
        LPBYTE valueData = new BYTE[maxValueDataSize]{};
        DWORD valueNameSize, valueDataSize, valueType;
        do {
            memset(valueName, 0, maxValueNameSize * sizeof(WCHAR));
            memset(valueData, 0, maxValueDataSize * sizeof(BYTE));
            valueDataSize = maxValueDataSize;
            valueNameSize = maxValueNameSize;
            result = RegEnumValueW(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);
            valueIndex++;
            if (result != ERROR_SUCCESS || valueType != REG_SZ)continue;
            std::wstring wsValueName(valueName, valueNameSize);
            std::wstring wsFontFile((LPWSTR)valueData, valueDataSize);
            ToUpper(wsFontFile);
            ToUpper(wsValueName);
            for (auto S : wsFontFile | std::views::split(L'\n'))
            {
                FontLinks.emplace_back();
                auto& F = FontLinks.back();
                F.Name = wsValueName;
                std::wstring_view Line{ S.data(), S.size() };
                auto p = Line.find_first_of(L",");
                if (p != std::string_view::npos)
                {
                    auto K = WTrimView(Line.substr(0, p));//ttf
                    auto V = WTrimView(Line.substr(p + 1, Line.size() - p - 1));//name
                    if (K.ends_with(L".TTF") || K.ends_with(L".TTC"))
                    {
                        SystemFonts.emplace_back(std::wstring(V), std::wstring(K));
                        F.Linked.push_back(SystemFonts.back());
                    }
                }
            }
            FontMapperFamilyFallback.push_back({ wsValueName, wsFontFile });
        } while (result != ERROR_NO_MORE_ITEMS);

        delete[] valueName;
        delete[] valueData;
        RegCloseKey(hKey);
    }
    void BuildFontQuery5()
    {
        WCHAR winDir[MAX_PATH]{};
        GetWindowsDirectoryW(winDir, MAX_PATH);
        FontDir = winDir;
        FontDir += L"\\Fonts\\";

        auto V = FindFileVec(FontDir + L"*.ttf");
        for (auto& W : V)
        {
            ToUpper(W);
            SystemFonts.push_back({ W,W });
        }
        V = FindFileVec(FontDir + L"*.ttc");
        for (auto& W : V)
        {
            ToUpper(W);
            SystemFonts.push_back({ W,W });
        }
        //MessageBoxW(NULL, FontDir.c_str(), L"FontDir!", MB_OK);
    }

    void BuildFontQuery()
    {
        BuildFontQuery1();
        BuildFontQuery2();
        BuildFontQuery3();
        BuildFontQuery4();
        BuildFontQuery5();
    }
}
