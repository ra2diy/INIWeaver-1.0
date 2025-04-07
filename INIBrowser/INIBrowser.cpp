
#include "FromEngine/Include.h"
#include "PreLink.h"


#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif


int WINAPI wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)

{
    using namespace PreLink;

    (void)hPrevInstance;
    (void)nCmdShow;

    hInst = hInstance;
    if (!InitializePath())return 2;
    ProcessCommandLine(lpCmdLine);
    if (!InitializeLanguage())return 3;
    InitializeLogger();
    InitializeResolution();

    if (int PreLoopRet = PreLoop1(); PreLoopRet)
        return PreLoopRet;

    std::thread FrontThr(IBF_Thr_FrontLoop);
    FrontThr.detach();
    std::thread SaveThr(IBS_Thr_SaveLoop);
    SaveThr.detach();

    PreLoop2();

    IBR_FullView::ViewSize = { FontHeight * 12.0f, FontHeight * 12.0f };

    ShellLoop();

    CleanUp();

    return 0;
}

