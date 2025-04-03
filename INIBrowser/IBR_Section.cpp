
#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include <imgui_internal.h>

const IBB_Section_Desc IBB_Section_DescNull{ "","" };

IBR_SectionData* _PROJ_CMD_READ _PROJ_CMD_NOINTERRUPT IBR_Section::GetSectionData() const
{
    auto It = Root->IBR_SectionMap.find(ID);
    return (It == Root->IBR_SectionMap.end()) ? nullptr : std::addressof(It->second);
}

bool _PROJ_CMD_NOINTERRUPT IBR_Section::Dragging() const
{
    auto It = Root->IBR_SectionMap.find(ID);
    return (It == Root->IBR_SectionMap.end()) ? false : It->second.Dragging;
}

IBB_Section_Desc* _PROJ_CMD_NOINTERRUPT IBR_Section::GetDesc() const
{
    auto It = Root->IBR_SectionMap.find(ID);
    return (It == Root->IBR_SectionMap.end()) ? nullptr : std::addressof(It->second.Desc);
}

void _PROJ_CMD_NOINTERRUPT IBR_Section::SetReOffset(const ImVec2& Offset)
{
    auto It = Root->IBR_SectionMap.find(ID);
    if (It != Root->IBR_SectionMap.end())It->second.ReOffset = Offset;
}

IBB_Section* IBR_Section::GetBack_Inl() const
{
    auto pDesc = GetDesc();
    if (pDesc == nullptr)return nullptr;
    else return const_cast<IBB_Section*>(IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ *pDesc }));
}

IBB_Section* _PROJ_CMD_READ IBR_Section::GetBack()
{
    IBD_RInterruptF(x);
    return GetBack_Inl();
}
const IBB_Section* _PROJ_CMD_READ IBR_Section::GetBack() const
{
    IBD_RInterruptF(x);
    return GetBack_Inl();
}
const IBB_Section* _PROJ_CMD_NOINTERRUPT _PROJ_CMD_READ IBR_Section::GetBack_Unsafe() const
{
    return GetBack_Inl();
}
template<typename T>
T _PROJ_CMD_READ _PROJ_CMD_WRITE IBR_Section::OperateBackData(const std::function<T(IBB_Section*)>& Function)
{
    IBD_RInterruptF(x);
    return Function(GetBack_Inl());
}
const IBB_Section_Desc& _PROJ_CMD_NOINTERRUPT IBR_Section::GetSectionDesc() _PROJ_CMD_BACK_CONST const
{
    auto psd = GetDesc();
    return psd == nullptr ? IBB_Section_DescNull : *psd;
}
bool _PROJ_CMD_READ IBR_Section::HasBack() const
{
    IBD_RInterruptF(x);
    return GetBack_Inl() != nullptr;
}
_RETURN_BACK_DATA IBB_VariableList* _PROJ_CMD_READ IBR_Section::GetVarList() _PROJ_CMD_BACK_CONST const
{
    IBD_RInterruptF(x);
    auto pSec = GetBack_Inl();
    if (pSec == nullptr)return nullptr;
    return &(pSec->VarList);
}
IBB_VariableList _PROJ_CMD_READ IBR_Section::GetVarListCopy() _PROJ_CMD_BACK_CONST const
{
    IBD_RInterruptF(x);
    auto pSec = GetBack_Inl();
    if (pSec == nullptr)return {};
    return pSec->VarList;
}
IBB_VariableList _PROJ_CMD_READ IBR_Section::GetVarListFullCopy(bool PrintExtraData) _PROJ_CMD_BACK_CONST const
{
    IBD_RInterruptF(x);
    auto pSec = GetBack_Inl();
    if (pSec == nullptr)return {};
    return pSec->GetLineList(PrintExtraData);
}
bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Section::SetVarList(const IBB_VariableList& NewList)
{
    IBD_RInterruptF(x);
    IBG_Undo.SomethingShouldBeHere();
    auto pSec = GetBack_Inl();
    if (pSec == nullptr)return false;
    pSec->VarList = NewList;
    return true;
}
bool _PROJ_CMD_READ IBR_Section::GetClipData(ModuleClipData& Clip, bool UsePosAsDelta) _PROJ_CMD_BACK_CONST
{
    IBD_RInterruptF(x);
    auto bk = GetBack_Inl();
    if (!bk)return false;
    auto dt = GetSectionData();
    if (!dt)return false;
    Clip.DisplayName = dt->DisplayName;
    Clip.EqSize = dt->EqSize;
    Clip.EqDelta = UsePosAsDelta ? dt->EqPos : dt->EqDelta;
    Clip.Ignore = dt->Ignore;
    bk->GetClipData(Clip);
    return true;
}
ImColor _PROJ_CMD_READ IBR_Section::GetRegTypeColor() _PROJ_CMD_BACK_CONST const
{
    IBD_RInterruptF(x);
    auto bk = GetBack_Inl();
    if (!bk)return IBB_DefaultRegType::DefaultColor;
    else return IBB_DefaultRegType::GetRegType(bk->Register).FrameColor;
}
bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Section::DuplicateSection(const IBB_Section_Desc& NewDesc) const
{
    IBD_RInterruptF(x);
    IBG_Undo.SomethingShouldBeHere();
    auto pSec = GetBack_Inl();
    if (pSec == nullptr)return false;

    if (!IBF_Inst_Project.Project.CreateNewSection(NewDesc))return false;

    IBF_Inst_Project.UpdateCreateSection(NewDesc);

    auto pNSec = const_cast<IBB_Section*>(IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ NewDesc }));
    if (pNSec == nullptr)return false;
    bool Ret = pNSec->GenerateAsDuplicate(*pSec);

    pNSec->RedirectLinkAsDupicate();

    //TODO: Check if Update Works
    //TODO: Undo Action
    return Ret;
}
IBR_Section  _PROJ_CMD_READ _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Section::DuplicateSectionAndBack(const IBB_Section_Desc& NewDesc) _PROJ_CMD_BACK_CONST const
{
    DuplicateSection(NewDesc);
    return Root->GetSection(NewDesc);
}

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO _PROJ_CMD_UPDATE IBR_Section::Rename(const std::string& NewName)
{
    IBG_Undo.SomethingShouldBeHere();
    bool Ret = true;
    auto pDesc = GetDesc();
    if (pDesc == nullptr)return false;

    {
        IBD_RInterruptF(x);
        auto pSec = const_cast<IBB_Section*>(IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ *pDesc }));
        IBB_Section_Desc desc = *pDesc; desc.Sec = NewName;
        auto npSec = const_cast<IBB_Section*>(IBF_Inst_Project.Project.GetSec(IBB_Project_Index{ desc }));
        if (pSec == nullptr || npSec != nullptr)Ret = false;
        else
        {
            auto OldName = pSec->Name;
            Ret = pSec->Rename(NewName);

            auto pIni = pSec->Root;
            auto Node = pIni->Secs.extract(OldName);
            Node.key() = NewName;
            auto sp = pIni->Secs.insert(std::move(Node));
            for (auto& s : pIni->Secs_ByName)if (s == OldName)s = NewName;
        }
    }

    pDesc->Sec = NewName;
    //Update Contained in Rename so no Extra
    //TODO: Undo Action
    return Ret;
}

bool _PROJ_CMD_WRITE _PROJ_CMD_CAN_UNDO IBR_Section::Register(const std::string& Name, const std::string& IniName) _PROJ_CMD_BACK_CONST const
{
    IBD_RInterruptF(x);
    IBG_Undo.SomethingShouldBeHere();
    return IBF_Inst_Project.Project.RegisterSection(Name, IniName, *GetBack_Inl());
}

