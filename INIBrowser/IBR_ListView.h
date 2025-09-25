#pragma once
#include "IBB_Index.h"
#include <memory>

bool StrCmpZHCN(const std::string& l, const std::string& r);
bool StringMatch(std::string str, std::string match, bool Full, bool CaseSensitive, bool Regex);

namespace IBR_ListView
{
    enum class SortBy
    {
        Default,
        RegName,
        DisplayName,
        RegType,
        COUNT
    };
    const static int SortTypeCount = static_cast<int>(SortBy::COUNT);
    extern std::vector<IBB_Project_Index> CurrentList;

    void RenderUI();


    inline namespace __
    {
        BufString _TEXT_UTF8& GetSearchBuffer();
        void SetSortBy(SortBy Type);
        void InitSort();
        void ClearSort();
        const char* GetSortName(SortBy Type);
        const char* GetCurrentSortName();
        SortBy GetCurrentSortBy();
        void Reverse();

        //false 为升序 true 为降序
        bool IsReversed();
    }
}
