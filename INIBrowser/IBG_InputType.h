#pragma once
#include "IBG_InputType_Defines.h"
#include "FromEngine/Include.h"
#include "IBB_OutputFormat.h"
#include "IBR_LinkNode.h"
#include <variant>

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


struct IICStatus
{
    enum _ : uint8_t {
        Input,
        Link
    } InputMethod{
        Input
    };

    bool Load(const JsonObject& Obj);
};

struct IBG_InputComponent : public std::enable_shared_from_this<IBG_InputComponent>
{
    virtual ~IBG_InputComponent() = default;
    virtual IBB_UpdateResult RenderUI(IBB_ValueContainer& Cont, IICStatus& Status) = 0;
    virtual std::string FormatValue(IBB_InputValue&, const IBB_InputFormat&) = 0;
    virtual void ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format) = 0;
    virtual bool CanProvideState(IBB_ValueContainer& Cont) const = 0;
    virtual void ResetState(IBB_ValueContainer& Cont) const = 0;
    virtual int GetCurrentTargetValueID() const = 0;//注意！这个函数目前仅用于链接节点，如果扩展用途需要修改函数！
    virtual bool SupportLinks() const = 0;

    IICStatus InitialStatus;
    LinkNodeSetting NodeSetting;
    bool UseCustomSetting;//true : 保持自定义值 false : NodeSetting会随时刷新
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
    static IICPtr CreateInputComponent_Special(IBB_ValueContainer& Cont, const JsonObject& Obj);
    static IICPtr CreateInputComponent(IBB_ValueContainer& Cont, const JsonObject& Obj);
    static IFCPtr CreateFormatComponent(IBB_ValueContainer& Cont, const JsonObject& Obj);

    static IICPtr GetParseErrorInputComponent(const std::string& Desc);
    static IFCPtr GetParseErrorFormatComponent(const std::string& Desc);

    static IICVPtr CreateInputComponentVector(IBB_ValueContainer& Cont, const JsonObject& Obj, bool& HasError);
    static IFCVPtr CreateFormatComponentVector(IBB_ValueContainer& Cont, const JsonObject& Obj, bool& HasError);
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



struct IBG_InputForm
{
    IICVPtr InputComponents;
    IFCVPtr FormatComponents;
    
    IBB_InputValue& GetValue(int ValueID);
    IBG_InputFormUIResult RenderUI(const LinkNodeSetting& Default); //returns whether any value changed
    void CheckStatus();
    const std::string& GetFormattedString();
    void ParseFromString(const std::string& Str);
    void ResetState();
    bool Load(const JsonObject& Obj);
    IIFPtr Duplicate() const;
    void EnableLinkNode() { LinkNodeEnabled = true; }
    const std::string& RegenFormattedString() { Dirty = true; return GetFormattedString(); }

private:
    std::string FormattedString;
    IBB_ValueContainer ValueContainer;
    std::vector<IICStatus> ComponentStatus;
    bool Dirty{ true };
    bool LinkNodeEnabled{ false };

public:
    std::vector<IICStatus>& GetComponentStatus() { return ComponentStatus; }
    IBB_ValueContainer& GetValues() { return ValueContainer; }
    void SetValues(const IBB_ValueContainer& Cont) { ValueContainer = Cont; }
    void SetValues(IBB_ValueContainer&& Cont) { ValueContainer = std::move(Cont); }
};

using IIFWrapper = std::variant< IIFPtr, IIFPtr*, std::monostate >;
struct IIFWrapper_Wrapper
{
    IIFWrapper _;
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
    KVFormatter_t KVFmt;
    bool Load(const JsonObject& Obj);

    IBG_InputType(const IBG_InputType&);
    IBG_InputType() = default;
    IBG_InputType(IBG_InputType&&) = default;
};

struct IICDescStr
{
    std::string Short, Long;
    static IICDescStr Load(JsonObject Obj);
};

StrBoolType StrBoolTypeFromString(const std::string& str, StrBoolType Default);
bool InputTextStdString(const char* label, std::string& str, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
