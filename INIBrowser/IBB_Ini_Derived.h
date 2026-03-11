#pragma once
#include "IBB_Ini.h"

struct IBB_IniLine_Data_String final : public IBB_IniLine_Data_Base
{
    std::string Value{};

    IBB_IniLine_Data_String() {}

    bool SetValue(const std::string& Val);
    bool MergeValue(const std::string& Val);
    bool MergeData(const IBB_IniLine_Data_Base* data);
    bool Clear();

    std::string GetString() const { return Value; }
    std::string GetStringForExport() const;

    virtual ~IBB_IniLine_Data_String() {}
};
