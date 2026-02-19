#pragma once

#define IDI_ICON1 101
#define DllLoadFunc(Fn) (Fn=(decltype(Fn))GetProcAddress(TElib,#Fn))

#include "MainStage.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include <Shlwapi.h>
#include "IBR_Font.h"
#include <filesystem>
#include "IBR_HotKey.h"
#include "IBR_Debug.h"
#include <dbghelp.h>
#include "IBB_FileChecker.h"

#pragma comment(lib, "Dbghelp.lib")

bool ShouldCloseShellLoop = false;
bool GotoCloseShellLoop = false;
bool EnableDebugList = false;
bool NoTopExceptionHandler = false;
bool NoVSync = false;
std::atomic_bool LoadDatabaseComplete{ false };
JsonFile Cfg;//Config.json

std::set<int> DisabledGLFWErrors;
std::string TimeNowU8();

void SuppressGLFWError(int errorcode)
{
    DisabledGLFWErrors.insert(errorcode);
}

void UnsuppressGLFWError(int errorcode)
{
    DisabledGLFWErrors.erase(errorcode);
}

bool IssuppressGLFWError(int errorcode)
{
    return DisabledGLFWErrors.find(errorcode) != DisabledGLFWErrors.end();
}

namespace Initialize
{
    



    void glfw_error_callback(int error, const char* description)
    {
        if(IssuppressGLFWError(error))
            return;

        const auto descw = UTF8toUnicode(description);
        auto gle = std::vformat(locw("Error_GlfwErrorText"), std::make_wformat_args(error, descw));
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(gle);
        }
        MessageBoxW(NULL,
            gle.c_str(),
            _AppNameW, MB_ICONERROR);
    }

    bool InitializeLanguage()
    {
        if (!IBR_L10n::LoadFromINI(L"\\Resources\\Language.ini"))
            return false;
        return true;
    }

    void InitConfigJson()
    {
        IBB_FileCheck(".\\Resources\\config.json", false, true, false);

        IBR_PopupManager::AddJsonParseErrorPopup(Cfg.ParseFromFileChecked(".\\Resources\\config.json", loc("Error_JsonParseErrorPos"), nullptr)
            , UnicodetoUTF8(std::vformat(locw("Error_JsonSyntaxError"), std::make_wformat_args(L"Config.json"))));
        if (!Cfg.Available())
        {
            //MessageBoxA(NULL, "config.json 读取失败！", AppNameA, MB_OK);
            FontPath.clear();
        }
        else
        {
            auto Obj = Cfg.GetObj();
            FontPath = Obj.ItemStringOr("FontName", "");

            auto S = Obj.GetObjectItem("StyleLight");
            if (S.Available())IBR_Color::LoadLight(S);

            S = Obj.GetObjectItem("StyleDark");
            if (S.Available())IBR_Color::LoadDark(S);

            auto NodeStyleStr = Obj.ItemStringOr("NodeStyle", "Circle");
            if (NodeStyleStr == "Circle")GlobalNodeStyle = ImGuiRadioButtonFlags_Circle;
            else if (NodeStyleStr == "Rounded")GlobalNodeStyle = ImGuiRadioButtonFlags_RoundedSquare;
            else if (NodeStyleStr == "Square")GlobalNodeStyle = ImGuiRadioButtonFlags_Square;
            else GlobalNodeStyle = ImGuiRadioButtonFlags_Circle;
        }
        auto A0 = UTF8toUnicode(FontPath);
        auto Fin = IBR_Font::SearchFont(A0);
        FontPath = UnicodetoUTF8(Fin);

        if (FontPath.empty())
        {
            std::vector<std::wstring> WPFallback{
                L"Microsoft Yahei", L"Microsoft Jhenghei", L"SimHei", L"SimSun", L"Segoe UI", L"Consolas"
            };
            for (auto& S : WPFallback)
            {
                auto Alt = IBR_Font::SearchFont(S);
                if (!Alt.empty())
                {
                    auto P = std::filesystem::path(Alt);
                    if (std::filesystem::exists(P))
                    {
                        FontPath = UnicodetoUTF8(Alt);
                        if (EnableLog)
                        {
                            GlobalLog.AddLog_CurTime(false);
                            GlobalLog.AddLog(std::vformat(locw("Log_FallbackFont"), std::make_wformat_args(Alt)));
                        }
                        break;
                    }
                }
            }
        }

        A0 = UTF8toUnicode(FontPath);

        IBB_FileCheck(A0, false, true, false);

        if (FontPath.empty())
        {
            MessageBoxW(NULL, std::vformat(locw("Error_InvalidFontName"), std::make_wformat_args(A0)).c_str()
                , locwc("Error_FailedToLoadFont"), MB_ICONERROR);
            exit(0);
        }
        auto P = std::filesystem::path(A0);
        if (!std::filesystem::exists(P))
        {
            MessageBoxW(NULL, std::vformat(locw("Error_InvalidTTF"), std::make_wformat_args(A0)).c_str()
                , locwc("Error_FailedToLoadFont"), MB_ICONERROR);
            exit(0);
        }
        //MessageBoxW(NULL, Fin.c_str(), L"SearchFont", MB_OK);
    }

    void InitializeHotKeys()
    {
        auto o = Cfg.GetObj();
        if (!o.Available())return;
        auto S = o.GetObjectItem("HotKeys");
        IBR_HotKey::InitFromJson(S);
    }

    void InitializeStyle()
    {
        FontHeight = IBG_GetSetting().FontSize;
        KeyPerPage = IBG_GetSetting().MenuLinePerPage;
        if (IBG_GetSetting().DarkMode)IBR_Color::StyleDark();
        else IBR_Color::StyleLight();
    }

    bool LoadMyRange = false;
    ImVector<ImWchar> myRange;

    void InitializeFontFile()
    {
        using namespace PreLink;
        ImGuiIO& io = ImGui::GetIO();
        if (LoadMyRange)
        {
            font = io.Fonts->AddFontFromFileTTF(FontPath.c_str(), (float)FontHeight, NULL, myRange.Data);
        }
        else
        {
            font = io.Fonts->AddFontFromFileTTF(FontPath.c_str(), (float)FontHeight, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        }
        if (font == NULL)
        {
            auto A0 = UTF8toUnicode(FontPath);
            MessageBoxW(nullptr, std::vformat(locw("Error_InvalidTTF"), std::make_wformat_args(A0)).c_str(), locwc("Error_FailedToLoadFont"), MB_ICONERROR);
        }
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            auto wf = UTF8toUnicode(FontPath);
            GlobalLog.AddLog(std::vformat(locw("Log_LoadFont"), std::make_wformat_args(wf)));
        }
    }

    void CallInitializeDatabase()
    {
        IBRF_CoreBump.SendToF([] {
            IBB_DefaultRegType::LoadFromFile(L".\\Global\\RegisterTypes*.json");//FIRST !
            IBF_Inst_DefaultTypeList.ReadAltSetting(L".\\Global\\TypeAlt*");//SECOND ! DEPENDENT
            IBB_ModuleAltDefault::Load(L".\\Global\\Modules\\*.*", L".\\Global\\ImageModules\\*.*", L"\\Global\\Modules\\");//THIRD ! DEPENDENT
            IBR_Inst_Debug.DebugLoad();
            LoadDatabaseComplete = true;
            });
    }



    void CleanUp()
    {
        // Cleanup
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        //DeleteFileA("imgui.ini");
        glfwDestroyWindow(PreLink::window);
        glfwTerminate();
    }

    PEXCEPTION_POINTERS pException;
    HANDLE CrashEvent;
    HANDLE CrashFinishEvent;
    BufString CrashBuf;

    LONG WINAPI TopLevelFilter(PEXCEPTION_POINTERS pExceptionInfo)
    {
        pException = pExceptionInfo;
        SetEvent(CrashEvent);
        WaitForSingleObject(CrashFinishEvent, INFINITE);
        return 0;
    }

    DWORD WINAPI CrashLoggerThread(LPVOID)
    {
        WaitForSingleObject(CrashEvent, INFINITE);

        auto Addr = pException->ExceptionRecord->ExceptionAddress;
        auto Code = pException->ExceptionRecord->ExceptionCode;

        if (Code == EXCEPTION_ACCESS_VIOLATION)
        {
            DWORD op = pException->ExceptionRecord->ExceptionInformation[0];
            PVOID target = (PVOID)pException->ExceptionRecord->ExceptionInformation[1];
            const char* Access = (
                op == 0 ? "READ" : (
                op == 1 ? "WRITE" : (
                op == 8 ? "EXECUTE" : (
                "UNKNOWN"
                ))));
            sprintf_s(CrashBuf, "Addr = %p\nCode = %p\n%s(%d) at %p", Addr, (PVOID)Code, Access, op, target);
        }
        else
        {
            sprintf_s(CrashBuf, "Addr = %p\nCode = %p\n", Addr, (PVOID)Code);
        }

        const auto MS_VC_EXCEPTION = 0xE06D7363;
        if (Code != MS_VC_EXCEPTION)
        {
            MessageBoxW(nullptr, UTF8toUnicode(CrashBuf).c_str(), locwc("Error_FatalError"), MB_ICONERROR);
        }
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(locc("Log_FatalErrorDesc"));
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(CrashBuf);
        }

        HANDLE hFile = CreateFileW(L"Crash.dmp", GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION mdei;
            mdei.ThreadId = GetCurrentThreadId();
            mdei.ExceptionPointers = pException;
            mdei.ClientPointers = FALSE;
            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                hFile, MiniDumpWithDataSegs, &mdei, NULL, NULL);
            CloseHandle(hFile);
        }

        MessageBoxW(nullptr, locwc("Error_CheckCrashDump"), locwc("Error_FatalError"), MB_ICONERROR);

        SetEvent(CrashFinishEvent);
        return 0;
    }

    void InitializeSEHHandler()
    {
        if (NoTopExceptionHandler) return;
        CrashEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        CrashFinishEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        SetUnhandledExceptionFilter(TopLevelFilter);
        HANDLE hThread = CreateThread(NULL, 0, CrashLoggerThread, NULL, 0, NULL);
        CloseHandle(hThread);
    }

    void CheckFontAtlas()
    {
        using namespace PreLink;

        if (font->ContainerAtlas == NULL)
        {
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog("font->ContainerAtlas == NULL");
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(locc("Log_PleaseRestart"));
            }
            MessageBoxW(nullptr, L"font->ContainerAtlas == NULL", locwc("Error_FailedToLoadFont"), MB_ICONERROR);
            MessageBoxW(nullptr, locwc("Log_PleaseRestart"), locwc("Error_FailedToLaunch"), MB_ICONERROR);
            CleanUp();
            exit(0);
        }
    }

    uint64_t TimeWait;
    void AdjustFrameRate()
    {
        if (IBF_Inst_Setting.FrameRateLimit() != -1)
        {
            int Uax = 1000000 / IBF_Inst_Setting.FrameRateLimit();
            while (GetSysTimeMicros() > TimeWait + 5000000) TimeWait += 1000000;
            while (GetSysTimeMicros() < TimeWait)Sleep(Uax / 1000);
            TimeWait += Uax;
        }
        ShellLoopLastTime = GetSysTimeMicros();
    }

    void DrawBackground()
    {
        using namespace PreLink;

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        auto& cc = IBR_Color::BackgroundColor.Value;
        glClearColor(cc.x * cc.w, cc.y * cc.w, cc.z * cc.w, cc.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void CheckIfClose()
    {
        if (glfwWindowShouldClose(PreLink::window))
        {
            if (IBR_ProjectManager::IsOpen())
            {
                GotoCloseShellLoop = true;
                IBR_ProjectManager::CloseAction();
            }
            else ShouldCloseShellLoop = true;
        }
    }

    void ShellLoop_Unprotected()
    {
        

        TimeWait = GetSysTimeMicros();

        while (!ShouldCloseShellLoop)
        {
            AdjustFrameRate();
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL2_NewFrame();
            ImGui_ImplGlfw_NewFrame();

            CheckFontAtlas();

            ImGui::NewFrame();

            ControlPanel();

            // Rendering
            ImGui::Render();
            if(!IBR_Inst_Debug.DontDrawBg) DrawBackground();

            // If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
            // you may need to backup/reset/restore other state, e.g. for current shader using the commented lines below.
            //GLint last_program;
            //glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
            //glUseProgram(0);
            ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
            //glUseProgram(last_program);

            glfwMakeContextCurrent(PreLink::window);
            glfwSwapBuffers(PreLink::window);

            CheckIfClose();
        }
    }

    void ShellLoop_Protected()
    {
        try
        {
            ShellLoop_Unprotected();
        }
        catch (std::exception& e)
        {
            MessageBoxW(nullptr, UTF8toUnicode(e.what()).c_str(), locwc("Error_ThrowException"), MB_ICONERROR);
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(locc("Log_MainLoopErrorDesc"));
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(e.what());
            }
            throw(e);
        }
        catch (...)
        {
            MessageBoxW(nullptr, locwc("Error_UnknownException"), locwc("Error_ThrowException"), MB_ICONERROR);
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(locc("Error_UnknownException"));
            }
        }
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(locc("Log_MainLoopFinish"));
        }
    }

    void ShellLoop()
    {
        if (NoTopExceptionHandler)
        {
            ShellLoop_Unprotected();
        }
        else
        {
            ShellLoop_Protected();
        }
    }

    void InitializeLogger()
    {
        GlobalLog.SetPath(CurrentDirA + std::string("\\browser.log"));
        GlobalLogB.SetPath(CurrentDirA + std::string("\\backend.log"));
        GlobalLog.ClearLog();
        GlobalLogB.ClearLog();

        GlobalLog.AddLog_CurTime(false);
        auto cwa = locw("AppName");
        GlobalLog.AddLog(std::vformat(locw("Log_InitializeLogger1"), std::make_wformat_args(cwa, VersionW)));
        GlobalLog.AddLog_CurTime(false);
        GlobalLog.AddLog(locc("Log_InitializeLogger2"));
    }

    bool FixLaunchPath()
    {
        TEMPLOG("wchar_t szExePath[MAX_PATH];")
            wchar_t szExePath[MAX_PATH];

        TEMPLOG("if (GetModuleFileNameW(NULL, szExePath, MAX_PATH) == 0)")
            if (GetModuleFileNameW(NULL, szExePath, MAX_PATH) == 0)
            {
                TEMPLOG("return false;")
                    return false;
            }

        TEMPLOG("if (!PathRemoveFileSpecW(szExePath))")
            if (!PathRemoveFileSpecW(szExePath))
            {
                TEMPLOG("return false;")
                    return false;
            }

        TEMPLOG("if (!SetCurrentDirectoryW(szExePath))")
            if (!SetCurrentDirectoryW(szExePath))
            {
                TEMPLOG("return false;")
                    return false;
            }

        TEMPLOG("wchar_t szExePath[MAX_PATH];")
            return true;
    }

    bool InitializePath()
    {
        TEMPLOG("if (!FixLaunchPath())")
            if (!FixLaunchPath())
            {
                MessageBoxW(NULL, locwc("Error_CannotInitializePath"), locwc("Error_FalledToLaunch"), MB_OK);
                TEMPLOG("return false;")
                    return false;
            }


        char DesktopTmp[300];
        wchar_t DefaultpathTmp[300];

        TEMPLOG("SHGetSpecialFolderPathW(0, DefaultpathTmp, CSIDL_DESKTOPDIRECTORY, 0);")
            SHGetSpecialFolderPathW(0, DefaultpathTmp, CSIDL_DESKTOPDIRECTORY, 0);
        SHGetSpecialFolderPathA(0, DesktopTmp, CSIDL_DESKTOPDIRECTORY, 0);

        TEMPLOG("GetCurrentDirectoryW(MAX_PATH, CurrentDirW);")
            GetCurrentDirectoryW(MAX_PATH, CurrentDirW);
        GetCurrentDirectoryA(MAX_PATH, CurrentDirA);

        TEMPLOG("IBR_RecentManager::Path = CurrentDirW;")
            IBR_RecentManager::Path = CurrentDirW;

        TEMPLOG("IBR_RecentManager::Path += L\"\\Resources\\recent.dat\";")
            IBR_RecentManager::Path += L"\\Resources\\recent.dat";

        TEMPLOG("Defaultpath = DefaultpathTmp;")
            Defaultpath = DefaultpathTmp;
        Desktop = DesktopTmp;

        return true;
    }

    void InitializeResolution()
    {
        RScrX = GetSystemMetrics(SM_CXSCREEN);
        RScrY = GetSystemMetrics(SM_CYSCREEN);

        IBR_DynamicData::Read(1280, 720);//默认分辨率
        IBR_DynamicData::SetRandom();
    }

    void ProcessCommandLine(LPWSTR lpCmdLine)
    {
        LPWSTR* szArglist;
        int nArgs;
        szArglist = CommandLineToArgvW(lpCmdLine, &nArgs);

        for (int i = 0; i < nArgs; i++)
        {
            if (!_wcsicmp(PathFindExtensionW(szArglist[i]), ExtensionNameW) && PathFileExistsW(szArglist[i]))
            {
                //delayed open
                IBRF_CoreBump.SendToR({ [Path = std::wstring(szArglist[i])]() {IBR_ProjectManager::OpenRecentAction(Path); } });
            }
            else if (!_wcsicmp(szArglist[i], L"-log"))
            {
                EnableLog = true;
            }
            else if (!_wcsicmp(szArglist[i], L"-logex"))
            {
                EnableLogEx = true;
            }
            else if (!_wcsicmp(szArglist[i], L"-nolog"))
            {
                EnableLog = false;
            }
            else if (!_wcsicmp(szArglist[i], L"-nologex"))
            {
                EnableLogEx = false;
            }
            else if (!_wcsicmp(szArglist[i], L"-debugmenu"))
            {
                EnableDebugList = true;
            }
            else if (!_wcsicmp(szArglist[i], L"-notophandler"))
            {
                NoTopExceptionHandler = true;
            }
            else if (!_wcsicmp(szArglist[i], L"-novsync"))
            {
                NoVSync = true;
            }
            else if (!_wcsnicmp(szArglist[i], L"-width=", 7))
            {
                auto W = _wtoi(szArglist[i] + 7);
                if (W > 0) IBR_DynamicData::SetDefaultWidth(W);
            }
            else if (!_wcsnicmp(szArglist[i], L"-height=", 8))
            {
                auto H = _wtoi(szArglist[i] + 8);
                if (H > 0) IBR_DynamicData::SetDefaultHeight(H);
            }
        }

        LocalFree(szArglist);
    }

    int Initialize_Stage_I(HINSTANCE hInstance, LPWSTR lpCmdLine)
    {
        TEMPLOG("PreLink::hInst = hInstance;")
            PreLink::hInst = hInstance;

        TEMPLOG("if (!InitializePath())return 2;")
            if (!InitializePath())return 2;

        TEMPLOG(" ProcessCommandLine(lpCmdLine);")
            ProcessCommandLine(lpCmdLine);

        TEMPLOG("if (!InitializeLanguage())return 3;")
            if (!InitializeLanguage())return 3;

        TEMPLOG("InitializeLogger();")
            InitializeLogger();

        TEMPLOG("InitializeResolution();")
            InitializeResolution();

        TEMPLOG("InitializeSEHHandler();")
            InitializeSEHHandler();

        TEMPLOG("return 0;")
            return 0;
    }

    int Initialize_Stage_II()
    {
        using namespace PreLink;

        TEMPLOG("glfwSetErrorCallback(glfw_error_callback);");
        glfwSetErrorCallback(glfw_error_callback);

        TEMPLOG("if (!glfwInit())return 1;");
        if (!glfwInit())return 1;

        TEMPLOG("glfwWindowHint(GLFW_VISIBLE, GL_FALSE);");
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

        TEMPLOG("window = glfwCreateWindow(ScrX, ScrY, _AppName, NULL, NULL);");
        window = glfwCreateWindow(ScrX, ScrY, _AppName, NULL, NULL);

        TEMPLOG("MainWindowHandle = glfwGetWin32Window(window);");
        MainWindowHandle = glfwGetWin32Window(window);
        //glfwHideWindow(window);

        TEMPLOG("SetClassLongW(MainWindowHandle, GCL_HICON, (LONG)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)));");
        SetClassLongW(MainWindowHandle, GCL_HICON, (LONG)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)));
        //ShowWindow(MainWindowHandle, SW_HIDE);

        TEMPLOG("if (window == NULL)return 1;");
        if (window == NULL)return 1;

        TEMPLOG("glfwMakeContextCurrent(window);");
        glfwMakeContextCurrent(window);

        TEMPLOG("if(!NoVSync) glfwSwapInterval(1); ");
        if(!NoVSync) glfwSwapInterval(1); // Enable vsync

        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(locc("Log_OpenWindow"));
        }
        return 0;
    }

    void Initialize_Stage_III()
    {
        TEMPLOG(" std::thread FrontThr(IBF_Thr_FrontLoop);");
        std::thread FrontThr(IBF_Thr_FrontLoop);
        TEMPLOG(" FrontThr.detach();");
        FrontThr.detach();
        TEMPLOG("std::thread SaveThr(IBS_Thr_SaveLoop);");
        std::thread SaveThr(IBS_Thr_SaveLoop);
        TEMPLOG("SaveThr.detach();");
        SaveThr.detach();
    }

    void Initialize_Stage_IV()
    {
        using namespace PreLink;

        IBR_FullView::ViewSize = { FontHeight * 12.0f, FontHeight * 12.0f };
        IBR_FullView::CurrentEqMax = IBR_FullView::GetDefaultEqMax();

        IBR_Inst_Setting.SetSettingName(SettingFileName);

        IBB_FileCheck(SettingFileName, true, true, false);

        IBR_Inst_Setting.CallReadSetting();

        std::string EncodingStr;//MBCS Unicode UTF8

        IBR_Font::BuildFontQuery();

        TEMPLOG(" InitConfigJson();");
        InitConfigJson();

        IBB_FileCheck(".\\Resources\\hint.txt", false, true, false);

        TEMPLOG("IBR_HintManager::Load();");
        IBR_HintManager::Load();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        TEMPLOG("ImGui::CreateContext();");
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = NULL;
        ImGui::GetIO().LogFilename = NULL;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        //ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();
        //ImGui::StyleColorsLight();

        IBB_FileCheck(IBR_RecentManager::Path, true, true, false);

        TEMPLOG("IBR_RecentManager::Load();");
        IBR_RecentManager::Load();
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(std::vformat(locw("Log_LoadRecent"), std::make_wformat_args(L".\\Resources\\recent.dat")));
        }

        ImGuiIO& io = ImGui::GetIO();
        ExtFileClass GetHint;

        IBB_FileCheck(".\\Resources\\load.txt", false, true, false);

        if (GetHint.Open(".\\Resources\\load.txt", "rb"))
        {
            ImFontGlyphRangesBuilder myGlyph;

            ImWchar base_ranges[] =
            {
                0x0020, 0x0FFF, // Basic Supplement
                0x2000, 0x206F, // General Punctuation
                0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
                0x31F0, 0x31FF, // Katakana Phonetic Extensions
                0xFF00, 0xFFEF, // Half-width characters
                0xFFFD, 0xFFFD, // Invalid
                0
            };
            unsigned char* charBuf;
            GetHint.Seek(0, SEEK_END);
            int Len = GetHint.Position();
            charBuf = new unsigned char[Len + 1]();
            GetHint.Seek(0, SEEK_SET);
            GetHint.Read(charBuf, 1, Len);
            GetHint.Close();
            charBuf[Len] = 0;
            myGlyph.AddRanges(base_ranges);
            myGlyph.AddText((const char*)charBuf);
            myGlyph.BuildRanges(&myRange);
            delete[]charBuf;
            //Sleep(12000);
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(std::vformat(locw("Log_LoadCharList"), std::make_wformat_args(L".\\Resources\\load.txt")));
            }

            //0x4e00, 0x9FAF, // CJK Ideograms
            //font = io.Fonts->AddFontFromFileTTF(FontPath.c_str(), (float)FontHeight, NULL, LoadRange.data());

            LoadMyRange = true;

        }
        else
        {
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(std::vformat(locw("Log_LoadCharListFailed"), std::make_wformat_args(L".\\Resources\\load.txt")));
            }

            LoadMyRange = false;
            font = io.Fonts->AddFontFromFileTTF(FontPath.c_str(), (float)FontHeight, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            //font = io.Fonts->AddFontFromFileTTF(FontPath.c_str(), (float)FontHeight, NULL, io.Fonts->GetGlyphRangesChineseFull());
        }


        // Setup Platform/Renderer backends
        TEMPLOG("ImGui_ImplGlfw_InitForOpenGL(window, true);");
        ImGui_ImplGlfw_InitForOpenGL(window, true);

        TEMPLOG("ImGui_ImplOpenGL2_Init();");
        ImGui_ImplOpenGL2_Init();

        TEMPLOG("glfwShowWindow(PreLink::window);");
        glfwShowWindow(PreLink::window);
        SetClassLong(MainWindowHandle, GCL_HICON, (LONG)LoadIconW(PreLink::hInst, MAKEINTRESOURCE(IDI_ICON1)));
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(locc("Log_GlfwShowWindow"));
        }

        while (!IBR_Inst_Setting.IsReadSettingComplete());

        TEMPLOG("CallInitializeDatabase();");
        CallInitializeDatabase();

        TEMPLOG("InitializeStyle();");
        InitializeStyle();

        TEMPLOG("InitializeHotKeys();");
        InitializeHotKeys();

        TEMPLOG("InitializeFontFile();");
        InitializeFontFile();

        TEMPLOG("glfwSetDropCallback(PreLink::window, IBR_ProjectManager::OnDropFile);");
        glfwSetDropCallback(PreLink::window, IBR_ProjectManager::OnDropFile);

        while (!LoadDatabaseComplete);

        TEMPLOG("IBR_ProjectManager::CreateAction();");
        IBR_ProjectManager::CreateAction();
    }
}

