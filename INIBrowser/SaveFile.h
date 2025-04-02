#pragma once

#include<string>
#include<vector>
#include<unordered_map>
#include<functional>
#include"FromEngine/external_file.h"

struct ReadFileHeader
{
    std::string UsingID;
    std::function<bool(const ExtFileClass&,int Version,int Length)> Process;
};

struct WriteFileHeader
{
    std::string UsingID;
    std::function<bool(const ExtFileClass&)> Process;
};

class SaveFile
{
    ExtFileClass File;
public:
    std::unordered_map<std::string, ReadFileHeader> ReadBlockProcess;
    std::vector<WriteFileHeader> WriteBlockProcess;
    bool Read(const wchar_t* Name);
    bool Write(const wchar_t* Name);
};

