
#include "Global.h"

//所有版本号相关数据
const std::string Version = u8"0.2b3";
const std::wstring VersionW = L"0.2b3";
const int VersionMajor = 0;
const int VersionMinor = 2;
const int VersionRelease = 3;
const int VersionN = VersionMajor * 10000 + VersionMinor * 100 + VersionRelease;
const std::string VersionNStr = u8"000203";

std::string GetVersionStr(int Ver)
{
    if (Ver < 200)
    {
        return loc("Back_OldVer1")+u8" b" + std::to_string(Ver % 100);
    }
    else
    {
        return std::to_string(Ver / 10000) + "." + std::to_string((Ver / 100) % 100) + "b" + std::to_string(Ver % 100);
    }
}

//统一的文件头
const int32_t SaveFileHeaderSign = 0x00114514;
const char* EmptyOnShowDesc = "\r\n\r\n\r\n";

//设置的实例
IBF_Setting IBF_Inst_Setting;
IBR_Setting IBR_Inst_Setting;

//设置的标识
std::atomic<bool> SettingLoadComplete{ false };
std::atomic<bool> SettingSaveComplete{ false };
const wchar_t* SettingFileName = L"./Resources/setting.dat";
const char* DefaultIniName = "Rules";

//日志
LogClass GlobalLog{ "browser.log" };
LogClass GlobalLogB{ "backend.log" };
BufString LogBuf, LogBufB;
bool EnableLog = true;
bool EnableLogEx = false;

//线程信息交换
IBRF_Bump IBRF_CoreBump;
uint64_t ShellLoopLastTime;
DWORD BackThreadID{ INT_MAX };

//设置内容
int KeyPerPage = 10;
int FontHeight = 24;
int WindowSizeAdjustX = 15, WindowSizeAdjustY = 0;
bool IsProjectOpen = false;//TODO:打开项目
HWND MainWindowHandle = 0;
int RScrX, RScrY, ScrX, ScrY;

//格式类型表
IBF_DefaultTypeList IBF_Inst_DefaultTypeList;
IBF_Project IBF_Inst_Project;
IBR_Project IBR_Inst_Project;
IBS_Project IBS_Inst_Project;

//调试
IBR_Debug IBR_Inst_Debug;

//随机种子
std::default_random_engine GlobalRnd;
const int ModuleRandomParameterLength = 16;

//prelink
namespace PreLink
{
    GLFWwindow* window;
    HINSTANCE hInst;
    ImFont* font;
}
