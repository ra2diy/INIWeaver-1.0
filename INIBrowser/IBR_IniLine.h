#pragma once
#include "IBG_InputType.h"
#include "IBR_LinkNode.h"
#include "IBB_PropStringPool.h"

struct IBR_InputManager
{
    using AfterInputType = std::function<void(const std::string&)>;
    IIFPtr Form;
    std::string ID;
    AfterInputType AfterInput;
    LinkNodeSetting LinkNode;

    IBR_InputManager(const std::string& InitialText, const std::string& id, const AfterInputType& Fn, IIFPtr&& InitialForm, LinkNodeSetting&& LNS);
    bool RenderUI();
    ~IBR_InputManager();
};

struct IBR_IniLine
{
    std::shared_ptr<IBR_InputManager> Input;
    bool HasInput{ false }, UseInput{ false };
    struct InitType
    {
        std::string InitText;
        std::string ID;
        IBR_InputManager::AfterInputType AfterInput;
        const IIFPtr& InitialForm;
        LinkNodeSetting LinkNode;

        InitType(const std::string& Text,
            const std::string& id,
            const IBR_InputManager::AfterInputType& Fn,
            const IIFPtr& Form,
            LinkNodeSetting&& LNS)
            :InitText(Text), ID(id), AfterInput(Fn), InitialForm(Form), LinkNode(std::move(LNS)) {
        }
    };
    bool NeedInit() { return HasInput && !Input; }
    void RenderUI(const std::string& Line, const char* Hint, InitType* Init = nullptr);
    void CloseInput();
};

struct BufferedLine
{
    IBR_IniLine Edit;
    const IBG_InputType* InputType;
    LinkNodeSetting LinkNode;
    std::string Buffer;
    DescPoolOffset Hint;
    std::string OnShowBuf;
    bool InputOnshow;
};

struct ActiveLine
{
    IBR_IniLine Edit;
};
