#pragma once

#define IDI_ICON1 101
#define DllLoadFunc(Fn) (Fn=(decltype(Fn))GetProcAddress(TElib,#Fn))

#include "MainStage.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include <Shlwapi.h>
bool ShouldCloseShellLoop = false;
bool GotoCloseShellLoop = false;

namespace PreLink
{
    static void glfw_error_callback(int error, const char* description)
    {
        static BufString gle;
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            sprintf(gle, "GLFW错误：错误码：%d 错误信息：%s", error, description);
            GlobalLog.AddLog(gle);
        }
    }

    int PreLoop1()
    {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return 1;
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        window = glfwCreateWindow(ScrX, ScrY, u8"INI浏览器", NULL, NULL);
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
            GlobalLog.AddLog("成功打开窗口。");
        }
        return 0;
    }

    void InitConfigJson()
    {
        JsonFile Cfg;
        IBR_PopupManager::AddJsonParseErrorPopup(Cfg.ParseFromFileChecked(".\\Resources\\config.json", u8"【出错位置】", nullptr)
            , u8"尝试解析 config.json 时发生语法错误。");
        if (!Cfg.Available())
        {
            //MessageBoxA(NULL, "config.json 读取失败！", AppNameA, MB_OK);
            FontPath += "msyh.ttf";
        }
        else
        {
            auto Obj = Cfg.GetObj();
            FontPath += Obj.ItemStringOr("FontName", "msyh.ttf");

            auto S = Obj.GetObjectItem("StyleLight");
            if (S.Available())IBR_Color::LoadLight(S);

            S = Obj.GetObjectItem("StyleDark");
            if (S.Available())IBR_Color::LoadDark(S);
        }
    }

    void PreLoop2()
    {
        IBR_Inst_Setting.SetSettingName(SettingFileName);
        IBR_Inst_Setting.CallReadSetting();

        std::string EncodingStr;//MBCS Unicode UTF8

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
            GlobalLog.AddLog(("成功载入最近打开列表：" + FontPath).c_str());
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
                0x0020, 0x00FF, // Basic Latin + Latin Supplement
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
                GlobalLog.AddLog("成功载入.\\Resources\\load.txt");
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
                GlobalLog.AddLog("未能载入.\\Resources\\load.txt 使用默认载入策略");
            }

            LoadMyRange = false;
            font = io.Fonts->AddFontFromFileTTF(FontPath.c_str(), (float)FontHeight, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            //font = io.Fonts->AddFontFromFileTTF(FontPath.c_str(), (float)FontHeight, NULL, io.Fonts->GetGlyphRangesChineseFull());
        }
        

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL2_Init();

        


        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog("注册OnExit完成。");
        }

        glfwShowWindow(PreLink::window);
        SetClassLong(MainWindowHandle, GCL_HICON, (LONG)LoadIconW(PreLink::hInst, MAKEINTRESOURCE(IDI_ICON1)));
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog("显示渲染窗口。");
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
            MessageBoxA(nullptr, "font == NULL", "字体载入失败！", MB_ICONERROR);
        }
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(("成功载入字体：" + FontPath).c_str());
        }

        IBB_DefaultRegType::LoadFromFile(".\\Global\\RegisterTypes.json");//FIRST !
        IBF_Inst_DefaultTypeList.ReadAltSetting(".\\Global\\TypeAlt");//SECOND ! DEPENDENT
        IBB_ModuleAltDefault::Load(L".\\Global\\Modules\\*.*", L".\\Global\\ImageModules\\*.*");//THIRD ! DEPENDENT
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
                        GlobalLog.AddLog("遇到这种情况请重启几次，感谢你的支持。");
                    }
                    MessageBoxA(nullptr, "font->ContainerAtlas == NULL", "字体载入失败！", MB_ICONERROR);
                    MessageBoxA(nullptr, "遇到这种情况请重启几次", "程序启动失败！", MB_ICONERROR);
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
            MessageBoxA(nullptr, e.what(), "抛出异常！", MB_ICONERROR);
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog("主循环异常，异常信息：");
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(e.what());
            }
            throw(e);
        }
        //*/
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog("主循环停止工作。");
        }
    }

    void InitializeLogger()
    {
        GlobalLog.SetPath(CurrentDirA + std::string("\\browser.log"));
        GlobalLogB.SetPath(CurrentDirA + std::string("\\backend.log"));
        GlobalLog.ClearLog();
        GlobalLogB.ClearLog();

        GlobalLog.AddLog_CurTime(false);
        GlobalLog.AddLog(("INI浏览器 V" + Version).c_str());
        GlobalLog.AddLog_CurTime(false);
        GlobalLog.AddLog("INI浏览器已开始运行。");
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
            MessageBoxA(NULL, "无法确定启动路径。", "错误", MB_OK);
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

