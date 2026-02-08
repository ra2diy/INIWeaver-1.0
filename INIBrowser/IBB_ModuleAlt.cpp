#include "IBB_ModuleAlt.h"
#include <wincrypt.h>
#include "IBFront.h"
#include "Global.h"
#include "Shlwapi.h"
#include <imgui_internal.h>
#include <minwindef.h>
#include <fileapi.h>
#include <handleapi.h>
#include <minwinbase.h>
#include <Windows.h>
#include <winscard.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <format>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <__msvc_string_view.hpp>
#include <imgui.h>
#include "FromEngine/external_file.h"
#include "FromEngine/global_tool_func.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/types.h"
#include "IBB_Components.h"
#include "IBG_Ini.h"
#include "IBRender.h"
#include "IBR_Localization.h"

#pragma comment(lib, "crypt32.lib")

// 将二进制数据转换为 Base64 字符串
std::string DataToBase64(const std::vector<BYTE>& Data) {
    DWORD dwLength = 0;

    // 计算 Base64 字符串的长度
    if (!CryptBinaryToStringA(Data.data(), Data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &dwLength)) {
        throw std::runtime_error("Failed to calculate Base64 length.");
    }

    // 分配足够的内存来存储 Base64 字符串
    std::string base64Str(dwLength, '\0');

    // 将二进制数据转换为 Base64 字符串
    if (!CryptBinaryToStringA(Data.data(), Data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &base64Str[0], &dwLength)) {
        throw std::runtime_error("Failed to convert data to Base64.");
    }

    // 去掉末尾的空字符
    base64Str.pop_back();

    return base64Str;
}

// 将 Base64 字符串转换为二进制数据
std::vector<BYTE> Base64ToData(const std::string_view Str) {
    DWORD dwLength = 0;

    // 计算二进制数据的长度
    if (!CryptStringToBinaryA(Str.data(), Str.size(), CRYPT_STRING_BASE64, nullptr, &dwLength, nullptr, nullptr)) {
        throw std::runtime_error("Failed to calculate binary data length.");
    }

    // 分配足够的内存来存储二进制数据
    std::vector<BYTE> data(dwLength);

    // 将 Base64 字符串转换为二进制数据
    if (!CryptStringToBinaryA(Str.data(), Str.size(), CRYPT_STRING_BASE64, data.data(), &dwLength, nullptr, nullptr)) {
        throw std::runtime_error("Failed to convert Base64 to data.");
    }

    // 调整大小以匹配实际数据长度
    data.resize(dwLength);

    return data;
}

void DrawFolderIcon(ImVec2 Pos, float Size)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 文件夹主体（黄色部分）
    ImVec2 folder_body_top_left(Pos.x, Pos.y + Size * 0.05f);
    ImVec2 folder_body_bottom_right(Pos.x + Size, Pos.y + Size * 0.8f);
    draw_list->AddRectFilled(folder_body_top_left, folder_body_bottom_right, IM_COL32(255, 215, 0, 255), Size * 0.05f);

    // 文件夹顶部曲线
    ImVec2 c1(Pos.x, Pos.y + Size * 0.15f);
    ImVec2 c2(Pos.x + Size * 0.3f, Pos.y + Size * 0.15f);
    ImVec2 c3(Pos.x + Size * 0.5f, Pos.y);
    ImVec2 c4(Pos.x + Size, Pos.y);
    draw_list->AddLine(c1, c2, IM_COL32(227, 161, 50, 255), Size * 0.1f);
    draw_list->AddLine(c2, c3, IM_COL32(227, 161, 50, 255), Size * 0.1f);
    draw_list->AddLine(c3, c4, IM_COL32(227, 161, 50, 255), Size * 0.1f);
}
void DrawOpenFolderIcon(ImVec2 Pos, float Size)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 文件夹主体（黄色部分）
    ImVec2 folder_body_top_left(Pos.x, Pos.y + Size * 0.05f);
    ImVec2 folder_body_bottom_right(Pos.x + Size, Pos.y + Size * 0.8f);
    draw_list->AddRectFilled(folder_body_top_left, folder_body_bottom_right, IM_COL32(255, 215, 0, 255), Size * 0.05f);

    // 文件夹顶部曲线
    ImVec2 c0(Pos.x, Pos.y + Size * 0.65f);
    ImVec2 c1(Pos.x + Size * 0.05f, Pos.y + Size * 0.15f);
    ImVec2 c2(Pos.x + Size * 0.35f, Pos.y + Size * 0.15f);
    ImVec2 c3(Pos.x + Size * 0.55f, Pos.y);
    ImVec2 c4(Pos.x + Size * 1.05f, Pos.y);
    ImVec2 c5(Pos.x + Size, Pos.y + Size * 0.65f);
    draw_list->AddLine(c0, c1, IM_COL32(227, 161, 50, 255), Size * 0.1f);
    draw_list->AddLine(c1, c2, IM_COL32(227, 161, 50, 255), Size * 0.1f);
    draw_list->AddLine(c2, c3, IM_COL32(227, 161, 50, 255), Size * 0.1f);
    draw_list->AddLine(c3, c4, IM_COL32(227, 161, 50, 255), Size * 0.1f);
    draw_list->AddLine(c4, c5, IM_COL32(227, 161, 50, 255), Size * 0.1f);
}


std::string_view TrimView(std::string_view Line)
{
    while (!Line.empty())
    {
        if (isspace(Line.front()))Line = Line.substr(1, Line.size() - 1);
        else break;
    }
    while (!Line.empty())
    {
        if (isspace(Line.back()))
        {
            if (Line.size() >= 2 && Line[Line.size() - 2] < 0)break;
            else Line = Line.substr(0, Line.size() - 1);
        }
        else break;
    }
    return Line;
}

void IniToken::Tokenize(std::string_view Line, bool UseDesc)
{
    Line = TrimView(Line);
    HasDesc = Empty = IsSection = false;
    if (Line.empty() || Line.front() == ';')
    {
        Empty = true;
        return;
    }
    auto p = Line.find_first_of(";");
    if (p != std::string_view::npos)
    {
        Line = Line.substr(0, p);
    }
    if (UseDesc)
    {
        p = Line.find_first_of("#");
        if (p != std::string_view::npos)
        {
            HasDesc = true;
            Desc = Line.substr(0, p);
            Line = Line.substr(p + 1, Line.size() - p);
        }
    }
    Line = TrimView(Line);
    if (Line.front() == '[')
    {
        p = Line.find_first_of("]");
        if (p != std::string_view::npos)
        {
            auto Sub = Line.substr(1, p - 1);
            Empty = false;
            IsSection = true;
            Key = TrimView(Sub);
            auto Suc = TrimView(Line.substr(p + 1, Line.size() - p - 1));
            if (!Suc.empty())
            {
                if (Suc.front() == ':')
                {
                    Suc = TrimView(Suc.substr(1, Suc.size() - 1));
                    if (!Suc.empty())
                    {
                        if (Suc.front() == '[')
                        {
                            auto q = Suc.find_first_of("]");
                            if (q != std::string_view::npos)
                            {
                                Suc = Suc.substr(1, q - 1);
                                Value = TrimView(Suc);
                            }
                        }

                    }
                }
            }
            return;
        }
    }
    if (Line.front() != '=')
    {
        p = Line.find_first_of("=");
        if (p != std::string_view::npos/* && Line.size() > p + 1*/)//accept empty value
        {
            auto Sub = TrimView(Line.substr(0, p));
            auto Val = TrimView(Line.substr(p + 1, Line.size() - p - 1));
            if (!Sub.empty() /* && !Val.empty()*/)//accept empty value
            {
                Empty = false;
                IsSection = false;
                Key = Sub;
                Value = Val;
                return;
            }
        }
    }
    Empty = true;
    return;
}

std::vector<std::string_view> GetLines(char* Text, bool SkipEmptyLine)
{
    std::vector<std::string_view> Res;
    size_t S = strlen(Text);
    for (size_t i = 0; i < S; ++i)
    {
        if (Text[i] == '\r' || Text[i] == '\n')Text[i] = 0;
    }
    for (size_t i = 0; i < S;)
    {
        auto Dt = Text + i;
        i++;
        if (!*Dt && SkipEmptyLine)continue;
        Res.push_back(Dt);
        i += strlen(Dt);
    }
    return Res;
}
std::vector<std::string_view> GetLines(std::string&& Text, bool SkipEmptyLine)
{
    return GetLines(Text.data(), SkipEmptyLine);
}
std::vector<std::string_view> GetLines(BytePointerArray Text, size_t ExtBytes, bool SkipEmptyLine)
{
    std::vector<std::string_view> Res;
    auto Data = (char*)Text.Data;
    memset(Data + Text.N - ExtBytes, 0, ExtBytes);
    for (size_t i = 0; i < Text.N; ++i)
    {
        if (Data[i] == '\r' || Data[i] == '\n')Data[i] = 0;
    }
    for (size_t i = 0; i < Text.N;)
    {
        auto Dt = Data + i;
        i++;
        if (!*Dt && SkipEmptyLine)continue;
        Res.push_back(Dt);
        i += strlen(Dt);
    }
    return Res;
}
std::vector<IniToken> GetTokens(const std::vector<std::string_view>& Lines, bool UseDesc)
{
    std::vector<IniToken> Result;
    Result.reserve(Lines.size());
    for (auto view : Lines)
    {
        Result.emplace_back(view, UseDesc);
    }
    return Result;
}

std::vector<IniToken> GetTokensFromFile(ExtFileClass& Loader)
{
    BytePointerArray Arr = Loader.ReadWholeFile(16);
    auto Ret = GetTokens(GetLines(Arr, 16));
    Arr.Delete();
    return Ret;
}

std::vector<std::vector<IniToken>> SplitTokens(std::vector<IniToken>&& Tok)
{
    std::vector<std::vector<IniToken>> Result;
    for (auto& t : Tok)
    {
        if (t.Empty)continue;
        if (t.IsSection)
        {
            Result.emplace_back();
            Result.back().push_back(std::move(t));
        }
        else if (!Result.empty())
        {
            Result.back().push_back(std::move(t));
        }
    }
    return Result;
}

//Include the first one ;Ignore Type;No Empty Tokens
std::unordered_map<std::string, std::string> GetVarList(const std::vector<IniToken>& Tok)
{
    std::unordered_map<std::string, std::string> s;
    for (auto& t : Tok)if(!t.Empty)s[t.Key] = t.Value;
    return s;
}

// 文件信息结构体
struct FileInfo {
    std::wstring Name;    // 文件名
    std::wstring FullPath; // 完整路径
};

// 文件遍历器类
class FindFileRange {
public:
    // 构造函数，接受通配符（如 L"*.txt"）
    FindFileRange(const std::wstring& pattern) : pattern(pattern) {
        // 开始查找文件
        hFind = FindFirstFileW(pattern.c_str(), &findData);
    }
    FindFileRange(const _TEXT_UTF8 std::string& Pattern) : pattern(UTF8toUnicode(Pattern)) {
        // 开始查找文件
        hFind = FindFirstFileW(pattern.c_str(), &findData);
    }

    // 析构函数，释放资源
    ~FindFileRange() {
        if (hFind != INVALID_HANDLE_VALUE) {
            FindClose(hFind);
        }
    }

    // 迭代器类
    class Iterator {
    public:
        Iterator(HANDLE hFind, WIN32_FIND_DATAW* findData, const std::wstring& pattern)
            : hFind(hFind), findData(findData), pattern(pattern) {
            if (hFind != INVALID_HANDLE_VALUE) {
                // 初始化当前文件信息
                currentFile.Name = findData->cFileName;
                currentFile.FullPath = pattern.substr(0, pattern.find_last_of(L'\\') + 1) + findData->cFileName;
            }
        }

        // 解引用操作符
        FileInfo& operator*() {
            return currentFile;
        }

        // 前置递增操作符
        Iterator& operator++() {
            if (FindNextFileW(hFind, findData) != 0) {
                // 更新当前文件信息
                currentFile.Name = findData->cFileName;
                currentFile.FullPath = pattern.substr(0, pattern.find_last_of(L'\\') + 1) + findData->cFileName;
            }
            else {
                // 没有更多文件，标记为结束
                hFind = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        // 不等操作符
        bool operator!=(const Iterator& other) const {
            return hFind != other.hFind;
        }

    private:
        HANDLE hFind;                // 查找句柄
        WIN32_FIND_DATAW* findData;  // 文件数据
        std::wstring pattern;        // 通配符
        FileInfo currentFile;        // 当前文件信息
    };

    // 返回起始迭代器
    Iterator begin() {
        return Iterator(hFind, &findData, pattern);
    }

    // 返回结束迭代器
    Iterator end() {
        return Iterator(INVALID_HANDLE_VALUE, nullptr, pattern);
    }

    bool empty()
    {
        return hFind == INVALID_HANDLE_VALUE;
    }

private:
    std::wstring pattern;    // 通配符
    HANDLE hFind;            // 查找句柄
    WIN32_FIND_DATAW findData; // 文件数据
};

std::vector<std::wstring> FindFileVec(const std::wstring& pattern)
{
    std::vector<std::wstring> result;
    FindFileRange finder(pattern);
    for (const auto& file : finder) {
        result.push_back(file.FullPath);
    }
    return result;
}

ClipWriteStream& operator<<(ClipWriteStream& stm, bool v)
{
    stm.Align(1);
    stm.PushBytes((LPCBYTE)&v, sizeof(v));
    return stm;
}
ClipWriteStream& operator<<(ClipWriteStream& stm, float v)
{
    stm.Align();
    stm.PushBytes((LPCBYTE)&v, sizeof(v));
    return stm;
}
ClipWriteStream& operator<<(ClipWriteStream& stm, size_t v)
{
    stm.Align();
    stm.PushBytes((LPCBYTE)&v, sizeof(v));
    return stm;
}
ClipWriteStream& operator<<(ClipWriteStream& stm, const std::string& v)
{
    stm << v.size();
    stm.PushBytes((LPCBYTE)v.c_str(), v.size());
    return stm;
}
ClipWriteStream& operator<<(ClipWriteStream& stm, const IniToken& v)
{
    stm << v.Empty << v.IsSection << v.Desc << v.Key << v.Value;
    return stm;
}
ClipWriteStream& operator<<(ClipWriteStream& stm, const ImVec2& v)
{
    stm << v.x << v.y;
    return stm;
}
ClipWriteStream& operator<<(ClipWriteStream& stm, const PairClipString& v)
{
    stm << v.A << v.B;
    return stm;
}
ClipWriteStream& operator<<(ClipWriteStream& stm, const PairClipOnShow& v)
{
    stm << v.Show << v.Str;
    return stm;
}

ClipReadStream& operator>>(ClipReadStream& stm, bool& v)
{
    stm.Align(1);
    v = stm.Get<bool>();
    return stm;
}
ClipReadStream& operator>>(ClipReadStream& stm, float& v)
{
    stm.Align();
    v = stm.Get<float>();
    return stm;
}
ClipReadStream& operator>>(ClipReadStream& stm, size_t& v)
{
    stm.Align();
    v = stm.Get<size_t>();
    return stm;
}
ClipReadStream& operator>>(ClipReadStream& stm, std::string& v)
{
    size_t x;
    stm >> x;
    v.resize(x);
    memcpy(v.data(), stm.GetByte(x), x);
    return stm;
}
ClipReadStream& operator>>(ClipReadStream& stm, IniToken& v)
{
    stm >> v.Empty >> v.IsSection >> v.Desc >> v.Key >> v.Value;
    return stm;
}
ClipReadStream& operator>>(ClipReadStream& stm, ImVec2& v)
{
    stm >> v.x >> v.y;
    return stm;
}
ClipReadStream& operator>>(ClipReadStream& stm, PairClipString& v)
{
    stm >> v.A >> v.B;
    return stm;
}
ClipReadStream& operator>>(ClipReadStream& stm, PairClipOnShow& v)
{
    stm >> v.Show >> v.Str;
    return stm;
}


ClipWriteStream& operator<<(ClipWriteStream& stm, const ModuleClipData& v)
{
    /*
    IsLinkGroup=false
    IsComment=false
    Ignore
    Desc
    Lines
    Inherit
    Register
    DefaultLinkKey
    DisplayName
    EqSize
    EqDelta
    VarList
    */
    if (!v.IsLinkGroup && !v.IsComment)
    {
        stm << v.IsLinkGroup << v.IsComment << v.Ignore << v.Desc
            << v.Lines << v.Inherit  << v.Register <<v.DefaultLinkKey
            << v.DisplayName << v.EqSize << v.EqDelta << v.VarList;
    }
    /*
    IsLinkGroup=false
    IsComment=true
    Desc
    EqSize
    EqDelta
    Comment
    */
    else if (!v.IsLinkGroup)
    {
        MessageBoxW(NULL, UTF8toUnicode(v.Comment).c_str(), L"COMMENT_WRITE", MB_OK);
        stm << v.IsLinkGroup << v.IsComment << v.Desc << v.EqSize << v.EqDelta << v.Comment;
    }
    /*
    IsLinkGroup=true
    Desc
    DefaultLinkKey
    EqSize
    EqDelta
    VarList
    LinkTo
    */
    else
    {
        stm << v.IsLinkGroup << v.Desc << v.DefaultLinkKey << v.EqSize << v.EqDelta << v.VarList << v.LinkGroup_LinkTo;
    }
    /*
    IncludedBySection
    IncludingSections
    */
    stm << v.IncludedBySection << v.IncludingSections << v.CollapsedInComposed << v.Frozen << v.Hidden;
    return stm;
}
ClipReadStream& operator>>(ClipReadStream& stm, ModuleClipData& v)
{
    stm >> v.IsLinkGroup;
    if (!v.IsLinkGroup)
    {
        stm >> v.IsComment;
    /*
    IsLinkGroup=false
    IsComment=false
    Ignore
    Desc
    Lines
    Inherit
    Register
    DefaultLinkKey
    DisplayName
    EqSize
    EqDelta
    VarList
    */
        if (!v.IsComment)
        {
            stm >> v.Ignore >> v.Desc >> v.Lines >> v.Inherit >> v.Register >> v.DefaultLinkKey
                >> v.DisplayName >> v.EqSize >> v.EqDelta >> v.VarList;
        }
    /*
    IsLinkGroup=false
    IsComment=true
    Desc
    EqSize
    EqDelta
    Comment
    */
        else
        {
            stm >> v.Desc >> v.EqSize >> v.EqDelta >> v.Comment;
            MessageBoxW(NULL, UTF8toUnicode(v.Comment).c_str(), L"COMMENT", MB_OK);
        }
    }
    /*
    IsLinkGroup=true
    Desc
    DefaultLinkKey
    EqSize
    EqDelta
    VarList
    LinkTo
    */
    else
    {
        stm >> v.Desc >> v.DefaultLinkKey >> v.EqSize >> v.EqDelta >> v.VarList >> v.LinkGroup_LinkTo;
    }

    /*
    IncludedBySection
    IncludingSections
    */
    if (stm.VersionAtLeast(10006))
    {
        stm >> v.IncludedBySection >> v.IncludingSections >> v.CollapsedInComposed >> v.Frozen >> v.Hidden;
    }
    else
    {
        v.IncludedBySection = { "","" };
        v.IncludingSections.clear();
        v.CollapsedInComposed = false;
        v.Frozen = false;
        v.Hidden = false;
    }
    return stm;
}


void subreplace(std::string& dst_str, const std::string& sub_str, const std::string& new_str)
{
    std::string::size_type pos = 0;
    while ((pos = dst_str.find(sub_str)) != std::string::npos)   //替换所有指定子串
    {
        dst_str.replace(pos, sub_str.length(), new_str);
    }
}

void ModuleClipData::Replace(const std::string& Parameter, const std::string& Argument)
{
#define X(S) subreplace(S, Parameter, Argument)
    X(Desc.A);
    X(Desc.B);
    //X(Comment);
    X(Inherit);
    X(Register);
    X(IncludedBySection.A);
    X(IncludedBySection.B);
    for (auto& Tk : Lines)
    {
        X(Tk.Desc);
        X(Tk.Key);
        X(Tk.Value);
    }
    for (auto& lt : LinkGroup_LinkTo)
    {
        X(lt.B);
        X(lt.A);
    }
    for (auto& lt : VarList)
    {
        X(lt.B);
        X(lt.A);
    }
    for (auto& lt : IncludingSections)
    {
        X(lt.B);
        X(lt.A);
    }
#undef X
}

void ModuleClipData::Load(const std::string_view Str, int ClipFormatVersion)
{
    ClipReadStream stm;
    stm.SetVersion(ClipFormatVersion);
    stm.Set(Str);
    stm >> *this;
    FromClipBoard = true;
}
std::string ModuleClipData::Save() const
{
    ClipWriteStream stm;
    stm << *this;
    return stm.Get();
}

JsonFile ClipStringToJson(const PairClipString& p)
{
    std::vector<JsonFile> vf;
    vf.resize(2);
    vf[0].GetObj().SetOrCreateString(p.A);
    vf[1].GetObj().SetOrCreateString(p.B);
    return GetArrayOfObjects(std::move(vf));
}
JsonFile VectorClipStringToJson(const std::vector<PairClipString>& vec)
{
    std::vector<JsonFile> vf;
    for (auto& p : vec)
    {
        vf.push_back(ClipStringToJson(p));
    }
    return GetArrayOfObjects(std::move(vf));
}

JsonFile ModuleClipData::ToJson() const
{
    /*
        bool IsLinkGroup;
    bool IsComment;
    bool Ignore;
    bool FromClipBoard;
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
    */
    JsonFile F;
    auto Obj = F.GetObj();
    Obj.SetOrCreateObject();
    Obj.AddBool("IsLinkGroup", IsLinkGroup);
    Obj.AddBool("IsComment", IsComment);
    Obj.AddBool("Ignore", Ignore);
    Obj.AddBool("FromClipBoard", FromClipBoard);
    Obj.AddBool("CollapsedInComposed", CollapsedInComposed);
    Obj.AddDouble("EqSizeX", EqSize.x);
    Obj.AddDouble("EqSizeY", EqSize.y);
    Obj.AddDouble("EqDeltaX", EqDelta.x);
    Obj.AddDouble("EqDeltaY", EqDelta.y);
    Obj.AddObjectItem("Desc", ClipStringToJson(Desc));
    Obj.AddString("Comment", Comment);
    Obj.AddString("Inherit", Inherit);
    Obj.AddString("Register", Register);
    Obj.AddObjectItem("DefaultLinkKey", VectorClipStringToJson(DefaultLinkKey));
    Obj.AddString("DisplayName", DisplayName);
    {
        std::vector<JsonFile> vf;
        for (auto& t : Lines)
        {
            vf.emplace_back();
            auto o = vf.back().GetObj();
            o.SetOrCreateObject();
            o.AddBool("Empty", t.Empty);
            o.AddBool("IsSection", t.IsSection);
            o.AddBool("HasDesc", t.HasDesc);
            o.AddString("Desc", t.Desc);
            o.AddString("Key", t.Key);
            o.AddString("Value", t.Value);
        }
        Obj.AddObjectItem("Lines", GetArrayOfObjects(std::move(vf)));
    }
    Obj.AddObjectItem("LinkGroup_LinkTo", VectorClipStringToJson(LinkGroup_LinkTo));
    Obj.AddObjectItem("VarList", VectorClipStringToJson(VarList));
    Obj.AddObjectItem("IncludedBySection", ClipStringToJson(IncludedBySection));
    Obj.AddObjectItem("IncludingSections", VectorClipStringToJson(IncludingSections));
    return F;
}

std::string IBB_ModuleAlt::GetFirstINI() const
{
    if (Modules.empty())return "";
    else return Modules[0].Desc.A;
}

bool IBB_ModuleAlt::Search(const std::string& Str, bool ConsiderRegName, bool ConsiderDescName, bool ConsiderDesc)
{
    if (!Available)return false;
    if (ConsiderRegName && Name.find(Str) != std::string::npos)return true;
    if (ConsiderDescName && DescShort.find(Str) != std::string::npos)return true;
    if (ConsiderDesc && DescLong.find(Str) != std::string::npos)return true;
    if (ConsiderDesc && ParamDescShort.find(Str) != std::string::npos)return true;
    if (ConsiderDesc && ParamDescLong.find(Str) != std::string::npos)return true;
    return false;
}

namespace OldClipMagic
{
    const std::string ClipMagic201 = "IniBrowserClipDataFormat_0.2b1";
    const std::string ClipMagic202 = "IniBrowserClipDataFormat_0.2b2";
    const std::string ClipMagic10000 = "IniBrowserClipDataFormat_1.0";
    const std::string ClipMagic10004 = "IniBrowserClipDataFormat_1.0.4";
}

const char* ClipMagicPrefix = "IniBrowserClipDataFormat_";
const std::string ClipMagic = ClipMagicPrefix + ClipDataFormatVersion;
const std::string ClipMagicEnd = "EndOfClipData";
std::string TimeNowU8();
extern int RFontHeight;

bool IBB_ModuleAlt::SaveToFile()
{
    using namespace std::string_literals;
    if (!Available)return false;
    ExtFileClass E;
    E.Open(Path.c_str(), L"w");
    if (!E.Available())return false;
    auto cwa = locw("AppName");
    auto cwb = UTF8toUnicode(TimeNowU8());
    E.PutStr(";" + UnicodetoUTF8(std::vformat(locw("Back_SaveModuleAltLine1"), std::make_wformat_args(cwa, VersionW)))); E.Ln();
    E.PutStr(";" + loc("Back_SaveModuleAltLine2")); E.Ln();
    E.PutStr(";" + UnicodetoUTF8(std::vformat(locw("Back_SaveModuleAltLine3"), std::make_wformat_args(cwb)))); E.Ln();
    E.Ln();

    for (size_t i = 0; i < Modules.size(); i++)
    {
        auto S = Modules[i].Desc.B;
        for (auto& N : Modules)
            N.Replace(S, Parameter + std::to_string(i));
    }

    E.PutStr("[Info]"); E.Ln();
    E.PutStr("Name = " + Name); E.Ln();
    E.PutStr("DescShort = " + DescShort); E.Ln();
    E.PutStr("DescLong = " + DescLong); E.Ln();
    E.PutStr("ParamDescShort = " + ParamDescShort); E.Ln();
    E.PutStr("ParamDescLong = " + ParamDescLong); E.Ln();

    IBB_ClipBoardData Clip;
    for (auto& M : Modules)
        if (M.IsComment || M.IsLinkGroup)
            Clip.Modules.push_back(M);
    Clip.ProjectRID = IBF_Inst_Project.CurrentProjectRID;

    if(!Clip.Modules.empty())E.PutStr("Data = " + Clip.GetString()); E.Ln();
    E.Ln(); E.Ln();

    E.PutStr("[Register]"); E.Ln();//INI#SEC=REG
    for (auto& M : Modules)
        if (!M.IsLinkGroup && !M.IsComment)
        {
            E.PutStr(M.Desc.A + "#" + M.Desc.B + " = " + M.Register);
            E.Ln();
        }
    E.Ln(); E.Ln();

    for (auto& M : Modules)
    {
        if (M.IsComment || M.IsLinkGroup)continue;
        if (M.Inherit.empty())
        {
            E.PutStr(M.Desc.A + "#[" + M.Desc.B + "]"); E.Ln();//INI#[SEC]
        }
        else
        {
            E.PutStr(M.Desc.A + "#[" + M.Desc.B + "]:[" + M.Inherit + "]"); E.Ln();//INI#[SEC]:[INHERIT]
        }
        
        {
            //EqDelta
            E.PutStr("ImportDeltaX = " + std::to_string(M.EqDelta.x / RFontHeight)); E.Ln();
            E.PutStr("ImportDeltaY = " + std::to_string(M.EqDelta.y / RFontHeight)); E.Ln();
        }

        for (auto& D : M.DefaultLinkKey)
        {
            E.PutStr(D.A + "#DefaultLink = " + D.B); E.Ln();
        }

        for (auto& D : M.VarList)
        {
            if (D.A == "_Local_AtFile")continue;
            E.PutStr(D.A + "#Var = " + D.B); E.Ln();
        }

        for (auto& L : M.Lines)
        {
            std::string l;
            if (L.HasDesc)
            {
                if (L.Desc != EmptyOnShowDesc)l = L.Desc;
                l.push_back('#');
            }
            l += L.Key;l += " = ";l += L.Value;
            E.PutStr(l); E.Ln();
        }

        E.Ln(); E.Ln();
    }

    return true;
}

std::unordered_map<std::string, std::unordered_map<std::string, std::string>> IniToMap(std::vector<std::vector<IniToken>>&& Secs)
{
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> Ret;
    for (auto& sec : Secs)
    {
        if (sec.empty())continue;
        Ret[sec[0].Key] = GetVarList(sec);
    }
    return Ret;
}

int GetClipFormatVersion(const std::string& Magic)
{
    if (Magic.starts_with(ClipMagic))return VersionN;
    if (Magic.starts_with(OldClipMagic::ClipMagic10000)                   ||
        Magic.starts_with(OldClipMagic::ClipMagic201)  ||
        Magic.starts_with(OldClipMagic::ClipMagic202)  ||
        Magic.starts_with(OldClipMagic::ClipMagic10004)
    ) return 10000;
    return 0;
}

int GetClipFormatVersion(int AppVersion)
{
    if (AppVersion > VersionN)return AppVersion;
    if (AppVersion >= 10006)return 10006;
    if (AppVersion >= 200)return 10000;
    return 0;
}

void IBB_ModuleAlt::LoadFromString(std::wstring_view FileName, std::string&& FileStr)
{
    Available = false;
    Path = FileName;
    auto Secs = SplitTokens(GetTokens(GetLines(std::move(FileStr))));

    std::map<IBB_Section_Desc, std::string> Reg;
    bool HasReg = false, HasInfo = false;
    bool FromClip = false;

    for (auto& sec : Secs)
    {
        if (sec.empty())continue;
        if (sec[0].Key == "Register")
        {
            for (size_t i = 1; i < sec.size(); i++)
            {
                if (sec[i].Desc.empty())sec[i].Desc = DefaultIniName;
                Reg[{sec[i].Desc, sec[i].Key}] = sec[i].Value;//INI#SEC=REG
            }
            HasReg = true;
        }
        else if (sec[0].Key == "Info")
        {
            auto L = GetVarList(sec);
            DescShort = L["DescShort"];
            Name = L["Name"];
            if (Name.empty())Name = DescShort;
            DescLong = L["DescLong"];
            ParamDescShort = L["ParamDescShort"];
            ParamDescLong = L["ParamDescLong"];
            Parameter = "****";

            //版本重载决议
            if (!L["Data"].empty())
            {
                auto Ver = GetClipFormatVersion(L["Data"]);
                IBB_ClipBoardData ClipData;
                IBB_ClipBoardData::ErrorContext.ModuleName = Name;
                IBB_ClipBoardData::ErrorContext.ModulePath = Path;
                if (ClipData.SetString(L["Data"], Ver))
                {
                    FromClip = true;
                    Modules.insert(Modules.end(), ClipData.Modules.begin(), ClipData.Modules.end());
                }
                else
                {
                    FromClip = false;
                    Modules.clear();
                }
            }

            HasInfo = true;
        }
        else
        {
            Modules.emplace_back();
            auto& M = Modules.back();//INI#[SEC]:[INHERIT]
            if (sec[0].Desc.empty())sec[0].Desc = DefaultIniName;
            M.Desc = { sec[0].Desc, sec[0].Key };
            M.Inherit = sec[0].Value;
            M.VarList.push_back({ "_Local_AtFile", sec[0].Desc });
            M.Comment = "";
            M.DisplayName = "";
            M.Ignore = false;
            M.IsComment = false;
            M.IsLinkGroup = false;
            M.FromClipBoard = false;
            M.LinkGroup_LinkTo.clear();
            M.EqDelta.x = (float)1e100;
            M.EqDelta.y = (float)1e100;

            for (size_t i = 1; i < sec.size(); i++)
            {
                if (sec[i].Key == "DefaultLink")
                {
                    //Type#DefaultLink=Key
                    M.DefaultLinkKey.push_back({ sec[i].Desc, sec[i].Value });//Type,Key

                    //TODO : ensure type 似了
                    //IBF_Inst_DefaultTypeList.EnsureType(sec[i].Value, sec[i].Desc);
                }
                else if (sec[i].Key == "Var")
                {
                    //Type#Var=Key
                    M.VarList.push_back({ sec[i].Desc, sec[i].Value });//Type,Key
                }
                else if (sec[i].Key == "ImportDeltaX")
                {
                    M.EqDelta.x = FontHeight * (float)std::strtod(sec[i].Value.c_str(), nullptr);
                    //GlobalLogB.AddLog(sec[i].Value.c_str());
                    //GlobalLogB.AddLog((int)M.EqDelta.x);
                    //Sleep(100);
                }
                else if (sec[i].Key == "ImportDeltaY")
                {
                    M.EqDelta.y = FontHeight * (float)std::strtod(sec[i].Value.c_str(), nullptr);
                    //Sleep(10);
                }
                else
                {
                    M.Lines.push_back(sec[i]);
                    if (M.Lines.back().HasDesc && M.Lines.back().Desc.empty())M.Lines.back().Desc = EmptyOnShowDesc;
                }
            }
        }
    }

    for (auto& M : Modules)
    {
        IBB_Section_Desc De = { M.Desc.A,M.Desc.B };
        auto It = Reg.find(De);
        if (It != Reg.end())
        {
            M.Register = It->second;
        }
        M.VarList.push_back({ "_Local_Category", M.Register });
    }

    FromClipBoard = FromClip;
    Available = FromClip || (HasReg && HasInfo && !Modules.empty());
}

void IBB_ClipBoardData::Generate(const std::vector<IBB_Section_Desc>& Module)
{
    ProjectRID = IBF_Inst_Project.CurrentProjectRID;
    Modules.reserve(Module.size());
    for (auto& m : Module)
    {
        Modules.emplace_back();
        if (!IBR_Inst_Project.GetSection(m).GetClipData(Modules.back(), false))
            Modules.pop_back();
    }
}

void IBB_ClipBoardData::GenerateAll(bool UsePosAsDelta, bool FromClipBoard)
{
    ProjectRID = IBF_Inst_Project.CurrentProjectRID;
    Modules.reserve(IBR_Inst_Project.IBR_Rev_SectionMap.size());
    for (auto& [D, I] : IBR_Inst_Project.IBR_Rev_SectionMap)
    {
        IBR_Section Sec{ &IBR_Inst_Project, I };
        if (!Sec.HasBack())continue;
        Modules.emplace_back();
        Modules.back().FromClipBoard = FromClipBoard;
        if (!Sec.GetClipData(Modules.back(), UsePosAsDelta))
            Modules.pop_back();
    }
}

void IBB_ModuleAlt::LoadFromFile(const wchar_t* FileName)
{
    LoadFromString(FileName, GetStringFromFile(FileName));
}

void IBB_ModuleAlt::LoadFromFile(const char* FileName)
{
    LoadFromString(UTF8toUnicode(FileName), GetStringFromFile(FileName));
}

//此处关联了IBR_Project::RenameAll()
//记得同时修改
bool ModuleClipData::NeedtoMangle() const
{
    bool P = false;
    std::string W{}, Q{};
    for (auto& v : VarList)
    {
        if (v.A == "_InitialSecName")W = v.B;
        else if (v.A == "UseOwnName")Q = v.B;
    }
    if (W == Desc.B && !IsTrueString(Q))
    {
        P = true;
    }
    if (W.empty())P = true;
    return P;
}

void MangleModules(std::vector<ModuleClipData>& Modules)
{
    for (auto& m : Modules)
    {
        if (!m.NeedtoMangle())continue;

        auto S = m.Desc.B;
        auto W = GenerateModuleTag();

        for (auto& v : m.VarList)
            if (v.A == "_InitialSecName")v.B = W;
        for (auto& n : Modules)
            n.Replace(S, W);
    }
}

void IBB_ClipBoardData::Mangle()
{
    MangleModules(Modules);
}

inline auto trim(std::string_view string) noexcept {
    auto const first = string.find_first_not_of(' ');
    if (first != std::string_view::npos) {
        auto const last = string.find_last_not_of(' ');
        string = string.substr(first, last - first + 1);
    }
    return string;
}

inline std::vector<std::string_view> SplitView(const std::string_view Text)//ORIG
{
    if (Text.empty())return {};
    size_t cur = 0, crl;
    std::vector<std::string_view> ret;
    while ((crl = Text.find_first_of(' ', cur)) != Text.npos)
    {
        ret.push_back(trim(Text.substr(cur, crl - cur)));
        cur = crl + 1;
    }
    ret.push_back(trim(Text.substr(cur)));
    return ret;
}

inline std::vector<std::string> SplitView_ToSV(const std::string_view Text)//ORIG
{
    if (Text.empty())return {};
    size_t cur = 0, crl;
    std::vector<std::string> ret;
    while ((crl = Text.find_first_of(',', cur)) != Text.npos)
    {
        ret.emplace_back(trim(Text.substr(cur, crl - cur)));
        cur = crl + 1;
    }
    ret.emplace_back(trim(Text.substr(cur)));
    return ret;
}


IBB_ClipBoardData::ErrorCtx IBB_ClipBoardData::ErrorContext{};

std::vector<BYTE> IBB_ClipBoardData::GetStream() const
{
    ClipWriteStream Stm;
    Stm << ProjectRID << Modules;
    return Stm.Buffer;
}



std::string IBB_ClipBoardData::GetString() const
{
    std::string Result = ClipMagic;
    Result.push_back(',');
    Result += std::to_string(ProjectRID);
    Result.push_back(',');
    for (auto& m : Modules)
    {
        Result += m.Save();
        Result.push_back(',');
    }
    Result += ClipMagicEnd;
    return Result;
}

bool CheckClipVersion(int ClipFormatVersion, const std::string& Ver_Prefix)
{
    if (ClipFormatVersion > VersionN || ClipFormatVersion < 10000)
    {
        auto VersionStr = UTF8toUnicode(GetVersionStr(ClipFormatVersion));
        if (ClipFormatVersion <= 0)
        {
            if (Ver_Prefix.starts_with(ClipMagicPrefix) && Ver_Prefix != ClipMagicPrefix)
                VersionStr = UTF8toUnicode(Ver_Prefix.substr(strlen(ClipMagicPrefix)));
            else VersionStr = UTF8toUnicode(Ver_Prefix);
        }
        auto ModuleNameW = UTF8toUnicode(IBB_ClipBoardData::ErrorContext.ModuleName);
        //ModulePath ModuleName VersionName VersionID
        auto ErrorStr = UnicodetoUTF8(
            std::vformat(locw("Error_ModuleIncompatible"), std::make_wformat_args(
                IBB_ClipBoardData::ErrorContext.ModulePath,
                ModuleNameW,
                VersionStr,
                ClipFormatVersion
        )));
        IBR_PopupManager::AddModuleParseErrorPopup(std::move(ErrorStr), loc("Error_ModuleParseError"));
        return false;
    }
    return true;
}

bool IBB_ClipBoardData::SetStream(const std::vector<BYTE>& Vec, int ClipFormatVersion)
{
    if (!CheckClipVersion(ClipFormatVersion, ""))return false;
    ClipReadStream Stm;
    Stm.SetVersion(ClipFormatVersion);
    Stm.Set(Vec);
    Stm >> ProjectRID >> Modules;
    return true;
}

JsonFile ModulesToJson(const std::vector<ModuleClipData>& Modules)
{
    std::vector<JsonFile> vf;
    for (auto& Clip : Modules)
    {
        vf.push_back(Clip.ToJson());
    }
    return GetArrayOfObjects(std::move(vf));
}

JsonFile IBB_ClipBoardData::ToJson() const
{
    JsonFile F;
    auto Obj = F.GetObj();
    Obj.SetOrCreateObject();
    Obj.AddInt("ProjectRID", ProjectRID);
    Obj.AddObjectItem("Modules", ModulesToJson(Modules));
    return F;
}

bool IBB_ClipBoardData::SetString(const std::string_view Str, int ClipFormatVersion)
{
    
    auto View = SplitView_ToSV(Str);
    if (View.size() <= 3)return false;

    if (ClipFormatVersion == INT_MAX)
        ClipFormatVersion = GetClipFormatVersion(View[0]);
    if (!CheckClipVersion(ClipFormatVersion, View[0]))return false;

    if (View.back() != ClipMagicEnd)return false;
    sscanf(View[1].c_str(), "%u", &ProjectRID);
    Modules.resize(View.size() - 3);
    try
    {
        for (size_t i = 2; i < View.size() - 1; i++)
            Modules[i - 2].Load(View[i], ClipFormatVersion);
    }
    catch(std::exception& e)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            auto w1 = UTF8toUnicode(e.what());
            GlobalLogB.AddLog(std::vformat(L"IBB_ClipBoardData::SetString ：" + locw("Error_CannotCopyToClipboard"), std::make_wformat_args(w1)));
        }
        return false;
    }
    Mangle();
    return true;
}

extern wchar_t CurrentDirW[5000];

namespace SearchModuleAlt
{
    void RenderModuleAltSelect(IBB_ModuleAlt* pModule);
}
namespace ImGui
{
    ImVec2 GetLineEndPos();
    ImVec2 GetLineBeginPos();
    bool IsWindowClicked(ImGuiMouseButton Button);
}
namespace IBR_PopupManager
{
    extern std::vector<StdMessage> DelayedPopupAction;
}

namespace IBB_ModuleAltDefault
{
    std::unordered_map<std::string, std::unique_ptr<IBB_ModuleAlt>> FlattenedModules;
    std::vector<std::string> FlattenedModuleName;

    struct ModuleTree
    {
        std::string _TEXT_UTF8 Name;
        std::vector<std::unique_ptr<ModuleTree>> Sub;
        std::unordered_map<std::string, IBB_ModuleAlt*> Modules;
        bool ChildMenuHovered{ false };
        bool LastHovered{ false };

        void ResetHover()
        {
            ChildMenuHovered = false;
            for (auto& S : Sub)
            {
                S->ResetHover();
            }
        }
        void RenderUI()
        {
            for (auto& S : Sub)
            {
                auto Pos = ImGui::GetCursorScreenPos();
                bool Hovered = false;
                ImRect R{ ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2{ ImGui::GetWindowWidth() + 0.5F * FontHeight, ImGui::GetTextLineHeightWithSpacing() } };
                if (R.Contains(ImGui::GetMousePos()))Hovered = true;
                //if (Hovered)ImGui::GetForegroundDrawList()->AddRect(R.Min, R.Max, IBR_Color::FocusLineColor);
                ImGui::Dummy(ImVec2((float)FontHeight, (float)FontHeight));
                ImGui::SameLine();
                ImGui::Text(S->Name.c_str());
                Hovered |= ImGui::IsItemHovered();
                bool V = S->ChildMenuHovered;
                for (auto& C : S->Sub)
                {
                    V |= C->ChildMenuHovered;
                }
                if (S->LastHovered || V)
                {
                    DrawOpenFolderIcon(Pos, (float)FontHeight);
                    if (!S->Sub.empty() || !S->Modules.empty())
                    {
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 1.0F * FontHeight);
                        ImGui::Text(u8">");
                        ImVec2 ppos = ImGui::GetLineEndPos();
                        ppos.y -= ImGui::GetTextLineHeightWithSpacing();
                        IBR_PopupManager::DelayedPopupAction.push_back(
                            [ppos, P = S.get()] {
                                P->ChildMenuHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RectOnly);
                                ImGui::SetWindowPos(ppos);
                                P->RenderUI();
                            }
                        );
                    }
                }
                else DrawFolderIcon(Pos, (float)FontHeight);
                S->LastHovered = Hovered;
            }
            for (auto& [N, M] : Modules)
            {
                SearchModuleAlt::RenderModuleAltSelect(M);
            }
        }

        void NewModule(IBB_ModuleAlt&& Mod)
        {
            if (!Mod.Available)return;
            auto it = FlattenedModules.find(Mod.Name);
            int I = 0;
            while (it != FlattenedModules.end())
                it = FlattenedModules.find(Mod.Name + "_" + std::to_string(++I));
            if (I) Mod.Name += "_" + std::to_string(I);
            FlattenedModuleName.push_back(Mod.Name);
            auto& M = FlattenedModules[Mod.Name];
            auto& N = Modules[Mod.Name];
            M.reset(new IBB_ModuleAlt(std::move(Mod)));
            N = M.get();
        }

        void LoadFromDir(const std::wstring& Dir)
        {
            //MessageBoxW(NULL, Dir.c_str(), L"Load", MB_OK);
            for (auto& File : FindFileRange(Dir))
            {
                if (PathIsDirectoryW(File.FullPath.c_str()))
                {
                    //if it is not . or ..
                    if (File.Name != L"." && File.Name != L"..")
                    {
                        //MessageBoxW(NULL, File.FullPath.c_str(), L"Directory", MB_OK);
                        Sub.emplace_back(new ModuleTree);
                        Sub.back()->Name = UnicodetoUTF8(File.Name);
                        Sub.back()->LoadFromDir(File.FullPath + L"\\*.*");
                    }
                }
                else
                {
                    //MessageBoxW(NULL, File.FullPath.c_str(), L"File", MB_OK);
                    IBB_ModuleAlt Mod;
                    Mod.LoadFromFile(File.FullPath.c_str());
                    NewModule(std::move(Mod));
                }
            }
        }
    };
    std::unordered_map<std::string, IBB_ModuleAlt> ArtModules;
    ModuleTree AllModules;
    std::wstring Range1;
    std::wstring Range2;
    std::wstring Range3;
    std::wstring GenerateModulePath()
    {
        return CurrentDirW + Range3 + RandWStr(12) + L".ini";
    }
    std::wstring GenerateModulePath_NoName()
    {
        return CurrentDirW + Range3;
    }
    
    void NewModuleII(IBB_ModuleAlt&& Mod)
    {
        if (!Mod.Available)return;
        auto it = ArtModules.find(Mod.Name);
        int I = 0;
        while (it != ArtModules.end())
            it = ArtModules.find(Mod.Name + "_" + std::to_string(++I));
        if (I) Mod.Name += "_" + std::to_string(I);
        ArtModules[Mod.Name] = std::move(Mod);
    }
    IBB_ModuleAlt* GetModuleII(const std::string& Name)
    {
        auto it = ArtModules.find(Name);
        if (it != ArtModules.end()) return it->second.Available ? &it->second : nullptr;
        else return nullptr;
    }
    //Voxel
    IBB_ModuleAlt* DefaultArt_Voxel()
    {
        return GetModuleII("DefaultArt_Voxel");
    }
    //DefaultArt_SHPVehicle
    IBB_ModuleAlt* DefaultArt_SHPVehicle()
    {
        return GetModuleII("DefaultArt_SHPVehicle");
    }
    //DefaultArt_SHPBuilding
    IBB_ModuleAlt* DefaultArt_SHPBuilding()
    {
        return GetModuleII("DefaultArt_SHPBuilding");
    }
    //DefaultArt_SHPInfantry
    IBB_ModuleAlt* DefaultArt_SHPInfantry()
    {
        return GetModuleII("DefaultArt_SHPInfantry");
    }
    //DefaultArt_Animation
    IBB_ModuleAlt* DefaultArt_Animation()
    {
        return GetModuleII("DefaultArt_Animation");
    }
    void Load(const wchar_t* FileRange, const wchar_t* FileRange2, const wchar_t* FileRange3)
    {
        Range1 = FileRange;
        Range2 = FileRange2;
        Range3 = FileRange3;
        AllModules.LoadFromDir(FileRange);
        for (auto& File : FindFileRange(FileRange2))
        {
            IBB_ModuleAlt Mod;
            Mod.LoadFromFile(File.FullPath.c_str());
            NewModuleII(std::move(Mod));
        }
        //for (auto& [K, V] : ArtModules)MessageBoxA(NULL, K.c_str(), "ArtModules", MB_OK);
    }
    IBB_ModuleAlt* GetModule(const std::string& Name)
    {
        auto it = FlattenedModules.find(Name);
        if (it != FlattenedModules.end()) return it->second->Available ? it->second.get() : nullptr;
        else return nullptr;
    }
    void NewModule(IBB_ModuleAlt&& M)
    {
        AllModules.NewModule(std::move(M));
    }
    std::vector<IBB_ModuleAlt*> Search(const std::string& Str, bool ConsiderName, bool ConsiderDesc, bool ConsiderParamDesc)
    {
        std::vector<IBB_ModuleAlt*> result;
        for (auto& [N, M] : FlattenedModules)
        {
            if (M->Search(Str, ConsiderName, ConsiderDesc, ConsiderParamDesc))
                result.push_back(M.get());
        }
        return result;
    }

    void Tree_RenderUI()
    {
        AllModules.RenderUI();
    }
    void Tree_ResetHover()
    {
        AllModules.ResetHover();
    }
}


std::vector<std::string> ParseCSVLine(std::string_view L)
{
    std::vector<std::string> Result;
    bool Quote{ false };
    size_t d = 0, i = 0;
    for (; i < L.size(); i++)
    {
        if (!Quote)
        {
            if (L[i] == ',')
            {
                Result.emplace_back(TrimView(L.substr(d, i - d)));
                d = i + 1;
            }
            else if (L[i] == '"')
            {
                Quote = true;
                d = i + 1;
            }
        }
        else
        {
            if (L[i] == '"')
            {
                Result.emplace_back(TrimView(L.substr(d, i - d)));
                Quote = false;
                while (i < L.size() && L[i] != ',')i++;
                d = i + 1;
            }
        }
    }
    if (i > d)Result.emplace_back(TrimView(L.substr(d, i - d)));
    return Result;
}

std::vector<std::vector<std::string>> ParseCSVText(const std::vector<std::string_view>& Vec)
{
    std::vector<std::vector<std::string>> Result;
    Result.reserve(Vec.size());
    for (auto& Line : Vec)Result.push_back(ParseCSVLine(Line));
    return Result;
}

const std::vector<std::vector<std::string>>& CSVReader::GetData() const
{
    return Data;
}

void CSVReader::ReadFromFile(const char* FileName)
{
    Data = ParseCSVText(GetLines(GetStringFromFile(FileName)));
}

void CSVReader::ReadFromFile(const wchar_t* FileName)
{
    Data = ParseCSVText(GetLines(GetStringFromFile(FileName)));
}
