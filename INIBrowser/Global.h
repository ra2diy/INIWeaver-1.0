#pragma once

#include "IBFront.h"
#include "IBRender.h"
#include "FromEngine/RFBump.h"
#include <atomic>

#define IBD_ShutDownDestructor(x) x##_ShutDownDestructor
#define IBD_Inst_ShutDownDestructor(x) x##_Inst_ShutDownDestructor
#define IBD_ShutDownDtorClass(x,fn) \
struct IBD_ShutDownDestructor(x)\
{\
    IBD_ShutDownDestructor(x)() {}\
    ~IBD_ShutDownDestructor(x)() { fn(); }\
}IBD_Inst_ShutDownDestructor(x)

#define IBD_BoolStr(v) ((v)?"true":"false")

//所有版本号相关数据
extern const std::string Version;
extern const std::wstring VersionW;
extern const int VersionMajor;
extern const int VersionMinor;
extern const int VersionRelease;
extern const int VersionN;
extern const std::string VersionNStr;
extern const std::string ClipDataFormatVersion;
std::string GetVersionStr(int Version);
int GetClipFormatVersion(const std::string& Magic);
int GetClipFormatVersion(int AppVersion);

//统一的文件头
extern const int32_t SaveFileHeaderSign;
extern const char* EmptyOnShowDesc;

//设置的实例
extern IBF_Setting IBF_Inst_Setting;
extern IBR_Setting IBR_Inst_Setting;

//设置的标识
extern std::atomic<bool> SettingLoadComplete;
extern std::atomic<bool> SettingSaveComplete;
extern const wchar_t* SettingFileName;
extern const char* DefaultIniName;
extern const char* InheritKeyName;

//日志
extern LogClass GlobalLog;
extern LogClass GlobalLogB;
extern BufString LogBuf, LogBufB;
extern bool EnableLog;//LOG
extern bool EnableLogEx;//EXTRA LOG

//线程信息交换
extern IBRF_Bump IBRF_CoreBump;
#define IBD_RInterruptF(x) IBG_RInterruptF_RangeLock __IBD_RInterruptF_VariableA_##x{ IBRF_CoreBump };
#define IBD_FInterruptR(x) IBG_FInterruptR_RangeLock __IBD_FInterruptR_VariableA_##x{ IBRF_CoreBump };
extern uint64_t ShellLoopLastTime;
extern DWORD BackThreadID;

//设置内容
extern int KeyPerPage;
extern int FontHeight;
extern int WindowSizeAdjustX, WindowSizeAdjustY;
extern bool IsProjectOpen;
extern HWND MainWindowHandle;
extern int RScrX, RScrY, ScrX, ScrY;

//格式类型表
extern IBF_DefaultTypeList IBF_Inst_DefaultTypeList;
extern IBF_Project IBF_Inst_Project;
extern IBR_Project IBR_Inst_Project;
extern IBS_Project IBS_Inst_Project;

//调试
extern IBR_Debug IBR_Inst_Debug;

//随机种子
extern std::default_random_engine GlobalRnd;
extern const int ModuleRandomParameterLength;

//prelink
namespace PreLink
{
    extern GLFWwindow* window;
    extern HINSTANCE hInst;
    extern ImFont* font;
}

#define ExtensionNameW L".iproj"
#define ExtensionNameA ".iproj"
#define ExtensionNameC "IPROJ"

extern typename ImGuiRadioButtonFlags GlobalNodeStyle;
