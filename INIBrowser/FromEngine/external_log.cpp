#pragma once
#include "external_log.h"
#include "global_tool_func.h"


#define __Open(_Mode) lk.lock(); if(!LogFile.Open(path.c_str(), #_Mode))return false;
#define __Exit LogFile.Close();lk.unlock();return true

bool LogClass::AddLog(const char* str, bool ln)
{
	__Open(a);
	LogFile.PutStr(str);
	if(ln)LogFile.PutChr('\n');
	__Exit;
}

bool LogClass::AddLogC(const char c, bool ln)
{
	static char Ln = '\n';
	__Open(ab);
	LogFile.Write(&c, 1, 1);
    if (ln)LogFile.Write(&Ln, 1, 1);
	__Exit;
}

bool LogClass::AddLog(const int num, bool ln)
{
	__Open(a);
	static char LogBuffer[3000];
	sprintf(LogBuffer, "%d", num);
	LogFile.PutStr(LogBuffer);
	if (ln)LogFile.PutChr('\n');
	__Exit;
}

bool LogClass::AddLog(const void* ptr, bool ln)
{
	__Open(a);
	static char LogBuffer[3000];
	sprintf(LogBuffer, "%p", ptr);
	LogFile.PutStr(LogBuffer);
	if (ln)LogFile.PutChr('\n');
	__Exit;
}

bool LogClass::AddLog_Hex(const int num, bool ln)
{
	__Open(a);
	static char LogBuffer[3000];
	sprintf(LogBuffer, "%.2X", num);
	LogFile.PutStr(LogBuffer);
	if (ln)LogFile.PutChr('\n');
	__Exit;
}

bool LogClass::AddLog_FixedHex(const int num, bool ln)
{
	__Open(a);
	static char LogBuffer[3000];
	sprintf(LogBuffer, "%.8X", num);
	LogFile.PutStr(LogBuffer);
	if (ln)LogFile.PutChr('\n');
	__Exit;
}

bool LogClass::ClearLog()
{
	__Open(w);
	__Exit;
}

bool LogClass::AddLog_CurTime(bool ln)
{
	__Open(a);
	LogFile.PutStr(("["  + TimeNow() + "]").c_str());
	if (ln)LogFile.PutChr('\n');
	__Exit;
}




int Ustrlen(const char* StrDest)
{
	int i = 0;
	while ((*StrDest) != '\0' || (*(StrDest + 1)) != '\0')
	{
		StrDest++;
		i++;
	}
	return i + 3;
}

int Ustrlen(const wchar_t* StrDest)
{
	return Ustrlen((char*)StrDest);
}
