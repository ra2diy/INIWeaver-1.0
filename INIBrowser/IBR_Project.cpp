
#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include<imgui_internal.h>


std::optional<std::vector<IBR_Project::id_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::AddModule(const IBB_ModuleAlt& Module, const std::string& Argument, bool UseMouseCenter)
{
    if (!Module.Available)return std::nullopt;
    std::vector<ModuleClipData> NewMD = Module.Modules;
    for (auto& M : NewMD)M.Replace(Module.Parameter, Argument);
    return AddModule(NewMD, UseMouseCenter);
}
std::optional<std::vector<IBR_Project::id_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::AddModule(const std::vector<ModuleClipData>& Modules, bool UseMouseCenter)
{
    if (Modules.empty())return std::vector<IBR_Project::id_t>();
    IBD_RInterruptF(x);
    bool Ret = true;
    std::vector<IBR_Project::id_t> Vec;
    for (auto& M : Modules)if (!M.IsLinkGroup)
    {
        auto R = AddModule_Impl(M, UseMouseCenter);
        if (R)Vec.push_back(R.value());
        else Ret = false;
    }
    for (auto& M : Modules)if (M.IsLinkGroup)
    {
        auto R = AddModule_Impl(M, UseMouseCenter);
        if (R)Vec.push_back(R.value());
        else Ret = false;
    }
    Ret &= IBF_Inst_Project.UpdateAll();
    if (Ret)return Vec;
    else return std::nullopt;
}
std::optional<IBR_Project::id_t> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::AddModule(const ModuleClipData& Modules, bool UseMouseCenter)
{
    IBD_RInterruptF(x);
    bool Ret = true;
    auto R = AddModule_Impl(Modules, UseMouseCenter);
    Ret &= R.has_value();
    Ret &= IBF_Inst_Project.UpdateAll();
    if (Ret)return R;
    else return std::nullopt;
}

std::optional<IBR_Project::id_t> _PROJ_CMD_WRITE _PROJ_CMD_UPDATE IBR_Project::AddModule_Impl(const ModuleClipData& Module, bool UseMouseCenter)
{
    ImVec2 InitEqPos;
    if (UseMouseCenter)
    {
        auto Mouse = ImGui::GetMousePos();
        if (ImGui::IsMousePosValid(&Mouse))
        {
            Mouse.x = std::max((float)IBR_RealCenter::WorkSpaceUL.x, Mouse.x);
            Mouse.x = std::min((float)IBR_RealCenter::WorkSpaceDR.x, Mouse.x);
            Mouse.y = std::max((float)IBR_RealCenter::WorkSpaceUL.y, Mouse.y);
            Mouse.y = std::min((float)IBR_RealCenter::WorkSpaceDR.y, Mouse.y);
        }
        else Mouse = IBR_RealCenter::Center;
        InitEqPos = IBR_WorkSpace::RePosToEqPos(Mouse) + Module.EqDelta;
    }
    else
    {
        InitEqPos = Module.EqDelta;
    }


    if (Module.IsComment)
    {
        auto Ret = CreateCommentBlock(InitEqPos, Module.Comment, Module.EqSize);
        if (Ret.HasBack())return Ret.ID;
        else return std::nullopt;
    }

    IBG_Undo.SomethingShouldBeHere();
    bool Ret = true;
    Ret &= IBF_Inst_Project.Project.AddModule(Module);
    IBB_Section_Desc D = { Module.Desc.A,Module.Desc.B };
    EnsureSection(D, Module.DisplayName);
    auto Sec = GetSection(D);
    if (Module.FromClipBoard || !UseMouseCenter)
    {
        auto sd = Sec.GetSectionData();
        if (!sd)return std::nullopt;
        //sd->Dragging = true;
        sd->EqDelta = Module.EqDelta;
        //M essage BoxA(NULL, ("(" + std::to_string(sd->EqDelta.x) + " , " + std::to_string(sd->EqDelta.y) + ")").c_str(), "Load1!", MB_OK);
        sd->EqSize = Module.EqSize;
        sd->Ignore = Module.Ignore;
        sd->EqPos = InitEqPos;

        IBRF_CoreBump.SendToR({ [sd, eq = sd->EqPos]() {sd->EqPos = eq; } });
    }
    if (Ret)return Sec.ID;
    else return std::nullopt;
}

bool _PROJ_CMD_WRITE _PROJ_CMD_UPDATE IBR_Project::UpdateAll()
{
    IBD_RInterruptF(x);
    return IBF_Inst_Project.UpdateAll();
}

bool _PROJ_CMD_WRITE _PROJ_CMD_UPDATE IBR_Project::ForceUpdate()
{
    IBD_RInterruptF(x);
    if (BackThreadID == INT_MAX)return false;
    IBG_SuspendThread(BackThreadID);
    IBRF_CoreBump.IBF_ForceProc();
    IBG_ResumeThread(BackThreadID);
    return IBF_Inst_Project.UpdateAll();
}

_TEXT_UTF8 std::string _PROJ_CMD_READ IBR_Project::GetText(bool PrintExtraData)
{
    IBD_RInterruptF(x);
    return IBF_Inst_Project.GetText(PrintExtraData);
}

IBR_Section _PROJ_CMD_READ IBR_Project::GetSection(const IBB_Section_Desc& Desc)
{
    if (EnableLogEx)
    {
        sprintf_s(LogBuf, "IBR_Project::GetSection : Desc=%s", Desc.GetText().c_str());
        GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(LogBuf);
    }

    auto rit = IBR_Rev_SectionMap.find(Desc);
    if (rit == IBR_Rev_SectionMap.end())
    {
        auto pBack = IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc });
        rit = IBR_Rev_SectionMap.insert({ Desc,MaxID }).first;
        if (pBack)
        {
            std::string N;
            {
                IBD_RInterruptF(x);
                auto& RegType = IBB_DefaultRegType::GetRegType(pBack->Register);
                if (RegType.RegNameAsDisplay)N = RegType.GetNoName(pBack->Name);
                else N = RegType.GetNoName();
            }
            IBG_Undo.SomethingShouldBeHere();
            auto Res = IBR_SectionMap.insert({ MaxID,IBR_SectionData{Desc,std::move(N)} });
            IBF_Inst_Project.DisplayNames[Res.first->second.DisplayName] = Res.first->second.Desc;
        }
        else
        {
            IBR_SectionMap.insert({ MaxID,IBR_SectionData{Desc,locc("Back_GunMu")} });
            auto gt = UTF8toUnicode(Desc.GetText());
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(std::vformat(L"IBR_Project::GetSection ： " + locw("Log_SectionNotExist"), std::make_wformat_args(gt)));
        }
        ++MaxID;
    }
    return IBR_Section{ this,rit->second };
}

void _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ IBR_Project::EnsureSection(const IBB_Section_Desc& Desc, const std::string& DisplayName) _PROJ_CMD_BACK_CONST
{
    
    auto rit = IBR_Rev_SectionMap.find(Desc);
    if (rit == IBR_Rev_SectionMap.end())
    {
        auto pBack = IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc });
        rit = IBR_Rev_SectionMap.insert({ Desc,MaxID }).first;
        std::string S;
        if (DisplayName.empty())
        {
            if (pBack)
            {
                IBD_RInterruptF(x);
                auto& RegType = IBB_DefaultRegType::GetRegType(pBack->Register);
                if (RegType.RegNameAsDisplay)S = RegType.GetNoName(pBack->Name);
                else S = RegType.GetNoName();
            }
        }
        else
        {
            IBG_Undo.SomethingShouldBeHere();
            auto it = IBF_Inst_Project.DisplayNames.find(DisplayName);
            int I = 0;
            while (it != IBF_Inst_Project.DisplayNames.end())
                it = IBF_Inst_Project.DisplayNames.find(DisplayName + "_" + std::to_string(++I));
            if (I) S = DisplayName + "_" + std::to_string(I);
            else S = DisplayName;
        }
        //MessageBoxA(MainWindowHandle, S.c_str(), Desc.GetText().c_str(), MB_OK);
        IBF_Inst_Project.DisplayNames[S] = Desc;

        if (pBack)IBR_SectionMap.insert({ MaxID,IBR_SectionData{Desc, std::move(S)} });
        else
        {
            IBR_SectionMap.insert({ MaxID,IBR_SectionData{Desc,locc("Back_GunMu")} });
            auto gt = UTF8toUnicode(Desc.GetText());
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(std::vformat(L"IBR_Project::GetSection ： " + locw("Log_SectionNotExist"), std::make_wformat_args(gt)));
        }
        ++MaxID;
    }
}

std::optional<IBR_Project::id_t> _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ IBR_Project::GetSectionID(const IBB_Section_Desc& Desc) _PROJ_CMD_BACK_CONST
{
    auto rit = IBR_Rev_SectionMap.find(Desc);
    if (rit == IBR_Rev_SectionMap.end())return std::nullopt;
    else return rit->second;
}

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::CreateSection(const IBB_Section_Desc& Desc)
{
    bool Ret;
    {
        IBD_RInterruptF(x);
        Ret = IBF_Inst_Project.Project.CreateNewSection(Desc);
        IBF_Inst_Project.UpdateCreateSection(Desc);
    }
    IBG_Undo.SomethingShouldBeHere();
    //TODO: Undo Action 
    return Ret;
}

IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::CreateCommentBlock(ImVec2 InitialEqPos, std::string_view InitialText, ImVec2 InitialEqSize)
{
    //MessageBoxW(NULL, UTF8toUnicode(InitialText.data()).c_str(), L"!", MB_OK);
    IBB_Section_Desc Desc;
    Desc.Ini = DefaultIniName;
    Desc.Sec = GenerateModuleTag();
    CreateSection(Desc);
    EnsureSection(Desc, loc("Back_CommentBlockName"));
    auto RSec=GetSection(Desc);

    auto BSec = RSec.GetBack();
    assert(BSec != nullptr);
    BSec->CreateAsCommentBlock = true;
    BSec->Register = loc("Back_CommentBlockName");

    auto RD = RSec.GetSectionData();
    assert(RD != nullptr);
    RD->IsComment = true;
    RD->EqPos = InitialEqPos;
    RD->CommentEdit = std::make_shared<BufString>();
    if(!InitialText.empty())
        strcpy(RD->CommentEdit.get(), InitialText.data());
    if (InitialEqSize.x != 0.0F || InitialEqSize.y != 0.0F)
        RD->EqSize = InitialEqSize;

    IBG_Undo.SomethingShouldBeHere();

    return RSec;
}

IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::CreateSectionAndBack(const IBB_Section_Desc& Desc)
{
    CreateSection(Desc);
    return GetSection(Desc);
}

bool _PROJ_CMD_READ IBR_Project::HasSection(const IBB_Section_Desc& Desc)
{
    IBD_RInterruptF(x);
    return IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc }) != nullptr;
}

bool _PROJ_CMD_READ IBR_Project::HasSection(IBR_Project::id_t id)
{
    auto it = IBR_SectionMap.find(id);
    if (it == IBR_SectionMap.end())return false;
    IBD_RInterruptF(x);
    return IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ it->second.Desc }) != nullptr;
}

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::DeleteSection(IBR_Project::id_t id)
{
    auto it = IBR_SectionMap.find(id);
    if (it == IBR_SectionMap.end())return false;
    return DeleteSection(it->second.Desc);
}

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::DeleteSection(const IBB_Section_Desc& Desc)
{
    if (Desc.Ini.empty() || Desc.Sec.empty())return false;
    bool Ret;
    {
        IBD_RInterruptF(x);
        if (IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc }) == nullptr)return false;
        auto ci = const_cast<IBB_Ini*>(IBF_Inst_Project.Project.GetIni(IBB_Project_Index{ Desc }));
        Ret = ci->DeleteSection(Desc.Sec);
        IBF_Inst_Project.UpdateAll();
    }
    IBG_Undo.SomethingShouldBeHere();
    auto RS = GetSection(Desc);
    IBF_Inst_Project.DisplayNames.erase(IBR_SectionMap[RS.ID].DisplayName);
    IBR_SectionMap.erase(RS.ID);
    IBR_Rev_SectionMap.erase(Desc);
    return Ret;
}//TODO
/*
bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::DeleteSection(const IBB_Section_Desc& Desc)
{
    bool Ret;
    {
        IBD_RInterruptF(x);
        if (IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc }) == nullptr)return false;
        auto ci = const_cast<IBB_Ini*>(IBF_Inst_Project.Project.GetIni(IBB_Project_Index{ Desc }));
        Ret = ci->DeleteSection(Desc.Sec);
    }
    auto RS = GetSection(Desc);
    IBR_SectionMap.erase(RS.ID);
    IBR_Rev_SectionMap.erase(Desc);
    return Ret;
}
*/


bool _PROJ_CMD_UPDATE IBR_Project::DataCheck()
{
    if (EnableLogEx)
    {
        GlobalLog.AddLog_CurTime(false);
        GlobalLog.AddLog("DataCheck!!");
    }

    for (auto& p : IBR_SectionMap)p.second.Exists = false;
    bool Ret = true;
    {
        IBD_RInterruptF(x);
        IBB_Section_Desc dsc;
        for (auto& ini : IBF_Inst_Project.Project.Inis)
        {
            dsc.Ini = ini.Name;
            if (dsc.Ini.empty())continue;
            for (auto& sp : ini.Secs)
            {
                auto& sec = sp.second;
                dsc.Sec = sec.Name;
                if (dsc.Sec.empty())continue;

                if (EnableLogEx)
                {
                    sprintf_s(LogBuf, "Checking %s", dsc.GetText().c_str());
                    GlobalLog.AddLog_CurTime(false); GlobalLog.AddLog(LogBuf);
                }

                auto sc = GetSection(dsc);
                IBR_SectionMap[sc.ID].Exists = true;
            }
        }
    }
    std::vector<IBB_Section_Desc> Descs;
    for (auto& p : IBR_SectionMap)
        if (p.second.Exists == false)Descs.push_back(p.second.Desc);
    for (auto& D : Descs)
        IBR_Inst_Project.DeleteSection(D);
    return Ret;
}

/*
WRITE:
    ret=Interrupt(Action);
    SendToF(Update);
    Push(UndoAction);
    return ret;

READ:
    Interrupt(Action);
*/

void IBR_Project::Load(const IBS_Project& Proj)
{
    IBR_WorkSpace::EqCenterPrev = IBR_FullView::EqCenter = Proj.FullView_EqCenter;
    IBR_WorkSpace::RatioPrev = IBR_FullView::Ratio = Proj.FullView_Ratio;
    IBR_EditFrame::Empty = true;
    IBB_ClipBoardData ClipData;
    if (ClipData.SetStream(Proj.Data))
        AddModule(ClipData.Modules, false);
    IBF_Inst_Project.Project.ChangeAfterSave = false;
    //MAGIC 忽略3帧之内的改动
    IBRF_CoreBump.SendToR({ []() {
        IBF_Inst_Project.Project.ChangeAfterSave = false;
        IBRF_CoreBump.SendToR({ []() {
            IBF_Inst_Project.Project.ChangeAfterSave = false;
            IBRF_CoreBump.SendToR({ []() {
                IBF_Inst_Project.Project.ChangeAfterSave = false;
                IBRF_CoreBump.SendToR({ []() {
                    IBF_Inst_Project.Project.ChangeAfterSave = false;
                } });
            } });
        } });
    } });
}

void IBR_Project::Save(IBS_Project& Proj)
{
    Proj.FullView_EqCenter = IBR_FullView::EqCenter;
    Proj.FullView_Ratio = IBR_FullView::Ratio;
    IBB_ClipBoardData ClipData;
    ClipData.GenerateAll(true, true);
    Proj.Data = ClipData.GetStream();
}
