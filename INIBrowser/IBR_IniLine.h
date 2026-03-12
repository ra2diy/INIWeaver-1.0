#pragma once
#include "IBG_InputType.h"
#include "IBR_LinkNode.h"
#include "IBB_PropStringPool.h"


struct IBR_IniLine
{
    void RenderUI(const char* Line, const char* Hint, IBB_IniLine& Back, bool IsWorkSpace);
};

struct SidebarLine
{
    IBR_IniLine Edit;
    const IBG_InputType* InputType;
    LinkNodeSetting LinkNode;
    std::string Buffer;
    DescPoolOffset Hint;
    std::string OnShowBuf;
    bool InputOnShow;

    void RenderUI(const char* Line, const char* Hint, IBB_IniLine& Back);
};

struct WorkSpaceLine
{
    IBR_IniLine Edit;
    void RenderUI(const char* Line, const char* Hint, IBB_IniLine& Back);
};
