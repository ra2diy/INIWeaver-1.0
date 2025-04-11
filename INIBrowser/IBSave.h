#pragma once
#include "FromEngine/Include.h"
#include "FromEngine/RFBump.h"


namespace InsertLoad
{
    struct SelectFileType
    {
        std::wstring InitialPath;
        std::wstring Title;
        std::wstring InitialFileName;
        const wchar_t* Filter;
    };
    struct SelectFileRet
    {
        std::wstring RetBuf;
        int PathOffset;
        bool Success;
    };

    SelectFileRet SelectFileName(HWND Root, const SelectFileType& Type, BOOL(_stdcall* Proc)(LPOPENFILENAMEW), bool UseFolder);

    inline SelectFileRet SelectOpenFileName(HWND Root, const SelectFileType& Type)
    {
        return SelectFileName(Root, Type, ::GetOpenFileNameW, false);
    }

    inline SelectFileRet SelectSaveFileName(HWND Root, const SelectFileType& Type)
    {
        return SelectFileName(Root, Type, ::GetSaveFileNameW, false);
    }

    inline SelectFileRet SelectFolderName(HWND Root, const SelectFileType& Type)
    {
        return SelectFileName(Root, Type, ::GetOpenFileNameW, true);
    }

}

inline std::string MinorDir(const std::string& ss)
{
    return std::string(ss.begin() + std::min(ss.find_last_of('\\') + 1, ss.length() - 1), ss.end());
}


void IBS_Push(const StdMessage& Msg);

void IBS_Complete();

void IBS_Thr_SaveLoop();


struct IBS_Project
{
    std::wstring ProjName;
    std::wstring Path;
    std::wstring LastOutputDir;
    std::vector<std::pair<std::string, std::wstring>> LastOutputIniName;

    uint64_t CreateTime;
    int CreateVersionMajor, CreateVersionMinor, CreateVersionRelease;
    float FullView_Ratio;
    ImVec2 FullView_EqCenter;
    std::vector<BYTE> Data;

    bool Save();
    bool Load();
    bool Save(const std::wstring& _Path);
    bool Load(const std::wstring& _Path);
};


