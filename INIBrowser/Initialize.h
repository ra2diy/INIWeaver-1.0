#pragma once

namespace Initialize
{

    void glfw_error_callback(int error, const char* description);

    bool InitializeLanguage();

    int Initialize_Stage_II();

    void InitConfigJson();

    void Initialize_Stage_III();

    void InitializeHotKeys();

    void InitializeStyle();

    void InitializeFontFile();

    void CallInitializeDatabase();

    void Initialize_Stage_IV();

    void CleanUp();

    void ShellLoop();

    void InitializeLogger();

    bool FixLaunchPath();

    bool InitializePath();

    void InitializeResolution();

    void ProcessCommandLine(LPWSTR lpCmdLine);

    int Initialize_Stage_I(HINSTANCE hInstance, LPWSTR lpCmdLine);

}

