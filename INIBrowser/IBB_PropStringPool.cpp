#include "IBB_PropStringPool.h"

const size_t StrPoolBaseSize = 1024;
const size_t DescPoolBaseSize = StrPoolBaseSize * 32;

const char* IBB_PropConstDescPool::Get(DescPoolOffset Offset) const
{
    if (Offset < Pool.size())return &Pool[Offset];
    else return "";
}
DescPoolOffset IBB_PropConstDescPool::Add(const std::string& Str)
{
    //这是专门压缩不重复字符串的字符串池
    //所以不需要检查是否已经存在，直接添加就行了
    auto Offset = Pool.size();
    Pool.insert(Pool.end(), Str.begin(), Str.end());
    Pool.push_back('\0');
    return Offset;
}
void IBB_PropConstDescPool::Clear()
{
    Pool.clear();
    Pool.reserve(DescPoolBaseSize);//预留一些空间，避免频繁扩容
}

std::string IBB_PropStringPool::Get(StrPoolID ID) const
{
    return std::string(Pool.Get((DescPoolOffset)ID));
}
StrPoolID IBB_PropStringPool::Add(const std::string& Str)
{
    size_t Hash = std::hash<std::string>{}(Str);
    auto itHash = std::find_if(Hashes.begin(), Hashes.end(), [Hash](const auto& H) { return H.first == Hash; });

    if (itHash != Hashes.end())
        return (StrPoolID)itHash->second;
    else
    {
        auto Offset = Pool.Add(Str);
        Hashes.emplace_back(Hash, Offset);
        return (StrPoolID)Offset;
    }
}
void IBB_PropStringPool::Clear()
{
    Pool.Clear();
}

IBB_PropConstDescPool IBB_Inst_DescPool;
IBB_PropStringPool IBB_Inst_StrPool;
