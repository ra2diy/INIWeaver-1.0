#pragma once
#include "IBB_Ini.h"
#include "IBG_InputType.h"

struct IBB_IniLine_Data_String final : public IBB_IniLine_Data_Base
{
    std::string Value{};
    IICStatus Status_Workspace, Status_Sidebar;

    IBB_IniLine_Data_String(const IBG_InputType& inp);

    bool SetValue(const std::string& Val);
    bool Clear();
    void RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace);
    void Replace(size_t CompIdx, const std::string& OldName, const std::string& NewName);

    bool FirstIsLink() const;
    std::string GetString() const { return Value; }
    std::string GetStringForExport() const;
    IIFPtr GetNewIIF(IBB_IniLine_Default* Default) const;

    virtual ~IBB_IniLine_Data_String() {}
};

struct IBB_IniLine_Data_Bool final : public IBB_IniLine_Data_Base
{
    bool Value { false };
    StrBoolType Type { StrBoolType::Str_yes_no };

    IBB_IniLine_Data_Bool(StrBoolType type) : Type(type) {}
    IBB_IniLine_Data_Bool(const IBG_InputType& inp);

    bool SetValue(const std::string& Val);
    bool Clear();
    void RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace);
    void Replace(size_t CompIdx, const std::string& OldName, const std::string& NewName);

    bool FirstIsLink() const;
    std::string GetString() const;
    std::string GetStringForExport() const;
    IIFPtr GetNewIIF(IBB_IniLine_Default* Default) const;

    virtual ~IBB_IniLine_Data_Bool() {}
};

struct IBB_IniLine_Data_IIF final : public IBB_IniLine_Data_Base
{
    IIFPtr Value;
    IBB_IniLine_Data_IIF(const IBB_IniLine_Default* Default);

    bool SetValue(const std::string& Val);
    bool Clear();
    void RenderUI(IBB_IniLine_Default* Default, const LinkNodeSetting& LinkNode, bool IsWorkspace);
    void Replace(size_t CompIdx, const std::string& OldName, const std::string& NewName);

    bool FirstIsLink() const;
    std::string GetString() const;
    std::string GetStringForExport() const;
    IIFPtr GetNewIIF(IBB_IniLine_Default* Default) const;

    virtual ~IBB_IniLine_Data_IIF() {}
};
