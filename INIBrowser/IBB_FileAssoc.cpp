
#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>      // 用于 PathQuoteSpaces
#include <iostream>
#include <ShlObj.h>
#include <atlbase.h>
#include <shobjidl.h>
#include <string>
#include <VersionHelpers.h>
#include "IBR_Localization.h"

#pragma comment(lib, "shlwapi.lib")

const std::wstring extension = L".iproj";
const std::wstring progId = L"Ra2diy.IniWeaver.iproj";   // 可根据需要改成唯一标识

// 辅助函数：检查是否以管理员身份运行
static bool IsRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup))
    {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}

// 设置文件关联（根据权限自动选择用户级或系统级）
bool SetFileAssociation()
{
    // 1. 获取当前可执行文件的完整路径
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0)
    {
        std::cerr << "Failed to get executable path." << std::endl;
        return false;
    }

    // 3. 决定注册表根键
    HKEY rootKey;
    if (IsRunningAsAdmin())
    {
        // 系统级关联：写入 HKEY_LOCAL_MACHINE\Software\Classes
        rootKey = HKEY_LOCAL_MACHINE;
        std::wcout << L"Running as admin, using system-wide association." << std::endl;
    }
    else
    {
        // 用户级关联：写入 HKEY_CURRENT_USER\Software\Classes
        rootKey = HKEY_CURRENT_USER;
        std::wcout << L"Not admin, using per-user association." << std::endl;
    }

    // 4. 构造注册表键路径
    std::wstring classesBase = L"Software\\Classes\\";
    std::wstring extKey = classesBase + extension;          
    std::wstring progIdKey = classesBase + progId;       
    std::wstring commandKey = progIdKey + L"\\shell\\open\\command";

    HKEY hKey;
    DWORD disposition;

    // 5. 关联扩展名 -> ProgID
    if (RegCreateKeyExW(rootKey, extKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, nullptr, &hKey, &disposition) != ERROR_SUCCESS)
    {
        std::cerr << "Failed to create extension key." << std::endl;
        return false;
    }
    // 设置默认值为 ProgID
    if (RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)progId.c_str(),
        static_cast<DWORD>((progId.size() + 1) * sizeof(wchar_t))) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        std::cerr << "Failed to set default value for extension." << std::endl;
        return false;
    }
    RegCloseKey(hKey);

    // 6. 创建 ProgID 键，设置显示名称（可选）
    if (RegCreateKeyExW(rootKey, progIdKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, nullptr, &hKey, &disposition) != ERROR_SUCCESS)
    {
        std::cerr << "Failed to create ProgID key." << std::endl;
        return false;
    }
    std::wstring displayName = L"INI织网者 工程文件";
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)displayName.c_str(),
        static_cast<DWORD>((displayName.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    // 7. 创建 shell\open\command 子键，设置打开命令
    if (RegCreateKeyExW(rootKey, commandKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, nullptr, &hKey, &disposition) != ERROR_SUCCESS)
    {
        std::cerr << "Failed to create command key." << std::endl;
        return false;
    }
    // 命令格式："exe路径" "%1"
    std::wstring command = L"\"" + std::wstring(exePath) + L"\" \"%1\"";
    if (RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)command.c_str(),
        static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t))) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        std::cerr << "Failed to set command value." << std::endl;
        return false;
    }
    RegCloseKey(hKey);

    // 8. 通知系统文件关联已更改
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    std::wcout << L"File association for " << extension << L" set successfully." << std::endl;
    return true;
}

bool IsDefaultForExtension()
{
    wchar_t current[MAX_PATH] = { 0 };
    DWORD cch = MAX_PATH;
    // 获取当前用户默认的 ProgID
    HRESULT hr = AssocQueryString(ASSOCF_INIT_IGNOREUNKNOWN,
        ASSOCSTR_PROGID,
        extension.c_str(),
        nullptr,
        current,
        &cch);
    if (SUCCEEDED(hr))
    {
        return (progId == current);
    }
    return false;
}

bool GuideUserToSetDefaultProgram()
{
    MessageBoxW(nullptr, locwc("GUI_FileAssocNotDefault"), locwc("GUI_FileAssocSetDefault"), MB_OK | MB_ICONINFORMATION);

    //对windows 8及以上版本，打开默认应用设置界面
    if (IsWindows8OrGreater())
    {
        std::wstring uri = L"ms-settings:defaultapps?fileExtension=" + extension;
        ShellExecute(nullptr, L"open", uri.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    else
    {
        ShellExecute(nullptr, L"open", L"control.exe", L"/name Microsoft.DefaultPrograms", nullptr, SW_SHOWNORMAL);
    }

    return true;
}
