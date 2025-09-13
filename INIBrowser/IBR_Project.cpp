
#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include<imgui_internal.h>
#include <ranges>


std::pair<bool, std::vector<IBR_Project::id_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::AddModule(const IBB_ModuleAlt& Module, const std::string& Argument, bool UseMouseCenter)
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
std::pair<bool, std::vector<IBR_Project::id_t>> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::AddModule(const std::vector<ModuleClipData>& Modules, bool UseMouseCenter)
{
    if (Modules.empty())return { true, {} };
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
    Ret &= SetModuleIncludeLink(Vec);
    Ret &= IBF_Inst_Project.UpdateAll();
    return { Ret, Vec };
}
std::optional<IBR_Project::id_t> _PROJ_CMD_WRITE  _PROJ_CMD_UPDATE IBR_Project::AddModule(const ModuleClipData& Modules, bool UseMouseCenter)
{
    IBD_RInterruptF(x);
    bool Ret = true;
    auto R = AddModule_Impl(Modules, UseMouseCenter);
    Ret &= R.has_value();
    if (R)Ret &= SetModuleIncludeLink(R.value());
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
    //if (Module.FromClipBoard || !UseMouseCenter )
    {
        auto sd = Sec.GetSectionData();
        if (!sd)return std::nullopt;
        //sd->Dragging = true;
        sd->EqDelta = Module.EqDelta;
        //M essage BoxA(NULL, ("(" + std::to_string(sd->EqDelta.x) + " , " + std::to_string(sd->EqDelta.y) + ")").c_str(), "Load1!", MB_OK);
        sd->EqSize = Module.EqSize;
        sd->Ignore = Module.Ignore;
        sd->EqPos = InitEqPos;
        sd->IncludedByModule_TmpDesc = { Module.IncludedBySection.A, Module.IncludedBySection.B };
        for (auto& lt : Module.IncludingSections)
            sd->IncludingModules_TmpDesc.push_back({ lt.A,lt.B });
        IBRF_CoreBump.SendToR({ [sd, eq = sd->EqPos]() {sd->EqPos = eq; } });
    }
    if (Ret)return Sec.ID;
    else return std::nullopt;
}


bool _PROJ_CMD_WRITE IBR_Project::SetModuleIncludeLink(IBR_Project::id_t ID)
{
    auto Sec = GetSectionFromID(ID).GetSectionData();
    if (!Sec)return false;
    Sec->IncludedByModule = GetSectionID(Sec->IncludedByModule_TmpDesc).value_or(INVALID_MODULE_ID);
    Sec->IncludingModules = Sec->IncludingModules_TmpDesc
        | std::views::transform([this](const auto& Desc) {return GetSectionID(Desc).value_or(INVALID_MODULE_ID); })
        | std::views::filter([](const auto& id) { return id != INVALID_MODULE_ID; })
        | std::ranges::to<std::vector<id_t>>();
    return true;
}

bool _PROJ_CMD_WRITE IBR_Project::SetModuleIncludeLink(const std::vector<IBR_Project::id_t>& IDs)
{
    return std::ranges::fold_left(IDs, true, [this](bool val, IBR_Project::id_t id) { return val & SetModuleIncludeLink(id); });
}

std::optional<IBR_Project::id_t> _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::ComposeSections(const std::vector<IBR_Project::id_t>& IDs)
{
    using namespace std::ranges;
    auto RSec = CreateSectionAndBack(
        IBB_Section_Desc{ DefaultIniName, GenerateModuleTag() },
        loc("Back_ComposeBlockName")
    );

    auto BSec = RSec.GetBack();
    auto RD = RSec.GetSectionData();
    if(BSec == nullptr)return std::nullopt;
    if(RD == nullptr)return std::nullopt;
    IBG_Undo.SomethingShouldBeHere();

    BSec->Register = loc("Back_ComposeBlockName");
    RD->IncludingModules = IDs;
    sort(RD->IncludingModules, [this](id_t l, id_t r)
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

std::optional<std::vector<IBR_Project::id_t>> _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::DecomposeSection(IBR_Project::id_t ID)
{
    auto pData = GetSectionFromID(ID).GetSectionData();
    if (!pData)return std::nullopt;
    IBG_Undo.SomethingShouldBeHere();
    if (!pData->Decomposable())return std::nullopt;

    //unlink
    std::ranges::for_each(pData->IncludingModules, [this](auto id) {
        auto pData = GetSectionFromID(id).GetSectionData();
        if (pData)
        {
            pData->IncludedByModule = INVALID_MODULE_ID;
            pData->IncludedByModule_TmpDesc = {};
            pData->CollapsedInComposed = false;
        }
        });
    auto ModuleIDs { std::move(pData->IncludingModules) };
    pData->IncludingModules.clear();

    DeleteSection(ID);
    return ModuleIDs;
}

std::optional<std::vector<IBR_Project::id_t>> _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO  _PROJ_CMD_UPDATE IBR_Project::DecomposeSection(IBB_Section_Desc Desc)
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
    auto RSec = CreateSectionAndBack(
        IBB_Section_Desc { DefaultIniName, GenerateModuleTag() },
        loc("Back_CommentBlockName")
    );

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

IBR_Section _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::CreateSectionAndBack(const IBB_Section_Desc& Desc, _TEXT_UTF8 const std::string& DisplayName)
{
    CreateSection(Desc);
    EnsureSection(Desc, DisplayName);
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

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Project::DeleteSection(const std::vector <IBR_Project::id_t>& ids)
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
    bool Ret = true;
    for (auto& Desc : Descs)
    {
        if (Desc.Ini.empty() || Desc.Sec.empty())continue;
        {
            IBD_RInterruptF(x);
            if (IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ Desc }) == nullptr)continue;
            auto ci = const_cast<IBB_Ini*>(IBF_Inst_Project.Project.GetIni(IBB_Project_Index{ Desc }));
            Ret &= ci->DeleteSection(Desc.Sec);
            
        }
        IBG_Undo.SomethingShouldBeHere();
        auto RS = GetSection(Desc);
        IBF_Inst_Project.DisplayNames.erase(IBR_SectionMap[RS.ID].DisplayName);
        IBR_SectionMap.erase(RS.ID);
        IBR_Rev_SectionMap.erase(Desc);
    }
    {
        IBD_RInterruptF(x);
        IBF_Inst_Project.UpdateAll();
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
    if (ClipData.SetStream(Proj.Data, GetClipFormatVersion(Proj.GetCreateVersionN())))
        AddModule(ClipData.Modules, false);
    IBF_Inst_Project.Project.ChangeAfterSave = false;
    //MAGIC
    //DONT ASK ME WHY THERE IS SO MANY LAYERS OF SendToR
    //Actually 5 main loops are needed to fully initialize
    //So just delay 6 frames
    //If more frames will be needed you should add more layers
    //Or rewrite the Bump Logic to allow dalay in frames
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
    IBB_ClipBoardData ClipData;
    ClipData.GenerateAll(true, true);
    Proj.Data = ClipData.GetStream();
}

void IBR_Project::Clear()
{
    MaxID = 0;
    IBR_SectionMap.clear();
    IBR_Rev_SectionMap.clear();
    IBR_SecDragMap.clear();
    IBR_LineDragMap.clear();
    CopyTransform.clear();
    DragConditionText.clear();
    DragConditionTextAlt.clear();
    LinkList.clear();
}
