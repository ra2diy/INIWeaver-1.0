#pragma once
#include <memory>
#include <vector>
#include <optional>

class JsonObject;
struct IBG_InputComponent;
struct IBB_FormatComponent;
struct IBB_ValidateComponent;
struct IBB_ValueContainer;
struct IBB_InputState;
struct IBB_InputValue;
struct IBG_InputForm;
struct IBG_InputType;
struct LinkNodeSetting;
struct IBB_LineFormat;
struct IIT_AcceptorSetting;
using IICPtr = std::shared_ptr<IBG_InputComponent>;
using IFCPtr = std::shared_ptr<IBB_FormatComponent>;
using IISPtr = std::unique_ptr<IBB_InputState>;
using IIFPtr = std::unique_ptr<IBG_InputForm>;
using IICVPtr = std::shared_ptr<std::vector<IICPtr>>;
using IFCVPtr = std::shared_ptr<std::vector<IFCPtr>>;
using ILFVPtr = std::shared_ptr<std::vector<IBB_LineFormat>>;
using IASOpt = std::optional<IIT_AcceptorSetting>;

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
