#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include<imgui_internal.h>
#include "IBB_ModuleAlt.h"

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

    if (ImGui::Button(u8"剪贴板 -> JSON"))
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

    auto& mardp = IBR_WorkSpace::MassAfter_RightDownPos;
    auto msp = ImGui::GetMousePos();

    ImGui::Text("MousePos = (%.1f,%.1f)", msp.x, msp.y);
    ImGui::Text("MassAfter_RightDownPos = (%.1f,%.1f)", mardp.x, mardp.y);
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
    ImGui::Text("DblClickLeft = %s", ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ? "true" : "false");
    ImGui::Text("CTRL = %s", ImGui::GetIO().KeyCtrl ? "true" : "false");
    ImGui::Text("SHIFT = %s", ImGui::GetIO().KeyShift ? "true" : "false");
    ImGui::Text("ALT = %s", ImGui::GetIO().KeyAlt ? "true" : "false");
    ImGui::Text("SUPER = %s", ImGui::GetIO().KeySuper ? "true" : "false");
    if (UICond.LoopShow) { if (ImGui::ArrowButton("loopc", ImGuiDir_Down))UICond.LoopShow = false; }
    else { if (ImGui::ArrowButton("loopa", ImGuiDir_Right))UICond.LoopShow = true; }
    if (UICond.LoopShow)for (auto x : DebugVec)x();

    if (UICond.OnceShow) { if (ImGui::ArrowButton("loopd", ImGuiDir_Down))UICond.OnceShow = false; }
    else { if (ImGui::ArrowButton("loopb", ImGuiDir_Right))UICond.OnceShow = true; }
    ImGui::SameLine();
    if (ImGui::Button(u8"CLEAR"))DebugVecOnce.clear();
    if (UICond.OnceShow)for (auto x : DebugVecOnce)x();
}

void IBR_Debug::RenderUIOnce()
{
    /*
    _CALL_CMD IBR_Inst_Project.AddModule(u8"GenUnit1", { u8"LBWNB" }, IBB_Section_DescNull);
    _CALL_CMD IBR_Inst_Project.AddModule(u8"Weapons", { u8"FuckPrimary",u8"FuckSecondary" }, IBB_Section_Desc{ "Rule","LBWNB" });

    GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog("DEBUG_DATA");

    bool RA = _CALL_CMD IBR_Inst_Project.CreateSection(IBB_Section_Desc{ "Rule","LBWNB" });
    bool RB = _CALL_CMD IBR_Inst_Project.CreateSection(IBB_Section_Desc{ "Rule","LZNB" });
    GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(IBD_BoolStr(RA));
    GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(IBD_BoolStr(RB));

    {
        IBD_RInterruptF(x);
        IBR_Section SK = _CALL_CMD IBR_Inst_Project.GetSection(IBB_Section_Desc{ "Rule","LZNB" });
        GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(SK.GetBack());
        SK.GetBack()->VarList.Value["_Local_Category"] = "Infantry";
        SK.GetBack()->VarList.Value["_Local_AtFile"] = "Rule";
    }

    _CALL_CMD IBR_Inst_Project.AddModule(u8"Weapons", { u8"FuckPrimary",u8"FuckSecondary" }, IBB_Section_Desc{ "Rule","LZNB" });


    IBR_Section SC = _CALL_CMD IBR_Inst_Project.GetSection(IBB_Section_Desc{ "Rule","LZNB" });
    bool RSC = _CALL_CMD SC.Register("InfantryTypes", "Rule");

    bool RC = _CALL_CMD SC.DuplicateSection(IBB_Section_Desc{ "Rule","LZNB_II" });
    IBR_Section SD = _CALL_CMD IBR_Inst_Project.GetSection(IBB_Section_Desc{ "Rule","LZNB_II" });

    GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(SC.GetBack());
    GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(SD.GetBack());
    GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(IBD_BoolStr(RSC));
    GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(IBD_BoolStr(RC));

    {
        IBD_RInterruptF(x);
        auto sdp = SD.GetBack();
        GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(sdp->SubSecs.size());
        if (!sdp->SubSecs.empty())
        {
            auto GLine = sdp->SubSecs.at(0).Lines.begin()->second;
            GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(GLine.Default);
            GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(GLine.Default->Name.c_str());
            GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(GLine.Default->Property.Type.c_str());
            auto proc = GLine.Default->Property.Proc;
            GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(proc);
            GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(proc->GetString(GLine.Data).c_str());
            proc->SetValue(GLine.Data, "WTF_AHHH");
            _CALL_CMD IBR_Inst_Project.UpdateAll();
            GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(proc->GetString(GLine.Data).c_str());
        }
    }

    IBR_Section SKA = _CALL_CMD IBR_Inst_Project.GetSection(IBB_Section_Desc{ "Rule","FuckSecondary" });
    SKA.Rename("FuckSecondary_NewName");

    //_CALL_CMD IBR_Inst_Project.ForceUpdate();


    std::string s = _CALL_CMD IBR_Inst_Project.GetText(true);

    if (EnableLog)
    {
        GlobalLog.AddLog_CurTime(false);
        GlobalLog.AddLog("导出文本：\"");
        GlobalLog.AddLog(UTF8toMBCS(s).c_str(),false);
        GlobalLog.AddLog("\"");
    }

    //*
    _CALL_CMD IBR_Inst_Project.DeleteSection(SKA.GetSectionDesc());
    _CALL_CMD IBR_Inst_Project.UpdateAll();

    s = _CALL_CMD IBR_Inst_Project.GetText(true);

    if (EnableLog)
    {
        GlobalLog.AddLog_CurTime(false);
        GlobalLog.AddLog("导出文本II：\"");
        GlobalLog.AddLog(UTF8toMBCS(s).c_str(), false);
        GlobalLog.AddLog("\"");
    }
    //*/
}
/*
struct asd
{
    bool InOther;
    bool LastMoving;
    int id;
    std::string ID;
    std::set<asd*> oth;
    ImRect SZ;
};
asd ppasd[10];

void IBR_Debug::DebugLoad()
{
    int i = 0;
    for (auto& p : ppasd)
    {
        p.InOther = false;
        p.LastMoving = false;
        p.id = ++i;
        p.ID = u8"测试_" + RandStr(5);
    }
}



void IBR_Debug::RenderOnWorkSpace()
{
    for (auto& p : ppasd)
    {
        if (p.InOther)continue;
        ImGui::Begin(p.ID.c_str(), nullptr, ImGuiWindowFlags_NoClamping | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("%d", p.id);
        if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && p.LastMoving)
        {
            MessageBoxA(NULL, "!", "!!", MB_OK);
            auto wpi = ImGui::GetWindowPos();
            int Con = -1;
            for(int i=0;i<10;i++)
                if (ppasd[i].SZ.Contains(wpi))
                {
                    Con = i;
                    break;
                }
            if (Con != -1)
            {
                MessageBoxA(NULL, "!!!!", "!!", MB_OK);
                p.InOther = true;
                ppasd[Con].oth.insert(p.oth.begin(), p.oth.end());
                ppasd[Con].oth.insert(&p);
                p.oth.clear();
            }
        }
        
        for (auto& I : p.oth)
        {
            ImGui::BeginChild(I->id + 1145122, ImVec2(FontHeight * 10.0f, FontHeight * 10.0f), true, ImGuiWindowFlags_NoClamping);
            ImGui::Text("%d", I->id);
            ImGui::EndChild();
        }

        ImGuiWindow* pp = ImGui::GetCurrentContext()->MovingWindow;
        if (pp)pp = pp->RootWindow;
        if (pp == ImGui::GetCurrentWindow())p.LastMoving = true;
        else p.LastMoving = false;
        p.SZ = ImRect{ ImGui::GetWindowPos(),ImGui::GetWindowPos() + ImGui::GetWindowSize() };

        ImGui::End();
    }
}

void Crash()
{
    auto x = *((int*)0);
    ((void)x);
}*/

void IBR_Debug::DebugLoad()
{

}

void IBR_Debug::RenderOnWorkSpace()
{

}
