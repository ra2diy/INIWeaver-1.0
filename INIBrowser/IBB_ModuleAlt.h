#pragma once
#include "IBG_Ini.h"

class ClipWriteStream
{
public:
    std::vector<BYTE> Buffer;

    template<typename T>
    size_t Push(const T& Data, size_t ExtBytes)//返回写入的头偏移量
    {
        auto pData = (const BYTE*)&Data;
        auto sz = Buffer.size();
        Buffer.resize(sz + sizeof(T) + ExtBytes);
        memcpy((void*)(Buffer.data() + sz), pData, sizeof(T) + ExtBytes);
        return sz;
    }

    size_t PushBytes(const BYTE* Data, size_t Count)//返回写入的头偏移量
    {
        auto sz = Buffer.size();
        Buffer.resize(sz + Count);
        memcpy((void*)(Buffer.data() + sz), Data, Count);
        return sz;
    }

    void Align(size_t N = 4)
    {
        while (Buffer.size() % N)Buffer.push_back(0);
    }

    size_t PushZero(size_t Count)
    {
        auto sz = Buffer.size();
        Buffer.resize(sz + Count);
        return sz;
    }

    void Clear()
    {
        Buffer.clear();
    }

    size_t Size() const { return Buffer.size(); }
    std::string Get() const { return DataToBase64(Buffer); }
};

class ClipReadStream
{
private:
    std::vector<BYTE> Buffer;
    LPBYTE Begin;
    LPBYTE Cursor;
    int ClipFormatVersion;
public:
    void Init(LPBYTE begin)
    {
        Cursor = Begin = begin;
    }

    void Rewind()
    {
        Cursor = Begin;
    }

    LPBYTE GetByte(size_t MoveAfterRead)
    {
        LPBYTE Cur2 = Cursor;
        Cursor += MoveAfterRead;
        return Cur2;
    }

    void Align(size_t N = 4)
    {
        while ((Cursor - Begin) % N)++Cursor;
    }

    template<typename T>
    T& Get(size_t MoveAfterRead = sizeof(T))
    {
        return *((T*)GetByte(MoveAfterRead));
    }

    void Set(const std::string_view Str) 
    {
        Buffer = Base64ToData(Str);
        Init(Buffer.data());
    }

    void Set(const std::vector<BYTE>& Vec)
    {
        Buffer = Vec;
        Init(Buffer.data());
    }

    int GetVersion() const { return ClipFormatVersion; }
    void SetVersion(int v) { ClipFormatVersion = v; }
    bool VersionAtLeast(int v) const { return ClipFormatVersion >= v; }
};

struct PairClipString
{
    std::string A;
    std::string B;
    
};

struct PairClipOnShow
{
    std::string Str;
    bool Show;
};

struct ModuleClipData
{
    bool IsLinkGroup;
    bool IsComment;
    bool Ignore;
    bool FromClipBoard;
    bool CollapsedInComposed;
    bool Frozen;
    bool Hidden;
    ImVec2 EqSize;
    ImVec2 EqDelta;
    PairClipString Desc;
    std::string Comment;
    std::string Inherit;
    std::string Register;
    std::vector<PairClipString> DefaultLinkKey;
    std::string DisplayName;
    std::vector<IniToken> Lines;//STD Module Line
    std::vector<PairClipString> LinkGroup_LinkTo;
    std::vector<PairClipString> VarList;
    PairClipString IncludedBySection;
    std::vector<PairClipString> IncludingSections;

    void Replace(const std::string& Parameter, const std::string& Argument);
    void Load(const std::string_view Str, int ClipFormatVersion);
    bool NeedtoMangle() const;
    std::string Save() const;
    JsonFile ToJson() const;
};

ClipWriteStream& operator<<(ClipWriteStream& stm, bool v);
ClipWriteStream& operator<<(ClipWriteStream& stm, float v);
ClipWriteStream& operator<<(ClipWriteStream& stm, size_t v);
//ClipWriteStream& operator<<(ClipWriteStream& stm, uint32_t v);
ClipWriteStream& operator<<(ClipWriteStream& stm, const std::string& v);
ClipWriteStream& operator<<(ClipWriteStream& stm, const IniToken& v);
ClipWriteStream& operator<<(ClipWriteStream& stm, const ImVec2& v);
ClipWriteStream& operator<<(ClipWriteStream& stm, const PairClipString& v);
ClipWriteStream& operator<<(ClipWriteStream& stm, const PairClipOnShow& v);
template<typename T>
ClipWriteStream& operator<<(ClipWriteStream& stm, const std::vector<T>& v)
{
    stm << v.size();
    for (auto& w : v)
        stm << w;
    return stm;
}

ClipReadStream& operator>>(ClipReadStream& stm, bool& v);
ClipReadStream& operator>>(ClipReadStream& stm, float& v);
ClipReadStream& operator>>(ClipReadStream& stm, size_t& v);
//ClipReadStream& operator>>(ClipReadStream& stm, uint32_t& v);
ClipReadStream& operator>>(ClipReadStream& stm, std::string& v);
ClipReadStream& operator>>(ClipReadStream& stm, IniToken& v);
ClipReadStream& operator>>(ClipReadStream& stm, ImVec2& v);
ClipReadStream& operator>>(ClipReadStream& stm, PairClipString& v);
ClipReadStream& operator>>(ClipReadStream& stm, PairClipOnShow& v);
template<typename T>
ClipReadStream& operator>>(ClipReadStream& stm, std::vector<T>& v)
{
    size_t x;
    stm >> x;
    v.resize(x);
    for (auto& w : v)
        stm >> w;
    return stm;
}

ClipWriteStream& operator<<(ClipWriteStream& stm, const ModuleClipData& v);
ClipReadStream& operator>>(ClipReadStream& stm, ModuleClipData& v);

struct IBB_Section;
struct IBB_Section_Desc;

void MangleModules(std::vector<ModuleClipData>& Modules);
JsonFile ModulesToJson(const std::vector<ModuleClipData>& Modules);

struct IBB_ClipBoardData
{
    uint32_t ProjectRID;
    std::vector<ModuleClipData> Modules;

    void Mangle();
    void Generate(const std::vector<IBB_Section_Desc>& Modules);
    void GenerateAll(bool UsePosAsDelta, bool FromClipBoard);
    std::string GetString() const;
    std::vector<BYTE> GetStream() const;
    //Fill ErrorContext Before Calling
    bool SetString(const std::string_view Str, int ClipFormatVersion = INT_MAX);
    //Fill ErrorContext Before Calling
    bool SetStream(const std::vector<BYTE>& Vec, int ClipFormatVersion);
    JsonFile ToJson() const;

    struct ErrorCtx
    {
        std::wstring ModulePath;
        std::string _TEXT_UTF8 ModuleName;
    };
    static ErrorCtx ErrorContext;
};

struct IBB_ModuleAlt
{
    bool Available{ false };
    bool FromClipBoard{ false };
    std::string Name;
    std::string DescShort;
    std::string DescLong;
    std::string ParamDescShort;
    std::string ParamDescLong;
    std::string Parameter;
    std::vector<ModuleClipData> Modules;
    std::wstring Path;

    std::string GetFirstINI() const;
    bool Search(const std::string& Str, bool ConsiderRegName, bool ConsiderDescName, bool ConsiderDesc);
    void LoadFromFile(const wchar_t* FileName);
    void LoadFromFile(const char* FileName);
    void LoadFromString(std::wstring_view FileName, std::string&& FileStr);
    bool SaveToFile();
};

namespace IBB_ModuleAltDefault
{
    extern std::vector<std::string> FlattenedModuleName;
    void Load(const wchar_t* FileRange, const wchar_t* FileRange2, const wchar_t* FileRange3);
    std::wstring GenerateModulePath();
    std::wstring GenerateModulePath_NoName();
    IBB_ModuleAlt* GetModule(const std::string& Name);
    std::vector<IBB_ModuleAlt*> Search(const std::string& Str, bool ConsiderRegName, bool ConsiderDescName, bool ConsiderDesc);
	void NewModule(IBB_ModuleAlt&& M);
	void Tree_RenderUI();
	void Tree_ResetHover();

    IBB_ModuleAlt* DefaultArt_Voxel();
    IBB_ModuleAlt* DefaultArt_SHPVehicle();
    IBB_ModuleAlt* DefaultArt_SHPBuilding();
    IBB_ModuleAlt* DefaultArt_SHPInfantry();
    IBB_ModuleAlt* DefaultArt_Animation();
}

