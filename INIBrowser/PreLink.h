#pragma once

#define IDI_ICON1 101
#define DllLoadFunc(Fn) (Fn=(decltype(Fn))GetProcAddress(TElib,#Fn))

#include "MainStage.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include <Shlwapi.h>
#include "IBR_Font.h"
#include <filesystem>

bool ShouldCloseShellLoop = false;
bool GotoCloseShellLoop = false;

namespace PreLink
{
    static void glfw_error_callback(int error, const char* description)
    {
        auto gle = std::vformat(loc("Error_GlfwErrorText"), std::make_format_args(error, description));
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(gle.c_str());
        }
        MessageBoxW(NULL,
            UTF8toUnicode(gle).c_str(),
            _AppNameW, MB_ICONERROR);
    }

    bool InitializeLanguage()
    {
        if (!IBR_L10n::LoadFromINI(L"\\Resources\\Language.ini"))
            return false;
        return true;
    }

    int PreLoop1()
    {
        

        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return 1;
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        window = glfwCreateWindow(ScrX, ScrY, _AppName, NULL, NULL);
        MainWindowHandle = glfwGetWin32Window(window);
        //glfwHideWindow(window);
        SetClassLong(MainWindowHandle, GCL_HICON, (LONG)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)));
        //ShowWindow(MainWindowHandle, SW_HIDE);

        if (window == NULL)
            return 1;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(locc("Log_OpenWindow"));
        }
        return 0;
    }

    void InitConfigJson()
    {
        JsonFile Cfg;
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
        }
        auto A0 = UTF8toUnicode(FontPath);
        auto Fin = IBR_Font::SearchFont(A0);
        FontPath = UnicodetoUTF8(Fin);

        if(FontPath.empty())
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

        if (FontPath.empty())
        {
            MessageBoxW(NULL, std::vformat(locw("Error_InvalidFontName"), std::make_wformat_args(A0)).c_str()
                , locwc("Error_FailedToLoadFont"), MB_ICONERROR);
            exit(0);
        }
        auto P = std::filesystem::path(Fin);
        if (!std::filesystem::exists(P))
        {
            MessageBoxW(NULL, std::vformat(locw("Error_InvalidTTF"), std::make_wformat_args(A0)).c_str()
                , locwc("Error_FailedToLoadFont"), MB_ICONERROR);
            exit(0);
        }
        //MessageBoxW(NULL, Fin.c_str(), L"SearchFont", MB_OK);
    }

    void PreLoop2()
    {
        IBR_Inst_Setting.SetSettingName(SettingFileName);
        IBR_Inst_Setting.CallReadSetting();

        std::string EncodingStr;//MBCS Unicode UTF8

        IBR_Font::BuildFontQuery();
        InitConfigJson();
        
        IBR_HintManager::Load();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = NULL;
        ImGui::GetIO().LogFilename = NULL;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        //ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();
        //ImGui::StyleColorsLight();
        

        IBR_RecentManager::Load();
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(std::vformat(locw("Log_LoadRecent"), std::make_wformat_args(L".\\Resources\\recent.dat")));
        }

        ImGuiIO& io = ImGui::GetIO();
        ExtFileClass GetHint;
        static ImVector<ImWchar> myRange;
        bool LoadMyRange = false;
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
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL2_Init();

        glfwShowWindow(PreLink::window);
        SetClassLong(MainWindowHandle, GCL_HICON, (LONG)LoadIconW(PreLink::hInst, MAKEINTRESOURCE(IDI_ICON1)));
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(locc("Log_GlfwShowWindow"));
        }

        while (!IBR_Inst_Setting.IsReadSettingComplete());
        FontHeight = IBG_GetSetting().FontSize;
        KeyPerPage = IBG_GetSetting().MenuLinePerPage;
        if (IBG_GetSetting().DarkMode)IBR_Color::StyleDark();
        else IBR_Color::StyleLight();

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

        IBB_DefaultRegType::LoadFromFile(u8".\\Global\\RegisterTypes.json");//FIRST !
        IBF_Inst_DefaultTypeList.ReadAltSetting(u8".\\Global\\TypeAlt");//SECOND ! DEPENDENT
        IBB_ModuleAltDefault::Load(L".\\Global\\Modules\\*.*", L".\\Global\\ImageModules\\*.*", L"\\Global\\Modules\\");//THIRD ! DEPENDENT
        IBR_Inst_Debug.DebugLoad();

        glfwSetDropCallback(PreLink::window, IBR_ProjectManager::OnDropFile);
        IBR_ProjectManager::CreateAction();
    }

    void CleanUp()
    {
        
        // Cleanup
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        //DeleteFileA("imgui.ini");
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void ShellLoop()
    {
        try
        {
            uint64_t TimeWait = GetSysTimeMicros();
            
            while (!ShouldCloseShellLoop)
            {
                if (IBG_GetSetting().FrameRateLimit != -1)
                {
                    int Uax = 1000000 / IBG_GetSetting().FrameRateLimit;
                    while (GetSysTimeMicros() < TimeWait)Sleep(Uax / 1000);
                    TimeWait += Uax;
                }
                ShellLoopLastTime = GetSysTimeMicros();
                // Poll and handle events (inputs, window resize, etc.)
                // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
                // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
                // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
                // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
                glfwPollEvents();

                // Start the Dear ImGui frame
                ImGui_ImplOpenGL2_NewFrame();
                ImGui_ImplGlfw_NewFrame();

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

                ImGui::NewFrame();

                ControlPanel();

                {
                    static bool __First = true;
                    if (__First)
                    {
                        __First = false;
                        IBR_Inst_Debug.RenderUIOnce();
                    }
                }

                // Rendering
                ImGui::Render();
                int display_w, display_h;
                glfwGetFramebufferSize(window, &display_w, &display_h);
                glViewport(0, 0, display_w, display_h);
                auto& cc = IBR_Color::BackgroundColor.Value;
                glClearColor(cc.x * cc.w, cc.y * cc.w, cc.z * cc.w, cc.w);
                glClear(GL_COLOR_BUFFER_BIT);

                // If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
                // you may need to backup/reset/restore other state, e.g. for current shader using the commented lines below.
                //GLint last_program;
                //glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
                //glUseProgram(0);
                ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
                //glUseProgram(last_program);

                glfwMakeContextCurrent(window);
                glfwSwapBuffers(window);

                if (glfwWindowShouldClose(window))
                {
                    if (IBR_ProjectManager::IsOpen())
                    {
                        GotoCloseShellLoop = true;
                        IBR_ProjectManager::CloseAction();
                    }
                    else ShouldCloseShellLoop = true;
                }
            }
        }
        //*
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
        //*/
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(locc("Log_MainLoopFinish"));
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
        char szExePath[MAX_PATH];
        if (GetModuleFileNameA(NULL, szExePath, MAX_PATH) == 0)
            return false;
        if (!PathRemoveFileSpecA(szExePath))
            return false;
        if (!SetCurrentDirectoryA(szExePath))
            return false;
        return true;
    }

    bool InitializePath()
    {
        if (!FixLaunchPath())
        {
            MessageBoxW(NULL, locwc("Error_CannotInitializePath"), locwc("Error_FalledToLaunch"), MB_OK);
            return false;
        }

        char DesktopTmp[300];
        SHGetSpecialFolderPathA(0, DesktopTmp, CSIDL_DESKTOPDIRECTORY, 0);
        GetCurrentDirectoryW(MAX_PATH, CurrentDirW);
        GetCurrentDirectoryA(MAX_PATH, CurrentDirA);
        IBR_RecentManager::Path = CurrentDirW;
        IBR_RecentManager::Path += L"\\Resources\\recent.dat";
        Desktop = DesktopTmp;
        Defaultpath = MBCStoUnicode(Desktop);

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
                IBRF_CoreBump.SendToR({ [Path=std::wstring(szArglist[i])]() {IBR_ProjectManager::OpenRecentAction(Path); }});
            }
            else if(!_wcsicmp(szArglist[i], L"-log"))
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
}

