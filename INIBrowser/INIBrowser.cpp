
#include "FromEngine/Include.h"
#include "Initialize.h"


#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif


int WINAPI wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)

{
    using namespace Initialize;

    (void)hPrevInstance;
    (void)nCmdShow;

    /*

    初始化：
    阶段1： （Initialize_Stage_I）
        重定向当前路径
        解析命令行
        初始化语言
        初始化日志
        初始化分辨率
    阶段2： （Initialize_Stage_II）
        初始化GLFW
        创建窗口
        设置窗口图标
    阶段3： （Initialize_Stage_III）
        创建后端线程
        创建保存线程
    阶段4： （Initialize_Stage_IV）
        同时执行：
            后端线程：加载设置
            主线程：  初始化ImGui
                      读取提示信息
                      读取最近打开的文件
                      读取字体载入信息
                      显示窗口
        等待后端线程执行完成
        同时执行：
            后端线程：加载模块类型（RegisterTypes）
                      加载语句类型（TypeAlt）
                      加载模块库
                      初始化调试信息
            主线程：  设置显示主题
                      载入字库
                      初始化工作区
    */

    
    TEMPLOG("if (int Ret_I = Initialize_Stage_I(hInstance, lpCmdLine); Ret_I)")
    if (int Ret_I = Initialize_Stage_I(hInstance, lpCmdLine); Ret_I)
        return Ret_I;

    TEMPLOG("if (int Ret_II = Initialize_Stage_II(); Ret_II)")
    if (int Ret_II = Initialize_Stage_II(); Ret_II)
        return Ret_II;

    TEMPLOG("Initialize_Stage_III()")
    Initialize_Stage_III();

    TEMPLOG("Initialize_Stage_IV")
    Initialize_Stage_IV();

    TEMPLOG("ShellLoop();")
    ShellLoop();

    TEMPLOG("CleanUp");
    CleanUp();

    return 0;
}

