#pragma once

#include"types.h"
#include"..\cjson\cJSON.h"

#include<functional>
#include<string>
#include<algorithm>
#include<vector>
#include<utility>
#include<unordered_map>

void TemporaryLog(const wchar_t*);
#define TEMPLOG(S) //TemporaryLog(__FUNCTIONW__ L" : " L ## S);

template<typename T>
T gcd(T a, T b)
{
    while (b)
    {
        a %= b;
        std::swap(a, b);
    }
    return a;
}

template<typename T>
T lcm(T a, T b)
{
    return (a * b) / gcd(a, b);
}


// ANSI字符集转换成Unicode
std::wstring MBCStoUnicode(const std::string& MBCS);
// UTF-8字符集转换成Unicode
std::wstring UTF8toUnicode(const std::string& UTF8);
// Unicode字符集转换成UTF-8
std::string UnicodetoUTF8(const std::wstring& Unicode);
// Unicode字符集转换成ANSI
std::string UnicodetoMBCS(const std::wstring& Unicode);
// ANSI字符集转换成UTF-8
std::string MBCStoUTF8(const std::string& MBCS);
// UTF-8字符集转换成ANSI
std::string UTF8toMBCS(const std::string& MBCS);

uint64_t GetSysTimeMicros();
std::string TimeNow();

bool system_hide(const char* CommandLine);

bool IsTrueString(const std::string& s);


namespace std
{
    _NODISCARD inline string to_string(const void* _Val)
    {
        const auto _Len = static_cast<size_t>(_CSTD _scprintf("%p", _Val));
        string _Str(_Len, '\0');
        _CSTD sprintf_s(&_Str[0], _Len + 1, "%p", _Val);
        return _Str;
    }
}

/*
struct RandStrType
{
    constexpr static int Lower = 1;
    constexpr static int Upper = 2;
    constexpr static int Lower = 4;
};
*/

std::string RandStr(int Length);
std::wstring RandWStr(int Length);
std::string GenerateModuleTag();

class ExtFileClass;
std::string GetStringFromFile(ExtFileClass& File);
std::string GetStringFromFile(const char* FileName);
std::string GetStringFromFile(const wchar_t* FileName);

std::vector<int> cJSON_GetVectorInt(cJSON* Array);
std::vector<uint8_t> cJSON_GetVectorBool(cJSON* Array);
std::vector<std::string> cJSON_GetVectorString(cJSON* Array);
std::vector<cJSON*> cJSON_GetVectorObject(cJSON* Array);

class CSVReader
{
    std::vector<std::vector<std::string>> Data;
public:
    const std::vector<std::vector<std::string>>& GetData() const;
    void ReadFromFile(const char* FileName);
    void ReadFromFile(const wchar_t* FileName);
};

enum class StrBoolType :size_t
{
    Str_true_false,
    Str_True_False,
    Str_TRUE_FALSE,
    Str_yes_no,
    Str_Yes_No,
    Str_YES_NO,
    Str_t_f,
    Str_T_F,
    Str_y_n,
    Str_Y_N,
    Str_1_0
};
std::string StrBoolImpl(bool Val, StrBoolType Type);
const char* CStrBoolImpl(bool Val, StrBoolType Type);

class JsonFile;

class JsonObject
{
private:
    cJSON* Object{ nullptr };
public:
    JsonObject(cJSON* _F) { Object = _F; }
    JsonObject() : Object(nullptr) {}

    cJSON* GetRaw() const { return Object; }

    operator bool() const { return Object != nullptr; }
    bool operator!() const { return Object == nullptr; }

    bool Available() const { return Object != nullptr; }
    int GetType() const { return Object->type; }
    bool IsTypeNumber() const { return ((Object->type & 0xFF) == cJSON_Number); }
    bool IsTypeNull() const { return ((Object->type & 0xFF) == cJSON_NULL); }
    bool IsTypeBool() const { return ((Object->type & 0xFF) == cJSON_True) || ((Object->type & 0xFF) == cJSON_False); }
    bool IsTypeString() const { return ((Object->type & 0xFF) == cJSON_String); }
    bool IsTypeArray() const { return ((Object->type & 0xFF) == cJSON_Array); }
    bool IsTypeObject() const { return ((Object->type & 0xFF) == cJSON_Object); }

    bool IsPropReference() const { return ((Object->type & cJSON_IsReference) == cJSON_IsReference); }
    bool IsPropConstString() const { return ((Object->type & cJSON_StringIsConst) == cJSON_StringIsConst); }

    JsonObject GetChildItem() const { return Object->child; }
    JsonObject GetPrevItem() const { return Object->prev; }
    JsonObject GetNextItem() const { return Object->next; }
    std::string GetName() const { return Object->string; }

    inline bool HasItem(const std::string& Str) const { return GetObjectItem(Str).Available(); }

    int ItemInt(const std::string& Str) const;
    double ItemDouble(const std::string& Str) const;
    std::string ItemString(const std::string& Str) const;
    const char* ItemCString(const std::string& Str) const;
    bool ItemBool(const std::string& Str) const;
    bool ItemStrBool(const std::string& Str) const;
    size_t ItemArraySize(const std::string& Str) const;
    std::vector<int> ItemArrayInt(const std::string& Str) const;
    std::vector<uint8_t> ItemArrayBool(const std::string& Str) const;
    std::vector<std::string> ItemArrayString(const std::string& Str) const;
    std::vector<JsonObject> ItemArrayObject(const std::string& Str) const;
    std::unordered_map<std::string, JsonObject> ItemMapObject(const std::string& Str) const;
    std::unordered_map<std::string, double> ItemMapDouble(const std::string& Str) const;
    std::unordered_map<std::string, int> ItemMapInt(const std::string& Str) const;
    std::unordered_map<std::string, bool> ItemMapBool(const std::string& Str) const;
    std::unordered_map<std::string, std::string> ItemMapString(const std::string& Str) const;

#define XXXOR(NAME ,TYPE) \
    template<class... TArgs>\
    TYPE Item ## NAME ## Or(const std::string& Str, TArgs&&... Args) const\
    {\
        return HasItem(Str) ? Item ## NAME(Str) : TYPE{std::forward<TArgs>(Args)...};\
    }

    template<typename T> using StrUMap = std::unordered_map < std::string, T>;
    XXXOR(Int, int)
    XXXOR(Double, double)
    XXXOR(String, std::string)
    //XXXOR(CString, const char*)
    XXXOR(Bool, bool)
    XXXOR(StrBool, bool)
    XXXOR(ArraySize, size_t)
    XXXOR(ArrayInt, std::vector<int>)
    XXXOR(ArrayBool, std::vector<uint8_t>)
    XXXOR(ArrayString, std::vector<std::string>)
    XXXOR(ArrayObject, std::vector<JsonObject>)
    XXXOR(MapInt, StrUMap<int>)
    XXXOR(MapBool, StrUMap<bool>)
    XXXOR(MapString, StrUMap<std::string>)
    XXXOR(MapObject, StrUMap<JsonObject>)

#undef XXXOR

    int GetInt() const { return Object->valueint; }
    double GetDouble() const { return Object->valuedouble; }
    std::string GetString() const { return Object->valuestring; }
    const char* GetCString() const { return Object->valuestring; }
    bool GetBool() const { return ((Object->type & 0xFF) == cJSON_True) ? true : false; }
    bool GetStrBool() const { return (Object->valuestring != nullptr && IsTrueString(Object->valuestring)) ? true : false; }
    JsonObject GetObjectItem(const std::string& Str) const { return { cJSON_GetObjectItem(Object, Str.c_str()) }; }
    size_t ArraySize() const { return (size_t)cJSON_GetArraySize(Object); }
    JsonObject GetArrayItem(size_t N) const { return { cJSON_GetArrayItem(Object, N) }; }
    std::vector<int> GetArrayInt() const { return cJSON_GetVectorInt(Object); }
    std::vector<uint8_t> GetArrayBool() const { return cJSON_GetVectorBool(Object); }
    std::vector<std::string> GetArrayString() const { return cJSON_GetVectorString(Object); }
    std::vector<JsonObject> GetArrayObject() const;
    std::unordered_map<std::string, JsonObject> GetMapObject() const;
    std::unordered_map<std::string, double> GetMapDouble() const;
    std::unordered_map<std::string, int> GetMapInt() const;
    std::unordered_map<std::string, bool> GetMapBool() const;
    std::unordered_map<std::string, std::string> GetMapString() const;

    std::string PrintData() const;
    std::string PrintUnformatted() const;

    JsonObject CreateObjectItem(const std::string& Str) const;
    void AddObjectItem(const std::string& Str, JsonObject Child, bool NeedsCopy) const;
    void AddObjectItem(const std::string& Str, JsonFile&& File) const;
    void AddNull(const std::string& Str) const;
    void AddInt(const std::string& Str, int Val) const;
    void AddDouble(const std::string& Str, double Val) const;
    void AddString(const std::string& Str, const std::string& Val) const;
    void AddBool(const std::string& Str, bool Val) const;
    void AddStrBool(const std::string& Str, bool Val, StrBoolType Type) const;

    //返回原来的Obj
    JsonFile SwapNull() const;
    JsonFile SwapInt(int Val) const;
    JsonFile SwapDouble(double Val) const;
    JsonFile SwapString(const std::string& Val) const;
    JsonFile SwapBool(bool Val) const;
    JsonFile SwapStrBool(bool Val, StrBoolType Type) const;
    JsonFile CopyAndSwap(JsonObject Obj, bool Recurse) const;
    JsonFile RedirectAndSwap(JsonObject Obj);
    void SwapObject(JsonObject Obj) const;

    //给非空的Object
    void SetNull() const;
    void SetInt(int Val) const;
    void SetDouble(double Val) const;
    void SetString(const std::string& Val) const;
    void SetBool(bool Val) const;
    void SetStrBool(bool Val, StrBoolType Type) const;
    void CopyObject(JsonObject Obj, bool Recurse) const;
    void RedirectObject(JsonObject Obj);
    void SetObject();

    //给空节点的非const函数
    void CreateNull();
    void CreateInt(int Val);
    void CreateDouble(double Val);
    void CreateString(const std::string& Val);
    void CreateBool(bool Val);
    void CreateStrBool(bool Val, StrBoolType Type);
    void CreateCopy(JsonObject Obj, bool Recurse);
    void CreateObject();

    // 不知道是否为空时请调用以下函数 
    void SetOrCreateNull();
    void SetOrCreateInt(int Val);
    void SetOrCreateDouble(double Val);
    void SetOrCreateString(const std::string& Val);
    void SetOrCreateBool(bool Val);
    void SetOrCreateStrBool(bool Val, StrBoolType Type);
    void SetOrCreateCopy(JsonObject Obj, bool Recurse);
    void SetOrCreateObject();

    JsonFile DetachObjectItem(const std::string& Str);
    JsonFile DetachArrayItem(int Index);
    void DeleteObjectItem(const std::string& Str) { cJSON_DeleteItemFromObject(Object, Str.c_str()); }
    void DeleteArrayItem(int Index) { cJSON_DeleteItemFromArray(Object, Index); }

    //改变当前Object，合入的Json本身不变，数据仅复制
    //不需要可用性检查
    void Merge(JsonObject Obj);
};

class JsonFile
{
private:
    cJSON* File{ nullptr };
public:
    JsonFile(cJSON* _F) { File = _F; }
    JsonFile() : File(nullptr) {}
    JsonFile(JsonObject Obj) { File = Obj.GetRaw(); }
    ~JsonFile() { if (File != nullptr)cJSON_Delete(File); }
    JsonFile(const JsonFile&) = delete;
    JsonFile& operator=(const JsonFile&) = delete;
    JsonFile(JsonFile&& r) noexcept : File(r.File) { r.File = nullptr; }


    JsonObject GetObj() { if (!File) { File = cJSON_CreateNull(); } return JsonObject(File); }
    operator JsonObject() { return GetObj(); }
    

    cJSON* GetRaw() const { return File; }
    cJSON* Release() { auto _F = File; File = nullptr; return _F; }

    bool Available() const { return File != nullptr; }
    JsonFile Duplicate(bool Recurse) const { return cJSON_Duplicate(File, Recurse); }
    void Clear() { if (File != nullptr)cJSON_Delete(File); File = nullptr; }

    void Parse(std::string Str);
    void ParseWithOpts(std::string Str, const char** ReturnParseEnd, int RequireNullTerminated);
    void ParseFromFile(const char* FileName);
    void ParseFromFileWithOpts(const char* FileName, int RequireNullTerminated);
    std::string ParseTmpChecked(std::string&& Str, const std::string& ErrorTip, int* ErrorPosition);
    std::string ParseFromFileChecked(const char* FileName, const std::string& ErrorTip, int* ErrorPosition);
};

inline const char* Json_GetErrorPtr() { return cJSON_GetErrorPtr(); }
inline void Json_InitHooks(cJSON_Hooks* hooks) { cJSON_InitHooks(hooks); }

bool RegexFull_Nothrow(const std::string& Str, const std::string& Regex) throw();
bool RegexFull_Throw(const std::string& Str, const std::string& Regex);
bool RegexNone_Nothrow(const std::string& Str, const std::string& Regex) throw();
bool RegexNone_Throw(const std::string& Str, const std::string& Regex);
bool RegexNotFull_Nothrow(const std::string& Str, const std::string& Regex) throw();
bool RegexNotFull_Throw(const std::string& Str, const std::string& Regex);
bool RegexNotNone_Nothrow(const std::string& Str, const std::string& Regex) throw();
bool RegexNotNone_Throw(const std::string& Str, const std::string& Regex);

JsonFile GetArrayOfObjects(std::vector<JsonFile>&&);

#define _FAKE_DWORD unsigned long
bool IBG_SuspendThread(_FAKE_DWORD ThreadID);
bool IBG_ResumeThread(_FAKE_DWORD ThreadID);
