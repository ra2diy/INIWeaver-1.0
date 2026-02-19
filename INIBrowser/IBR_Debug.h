#pragma once
#include "FromEngine/Include.h"


struct IBR_Debug
{
    struct _Data
    {
        bool Nope;
    }Data;
    bool UseModuleProperties{ false };
    bool ShowWorkspaceWindowFrame{ false };
    bool DontGoToEdit{ false };
    bool DontDrawBg{ false };
    std::vector<StdMessage>DebugVec, DebugVecOnce;
    void AddMsgCycle(const StdMessage& Msg);
    void ClearMsgCycle();
    void AddMsgOnce(const StdMessage& Msg);

    void DebugLoad();

    void RenderUI();
    void RenderUIOnce();
    void RenderOnWorkSpace();
};
