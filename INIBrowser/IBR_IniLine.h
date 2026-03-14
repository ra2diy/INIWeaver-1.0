#pragma once
#include "IBG_InputType.h"
#include "IBR_LinkNode.h"
#include "IBB_PropStringPool.h"


struct IBR_IniLine
{
    static void RenderUI(const char* Line, const char* Hint, IBB_IniLine& Back, bool IsWorkSpace);
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
    static IBR_IniLine Edit;
    ImVec2 AcceptCenter{ 0.0f, 0.0f };
    int AcceptCount{ 0 };
    bool Collapsed{ true };
    void RenderUI(const char* Line, const char* Hint, IBB_IniLine& Back);
};
