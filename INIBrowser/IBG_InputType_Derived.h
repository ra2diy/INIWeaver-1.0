#pragma once
#include "IBG_InputType.h"

namespace ExportContext
{
    extern StrPoolID Key;
    extern size_t SameKeyIdx;//用于当Key重复时区分不同的Key
    extern bool OnExport;
}

struct IFC_PureText final : public IBB_FormatComponent
{
    IBB_ValueFormat Format;
    IFC_PureText(const std::string& Text);

    const IBB_ValueFormat& GetFormat();
};

struct IFC_LocalizedText final : public IBB_FormatComponent
{
    IBB_ValueFormat Format;
    std::string Text;
    std::string FallbackText;
    IFC_LocalizedText(const std::string& Key, const std::string& Fallback);

    const IBB_ValueFormat& GetFormat();
};

struct IFC_PrintF final : public IBB_FormatComponent
{
    IBB_ValueFormat Format;
    IFC_PrintF(int ValueID, const std::string& Format);

    const IBB_ValueFormat& GetFormat();
};

struct IFC_StdFormat final : public IBB_FormatComponent
{
    IBB_ValueFormat Format;
    IFC_StdFormat(int ValueID, const std::string& Format);

    const IBB_ValueFormat& GetFormat();
};

struct IFC_ToString final : public IBB_FormatComponent
{
    IBB_ValueFormat Format;
    IFC_ToString(int ValueID);

    const IBB_ValueFormat& GetFormat();
};

struct IFC_Error final : public IBB_FormatComponent
{
    IBB_ValueFormat Format;
    std::wstring TextW;
    IFC_Error(const std::string& Text);

    const IBB_ValueFormat& GetFormat();
};

struct IFC_Export_UseKey final : public IBB_FormatComponent
{
    IBB_ValueFormat Format;
    IFC_Export_UseKey();

    const IBB_ValueFormat& GetFormat();
};


struct IIS_String final : public IBB_InputState
{
    std::string Text{};

    IIS_String() = default;
    IIS_String(const std::string& initialText);
    std::string Format(const IBB_InputFormat& Format);
    void Parse(const IBB_InputFormat& Format, const std::string& Value);
    IISPtr Duplicate() const;
    std::string TryAccept(const IBB_InputFormat& Format, std::string& Value);
};

struct IIS_Bool final : public IBB_InputState
{
    bool Value{};
    StrBoolType FmtType{ StrBoolType::Str_yes_no };
    IIS_Bool() = default;
    IIS_Bool(bool initialValue, StrBoolType fmtType);
    std::string Format(const IBB_InputFormat& Format);
    void Parse(const IBB_InputFormat& Format, const std::string& Value);
    IISPtr Duplicate() const;
    std::string TryAccept(const IBB_InputFormat& Format, std::string& Value);
    void SetFmt(StrBoolType fmtType) { FmtType = fmtType; }
};

struct IIS_Int final : public IBB_InputState
{
    int Value{};
    IIS_Int() = default;
    IIS_Int(int initialValue);
    std::string Format(const IBB_InputFormat& Format);
    void Parse(const IBB_InputFormat& Format, const std::string& Value);
    IISPtr Duplicate() const;
    std::string TryAccept(const IBB_InputFormat& Format, std::string& Value);
};


struct IIC_PureText final : public IBG_InputComponent
{
    std::string Text;
    ImColor Color;
    bool Colored;
    bool Disabled;
    bool Wrapped;
    IIC_PureText(const std::string& InitialText, ImColor color, bool colored, bool disabled, bool wrapped);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
    int GetCurrentTargetValueID() const { return -1; }
    bool SupportLinks() const { return false; }
};

struct IIC_LocalizedText final : public IBG_InputComponent
{
    std::string Key;
    std::string FallbackText;
    ImColor Color;
    bool Colored;
    bool Disabled;
    bool Wrapped;
    IIC_LocalizedText(const std::string& key, const std::string& Fallback, ImColor color, bool colored, bool disabled, bool wrapped);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
    int GetCurrentTargetValueID() const { return -1; }
    bool SupportLinks() const { return false; }
};

struct IIC_SameLine final : public IBG_InputComponent
{
    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
    int GetCurrentTargetValueID() const { return -1; }
    bool SupportLinks() const { return false; }
};

struct IIC_NewLine final : public IBG_InputComponent
{
    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
    int GetCurrentTargetValueID() const { return -1; }
    bool SupportLinks() const { return false; }
};

struct IIC_Separator final : public IBG_InputComponent
{
    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat&);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
    int GetCurrentTargetValueID() const { return -1; }
    bool SupportLinks() const { return false; }
};

struct IIC_InputText final : public IBG_InputComponent
{
    IICDescStr Hint;
    int ValueID;
    IIC_InputText(IBB_ValueContainer& Cont, int valueid, const std::string& InitialText, const IICDescStr& hint);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
    int GetCurrentTargetValueID() const { return ValueID; }
    bool SupportLinks() const { return true; }
};

struct IIC_MultipleChoice final : public IBG_InputComponent
{
    std::unordered_map<std::string, IICDescStr> Options;//AllowedValue : DisplayName ; if empty then any value allowed
    std::vector<std::string> OptionOrder;
    bool SameLine;
    int MaxInOneLine;//如果SameLine为true，设置每行最大选项数，超过则换行；如果SameLine为false，则此项无效
    std::string HintID;
    std::string Delim;
    int ValueID;
    IICDescStr Hint;
    IIC_MultipleChoice(IBB_ValueContainer& Cont, int valueid, const std::string& InitialText, const IICDescStr& Hint, const std::string& DelimStr, bool sameline, int MaxInOneLine, const std::unordered_map<std::string, IICDescStr>& options, const std::vector<std::string>& OptionOrder);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
    int GetCurrentTargetValueID() const { return ValueID; }
    bool SupportLinks() const { return false; }
};

struct IIC_EnumCombo final : public IBG_InputComponent
{
    std::unordered_map<std::string, IICDescStr> Options;//AllowedValue : DisplayName ; if empty then any value allowed
    std::vector<std::string> OptionOrder;
    IICDescStr Hint;
    int ValueID;
    IIC_EnumCombo(IBB_ValueContainer& Cont, int valueid, const std::string& InitialValue, const IICDescStr& hint, const std::unordered_map<std::string, IICDescStr>& options, const std::vector<std::string>& OptionOrder);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
    int GetCurrentTargetValueID() const { return ValueID; }
    bool SupportLinks() const { return false; }
};

struct IIC_EnumRadio final : public IBG_InputComponent
{
    std::unordered_map<std::string, IICDescStr> Options;//AllowedValue : DisplayName ; if empty then any value allowed
    std::vector<std::string> OptionOrder;
    std::string Hint;
    bool SameLine;
    int MaxInOneLine;//如果SameLine为true，设置每行最大选项数，超过则换行；如果SameLine为false，则此项无效
    int ValueID;
    IIC_EnumRadio(IBB_ValueContainer& Cont, int valueid, const std::string& InitialValue, const std::unordered_map<std::string, IICDescStr>& options, bool sameline, int MaxInOneLine, const std::vector<std::string>& OptionOrder);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
    int GetCurrentTargetValueID() const { return ValueID; }
    bool SupportLinks() const { return false; }
};

struct IIC_Bool final : public IBG_InputComponent
{
    int ValueID;
    IICDescStr Hint;
    StrBoolType FmtType;
    IIC_Bool(IBB_ValueContainer& Cont, int valueid, bool InitialValue, StrBoolType fmt, const IICDescStr& hint);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
    int GetCurrentTargetValueID() const { return ValueID; }
    bool SupportLinks() const { return false; }
};

struct IIC_InputInt final : public IBG_InputComponent
{
    int Min, Max;
    IICDescStr Hint;
    int ValueID;
    IIC_InputInt(IBB_ValueContainer& Cont, int valueid, int InitialValue, int min, int max, const IICDescStr& hint);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
    int GetCurrentTargetValueID() const { return ValueID; }
    bool SupportLinks() const { return true; }
};

struct IIC_ColorPanel final : public IBG_InputComponent
{
    enum _Mode {
        RGB,
        BGR,
        HSV
    }Mode;
    std::string Hint;
    int ValueID1, ValueID2, ValueID3;
    IIC_ColorPanel() { ValueID1 = 0; ValueID2 = 1; ValueID3 = 2; Hint = "TEST"; Mode = _Mode::RGB; }
    //IIC_ColorPanel(IBB_ValueContainer& Cont, int valueid, int InitialValue, int min, int max, const std::string& hint);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
    int GetCurrentTargetValueID() const { return -1; }
    bool SupportLinks() const { return false; }
};

struct IIC_SliderInt final : public IBG_InputComponent
{
    int Min, Max;
    IICDescStr Hint;
    std::string SlideFormat;
    bool Logarithmic;
    int ValueID;
    IIC_SliderInt(IBB_ValueContainer& Cont, int valueid, int InitialValue, int min, int max, const IICDescStr& hint, const std::string& slidefmt, bool log);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
    int GetCurrentTargetValueID() const { return ValueID; }
    bool SupportLinks() const { return false; }
};

struct IIC_Error final : public IBG_InputComponent
{
    std::wstring TextW;
    IIC_Error(const std::string& Desc);
    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status);
    std::string FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
    int GetCurrentTargetValueID() const { return -1; }
    bool SupportLinks() const { return false; }
};


std::string IIC_Formatter(const std::string& Value, const IBB_InputFormat& Format);
std::string IIC_Formatter(bool Value, const IBB_InputFormat& Format, StrBoolType FmtType);
std::string IIC_Formatter(int Value, const IBB_InputFormat& Format);
void IIC_Parser_String(const std::string& StrValue, const IBB_InputFormat& Format, std::string& OutValue);
void IIC_Parser_Bool(const std::string& StrValue, const IBB_InputFormat& Format, bool& OutValue);
void IIC_Parser_Int(const std::string& StrValue, const IBB_InputFormat& Format, int& OutValue);
std::string IIC_Accepter_String(const IBB_InputFormat& Format, std::string& Value, std::string& OutValue);
std::string IIC_Accepter_Bool(const IBB_InputFormat& Format, std::string& Value, bool& OutValue);
std::string IIC_Accepter_Int(const IBB_InputFormat& Format, std::string& Value, int& OutValue);
