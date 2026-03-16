#pragma once

#include "FromEngine/Include.h"
#include "cjson/cJSON.h"

#ifndef _TEXT_UTF8
#define _TEXT_UTF8
#endif

struct IBG_SettingPack;

struct IBB_Section;
struct IBB_Ini;
struct IBB_Project;
struct IBB_Project_Index;
struct IBB_SectionID;
struct IBB_VariableList;
struct IBB_Link;
struct IBB_Section_Desc;

struct IBB_Module_Default;
struct IBB_Module;
struct IBB_Module_ParagraphList;

struct PairClipString;



using LinkSrcIdx = std::tuple<size_t, size_t, size_t>;//LineIdx, LineMult, ComponentIdx
using LinkSrcCIter = std::multimap<LinkSrcIdx, size_t>::const_iterator;
inline size_t GetLineIdx(const LinkSrcIdx& i) { return std::get<0>(i); }
inline size_t GetLineMult(const LinkSrcIdx& i) { return std::get<1>(i); }
inline size_t GetComponentIdx(const LinkSrcIdx& i) { return std::get<2>(i); }

namespace std
{
    template<>
    struct hash<LinkSrcIdx>
    {
        size_t operator()(const LinkSrcIdx& ID) const
        {
            size_t i = 0, h;
            h = std::hash<size_t>{}(std::get<0>(ID));
            i ^= h + 0x9e3779b9 + (i << 6) + (i >> 2);
            h = std::hash<size_t>{}(std::get<1>(ID));
            i ^= h + 0x9e3779b9 + (i << 6) + (i >> 2);
            h = std::hash<size_t>{}(std::get<2>(ID));
            i ^= h + 0x9e3779b9 + (i << 6) + (i >> 2);
            return i;
        }
    };
}


template<typename Str>
struct IBB_TDIndex
{
    bool UseIndex;
    int Index;
    Str Name;

    bool operator==(const IBB_TDIndex<Str>& A) const
    {
        if (A.UseIndex != UseIndex)return false;
        if (UseIndex)return Index == A.Index;
        else return Name == A.Name;
    }

    IBB_TDIndex() :UseIndex(false), Index(0), Name("") {}
    IBB_TDIndex(int _Index) :UseIndex(true), Index(_Index), Name() {}
    IBB_TDIndex(const Str& _Name) :UseIndex(false), Index(0), Name(_Name) {}
    IBB_TDIndex<Str>& Assign(int _Index) { UseIndex = true; Index = _Index; return *this; }
    IBB_TDIndex<Str>& Assign(const Str& _Name) { UseIndex = false; Name = _Name; return *this; }
    bool Load(JsonObject FromJson);

    template<typename T>
    typename std::vector<T>::iterator Search(std::vector<T>& Source, bool CanUseByName, bool Update, const std::function<Str(const T&)>& GetName);
    template<typename T>
    typename std::unordered_map<Str, T>::iterator Search(std::unordered_map<Str, T>& Source, bool CanUseByName, bool Update);
    template<typename T>
    typename std::vector<T>::iterator Search(std::vector<T>& Source, bool CanUseByName, const std::function<Str(const T&)>& GetName) const;
    template<typename T>
    typename std::unordered_map<Str, T>::iterator Search(std::unordered_map<Str, T>& Source, bool CanUseByName) const;
    template<typename T>
    typename std::vector<T>::const_iterator Search(const std::vector<T>& Source, bool CanUseByName, bool Update, const std::function<Str(const T&)>& GetName);
    template<typename T>
    typename std::unordered_map<Str, T>::const_iterator Search(const std::unordered_map<Str, T>& Source, bool CanUseByName, bool Update);
    template<typename T>
    typename std::vector<T>::const_iterator Search(const std::vector<T>& Source, bool CanUseByName, const std::function<Str(const T&)>& GetName) const;
    template<typename T>
    typename std::unordered_map<Str, T>::const_iterator Search(const std::unordered_map<Str, T>& Source, bool CanUseByName) const;
};


struct IBB_DIndex :public IBB_TDIndex<std::string>
{
    std::string GetText() const;
    IBB_DIndex() :IBB_TDIndex<std::string>() {}
    IBB_DIndex(int _Index) :IBB_TDIndex<std::string>(_Index) {}
    IBB_DIndex(const std::string& _Name) :IBB_TDIndex<std::string>(_Name) {}
};





struct IBB_Project_Index
{
    IBB_DIndex Ini;
    IBB_DIndex Section;

    IBB_Project_Index() {}
    IBB_Project_Index(const std::string& _Ini, const std::string& _Sec = "") :Ini(_Ini), Section(_Sec) {}
    IBB_Project_Index(const IBB_Section_Desc& Desc);
    IBB_Section_Desc ToDesc() const;
    PairClipString ToClipPair() const;

    operator IBB_Section_Desc() const;
    operator PairClipString() const;

    const IBB_Ini* GetIni(const IBB_Project& Proj);
    const IBB_Section* GetSec(const IBB_Project& Proj);
    const IBB_Ini* GetIni(const IBB_Project& Proj) const;
    const IBB_Section* GetSec(const IBB_Project& Proj) const;
    IBB_Ini* GetIni(IBB_Project& Proj);
    IBB_Section* GetSec(IBB_Project& Proj);
    IBB_Ini* GetIni(IBB_Project& Proj) const;
    IBB_Section* GetSec(IBB_Project& Proj) const;
    bool Empty() const;

    bool operator==(const IBB_Project_Index& A) const
    {
        return A.Ini == Ini && A.Section == Section;
    }

    std::string GetText() const;
};
bool operator<(const IBB_Project_Index& A, const IBB_Project_Index& B);




template<typename Str>
bool IBB_TDIndex<Str>::Load(JsonObject FromJson)
{
    auto Item = FromJson.GetObjectItem(u8"Index");
    if (Item.IsTypeString())
    {
        UseIndex = false;
        Name = Item.GetString();
        return true;
    }
    else if (Item.IsTypeNumber())
    {
        UseIndex = true;
        Index = Item.GetInt();
        return true;
    }
    else
    {
        return false;
    }
}

template<typename Str> template<typename T>
typename std::vector<T>::iterator IBB_TDIndex<Str>::Search(std::vector<T>& Source, bool CanUseByName, bool Update, const std::function<Str(const T&)>& GetName)
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            int i = 0;
            for (const auto& It : Source)
            {
                if (GetName(It) == Name)break;
                i++;
            }
            if (Update)Index = i;
            return Source.begin() + i;
        }
        else
        {
            Index = Source.size();
            return Source.end();
        }
    }
    else
    {
        typename std::vector<T>::iterator Data = Source.begin() + Index;
        if (Update)Name = GetName(*Data);
        return Data;
    }
}
template<typename Str> template<typename T>
typename std::unordered_map<Str, T>::iterator IBB_TDIndex<Str>::Search(std::unordered_map<Str, T>& Source, bool CanUseByName, bool Update)
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            auto Ret = Source.find(Name);
            if (Update)
            {
                if (Ret != Source.end())
                {
                    int i = 0;
                    for (const auto& It : Source)
                    {
                        if (It.first == Name)break;
                        i++;
                    }
                    Index = i;
                }
                else Index = Source.size();
            }
            return Ret;
        }
        else
        {
            Index = Source.size();
            return Source.end();
        }
    }
    else
    {
        int i = 0;
        Str Nr{};
        for (const auto& It : Source)
        {
            if (Index == i)
            {
                Nr = It.first;
                break;
            }
            i++;
        }
        if (Update && !Nr.empty())Name = Nr;
        return Source.find(Nr);
    }
}

template<typename Str> template<typename T>
typename std::vector<T>::iterator IBB_TDIndex<Str>::Search(std::vector<T>& Source, bool CanUseByName, const std::function<Str(const T&)>& GetName) const
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            int i = 0;
            for (const auto& It : Source)
            {
                if (GetName(It) == Name)break;
                i++;
            }
            return Source.begin() + i;
        }
        else return Source.end();
    }
    else
    {
        typename std::vector<T>::iterator Data = Source.begin() + Index;
        return Data;
    }
}
template<typename Str> template<typename T>
typename std::unordered_map<Str, T>::iterator IBB_TDIndex<Str>::Search(std::unordered_map<Str, T>& Source, bool CanUseByName) const
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            auto Ret = Source.find(Name);
            return Ret;
        }
        else return Source.end();
    }
    else
    {
        int i = 0;
        Str Nr{};
        for (const auto& It : Source)
        {
            if (Index == i)
            {
                Nr = It.first;
                break;
            }
            i++;
        }
        return Source.find(Nr);
    }
}

template<typename Str> template<typename T>
typename std::vector<T>::const_iterator IBB_TDIndex<Str>::Search(const std::vector<T>& Source, bool CanUseByName, bool Update, const std::function<Str(const T&)>& GetName)
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            int i = 0;
            for (const auto& It : Source)
            {
                if (GetName(It) == Name)break;
                i++;
            }
            if (Update)Index = i;
            return Source.cbegin() + i;
        }
        else
        {
            Index = Source.size();
            return Source.cend();
        }
    }
    else
    {
        typename std::vector<T>::const_iterator Data = Source.cbegin() + Index;
        if (Update)Name = GetName(*Data);
        return Data;
    }
}
template<typename Str> template<typename T>
typename std::unordered_map<Str, T>::const_iterator IBB_TDIndex<Str>::Search(const std::unordered_map<Str, T>& Source, bool CanUseByName, bool Update)
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            auto Ret = Source.find(Name);
            if (Update)
            {
                if (Ret != Source.cend())
                {
                    int i = 0;
                    for (const auto& It : Source)
                    {
                        if (It.first == Name)break;
                        i++;
                    }
                    Index = i;
                }
                else Index = Source.size();
            }
            return Ret;
        }
        else
        {
            Index = Source.size();
            return Source.cend();
        }
    }
    else
    {
        int i = 0;
        Str Nr{};
        for (const auto& It : Source)
        {
            if (Index == i)
            {
                Nr = It.first;
                break;
            }
            i++;
        }
        if (Update && !Nr.empty())Name = Nr;
        return Source.find(Nr);
    }
}

template<typename Str> template<typename T>
typename std::vector<T>::const_iterator IBB_TDIndex<Str>::Search(const std::vector<T>& Source, bool CanUseByName, const std::function<Str(const T&)>& GetName) const
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            int i = 0;
            for (const auto& It : Source)
            {
                if (GetName(It) == Name)break;
                i++;
            }
            return Source.cbegin() + i;
        }
        else return Source.cend();
    }
    else
    {
        typename std::vector<T>::const_iterator Data = Source.cbegin() + Index;
        return Data;
    }
}
template<typename Str> template<typename T>
typename std::unordered_map<Str, T>::const_iterator IBB_TDIndex<Str>::Search(const std::unordered_map<Str, T>& Source, bool CanUseByName) const
{
    if (!UseIndex)
    {
        if (CanUseByName)
        {
            auto Ret = Source.find(Name);
            return Ret;
        }
        else return Source.cend();
    }
    else
    {
        int i = 0;
        Str Nr{};
        for (const auto& It : Source)
        {
            if (Index == i)
            {
                Nr = It.first;
                break;
            }
            i++;
        }
        return Source.find(Nr);
    }
}

//这个东西实际上是IBB_Project_Index的代理。
struct IBB_SectionID
{
    uint64_t ID;

    bool operator==(const IBB_SectionID& A) const
    {
        return ID == A.ID;
    }

    bool operator<(const IBB_SectionID& A) const
    {
        return ID < A.ID;
    }

    IBB_SectionID() = default;
    IBB_SectionID(const IBB_SectionID&) = default;
    IBB_SectionID& operator=(const IBB_SectionID&) = default;
    IBB_SectionID(uint64_t _ID) :ID(_ID) {}
    IBB_SectionID(const IBB_Section_Desc& Desc);
    IBB_SectionID(const std::string& Ini, const std::string& Sec = "");


    const IBB_Ini* GetIni(const IBB_Project& Proj);
    const IBB_Section* GetSec(const IBB_Project& Proj);
    const IBB_Ini* GetIni(const IBB_Project& Proj) const;
    const IBB_Section* GetSec(const IBB_Project& Proj) const;
    IBB_Ini* GetIni(IBB_Project& Proj);
    IBB_Section* GetSec(IBB_Project& Proj);
    IBB_Ini* GetIni(IBB_Project& Proj) const;
    IBB_Section* GetSec(IBB_Project& Proj) const;
    bool Empty() const;

    std::string GetText() const;
    std::string Ini() const;
    std::string Section() const;

    IBB_Section_Desc ToDesc() const;
    PairClipString ToClipPair() const;
    IBB_Project_Index& ToIndex() const;

    operator IBB_Section_Desc() const;
    operator PairClipString() const;

    static void ClearCache();
};

namespace std
{
    template<>
    struct hash<IBB_SectionID>
    {
        size_t operator()(const IBB_SectionID& ID) const
        {
            return hash<uint64_t>()(ID.ID);
        }
    };
}

struct LinkNodeSetting
{
    StrPoolID LinkType;
    int LinkLimit;//1 for single;0 for no links;-1 for unlimited;others for limited
    ImU32 LinkCol;

    bool Load(JsonObject Obj, bool* HasCustom = nullptr);
};
