
#include <set>
#include "IBR_Project.h"
#include "IBFront.h"
#include "Global.h"
#include "IBSave.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include<imgui_internal.h>
#include <ranges>
#include "IBR_ListView.h"
#include "IBR_Misc.h"
#include "IBG_UndoTree.h"
#include "IBB_ModProject.h"

void _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::TriggerRefreshLink()
{
    RefreshLinkList = true;
}

std::pair<bool, std::vector<ModuleID_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::AddModule(const IBB_ModuleAlt& Module, const std::string& Argument, bool UseMouseCenter)
{
    if (!Module.Available)return { false ,{} };
    std::vector<ModuleClipData> NewMD = Module.Modules;
    
    if(Module.FromClipBoard)MangleModules(NewMD);
    else for (auto& M : NewMD)M.Replace(Module.Parameter, Argument);

    int X = (int)sqrt(NewMD.size());
    float Gap = 2.0f * FontHeight;
    float CX = X * Gap * 0.5f;
    float CY = (NewMD.size() / X) * Gap * 0.5f;
    for (size_t i = 0; i < NewMD.size(); i++)
    {
        if (abs(NewMD[i].EqDelta).max() > 1e6)//nonsense value
        {
            NewMD[i].EqDelta = { (i % X) * Gap - CX,(i / X) * Gap - CY };
        }
    }

    return AddModule(NewMD, UseMouseCenter);
}
std::pair<bool, std::vector<ModuleID_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::AddModule(const std::vector<ModuleClipData>& Modules, bool UseMouseCenter)
{
    if (Modules.empty())return { true, {} };
    IBD_RInterruptF(x);
    bool Ret = true;
    std::vector<ModuleID_t> Vec;
    for (auto& M : Modules)
    {
        auto R = AddModule_Impl(M, UseMouseCenter);
        if (R)Vec.push_back(R.value());
        else Ret = false;
    }
    Ret &= SetModuleIncludeLink(Vec);
    Ret &= IBF_Inst_Project.UpdateAll();
    IBR_ListView::NeedsUpdate();
    return { Ret, Vec };
}
std::optional<ModuleID_t> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::AddModule(const ModuleClipData& Modules, bool UseMouseCenter)
{
    IBD_RInterruptF(x);
    bool Ret = true;
    auto R = AddModule_Impl(Modules, UseMouseCenter);
    Ret &= R.has_value();
    if (R)Ret &= SetModuleIncludeLink(R.value());
    Ret &= IBF_Inst_Project.UpdateAll();
    IBR_ListView::NeedsUpdate();
    if (Ret)return R;
    else return std::nullopt;
}

std::optional<ModuleID_t> _PROJ_CMD_WRITE _PROJ_CMD_UPDATE IBR_Project::AddModule_Impl(const ModuleClipData& Module, bool UseMouseCenter)
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
    EnsureSection(D, Module.Register, Module.DisplayName);
    auto Sec = GetSection(D);
    //if (Module.FromClipBoard || !UseMouseCenter )
    {
        auto sd = Sec.GetSectionData();
        if (!sd)return std::nullopt;
        //sd->Dragging = true;
        sd->EqDelta = Module.EqDelta;
        //MessageBoxA(NULL, ("(" + std::to_string(sd->EqDelta.x) + " , " + std::to_string(sd->EqDelta.y) + ")").c_str(), "Load1!", MB_OK);
        sd->EqSize = Module.EqSize;
        sd->Ignore = Module.Ignore;
        sd->EqPos = InitEqPos;
        sd->CollapsedInComposed = Module.CollapsedInComposed;
        sd->IncludedByModule_TmpDesc = { Module.IncludedBySection.A, Module.IncludedBySection.B };
        sd->Hidden = Module.Hidden;
        sd->Frozen = Module.Frozen;
        for (auto& lt : Module.IncludingSections)
            sd->IncludingModules_TmpDesc.push_back({ lt.A,lt.B });
        IBRF_CoreBump.SendToR({ [sd, eq = sd->EqPos]() {sd->EqPos = eq; } });
    }
    if (Ret)return Sec.ID;
    else return std::nullopt;
}


bool _PROJ_CMD_WRITE IBR_Project::SetModuleIncludeLink(ModuleID_t ID)
{
    auto Sec = GetSectionFromID(ID).GetSectionData();
    if (!Sec)return false;
    Sec->IncludedByModule = GetSectionID(Sec->IncludedByModule_TmpDesc).value_or(INVALID_MODULE_ID);
    Sec->IncludingModules = Sec->IncludingModules_TmpDesc
        | std::views::transform([this](const auto& Desc) {return GetSectionID(Desc).value_or(INVALID_MODULE_ID); })
        | std::views::filter([](const auto& id) { return id != INVALID_MODULE_ID; })
        | std::ranges::to<std::vector<ModuleID_t>>();
    return true;
}

bool _PROJ_CMD_WRITE IBR_Project::SetModuleIncludeLink(const std::vector<ModuleID_t>& IDs)
{
    return std::ranges::fold_left(IDs, true, [this](bool val, ModuleID_t id) { return val & SetModuleIncludeLink(id); });
}

std::optional<ModuleID_t> _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::ComposeSections(const std::vector<ModuleID_t>& IDs)
{
    using namespace std::ranges;
    auto& Reg = loc("Back_ComposeBlockName");
    auto RSec = CreateSectionAndBack(
        IBB_Section_Desc{ DefaultIniName, GenerateModuleTag() },
        Reg,
        loc("Back_ComposeBlockName")
    );

    auto BSec = RSec.GetBack();
    auto RD = RSec.GetSectionData();
    if(BSec == nullptr)return std::nullopt;
    if(RD == nullptr)return std::nullopt;
    IBG_Undo.SomethingShouldBeHere();

    BSec->Register = NewPoolStr(Reg);
    RD->IncludingModules = IDs;

    //剔除被缩合的模块
    std::erase_if(RD->IncludingModules, [this](auto id) {
        auto Data = GetSectionFromID(id).GetSectionData();
        if (!Data)return true;
        return Data->IsIncluded();
    });

    sort(RD->IncludingModules, [this](ModuleID_t l, ModuleID_t r)
        {
            auto pDataL = GetSectionFromID(l).GetSectionData();
            auto pDataR = GetSectionFromID(r).GetSectionData();
            //sort by y position, then x position, then ID
            if (pDataL && pDataR)
            {
                if (abs(pDataL->EqPos.y - pDataR->EqPos.y) > 1e-6)
                    return pDataL->EqPos.y < pDataR->EqPos.y;
                else if (abs(pDataL->EqPos.x - pDataR->EqPos.x) > 1e-6)
                    return pDataL->EqPos.x < pDataR->EqPos.x;
                else
                    return l < r;
            }
            else return l < r;
        });
    for_each(IDs, [this, ID = RSec.ID](auto id) {
        if (auto pData = GetSectionFromID(id).GetSectionData(); pData)
        {
            pData->IncludedByModule = ID;
            pData->CollapsedInComposed = true;
        }
            
    });
    RD->EqPos = IBR_WorkSpace::GetMassCenter(IDs);
    
    return std::nullopt;
}

std::optional<std::vector<ModuleID_t>> _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::DecomposeSection(ModuleID_t ID)
{
    auto pComData = GetSectionFromID(ID).GetSectionData();
    if (!pComData)return std::nullopt;
    IBG_Undo.SomethingShouldBeHere();
    if (!pComData->Decomposable())return std::nullopt;

    //unlink
    std::ranges::for_each(pComData->IncludingModules, [this, ID, pComData](auto id) {
        auto pData = GetSectionFromID(id).GetSectionData();
        if (pData && pData->IncludedByModule == ID)
        {
            pData->IncludedByModule = INVALID_MODULE_ID;
            pData->IncludedByModule_TmpDesc = {};
            pData->CollapsedInComposed = false;
            pData->EqPos = pComData->EqPos + pData->EqDelta;
            pData->UpdatePosByEq = true;
        }
        });
    auto ModuleIDs { std::move(pComData->IncludingModules) };
    pComData->IncludingModules.clear();

    DeleteSection(ID);
    return ModuleIDs;
}

std::optional<std::vector<ModuleID_t>> _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO  _PROJ_CMD_UPDATE IBR_Project::DecomposeSection(IBB_Section_Desc Desc)
{
    auto ID = GetSectionID(Desc);
    if (!ID)return std::nullopt;
    return DecomposeSection(ID.value());
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

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::RenameAll()
{
    bool R = true;
    for (auto& [k, v] : IBR_SectionMap)
    {
        auto rs = GetSectionFromID(k);
        auto bk = rs.GetBack();
        if (bk)
        {
            bool P = false;
            //此处关联了ModuleClipData::NeedtoMangle()
            //记得同时修改
            auto Tag = GenerateModuleTag();
            {
                IBD_RInterruptF(x);
                auto& W = bk->VarList.GetVariable("_InitialSecName");
                auto& Q = bk->VarList.GetVariable("UseOwnName");
                if (W == bk->Name && !IsTrueString(Q))
                {
                    P = true;
                    bk->VarList.Value["_InitialSecName"] = Tag;
                }
            }
            if (P)R &= rs.Rename(Tag);
        }
        
    }
    return R;
}

IBR_Section _PROJ_CMD_READ IBR_Project::GetSection(const IBB_Section_Desc& Desc)
{
    auto rit = IBR_Rev_SectionMap.find(Desc);
    if (rit == IBR_Rev_SectionMap.end())
    {
        auto pBack = IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc });
        IBR_Rev_SectionMapII.insert({ IBB_SectionID{ Desc }, MaxID });
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
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog(std::vformat(L"IBR_Project::GetSection ： " + locw("Log_SectionNotExist"), std::make_wformat_args(gt)));
        }
        ++MaxID;
    }
    return IBR_Section{ this,rit->second };
}

IBR_Section _PROJ_CMD_READ IBR_Project::GetSection(IBB_SectionID id) _PROJ_CMD_BACK_CONST
{
    auto rit = IBR_Rev_SectionMapII.find(id);
    if (rit != IBR_Rev_SectionMapII.end())
        return IBR_Section{ this,rit->second };
    else return GetSection(id.ToDesc());
}

std::string GenerateDisplayName(const std::string& Register, const std::string& DisplayName, const IBB_Section* pBack)
{
    std::string S;
    if (DisplayName.empty())
    {
        if (pBack)
        {
            auto& RegType = IBB_DefaultRegType::GetRegType(pBack->Register);
            if (RegType.RegNameAsDisplay)S = RegType.GetNoName(pBack->Name);
            else S = RegType.GetNoName();
        }
    }
    else
    {
        IBG_Undo.SomethingShouldBeHere();
        auto& RegType = IBB_DefaultRegType::GetRegType(Register);

        if (DisplayName.starts_with(RegType.Name))
        {
            auto Suffix = DisplayName.substr(RegType.Name.size());
            //如果Suffix是纯数字，则认为是自动生成的，按空DisplayName处理
            if (!Suffix.empty() && std::ranges::all_of(Suffix, ::isdigit))
            {
                if (RegType.RegNameAsDisplay)S = RegType.GetNoName(pBack->Name);
                else S = RegType.GetNoName();
                return S;
            }
        }

        auto it = IBF_Inst_Project.DisplayNames.find(DisplayName);
        int I = 0;
        while (it != IBF_Inst_Project.DisplayNames.end())
            it = IBF_Inst_Project.DisplayNames.find(DisplayName + "_" + std::to_string(++I));
        if (I) S = DisplayName + "_" + std::to_string(I);
        else S = DisplayName;
    }

    return S;
}

void _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ IBR_Project::EnsureSection(const IBB_Section_Desc& Desc, _TEXT_UTF8 const std::string& Register, const std::string& DisplayName) _PROJ_CMD_BACK_CONST
{
    
    auto rit = IBR_Rev_SectionMap.find(Desc);
    if (rit == IBR_Rev_SectionMap.end())
    {
        auto pBack = IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc });
        IBR_Rev_SectionMapII.insert({ IBB_SectionID{ Desc }, MaxID });
        rit = IBR_Rev_SectionMap.insert({ Desc,MaxID }).first;
        std::string S = GenerateDisplayName(Register, DisplayName, pBack);
        IBF_Inst_Project.DisplayNames[S] = Desc;

        if (pBack)IBR_SectionMap.insert({ MaxID,IBR_SectionData{Desc, std::move(S)} });
        else
        {
            IBR_SectionMap.insert({ MaxID,IBR_SectionData{Desc,locc("Back_GunMu")} });
            auto gt = UTF8toUnicode(Desc.GetText());
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog(std::vformat(L"IBR_Project::GetSection ： " + locw("Log_SectionNotExist"), std::make_wformat_args(gt)));
        }
        ++MaxID;
    }
}

std::optional<ModuleID_t> _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ IBR_Project::GetSectionID(const IBB_Section_Desc& Desc) _PROJ_CMD_BACK_CONST
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
    auto& Reg = loc("Back_CommentBlockName");
    auto RSec = CreateSectionAndBack(
        IBB_Section_Desc { DefaultIniName, GenerateModuleTag() },
        Reg,
        loc("Back_CommentBlockName")
    );

    auto BSec = RSec.GetBack();
    assert(BSec != nullptr);
    BSec->CreateAsCommentBlock = true;
    BSec->Register = NewPoolStr(Reg);

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
    IBR_ListView::NeedsUpdate();

    return RSec;
}

IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::CreateSingleValBlock(ImVec2 InitialEqPos, const std::string& InitialValue)
{
    auto RSec = CreateSectionAndBack(
        IBB_Section_Desc{ DefaultIniName, RandStr(16) },
        AnyTypeName,
        loc("Back_SingleValBlockName")
    );

    auto BSec = RSec.GetBack();
    assert(BSec != nullptr);
    BSec->SingleVal = true;
    BSec->SkipExport = true;
    BSec->SkipTitle = true;
    BSec->MergeLine(SingleValID(), Index_AlwaysNew, InitialValue, IBB_IniMergeMode::Replace);
    BSec->OnShow[SingleValID()] = EmptyOnShowDesc;
    BSec->Register = AnyTypeID();

    auto RD = RSec.GetSectionData();
    assert(RD != nullptr);
    RD->EqPos = InitialEqPos;

    IBG_Undo.SomethingShouldBeHere();
    IBR_ListView::NeedsUpdate();

    return RSec;
}

IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::CreateSectionAndBack(const IBB_Section_Desc& Desc, _TEXT_UTF8 const std::string& Register, _TEXT_UTF8 const std::string& DisplayName)
{
    CreateSection(Desc);
    EnsureSection(Desc, Register, DisplayName);
    return GetSection(Desc);
}

bool _PROJ_CMD_READ IBR_Project::HasSection(const IBB_Section_Desc& Desc)
{
    IBD_RInterruptF(x);
    return IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc }) != nullptr;
}

bool _PROJ_CMD_READ IBR_Project::HasSection(ModuleID_t id)
{
    auto it = IBR_SectionMap.find(id);
    if (it == IBR_SectionMap.end())return false;
    IBD_RInterruptF(x);
    return IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ it->second.Desc }) != nullptr;
}

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::DeleteSection(ModuleID_t id)
{
    auto it = IBR_SectionMap.find(id);
    if (it == IBR_SectionMap.end())return false;
    return DeleteSection(it->second.Desc);
}

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::DeleteSection(const std::vector <ModuleID_t>& ids)
{
    std::vector<IBB_Section_Desc> Descs;
    for (auto& id : ids)
    {
        auto it = IBR_SectionMap.find(id);
        if (it == IBR_SectionMap.end())continue;
        Descs.push_back(it->second.Desc);
    }
    return DeleteSection(Descs);
}

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::DeleteSection(const std::vector<IBB_Section_Desc>& Descs)
{
    int UpdateCount = 0;
    bool Ret = true;
    for (auto& Desc : Descs)
    {
        if (Desc.Ini.empty() || Desc.Sec.empty())continue;
        {
            IBD_RInterruptF(x);
            UpdateCount++;
            if (IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc }) == nullptr)continue;
            auto ci = const_cast<IBB_Ini*>(IBF_Inst_Project.Project.GetIni(IBB_Project_Index{ Desc }));
            Ret &= ci->DeleteSection(Desc.Sec);
            
        }
        IBG_Undo.SomethingShouldBeHere();
        auto RS = GetSection(Desc);
        IBF_Inst_Project.DisplayNames.erase(IBR_SectionMap[RS.ID].DisplayName);
        IBR_SectionMap.erase(RS.ID);
        IBR_Rev_SectionMap.erase(Desc);
        IBR_Rev_SectionMapII.erase(IBB_SectionID{ Desc });
    }
    if(UpdateCount)
    {
        IBD_RInterruptF(x);
        IBF_Inst_Project.UpdateAll();
        IBR_ListView::NeedsUpdate();
    }
    return Ret;
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
        IBR_ListView::NeedsUpdate();
    }
    //Desc这个入参可能是这些map里面的引用，所以必须提前复制一份
    auto _Desc = Desc;
    IBG_Undo.SomethingShouldBeHere();
    auto RS = GetSection(_Desc);
    IBF_Inst_Project.DisplayNames.erase(IBR_SectionMap[RS.ID].DisplayName);
    IBR_SectionMap.erase(RS.ID);
    IBR_Rev_SectionMap.erase(_Desc);
    IBR_Rev_SectionMapII.erase(IBB_SectionID{_Desc});
    return Ret;
}//TODO



bool _PROJ_CMD_UPDATE IBR_Project::DataCheck()
{
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        GlobalLogB.AddLog("DataCheck!!");
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
                    GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog(LogBuf);
                }

                auto sc = GetSection(dsc);
                IBR_SectionMap[sc.ID].Exists = true;
            }
        }
    }
    std::vector<IBB_Section_Desc> Descs;
    for (auto& p : IBR_SectionMap)
        if (p.second.Exists == false)Descs.push_back(p.second.Desc);
    IBR_Inst_Project.DeleteSection(Descs);
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
    IBB_ClipBoardData::ErrorContext.ModulePath = Proj.Path;
    IBB_ClipBoardData::ErrorContext.ModuleName = UnicodetoUTF8(Proj.ProjName);

    // [LOG] before SetStream
//     GlobalLogB.AddLog_CurTime(false);
/*    GlobalLogB.AddLog((std::string("DBG[Load] DataSz=") + std::to_string(Proj.Data.size())
        + " Path=" + UnicodetoUTF8(Proj.Path)).c_str()); */

    if (ClipData.SetStream(Proj.Data, GetClipFormatVersion(Proj.GetCreateVersionN())))
    {
        // [LOG] dump loaded modules
        std::string lg = "DBG[Load] SetStream OK: ModuleCount=" + std::to_string(ClipData.Modules.size());
//         GlobalLogB.AddLog(lg.c_str());
        for (size_t i = 0; i < ClipData.Modules.size(); i++)
        {
            auto& M = ClipData.Modules[i];
            std::string mlg = "DBG[Load]  M[" + std::to_string(i) + "]: Reg=" + M.Register
                + " DescA=" + M.Desc.A + " DescB=" + M.Desc.B
                + " Comment=" + (M.IsComment ? M.Comment : "NO")
                + " Hidden=" + std::to_string(M.Hidden)
                + " EqSize=(" + std::to_string(M.EqSize.x) + "," + std::to_string(M.EqSize.y) + ")"
                + " EqDelta=(" + std::to_string(M.EqDelta.x) + "," + std::to_string(M.EqDelta.y) + ")"
                + " VarCount=" + std::to_string(M.VarList.size())
                + " LinesCount=" + std::to_string(M.Lines.size());
            for (auto& v : M.VarList)
                mlg += " [" + v.A + "=" + v.B.substr(0, std::min(v.B.size(), size_t(60))) + "]";
//             GlobalLogB.AddLog(mlg.c_str());
            int lc = 0;
            for (auto& line : M.Lines)
            {
                // if (lc >= 10) { GlobalLogB.AddLog("DBG[Load]   ...(lines truncated)"); break; }
                // GlobalLogB.AddLog((std::string("DBG[Load]   LINE: ") + line.Key + "=" + line.Value.substr(0, 60)).c_str());
                lc++;
            }
        }
        AddModule(ClipData.Modules, false);
        // [LOG] after AddModule
/*        GlobalLogB.AddLog((std::string("DBG[Load] AFTER_AddModule: RevMapSz=")
            + std::to_string(IBR_Inst_Project.IBR_Rev_SectionMap.size())).c_str()); */

        // Read binary metadata tail (BuildOutputDir etc.)
        if (Proj.Path.ends_with(L".modproj"))
        {
            auto meta = IBB_ModProject::ReadMetaTail(Proj.Data);
            auto it = meta.find("BuildOutputDir");
            if (it != meta.end() && !it->second.empty())
            {
                IBF_Inst_ModProject.BuildOutputDir = UTF8toUnicode(it->second);
                // GlobalLogB.AddLog((std::string("DBG[Load] MetaTail: BuildOutputDir=") + it->second).c_str());
            }
        }
    }
    else
    {
        // GlobalLogB.AddLog("DBG[Load] SetStream FAILED");
    }
    IBF_Inst_Project.Project.ChangeAfterSave = false;
    //MAGIC
    //DONT ASK ME WHY THERE IS SO MANY LAYERS OF SendToR
    //Actually 5 main loops are needed to fully initialize
    //So just delay 6 frames
    //If more frames will be needed you should add more layers
    //Or rewrite the Bump Logic to allow dalay in frames
    IBRF_CoreBump.SendToR({ []() {
        // Mark iproj_ref modules whose .iproj files are missing
        for (auto& [D, I] : IBR_Inst_Project.IBR_Rev_SectionMap)
        {
            IBR_Section Sec{ &IBR_Inst_Project, I };
            auto sd = Sec.GetSectionData();
            if (!sd) continue;
            auto bk = Sec.GetBack();
            if (!bk) continue;
            auto ip = bk->VarList.GetVariable("iproj_path");
            if (!ip.empty() && GetFileAttributesW(UTF8toUnicode(ip).c_str()) == INVALID_FILE_ATTRIBUTES)
                sd->Missing = true;
        }
        IBF_Inst_Project.Project.ChangeAfterSave = false;
        IBRF_CoreBump.SendToR({ []() {
            IBF_Inst_Project.Project.ChangeAfterSave = false;
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
        } });
    } });
}

void IBR_Project::Save(IBS_Project& Proj)
{
    Proj.FullView_EqCenter = IBR_FullView::EqCenter;
    Proj.FullView_Ratio = IBR_FullView::Ratio;

    // [LOG] pre-GenerateAll state
    {
        std::string lg = "DBG[Save] PRE_GEN: RevMapSz=" + std::to_string(IBR_Rev_SectionMap.size())
            + " IBR_SectionMapSz=" + std::to_string(IBR_SectionMap.size())
            + " Proj_IniCount=" + std::to_string(IBF_Inst_Project.Project.Inis.size());
        for (auto& Ini : IBF_Inst_Project.Project.Inis)
            lg += " Ini[" + Ini.Name + "]_SecCount=" + std::to_string(Ini.Secs.size());
//         GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog(lg.c_str());
    }

    IBB_ClipBoardData ClipData;
    ClipData.GenerateAll(true, true);

    // [LOG] post-GenerateAll: dump each module's key fields
    {
        std::string lg = "DBG[Save] POST_GEN: ModuleCount=" + std::to_string(ClipData.Modules.size());
//         GlobalLogB.AddLog(lg.c_str());
        for (size_t i = 0; i < ClipData.Modules.size(); i++)
        {
            auto& M = ClipData.Modules[i];
            std::string mlg = "DBG[Save]  M[" + std::to_string(i) + "]: Reg=" + M.Register
                + " DescA=" + M.Desc.A + " DescB=" + M.Desc.B
                + " Comment=" + (M.IsComment ? M.Comment : "NO")
                + " Hidden=" + std::to_string(M.Hidden)
                + " EqSize=(" + std::to_string(M.EqSize.x) + "," + std::to_string(M.EqSize.y) + ")"
                + " EqDelta=(" + std::to_string(M.EqDelta.x) + "," + std::to_string(M.EqDelta.y) + ")"
                + " VarCount=" + std::to_string(M.VarList.size())
                + " LinesCount=" + std::to_string(M.Lines.size());
            for (auto& v : M.VarList)
                mlg += " [" + v.A + "=" + v.B.substr(0, std::min(v.B.size(), size_t(60))) + "]";
//             GlobalLogB.AddLog(mlg.c_str());
            int lc = 0;
            for (auto& line : M.Lines)
            {
                // if (lc >= 10) { GlobalLogB.AddLog("DBG[Load]   ...(lines truncated)"); break; }
                // GlobalLogB.AddLog((std::string("DBG[Load]   LINE: ") + line.Key + "=" + line.Value.substr(0, 60)).c_str());
                lc++;
            }
        }
    }

    // Deduplicate VarList entries
    for (auto& M : ClipData.Modules)
    {
        std::set<std::pair<std::string, std::string>> seen;
        std::vector<PairClipString> deduped;
        for (auto& v : M.VarList)
            if (seen.insert({ v.A, v.B }).second)
                deduped.push_back(std::move(v));
        M.VarList = std::move(deduped);
    }

    // For each AssetFile .hva/.vxl, also add the counterpart
    for (auto& M : ClipData.Modules)
    {
        std::set<std::string> toAdd;
        for (auto& v : M.VarList)
        {
            if (v.A != "AssetFile") continue;
            std::string p = v.B;
            if (p.size() > 4)
            {
                auto ext = p.substr(p.size() - 4);
                if (_stricmp(ext.c_str(), ".hva") == 0)
                    toAdd.insert(p.substr(0, p.size() - 4) + ".vxl");
                else if (_stricmp(ext.c_str(), ".vxl") == 0)
                    toAdd.insert(p.substr(0, p.size() - 4) + ".hva");
            }
        }
        for (auto& add : toAdd)
            M.VarList.push_back({ "AssetFile", add });
    }

    // iproj_ref: sync ModProjPath metadata (diff current vs previous save)
    if (IBF_Inst_Project.Project.Path.ends_with(L".modproj"))
    {
        auto& mpPath = IBF_Inst_Project.Project.Path;

        // Collect current iproj paths
        std::set<std::string> curSet;
        for (auto& M : ClipData.Modules)
            for (auto& v : M.VarList)
                if (v.A == "iproj_path") curSet.insert(v.B);

        // GlobalLogB.AddLog((std::string("DBG[Save] MODPROJ curSetSz=") + std::to_string(curSet.size())).c_str());

        // Collect previous iproj paths from last save on disk
        std::set<std::string> prevSet;
        if (GetFileAttributesW(mpPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            auto origData = std::move(IBS_Inst_Project.Data);
            auto origPath = std::move(IBS_Inst_Project.Path);
            auto origRatio = IBS_Inst_Project.FullView_Ratio;
            auto origCenter = IBS_Inst_Project.FullView_EqCenter;
            IBS_Inst_Project.Path = mpPath;
            if (IBS_Inst_Project.Load())
            {
                IBB_ClipBoardData oldClip;
                oldClip.SetStream(IBS_Inst_Project.Data, VersionN);
                for (auto& M : oldClip.Modules)
                    for (auto& v : M.VarList)
                        if (v.A == "iproj_path") prevSet.insert(v.B);
            }
            IBS_Inst_Project.Data = std::move(origData);
            IBS_Inst_Project.Path = std::move(origPath);
            IBS_Inst_Project.FullView_Ratio = origRatio;
            IBS_Inst_Project.FullView_EqCenter = origCenter;
        }

        // GlobalLogB.AddLog((std::string("DBG[Save] MODPROJ prevSetSz=") + std::to_string(prevSet.size())).c_str());

        // Remove from deleted iproj refs
        for (auto& oldPath : prevSet)
            if (!curSet.count(oldPath))
            {
                // GlobalLogB.AddLog((std::string("DBG[Save] MODPROJ REMOVE: ") + oldPath).c_str());
                IBB_ModProject::RemoveIprojModProjPath(UTF8toUnicode(oldPath), mpPath);
            }

        // Add for current iproj refs
        for (auto& path : curSet)
        {
            // GlobalLogB.AddLog((std::string("DBG[Save] MODPROJ ADD: ") + path).c_str());
            IBB_ModProject::AddIprojModProjPath(UTF8toUnicode(path), mpPath);
        }

        // [LOG] IBS state after sync
/*        GlobalLogB.AddLog((std::string("DBG[Save] MODPROJ AFTER_SYNC: IBS_DataSz=") + std::to_string(IBS_Inst_Project.Data.size())
            + " IBS_Path=" + UnicodetoUTF8(IBS_Inst_Project.Path)).c_str()); */
    }

    // Resolve .pal asset paths from Palette=/CustomPalette= in SHP modules
    for (auto& M : ClipData.Modules)
    {
        std::string paletteName, customPalette;
        for (auto& tok : M.Lines)
        {
            if (tok.Key == "Palette" && !tok.Value.empty())
                paletteName = tok.Value;
            else if (tok.Key == "CustomPalette" && !tok.Value.empty())
                customPalette = tok.Value;
        }
        if (paletteName.empty() && customPalette.empty()) continue;

        // Find .shp path from existing AssetFile entries
        std::wstring shpDir;
        for (auto& v : M.VarList)
        {
            if (v.A == "AssetFile")
            {
                auto w = UTF8toUnicode(v.B);
                auto p = w.rfind(L'\\');
                if (p != std::wstring::npos)
                    shpDir = w.substr(0, p + 1);
                break;
            }
        }
        if (shpDir.empty()) continue;

        // Derive .pal paths
        std::vector<std::wstring> palFiles;
        if (!customPalette.empty())
        {
            palFiles.push_back(UTF8toUnicode(customPalette));
        }
        else if (!paletteName.empty())
        {
            static const wchar_t* suffixes[] = { L"tem",L"sno",L"urb",L"unb",L"lun",L"des" };
            auto nameW = UTF8toUnicode(paletteName);
            for (auto* sfx : suffixes)
                palFiles.push_back(nameW + sfx + L".pal");
        }

        for (auto& pf : palFiles)
            M.VarList.push_back({ "AssetFile", UnicodetoUTF8(shpDir + pf) });
    }

    // iproj_ref: collect rename mappings (backend VarList for correct old path)
    struct RenameEntry { std::string oldPath; std::wstring oldW, newW; ModuleClipData* M; PairClipString* v; };
    std::vector<RenameEntry> renames;
    for (auto& M : ClipData.Modules)
    {
        for (auto& v : M.VarList)
        {
            if (v.A != "iproj_path") continue;
            auto sec = IBR_Inst_Project.GetSection(IBB_Section_Desc{ M.Desc.A, M.Desc.B });
            auto bk = sec.GetBack();
            if (!bk) break;
            auto oldBk = bk->VarList.GetVariable("iproj_path");
            if (oldBk.empty()) break;
            auto oldW = UTF8toUnicode(oldBk);
            auto p = oldW.rfind(L'\\');
            auto oldName = (p != std::wstring::npos ? oldW.substr(p + 1) : oldW);
            auto newW = (p != std::wstring::npos ? oldW.substr(0, p + 1) : L"") + UTF8toUnicode(M.Desc.B) + L".iproj";
            if (_wcsicmp(oldName.c_str(), (UTF8toUnicode(M.Desc.B) + L".iproj").c_str()) != 0)
                renames.push_back({ oldBk, oldW, newW, &M, &v });
            break;
        }
    }
    // Phase 2: propagate rename to other referencing modprojs, then MoveFileW
    for (auto& r : renames)
    {
        auto& mpSelf = IBF_Inst_Project.Project.Path;
        auto paths = IBB_ModProject::GetIprojModProjPaths(r.oldW);
        for (auto& mpPath : paths)
        {
            if (_wcsicmp(mpPath.c_str(), mpSelf.c_str()) == 0) continue;
            auto origData = std::move(IBS_Inst_Project.Data);
            auto origPath = std::move(IBS_Inst_Project.Path);
            IBS_Inst_Project.Path = mpPath;
            if (IBS_Inst_Project.Load())
            {
                IBB_ClipBoardData mpClip;
                mpClip.SetStream(IBS_Inst_Project.Data, VersionN);
                for (auto& mpM : mpClip.Modules)
                {
                    bool isTarget = false;
                    for (auto& mpV : mpM.VarList)
                        if (mpV.A == "iproj_path" && mpV.B == r.oldPath)
                            { mpV.B = UnicodetoUTF8(r.newW); isTarget = true; }
                    if (isTarget)
                        mpM.Desc.B = r.M->Desc.B;
                }
                IBS_Inst_Project.Data = mpClip.GetStream();
                IBS_Inst_Project.Save();
            }
            IBS_Inst_Project.Data = std::move(origData);
            IBS_Inst_Project.Path = std::move(origPath);
        }
        if (MoveFileW(r.oldW.c_str(), r.newW.c_str()))
        {
            r.v->B = UnicodetoUTF8(r.newW);
            auto sec = IBR_Inst_Project.GetSection(IBB_Section_Desc{ r.M->Desc.A, r.M->Desc.B });
            auto bk = sec.GetBack();
            if (bk) bk->VarList.Value["iproj_path"] = r.v->B;
        }
    }

    // Preserve existing binary metadata tail before GetStream overwrites Proj.Data
    auto oldMeta = IBB_ModProject::ReadMetaTail(Proj.Data);

    Proj.Data = ClipData.GetStream();

    // Merge in new BuildOutputDir (modproj only), then write all metadata back
    if (IsModProject() && !IBF_Inst_ModProject.BuildOutputDir.empty())
        oldMeta["BuildOutputDir"] = UnicodetoUTF8(IBF_Inst_ModProject.BuildOutputDir);
    if (!oldMeta.empty())
    {
        IBB_ModProject::WriteMetaTail(Proj.Data, oldMeta);
        // GlobalLogB.AddLog((std::string("DBG[Save] MetaTail: keys=") + std::to_string(oldMeta.size())).c_str());
    }

/*    GlobalLogB.AddLog((std::string("DBG[Save] FINAL: Proj_DataSz=") + std::to_string(Proj.Data.size())
        + " ClipModules=" + std::to_string(ClipData.Modules.size())).c_str()); */
}

void IBR_Project::Clear()
{
    MaxID = 0;
    IBR_SectionMap.clear();
    IBR_Rev_SectionMap.clear();
    IBR_Rev_SectionMapII.clear();
    IBR_SecDragMap.clear();
    DragConditionText.clear();
    DragConditionTextAlt.clear();
    LinkList.clear();
}
