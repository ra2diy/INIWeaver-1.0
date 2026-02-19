#include "IBB_FileChecker.h"
#include "FromEngine/Include.h"
#include "IBR_Components.h"
#include "IBR_Localization.h"
#include "Global.h"
#include <filesystem>

void IBB_FileCheck(const char* FileName, bool AllowNotExist, bool PopupOnError, bool Critical)
{
    IBB_FileCheck(UTF8toUnicode(FileName), AllowNotExist, PopupOnError, Critical);
}
void IBB_FileCheck(const wchar_t* FileName, bool AllowNotExist, bool PopupOnError, bool Critical)
{
    IBB_FileCheck(std::wstring(FileName), AllowNotExist, PopupOnError, Critical);
}
void IBB_FileCheck(const std::string& FileName, bool AllowNotExist, bool PopupOnError, bool Critical)
{
    IBB_FileCheck(UTF8toUnicode(FileName), AllowNotExist, PopupOnError, Critical);
}
void IBB_FileCheck(const std::wstring& FileName, bool AllowNotExist, bool PopupOnError, bool Critical)
{
    //检查是否是文件
    //检查是否存在
    //检查是否拒绝访问

    //如果不存在，在AllowNotExist时检查通过
    //如果需要报错，若PopupOnError则推送一个弹窗
    //如果Critical则MessageBox接直接退出程序

    bool IsFile, Exists, HasAccess;
    if (FileName.empty())
    {
        IsFile = false;
        Exists = false;
        HasAccess = false;
    }
    else
    {
        try
        {
            auto Path = std::filesystem::path(FileName);

            //如果文件名是.或者..，则直接返回
            if (Path.filename() == "." || Path.filename() == "..")
                return;

            IsFile = std::filesystem::is_regular_file(Path);
            Exists = std::filesystem::exists(Path);
            auto perm = std::filesystem::status(Path).permissions();
            if (Exists)
            {
                HasAccess = ((perm & std::filesystem::perms::owner_read) != std::filesystem::perms::none);
                HasAccess &= ExtFileClass().Open(FileName.c_str(), L"r");
            }
            else HasAccess = false;
        }
        catch (...)
        {
            IsFile = false;
            Exists = false;
            HasAccess = false;
        }
    }

    if (EnableLogEx)
    {
        sprintf_s(LogBufB, "IBB_FileCheck : <- FileName=%s, IsFile = %s, Exists = %s, HasAccess = %s", UnicodetoUTF8(FileName).c_str(), IBD_BoolStr(IsFile), IBD_BoolStr(Exists), IBD_BoolStr(HasAccess));
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog(LogBufB);
    }

    const auto ReportError = [&](const std::wstring& Info) {
        if (PopupOnError)
        {
            IBR_PopupManager::AddLoadConfigErrorPopup(UnicodetoUTF8(Info), loc("Log_FailedToLoadConfig"));
        }
        auto Str = std::vformat(locw("Log_LoadConfigErrorInfo"), std::make_wformat_args(Info));
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog(UnicodetoUTF8(Str).c_str());
        }
        if (Critical)
        {
            MessageBoxW(NULL, Str.c_str(), locwc("Error_FatalError"), MB_ICONERROR);
            ExitProcess(0);
        }
    };

    if (!IsFile && !AllowNotExist)
    {
        ReportError(std::vformat(locw("Error_PathNotAFile"), std::make_wformat_args(FileName)));
    }
    else if (!Exists && !AllowNotExist)
    {
        ReportError(std::vformat(locw("Error_FileNotExist"), std::make_wformat_args(FileName)));
    }
    else if (Exists && !HasAccess)
    {
        ReportError(std::vformat(locw("Error_FileAccessDenied"), std::make_wformat_args(FileName)));
    }
}
