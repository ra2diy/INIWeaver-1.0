
#include "Global.h"

//���а汾���������
const std::string Version = u8"1.0.4";
const std::wstring VersionW = L"1.0.4";
const int VersionMajor = 1;
const int VersionMinor = 0;
const int VersionRelease = 4;
const int VersionN = VersionMajor * 10000 + VersionMinor * 100 + VersionRelease;
const std::string VersionNStr = u8"010004";

std::string GetVersionStr(int Ver)
{
    if (Ver < 200)
    {
        if (Ver % 100)return loc("Back_OldVer1") + u8" b" + std::to_string(Ver % 100);
        else return loc("Back_OldVer1");
    }
    else if(Ver < 10000)
    {
        if (Ver % 100)return std::to_string(Ver / 10000) + "." + std::to_string((Ver / 100) % 100) + "b" + std::to_string(Ver % 100);
        else return std::to_string(Ver / 10000) + "." + std::to_string((Ver / 100) % 100);
    }
    else
    {
        if (Ver % 100)return std::to_string(Ver / 10000) + "." + std::to_string((Ver / 100) % 100) + "." + std::to_string(Ver % 100);
        else return std::to_string(Ver / 10000) + "." + std::to_string((Ver / 100) % 100);
    }
}

//ͳһ���ļ�ͷ
const int32_t SaveFileHeaderSign = 0x00114514;
const char* EmptyOnShowDesc = "\r\n\r\n\r\n";

//���õ�ʵ��
IBF_Setting IBF_Inst_Setting;
IBR_Setting IBR_Inst_Setting;

//���õı�ʶ
std::atomic<bool> SettingLoadComplete{ false };
std::atomic<bool> SettingSaveComplete{ false };
const wchar_t* SettingFileName = L"./Resources/setting.dat";
const char* DefaultIniName = "Rules";

//��־
LogClass GlobalLog{ "browser.log" };
LogClass GlobalLogB{ "backend.log" };
BufString LogBuf, LogBufB;
bool EnableLog = true;
bool EnableLogEx = false;

//�߳���Ϣ����
IBRF_Bump IBRF_CoreBump;
uint64_t ShellLoopLastTime;
DWORD BackThreadID{ INT_MAX };

//��������
int KeyPerPage = 10;
int FontHeight = 24;
int WindowSizeAdjustX = 15, WindowSizeAdjustY = 0;
bool IsProjectOpen = false;//TODO:����Ŀ
HWND MainWindowHandle = 0;
int RScrX, RScrY, ScrX, ScrY;

//��ʽ���ͱ�
IBF_DefaultTypeList IBF_Inst_DefaultTypeList;
IBF_Project IBF_Inst_Project;
IBR_Project IBR_Inst_Project;
IBS_Project IBS_Inst_Project;

//����
IBR_Debug IBR_Inst_Debug;

//�������
std::default_random_engine GlobalRnd;
const int ModuleRandomParameterLength = 16;

//prelink
namespace PreLink
{
    GLFWwindow* window;
    HINSTANCE hInst;
    ImFont* font;
}
