#pragma once
#include <string>

void IBB_FileCheck(const char* FileName, bool AllowNotExist, bool PopupOnError, bool Critical);
void IBB_FileCheck(const wchar_t* FileName, bool AllowNotExist, bool PopupOnError, bool Critical);
void IBB_FileCheck(const std::string& FileName, bool AllowNotExist, bool PopupOnError, bool Critical);
void IBB_FileCheck(const std::wstring& FileName, bool AllowNotExist, bool PopupOnError, bool Critical);
