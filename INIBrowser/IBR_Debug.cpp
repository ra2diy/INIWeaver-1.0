#include "IBR_Components.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include<imgui_internal.h>
#include "IBB_ModuleAlt.h"
#include "IBR_Debug.h"
#include "IBR_LinkNode.h"

namespace PreLink
{
    void CleanUp();
}

int IBG_ExitProcess()
{
    /*
    if (EnableLog)
    {
        GlobalLog.AddLog_CurTime(false);
        GlobalLog.AddLog("程序正在结束运行。");
    }
    //*/

    IBF_Inst_Setting.SaveSetting(IBR_Inst_Setting.SettingName);

    IBR_DynamicData::Save();

    /*
    if (EnableLog)
    {
        GlobalLog.AddLog_CurTime(false);
        GlobalLog.AddLog("程序已经结束运行。");
    }
    //*/

    return 0;
}

//修复关闭延时的Bug
struct OnClose
{
    ~OnClose()
    {
        IBG_ExitProcess();
    }
};

namespace IBR_WorkSpace
{
    extern ImVec2 DragStartMouse, DragStartEqCenter, DragStartReCenter;
    extern ImVec2 ExtraMove, MousePosExt, DragStartEqMouse, DragCurMouse, DragCurEqMouse;
    extern int UpdatePrevResult;
    extern float NewRatio;
    extern bool NeedChangeRatio, HoldingModules, InitHolding, IsMassSelecting, IsMassAfter, HasRightDownToWait, HasLefttDownToWait, MoveAfterMass;
    extern bool LastClickable, LastOnWindow, LastCont, Cont;
    extern bool OnCombo, OnPopupMenu;
    extern ImVec2 MassAfter_RightDownPos;
}


void IBR_Debug::AddMsgCycle(const StdMessage& Msg)
{
    DebugVec.push_back(Msg);
}
void IBR_Debug::AddMsgOnce(const StdMessage& Msg)
{
    DebugVecOnce.push_back(Msg);
}
void IBR_Debug::ClearMsgCycle()
{
    DebugVec.clear();
}

void IBR_Debug::RenderUI()
{
    //_CALL_CMD IBR_Inst_Project.GetText(true);
    static bool Ext = false;
    ImGui::Checkbox(locc("GUI_DebugOutputExtra"), &Ext);
    if (ImGui::Button(locc("GUI_DebugCopyOutput")))
    {
        ImGui::LogToClipboard();
        ImGui::LogText(_CALL_CMD IBR_Inst_Project.GetText(Ext).c_str());
        ImGui::LogFinish();
    }

    ImGui::Checkbox(locc("GUI_DebugModuleProperties"), &UseModuleProperties);
    ImGui::Checkbox(locc("GUI_DebugShowDecisionBox"), &ShowWorkspaceWindowFrame);
    ImGui::Checkbox(locc("GUI_DebugNoEnterEdit"), &DontGoToEdit);
    ImGui::Checkbox(locc("GUI_DebugCrazyRendering"), &DontDrawBg);
    ImGui::Checkbox(locc("GUI_DebugLinkInspect"), &LinkDebugMode);

    if (ImGui::Button(locc("GUI_DebugClipboard2Json")))
    {
        auto Clip = ImGui::GetClipboardText();
        IBB_ClipBoardData CB;
        if (CB.SetString(Clip))
        {
            auto J = CB.ToJson();
            ImGui::LogToClipboard();
            ImGui::LogText(J.GetObj().PrintData().c_str());
            ImGui::LogFinish();
        }
    }

    InputTextStdString(locc("GUI_DebugStringPoolQuery"), PoolQueryBuf);
    if (ImGui::SmallButton(locc("GUI_DebugStringToID")))
    {
        auto Result = NewPoolStr(PoolQueryBuf);
        char Res[32]{};
        _itoa(Result, Res, 16);
        LastQueryResult = loc("GUI_DebugStringToID") + " : " + PoolQueryBuf + " -> " + Res;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(locc("GUI_DebugIDToString")))
    {
        auto Result = PoolStr(strtol(PoolQueryBuf.c_str(), nullptr, 16));
        LastQueryResult = loc("GUI_DebugIDToString") + " : " + PoolQueryBuf + " -> " + Result;
    }
    ImGui::TextWrappedEx(LastQueryResult.c_str());

    if (ImGui::SmallButton(locc("GUI_DebugTriggerRefreshLink")))
    {
        IBR_Inst_Project.TriggerRefreshLink();
    }


    auto& mardp = IBR_WorkSpace::MassAfter_RightDownPos;
    auto msp = ImGui::GetMousePos();

    if(ImGui::TreeNode(locc("GUI_DebugUIState")))
    {
        ImGui::Text("MousePos = (%.1f,%.1f)", msp.x, msp.y);
        if (mardp.x == FLT_MAX || mardp.y == FLT_MAX || mardp.x == -FLT_MAX || mardp.y == -FLT_MAX)ImGui::Text("MassAfter_RightDownPos = INVALID");
        else ImGui::Text("MassAfter_RightDownPos = (%.1f, %.1f)", mardp.x, mardp.y);
        ImGui::Text("Current EqMax = (%.1f, %.1f)", IBR_FullView::CurrentEqMax.x, IBR_FullView::CurrentEqMax.y);
        ImGui::Text("ScreenSize = (%d, %d)", IBR_UICondition::CurrentScreenWidth, IBR_UICondition::CurrentScreenHeight);
        ImGui::Text("IsBgDragging = %s", IBR_WorkSpace::IsBgDragging ? "true" : "false");
        ImGui::Text("HoldingModules = %s", IBR_WorkSpace::HoldingModules ? "true" : "false");
        ImGui::Text("IsMassSelecting = %s", IBR_WorkSpace::IsMassSelecting ? "true" : "false");
        ImGui::Text("IsMassAfter = %s", IBR_WorkSpace::IsMassAfter ? "true" : "false");
        ImGui::Text("HasRightDownToWait = %s", IBR_WorkSpace::HasRightDownToWait ? "true" : "false");
        ImGui::Text("HasLeftDownToWait = %s", IBR_WorkSpace::HasLefttDownToWait ? "true" : "false");
        ImGui::Text("MoveAfterMass = %s", IBR_WorkSpace::MoveAfterMass ? "true" : "false");
        ImGui::Text("LastClickable = %s", IBR_WorkSpace::LastClickable ? "true" : "false");
        ImGui::Text("LastOnWindow = %s", IBR_WorkSpace::LastOnWindow ? "true" : "false");
        ImGui::Text("LastCont = %s", IBR_WorkSpace::LastCont ? "true" : "false");
        ImGui::Text("OnCombo = %s", IBR_WorkSpace::OnCombo ? "true" : "false");
        ImGui::Text("OnPopupMenu = %s", IBR_WorkSpace::OnPopupMenu ? "true" : "false");
        ImGui::Text("HasDragNow = %s", LinkNodeContext::HasDragNow ? "true" : "false");
        ImGui::Text("DblClickLeft = %s", ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ? "true" : "false");
        ImGui::Text("CTRL = %s", ImGui::GetIO().KeyCtrl ? "true" : "false");
        ImGui::Text("SHIFT = %s", ImGui::GetIO().KeyShift ? "true" : "false");
        ImGui::Text("ALT = %s", ImGui::GetIO().KeyAlt ? "true" : "false");
        ImGui::Text("SUPER = %s", ImGui::GetIO().KeySuper ? "true" : "false");

        ImGui::TreePop();
    }

    if (ImGui::TreeNode(locc("GUI_DebugRealTimeInfo")))
    {
        for (auto x : DebugVec)x();
        ImGui::TreePop();
    }

    if (ImGui::Button(locc("GUI_DebugClearOnceInfo")))DebugVecOnce.clear();
    if (ImGui::TreeNode(locc("GUI_DebugOnceInfo")))
    {
        for (auto x : DebugVecOnce)x();
        ImGui::TreePop();
    }
}

void IBR_Debug::RenderUIOnce()
{

}

void IBR_Debug::DebugLoad()
{

}

void IBR_Debug::RenderOnWorkSpace()
{

}
