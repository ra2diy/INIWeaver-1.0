#pragma once
#include "IBG_InputType.h"
#include "IBR_LinkNode.h"
#include "IBB_PropStringPool.h"


struct IBR_IniLine
{
    static void RenderUI(
        const char* Line,
        const char* Hint,
        IBB_IniLine& Back,
        bool IsWorkSpace,
        bool* RightClicked,
        bool ExternalSwitchInput
    );
};

struct SidebarLine
{
    IBR_IniLine Edit;
    LinkNodeSetting LinkNode;
    DescPoolOffset Hint;
    std::string OnShowBuf;
    bool InputOnShow;

    void RenderUI(const char* Line, const char* Hint, IBB_IniLine& Back);
};

struct WorkSpaceLine
{
    static IBR_IniLine Edit;
    std::vector<ImVec2> AcceptCenter;
    int AcceptCount{ 0 };
    bool Collapsed{ true };
    bool SpecialAccept{ false };
    bool Highlight{ false };
    bool SwitchInput{ false };
    bool InputOnShow{ false };
    void RenderUI(const char* Line, const char* Hint, IBB_IniLine& Back);
};
