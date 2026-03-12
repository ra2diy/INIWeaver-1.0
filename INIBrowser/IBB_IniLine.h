#pragma once
#include "IBB_Ini.h"
#include "IBG_InputType.h"

struct IBB_IniLine_Data_String final : public IBB_IniLine_Data_Base
{
    std::string Value{};
    IICStatus Status_Workspace, Status_Sidebar;

    IBB_IniLine_Data_String() {}

    bool SetValue(const std::string& Val);
    bool MergeValue(const std::string& Val);
    bool Clear();
    void RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace);

    bool FirstIsLink() const;
    std::string GetString() const { return Value; }
    std::string GetStringForExport() const;

    virtual ~IBB_IniLine_Data_String() {}
};

struct IBB_IniLine_Data_Bool final : public IBB_IniLine_Data_Base
{
    bool Value { false };
    StrBoolType Type { StrBoolType::Str_yes_no };

    IBB_IniLine_Data_Bool() {}

    bool SetValue(const std::string& Val);
    bool MergeValue(const std::string& Val);
    bool Clear();
    void RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace);

    bool FirstIsLink() const;
    std::string GetString() const;
    std::string GetStringForExport() const;

    virtual ~IBB_IniLine_Data_Bool() {}
};

struct IBB_IniLine_Data_IIF final : public IBB_IniLine_Data_Base
{
    IIFPtr Value;
    IBB_IniLine_Data_IIF(const IBB_IniLine_Default* Default);

    bool SetValue(const std::string& Val);
    bool MergeValue(const std::string& Val);
    bool Clear();
    void RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace);

    bool FirstIsLink() const;
    std::string GetString() const;
    std::string GetStringForExport() const;

    virtual ~IBB_IniLine_Data_IIF() {}
};
