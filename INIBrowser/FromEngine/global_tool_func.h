#pragma once

#include"types.h"
#include"..\cjson\cJSON.h"

#include<functional>
#include<string>
#include<algorithm>
#include<vector>
#include<utility>
#include<unordered_map>


hash_t StrHash(const std::string& __Str);
hash_t StrHash(const char* __Str);

char* RemoveFrontSpace(char* src);
void RemoveBackSpace(char* src);
void RemoveBackSpace(std::string& src);

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
};

class JsonObject
{
private:
    cJSON* Object{ nullptr };
public:
    JsonObject(cJSON* _F) { Object = _F; }
    JsonObject() : Object(nullptr) {}

    cJSON* GetRaw() const { return Object; }

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

#define XXXOR(NAME ,TYPE) \
    template<class... TArgs>\
    TYPE Item ## NAME ## Or(const std::string& Str, TArgs&&... Args) const\
    {\
        return HasItem(Str) ? Item ## NAME(Str) : TYPE{std::forward<TArgs>(Args)...};\
    }

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
};

class JsonFile
{
private:
    cJSON* File{ nullptr };
    JsonFile(cJSON* _F) { File = _F; }
public:
    JsonFile() : File(nullptr) {}
    JsonFile(JsonObject Obj) { File = Obj.GetRaw(); }
    ~JsonFile() { if (File != nullptr)cJSON_Delete(File); }

    operator JsonObject() const { return JsonObject(File); }
    JsonObject GetObj() const { return JsonObject(File); }

    cJSON* GetRaw() const { return File; }

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


#define _FAKE_DWORD unsigned long
bool IBG_SuspendThread(_FAKE_DWORD ThreadID);
bool IBG_ResumeThread(_FAKE_DWORD ThreadID);
