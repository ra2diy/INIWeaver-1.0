#pragma once
#include "FromEngine/Include.h"

struct IBB_InputFormat
{
    enum _Type
    {
        UseFormat,
        ToString,
        PrintF,
        StdFormat
    }Type;

    std::string String;
};

struct IBB_ValueFormat
{
    IBB_InputFormat Format;
    int ValueID;

    IBB_ValueFormat(IBB_InputFormat::_Type type, const std::string& String);
    IBB_ValueFormat(IBB_InputFormat::_Type type, const std::string& String, int id);
};

struct IBB_UpdateResult
{
    bool Updated;
    bool Active;
    int ValueID;
};

struct IBG_InputComponent;
struct IBB_FormatComponent;
struct IBB_ValueContainer;
struct IBB_InputState;
using IICPtr = std::shared_ptr<IBG_InputComponent>;
using IFCPtr = std::shared_ptr<IBB_FormatComponent>;
using IISPtr = std::unique_ptr<IBB_InputState>;

struct IBG_InputComponent : public std::enable_shared_from_this<IBG_InputComponent>
{
    virtual ~IBG_InputComponent() = default;
    virtual IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont) = 0;
    virtual std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format) = 0;
    virtual void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format) = 0;
    virtual bool CanProvideState(IBB_ValueContainer& Cont) const = 0;
    virtual void ResetState(IBB_ValueContainer& Cont) const = 0;
};

struct IBB_FormatComponent : public std::enable_shared_from_this<IBB_FormatComponent>
{
    virtual ~IBB_FormatComponent() = default;
    virtual const IBB_ValueFormat& GetFormat() = 0;
};

struct IBB_InputState
{
    virtual ~IBB_InputState() = default;
    virtual std::string Format(const IBB_InputFormat& Format) = 0;
    virtual void Parse(const IBB_InputFormat& Format, const std::string& Value) = 0;
    virtual IISPtr Duplicate() const = 0;
    virtual std::string TryAccept(const IBB_InputFormat& Format, std::string& Value) = 0;
};

struct IBB_InputUpdateInfo
{
    IBB_ValueContainer* Cont;
    IBG_InputComponent* Source;
};

struct IBB_InputValue
{
    std::string Value;
    bool Dirty{ true };
    IBB_InputUpdateInfo LastUpdate;
    IISPtr StateValPtr;

    template<typename T>
    bool CorrectState() const
    {
        if (!Dirty) return true;
        if (!LastUpdate.Cont || !LastUpdate.Source) return false;
        return dynamic_cast<T*>(StateValPtr.get()) != nullptr;
    }

    template<typename T>
    T* StateValue() const
    {
        return dynamic_cast<T*>(StateValPtr.get());
    }

    template<typename T, typename... TArgs>
    void ResetState(TArgs&&... Args)
    {
        StateValPtr = std::make_unique<T>(std::forward<TArgs>(Args)...);
    }


    IBB_InputValue() = default;
    IBB_InputValue(IBB_InputValue&&) = default;
    IBB_InputValue(const IBB_InputValue&);

    void NeedsUpdate(IBB_ValueContainer& Cont, IBG_InputComponent& Source);
    void UpdateValue(const IBB_InputFormat& Format);
};



struct InputFormComponentFactory
{
    static IICPtr CreateInputComponent(IBB_ValueContainer& Cont, const JsonObject& Obj);
    static IFCPtr CreateFormatComponent(IBB_ValueContainer& Cont, const JsonObject& Obj);

    static IICPtr GetParseErrorInputComponent(const std::string& Desc);
    static IFCPtr GetParseErrorFormatComponent(const std::string& Desc);
};

struct IBB_ValueContainer
{
    std::map<int, IBB_InputValue> Values;
    IBB_InputValue& GetValue(int ValueID);
    void Clear();
};

struct IBG_InputFormUIResult
{
    bool Changed;
    bool Active;
};

struct IBG_InputForm;
using IIFPtr = std::unique_ptr<IBG_InputForm>;

struct IBG_InputForm
{
    std::shared_ptr<std::vector<IICPtr>> InputComponents;
    std::shared_ptr<std::vector<IFCPtr>> FormatComponents;
    
    IBB_InputValue& GetValue(int ValueID);
    IBG_InputFormUIResult RenderUI(); //returns whether any value changed
    const std::string& GetFormattedString();
    void ParseFromString(const std::string& Str);
    void ResetState();
    bool Load(const JsonObject& Obj);
    IIFPtr Duplicate() const;

private:
    std::string FormattedString;
    IBB_ValueContainer ValueContainer;
    bool Dirty{ true };
};



struct IBG_InputType
{
    enum _Type
    {
        Link,
        Form
    }Type;
    IIFPtr Sidebar;
    IIFPtr WorkSpace;
    bool Load(const JsonObject& Obj);

    IBG_InputType(const IBG_InputType&);
    IBG_InputType() = default;
    IBG_InputType(IBG_InputType&&) = default;
};





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

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
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

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
};

struct IIC_SameLine final : public IBG_InputComponent
{
    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
};

struct IIC_NewLine final : public IBG_InputComponent
{
    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
};

struct IIC_Separator final : public IBG_InputComponent
{
    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
};

struct IIC_InputText final : public IBG_InputComponent
{
    std::string Hint;
    int ValueID;
    IIC_InputText(IBB_ValueContainer& Cont, int valueid, const std::string& InitialText, const std::string& hint);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
};

struct IIC_EnumCombo final : public IBG_InputComponent
{
    std::unordered_map<std::string, std::string> Options;//AllowedValue : DisplayName ; if empty then any value allowed
    std::string Hint;
    int ValueID;
    IIC_EnumCombo(IBB_ValueContainer& Cont, int valueid, const std::string& InitialValue, const std::string& hint, const std::unordered_map<std::string, std::string>& options);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
};

struct IIC_EnumRadio final : public IBG_InputComponent
{
    std::unordered_map<std::string, std::string> Options;//AllowedValue : DisplayName ; if empty then any value allowed
    std::string Hint;
    bool SameLine;
    int ValueID;
    IIC_EnumRadio(IBB_ValueContainer& Cont, int valueid, const std::string& InitialValue, const std::unordered_map<std::string, std::string>& options, bool sameline);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
};

struct IIC_Bool final : public IBG_InputComponent
{
    int ValueID;
    std::string Hint;
    StrBoolType FmtType;
    IIC_Bool(IBB_ValueContainer& Cont, int valueid, bool InitialValue, StrBoolType fmt, const std::string& hint);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
};

struct IIC_InputInt final : public IBG_InputComponent
{
    int Min, Max;
    std::string Hint;
    int ValueID;
    IIC_InputInt(IBB_ValueContainer& Cont, int valueid, int InitialValue, int min, int max, const std::string& hint);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
};

struct IIC_SliderInt final : public IBG_InputComponent
{
    int Min, Max;
    std::string Hint;
    std::string SlideFormat;
    bool Logarithmic;
    int ValueID;
    IIC_SliderInt(IBB_ValueContainer& Cont, int valueid, int InitialValue, int min, int max, const std::string& hint, const std::string& slidefmt, bool log);

    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    bool CanProvideState(IBB_ValueContainer& Cont) const;
    void ResetState(IBB_ValueContainer& Cont) const;
};

struct IIC_Error final : public IBG_InputComponent
{
    std::wstring TextW;
    IIC_Error(const std::string& Desc);
    IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont);
    std::string FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format);
    void ParseValue(IBB_ValueContainer&, const IBB_InputFormat&) {}
    bool CanProvideState(IBB_ValueContainer&) const { return true; }
    void ResetState(IBB_ValueContainer&) const {}
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


StrBoolType StrBoolTypeFromString(const std::string& str, StrBoolType Default);
bool InputTextStdString(const char* label, std::string& str, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
