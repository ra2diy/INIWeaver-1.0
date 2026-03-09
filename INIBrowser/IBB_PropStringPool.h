#pragma once
#include "FromEngine/Include.h"


using DescPoolOffset = size_t;
using StrPoolID = size_t;

//压缩不重复字符串的字符串池，将大量互不相同的字符串合并，
//存储在一个连续的字符数组中，通过偏移量获取字符串引用
struct IBB_PropConstDescPool
{
    std::vector<char> Pool;
    //Get返回的引用可能会失效！请在调用Get后立即使用返回的引用，不要保存它！如果需要保存，请先复制一份！
    const char* Get(DescPoolOffset Offset) const;
    DescPoolOffset Add(const std::string& Str);
    void Clear();
};

extern IBB_PropConstDescPool IBB_Inst_DescPool;

//压缩重复字符串的字符串池，返回ID而不是引用，使用时通过ID获取字符串引用
struct IBB_PropStringPool
{
    std::vector<std::pair<size_t, DescPoolOffset>> Hashes;
    IBB_PropConstDescPool Pool;
    //Get返回的引用可能会失效！请在调用Get后立即使用返回的引用，不要保存它！如果需要保存，请先复制一份！
    std::string Get(StrPoolID ID) const;
    StrPoolID Add(const std::string& Str);
    void Clear();
};

extern IBB_PropStringPool IBB_Inst_StrPool;

#define NewPoolStr(Str) IBB_Inst_StrPool.Add(Str)
#define PoolStr(Str) IBB_Inst_StrPool.Get(Str)

#define NewPoolDesc(Str) IBB_Inst_DescPool.Add(Str)
#define PoolDesc(Str) IBB_Inst_DescPool.Get(Str)

