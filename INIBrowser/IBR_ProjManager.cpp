#include "IBR_Project.h"
#include "IBR_Misc.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_RegType.h"
#include "IBB_ModuleAlt.h"
#include <imgui_internal.h>
#include <shlwapi.h>
#include <ranges>
#include <format>
#include "IBR_HotKey.h"
#include "IBR_ListView.h"
#include "IBR_Components.h"
#include "IBB_OutputOrder.h"
#include "IBG_UndoTree.h"
#include "IBB_ModProject.h"

extern wchar_t CurrentDirW[];
extern bool ShouldCloseShellLoop;
extern bool GotoCloseShellLoop;
extern const char* Internal_IniName;
extern std::atomic_bool LoadDatabaseComplete;

std::string ExtName(const std::string& ss);//拓展名，无'.' 
std::string FileNameNoExt(const std::string& ss);//文件名，无'.' 
std::string FileName(const std::string& ss);//文件名
std::wstring FileName(const std::wstring& ss);//文件名
bool IsExistingDir(const wchar_t* Path);
extern bool RefreshLangBuffer2;
std::vector<std::string> SplitParam(const std::string& Text);

std::wstring RemoveSpec(std::wstring W)
{
    PathRemoveFileSpecW(W.data());
    W.resize(wcslen(W.data()));
    return W;
}

namespace IBB_DefaultRegType
{
    extern std::unordered_set<StrPoolID> RingCheckKeys;
    extern std::unordered_map<StrPoolID, IBB_SubSec_Default::_Type> InSubSecKeys;
}

namespace IBR_ProjectManager
{
    InfoStack<StdMessage> ActionAfterClose;
    std::atomic_bool OutputComplete{ false };

#define _IN_SAVE_THREAD
#define _IN_FRONT_THREAD
#define _IN_RENDER_THREAD
#define _IN_ANY_THREAD

    //ClearCurrentPopup的调用起始点没有线程要求
    inline void _IN_RENDER_THREAD SetWaitingPopup()
    {
        IBR_PopupManager::SetCurrentPopup(
            std::move(IBR_PopupManager::Popup{}
                .CreateModal(loc("GUI_WaitingTitle"), false)
                .SetFlag(
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoNav |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize)
                .SetSize({ FontHeight * 10.0F,FontHeight * 6.0F })
                .PushTextBack(loc("GUI_WaitingText"))));
    }

    bool _IN_RENDER_THREAD Create()
    {
        auto& proj = IBF_Inst_Project.Project;
        proj.ProjName = locw("Back_DefaultProjectName");
        proj.Path = L"";
        proj.LastOutputDir = L"";
        proj.LastOutputIniName.clear();
        proj.IsNewlyCreated = true;
        proj.ChangeAfterSave = false;
        proj.LastUpdate = GetSysTimeMicros();
        proj.CreateTime = GetSysTimeMicros();
        proj.CreateVersionMajor = VersionMajor;
        proj.CreateVersionMinor = VersionMinor;
        proj.CreateVersionRelease = VersionRelease;

        /*
        必须加这两行以应对一个十分刁钻的bug：
        在项目里面，
        如果第一个模块在art这个ini中，
        且这个模块的第一个语句不在字典当中，
        且这个语句的连接类型在任何配置当中都没出现过，
        且Rules在任何配置当中也没出现过
        且连接类型需要向Rules写入注册表，
        那么就会在载入这个项目时崩溃
        */
        proj.CreateIni(DefaultIniName);
        proj.CreateIni(Internal_IniName);

        auto A1 = (uint64_t)GlobalRnd();
        auto A2 = (uint64_t)GlobalRnd();
        IBF_Inst_Project.PersistentID = A1 << 32 | A2;
        IBF_Inst_Project.CurrentProjectRID = GlobalRnd();
        IBB_DefaultRegType::ClearModuleCount();
        IsProjectOpen = true;
        return true;
    }
    bool _IN_RENDER_THREAD Close() {
        IsProjectOpen = false;
        IBB_DefaultRegType::ClearModuleCount();
        {
            IBD_RInterruptF(x);
            IBF_Inst_Project.Clear();
            IBR_Inst_Project.Clear();
            IBG_Undo.Clear();
            IBR_WorkSpace::Close();
            IBR_EditFrame::Clear();
            IBR_PopupManager::ClearRightClickMenu();
            IBR_ListView::ClearSort();
        }
        if (GotoCloseShellLoop)
        {
            ShouldCloseShellLoop = true;
        }
        if (!ActionAfterClose.Empty())
        {
            IBRF_CoreBump.SendToR({ []() {
                for (auto& Action : ActionAfterClose.Release())
                    Action();
            } });
        }
        return true;
    }
    bool _IN_ANY_THREAD IsOpen() { return IsProjectOpen; }

    void _IN_RENDER_THREAD AskFilePath(const InsertLoad::SelectFileType& Type, BOOL(_stdcall* Proc)(LPOPENFILENAMEW), const std::function<void(const std::optional<std::wstring>&)>& _IN_SAVE_THREAD Next)
    {
        IBS_Push([=]()
            {
                auto Ret = InsertLoad::SelectFileName(MainWindowHandle, Type, Proc, false);
                if (Ret.Success)Next(Ret.RetBuf);
                else Next(std::nullopt);
            });
    }

    const std::wstring& PathFilter()
    {
        static std::wstring Filter;
        if (Filter.empty() || RefreshLangBuffer2)
        {
            auto T2 = locw("GUI_OutputModule_Type2");
            auto T3 = locw("GUI_OutputModule_Type3");
            auto T5 = locw("GUI_OutputModule_Type5");
            auto L0 = (int)wcslen(L"*" ExtensionNameW);
            auto LM = (int)wcslen(L"*.modproj");
            auto LA = 4;
            Filter.clear();
            Filter.reserve(T3.size() + 1 + L0 + 1 + T5.size() + 1 + LM + 1 + T2.size() + 1 + LA + 1);
            Filter.append(T3);       Filter.push_back(L'\0');
            Filter.append(L"*" ExtensionNameW); Filter.push_back(L'\0');
            Filter.append(T5);       Filter.push_back(L'\0');
            Filter.append(L"*.modproj");       Filter.push_back(L'\0');
            Filter.append(T2);       Filter.push_back(L'\0');
            Filter.append(L"*.*");   Filter.push_back(L'\0');
        }
        RefreshLangBuffer2 = false;
        return Filter;
    }

    void _IN_RENDER_THREAD AskLoadPath(const std::function<void(const std::optional<std::wstring>&)>& _IN_SAVE_THREAD Next)
    {
        AskFilePath(InsertLoad::SelectFileType{ CurrentDirW ,locw("GUI_OpenProject"),
            L"",PathFilter().c_str()}, ::GetOpenFileNameW, Next);
    }
    void _IN_RENDER_THREAD AskSavePath(const std::function<void(const std::optional<std::wstring>&)>& _IN_SAVE_THREAD Next)
    {
        AskFilePath(InsertLoad::SelectFileType{ CurrentDirW ,locw("GUI_SaveProject"),
            IBF_Inst_Project.Project.IsNewlyCreated ? locw("Back_DefaultProjectName") + ExtensionNameW : IBF_Inst_Project.Project.ProjName
            ,PathFilter().c_str() }, ::GetSaveFileNameW, Next);
    }

    void _IN_SAVE_THREAD Save(std::wstring Path, const std::function<void(bool)>& _IN_SAVE_THREAD Next)
    {
        if (!Path.ends_with(ExtensionNameW) && !Path.ends_with(L".modproj"))Path += ExtensionNameW;
        IBR_RecentManager::Push(Path);
        IBR_RecentManager::Save();

        auto SigF = std::make_shared<std::atomic_bool>(false);
        auto SigR = std::make_shared<std::atomic_bool>(false);
        auto Sent = std::make_shared<std::atomic_bool>(false);
        IBRF_CoreBump.SendToR({ [=]()
            {
                IBR_Inst_Project.Save(IBS_Inst_Project);
                *SigR = true;
                if (*SigF && (!*Sent)) { *Sent = true; IBS_Push([=]() {Next(IBS_Inst_Project.Save(Path)); }); }
            },nullptr });
        IBRF_CoreBump.SendToF({ [=]()
            {
                IBF_Inst_Project.Save(IBS_Inst_Project);
                *SigF = true;
                if (*SigR && (!*Sent)) { *Sent = true; IBS_Push([=]() {Next(IBS_Inst_Project.Save(Path)); }); }
            } });
    }
    void _IN_SAVE_THREAD Load(const std::wstring& Path, const std::function<void(bool)>& _IN_SAVE_THREAD Next)
    {
        if (IBS_Inst_Project.Load(Path))
        {
            while (!LoadDatabaseComplete)Sleep(0);
            std::shared_ptr<bool> SigF = std::make_shared<bool>(false), SigR = std::make_shared<bool>(false);
            IBRF_CoreBump.SendToR({ [=]()
                {
                    IBR_Inst_Project.Load(IBS_Inst_Project);
                    *SigR = true;
                },nullptr });
            IBRF_CoreBump.SendToF({ [=]()
                {
                    IBF_Inst_Project.Load(IBS_Inst_Project);
                    *SigF = true;
                    while (!*SigR);
                    Next(true);
                } });
        }
        else Next(false);
    }
    void _IN_RENDER_THREAD AskIfSave(const std::function<void(bool)>& _IN_RENDER_THREAD Next)
    {
        IBR_PopupManager::SetCurrentPopup(std::move(
            IBR_PopupManager::Popup{}
        .CreateModal(loc("GUI_WaitingTitle"), false, []() {IBR_HintManager::SetHint(loc("GUI_ActionCanceled"), HintStayTimeMillis); })
            .SetFlag(
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize)
            .SetSize({ FontHeight * 15.0F,FontHeight * 7.0F })
            .PushTextBack(loc("GUI_AskIfSave"))
            .PushMsgBack([=]()
                {
                    if (ImGui::Button((loc("GUI_AskIfSave_Yes") + u8"##AskIfSave").c_str(), { FontHeight * 4.0f,FontHeight * 2.0f }))
                    {
                        IBRF_CoreBump.SendToR({ [=]() {  Next(true); IBR_PopupManager::ClearCurrentPopup(); }, nullptr });
                    }ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FontHeight * 0.7f);
                    if (ImGui::Button((loc("GUI_AskIfSave_No") + u8"##AskIfSave").c_str(), { FontHeight * 4.0f,FontHeight * 2.0f }))
                    {
                        IBRF_CoreBump.SendToR({ [=]() {  Next(false); IBR_PopupManager::ClearCurrentPopup(); }, nullptr});
                    }ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FontHeight * 0.7f);
                    if (ImGui::Button((loc("GUI_AskIfSave_Cancel") + u8"##AskIfSave").c_str(), { FontHeight * 4.0f,FontHeight * 2.0f }))
                    {
                        IBRF_CoreBump.SendToR({ [=]() {IBR_PopupManager::ClearCurrentPopup();
                        GotoCloseShellLoop = false; glfwSetWindowShouldClose(PreLink::window, false); }, nullptr });
                    }
                })));
    }

    void _IN_FRONT_THREAD OutputRegister(std::vector<std::unordered_map<std::string, std::string>>& TextPieces)
    {
        auto& Proj = IBF_Inst_Project.Project;
        std::unordered_map<std::string, std::vector<std::string>> Reg;
        for (auto& Ini : Proj.Inis)
            for (auto& [SN, Sec] : Ini.Secs)
                Reg[PoolStr(Sec.Register)].push_back(Sec.Name);
        for (auto& [N, R] : Reg)
        {
            auto& RegType = IBB_DefaultRegType::GetRegType(N);
            if (!RegType.Export)continue;
            IBB_Project_Index Idx(RegType.IniType);
            Idx.GetIni(Proj);
            //MessageBoxA(NULL, Idx.GetIni(Proj)->Name.c_str(), std::to_string(Idx.Ini.Index).c_str(), MB_OK);
            std::string V;
            const auto& ExportName = RegType.ExportName.empty() ? N : RegType.ExportName;
            V += '[';V += ExportName;V += "]\n";
            for (auto& v : R)
            {
                V += v; V += '='; V += v; V += '\n';
            }
            TextPieces[Idx.Ini.Index][N] = std::move(V);
        }
    }
    void _IN_FRONT_THREAD Output(const std::wstring& Path, const std::vector<std::wstring>& TargetIniPath,const std::set<IBB_Section_Desc>& IgnoredSection, bool TriggerAfterAction)
    {
        using namespace std::string_literals;
        std::string TimeNowU8();
        auto& Inis = IBF_Inst_Project.Project.Inis;
        if (TargetIniPath.size() != Inis.size())
        {
            MessageBoxW(NULL, locwc("Error_INICountNotMatched"), locwc("GUI_OutputFailure"), MB_OK);
            IBRF_CoreBump.SendToR({ [] {
                IBR_HintManager::SetHint(loc("GUI_OutputFailure"),HintStayTimeMillis);
                IBR_PopupManager::ClearCurrentPopup(); } });
            return;
        }
        ExportContext::MergedDescs.clear();

        std::map<IBB_Section_Desc, std::string> DisplayRev;
        for (auto& [K, V] : IBF_Inst_Project.DisplayNames)DisplayRev[V] = K;

        //auto ImportOrder = TopoSortByImport(IBF_Inst_Project.Project);
        auto KeyOrders = IBB_DefaultRegType::RingCheckKeys |
            std::views::transform([](StrPoolID Key) { return std::make_pair(Key, TopoSortByKeyID(IBF_Inst_Project.Project, Key)); }) |
            std::ranges::to<std::vector>();
        auto KeyOrderHasCycle = std::ranges::any_of(KeyOrders, [](const auto& b) { return b.second.HasCycle; });
        auto InheritOrder = TopoSortByKeyID(IBF_Inst_Project.Project, InheritKeyID());
        auto LinkOrder = TopoSortByKeyLink(IBF_Inst_Project.Project);
        auto HasCycle = KeyOrderHasCycle || InheritOrder.HasCycle || LinkOrder.HasCycle;

        if (HasCycle)
        {
            IBRF_CoreBump.SendToR({ [] {
                IBR_HintManager::SetHint(loc("GUI_OutputFailure"),HintStayTimeMillis);
                IBR_PopupManager::ClearCurrentPopup();
            } });
        }
        for (auto& [KeyID, KeyOrder] : KeyOrders)
        {
            if (KeyID == InheritKeyID())continue;//继承的环会在后面单独弹窗
            if (KeyOrder.HasCycle)
            {
                auto Str = KeyOrder.Order_Or_Ring |
                    std::views::transform([&DisplayRev](const IBB_Section* Sec) -> std::string {
                    auto Desc = Sec->GetThisDesc();
                    if (DisplayRev.contains(Desc))return DisplayRev[Desc];
                    else return Desc.GetText();
                        }) |
                    std::views::join_with(" -> \n"s) |
                            std::ranges::to<std::string>();

                        auto StrW = UTF8toUnicode(Str);
                        auto Fmt = UnicodetoUTF8(std::vformat(locw("Log_CycleInfo"), std::make_wformat_args(StrW)));
                        auto Key = UTF8toUnicode(PoolStr(KeyID));
                        auto Fmt2 = UnicodetoUTF8(std::vformat(locw("Log_KeyCycleDetected"), std::make_wformat_args(Key)));
                        IBRF_CoreBump.SendToR({ [F = std::move(Fmt), F2 = std::move(Fmt2)]() mutable {
                            IBR_PopupManager::AddOutputErrorPopup(std::move(F), F2);
                        } });
            }
        }
        if (InheritOrder.HasCycle)
        {
            auto Str = InheritOrder.Order_Or_Ring |
                std::views::transform([&DisplayRev](const IBB_Section* Sec) -> std::string {
                auto Desc = Sec->GetThisDesc();
                if (DisplayRev.contains(Desc))return DisplayRev[Desc];
                else return Desc.GetText();
                    }) |
                std::views::join_with(" -> \n"s) |
                        std::ranges::to<std::string>();

                auto StrW = UTF8toUnicode(Str);
                auto Fmt = UnicodetoUTF8(std::vformat(locw("Log_CycleInfo"), std::make_wformat_args(StrW)));
                IBRF_CoreBump.SendToR({ [F = std::move(Fmt)] () mutable {
                    IBR_PopupManager::AddOutputErrorPopup(std::move(F), loc("Log_InheritCycleDetected"));
                } });
        }
        if (LinkOrder.HasCycle)
        {
            auto Str = LinkOrder.Order_Or_Ring |
                std::views::transform([&DisplayRev](const IBB_LineLocation& Loc) -> std::string {
                auto Desc = Loc.Sec.ToDesc();
                if (DisplayRev.contains(Desc))return DisplayRev[Desc] + Loc.GetTextBackPart();
                else return Loc.GetText();
                    }) |
                std::views::join_with(" -> \n"s) |
                        std::ranges::to<std::string>();

                    auto StrW = UTF8toUnicode(Str);
                    auto Fmt = UnicodetoUTF8(std::vformat(locw("Log_CycleInfo"), std::make_wformat_args(StrW)));
                    IBRF_CoreBump.SendToR({ [F = std::move(Fmt)]() mutable {
                        IBR_PopupManager::AddOutputErrorPopup(std::move(F), loc("Log_KeyLinkCycleDetected"));
                    } });
        }
        if (HasCycle)return;

        size_t N = TargetIniPath.size();
        std::vector<std::unordered_map<std::string, std::string>>TextPieces;
        std::unordered_map<std::string, std::vector<std::string>>TextPieceOrder;

        //Generate TextPieces
        TextPieces.resize(N);
        OutputRegister(TextPieces);
        for (size_t I = 0; I < N; I++)
        {
            for (auto& [SN, Sec] : Inis[I].Secs)
            {
                for (auto& Sub : Sec.SubSecs)for (auto& L : Sub.Lines)
                    if (L.first == InheritKeyID())Sec.Inherit = L.second.Indexed(0)->GetString();
                IBB_Section_Desc Desc = { Inis[I].Name, Sec.Name };
                if (
                    IgnoredSection.contains(Desc) ||
                    Sec.IsComment() ||
                    Sec.SkipExport  ||
                    Sec.SingleVal
                )continue;
                std::string V;

                auto Inherit = Sec.FinalInherit();

                V += ';'; V += DisplayRev[Desc];  V += '\n';
                if (Inherit.empty())
                {
                    V += '['; V += Sec.Name;
                    V += "]\n";
                }
                else
                {
                    for (auto&& inh : SplitParam(Inherit))
                    {
                        V += '['; V += Sec.Name;
                        V += "]:["; V += inh;
                        V += "]\n";
                    }
                }
                V += Sec.GetText(false, true);
                
                TextPieces[I][Sec.Name] = std::move(V);
            }
        }

        for (auto Sec : InheritOrder.Order_Or_Ring)
            TextPieceOrder[Sec->Root->Name].push_back(Sec->Name);

        for (size_t I = 0; I < N; I++)
        {
            auto& IniName = Inis[I].Name;
            if (IniName == Internal_IniName)continue;//不导出这个
            if (TextPieces[I].empty())continue;

            ExtFileClass F;
            if (F.Open(TargetIniPath[I].c_str(), L"w"))
            {
                auto cwa = locw("AppName");
                auto cwb = UTF8toUnicode(TimeNowU8());
                F.PutStr(";" + UnicodetoUTF8(std::vformat(locw("Back_OutputINILine1"), std::make_wformat_args(cwa, VersionW)))); F.Ln();
                F.PutStr(";" + UnicodetoUTF8(std::vformat(locw("Back_OutputINILine2"), std::make_wformat_args(IBF_Inst_Project.Project.ProjName)))); F.Ln();
                F.PutStr(";" + UnicodetoUTF8(std::vformat(locw("Back_OutputINILine3"), std::make_wformat_args(cwb)))); F.Ln();

                F.Ln();
                F.PutStr(";" + loc("Back_OutputINILine4")); F.Ln();
                F.PutStr("[INI_INFO]"); F.Ln();
                F.PutStr("IniType = " + IniName); F.Ln(); F.Ln();

                for (auto& S : TextPieceOrder[IniName])
                {
                    if (ExportContext::MergedDescs.contains({ IniName, S }))
                        continue;
                    auto& V = TextPieces[I][S];
                    if (V.empty())continue;
                    F.PutStr(V);
                    F.Ln();
                    F.Ln();
                    TextPieces[I].erase(S);
                }

                for (auto& [K, V] : TextPieces[I])
                {
                    if (ExportContext::MergedDescs.contains({ IniName, K }))
                        continue;
                    if (V.empty())continue;
                    F.PutStr(V);
                    F.Ln();
                    F.Ln();
                }
            }
            else
            {
                MessageBoxW(NULL, std::vformat(locw("Error_CannotOpenAndWrite"),
                    std::make_wformat_args(TargetIniPath[I])).c_str(), locwc("GUI_OutputFailure"), MB_OK);
            }
        }

        if (TriggerAfterAction)
        {
            IBRF_CoreBump.SendToR({ [] {
                IBR_HintManager::SetHint(loc("GUI_OutputSuccess"),HintStayTimeMillis);
                IBR_PopupManager::ClearCurrentPopup(); } });
            if (IBF_Inst_Setting.OpenFolderOnOutput())
                ShellExecuteW(NULL, L"open", L"explorer.exe", Path.c_str(), NULL, SW_SHOWNORMAL);
        }

        OutputComplete = true;
        IBR_PopupManager::ClearPopupDelayed();
    }
    std::set<IBB_Section_Desc> _IN_RENDER_THREAD GetIgnoredSection()
    {
        std::set<IBB_Section_Desc> Result;
        for (auto& [id, sd] : IBR_Inst_Project.IBR_SectionMap)
        {
             if(sd.Ignore)Result.insert(sd.Desc);
             if (sd.IsVirtualBlock())Result.insert(sd.Desc);
        }
        return Result;
    }

    void _IN_RENDER_THREAD OpenAction()
    {
        SetWaitingPopup();
        AskLoadPath([](auto ws)
            {
                if (ws)
                {
                    IBR_RecentManager::Push(ws.value());
                    IBR_RecentManager::Save();
                    Load(ws.value(), [](bool OK) {IBRF_CoreBump.SendToR({ [=]()
                            {
                                IBR_PopupManager::ClearCurrentPopup();
                                if (OK)
                                {
                                    IBR_HintManager::SetHint(loc("GUI_LoadSuccess"), HintStayTimeMillis);
                                    IBF_Inst_Project.CurrentProjectRID = GlobalRnd();
                                    IsProjectOpen = true;
                                }
                                else IBR_HintManager::SetHint(loc("GUI_LoadFailure"), HintStayTimeMillis);
                            }, nullptr }); });
                }
                else {
                    IBR_HintManager::SetHint(loc("GUI_ActionCanceled"), HintStayTimeMillis);
                    IBR_PopupManager::ClearPopupDelayed();
                    CreateAction();
                }
            });
    }
    void _IN_RENDER_THREAD OpenRecentAction(const std::wstring& Path)
    {
        SetWaitingPopup();

        IBR_RecentManager::Push(Path);
        IBR_RecentManager::Save();
        IBS_Push([=]()
            {
                Load(Path, [](bool OK) {IBRF_CoreBump.SendToR({ [=]()
                    {
                        IBR_PopupManager::ClearCurrentPopup();
                        if (OK)
                            {
                                IBR_HintManager::SetHint(loc("GUI_LoadSuccess"), HintStayTimeMillis);
                                IBF_Inst_Project.CurrentProjectRID = GlobalRnd();
                                IsProjectOpen = true;
                            }
                        else
                        {
                            IBR_HintManager::SetHint(loc("GUI_LoadFailure"), HintStayTimeMillis);
                            IBR_RecentManager::WanDuZiLe();
                        }
                    }, nullptr }); });
            });
    }
    void _IN_RENDER_THREAD CreateAction()
    {
        Create();
    }

    void _IN_RENDER_THREAD CloseAction()
    {

        if (!IBF_Inst_Project.Project.ChangeAfterSave)
        {
            IBRF_CoreBump.SendToR({ [=]() {Close(); } });
            return;
        }
        AskIfSave([](bool OK)
            {
                if (OK)
                {
                    if (!IBF_Inst_Project.Project.IsNewlyCreated)
                    {
                        SetWaitingPopup();
                        IBS_Push([]()
                            {
                                Save(IBF_Inst_Project.Project.Path, [](bool OK2) 
                                    {
                                        IBR_PopupManager::ClearCurrentPopup();
                                        if (OK2)
                                        {
                                            Close();
                                            IBR_HintManager::SetHint(loc("GUI_SaveAndCloseSuccess"), HintStayTimeMillis);
                                        }
                                        else IBR_HintManager::SetHint(loc("GUI_SaveAndCloseFailure"), HintStayTimeMillis);
                                    });
                            });
                    }
                    else
                    {
                        SetWaitingPopup();
                        AskSavePath([](auto ws)
                            {
                                if (ws)Save(ws.value(), [](bool OK2) 
                                    {
                                        IBR_PopupManager::ClearCurrentPopup();
                                        if (OK2)
                                        {
                                            Close();
                                            IBR_HintManager::SetHint(loc("GUI_SaveAndCloseSuccess"), HintStayTimeMillis);
                                        }
                                        else IBR_HintManager::SetHint(loc("GUI_SaveAndCloseFailure"), HintStayTimeMillis);
                                    });
                                else { IBR_HintManager::SetHint(loc("GUI_ActionCanceled"), HintStayTimeMillis); IBR_PopupManager::ClearPopupDelayed(); }
                            });
                    }
                }

                else
                {
                    IBRF_CoreBump.SendToR({ [=]() {Close(); } });
                    IBR_HintManager::SetHint(loc("GUI_SaveAndCloseSuccess"), HintStayTimeMillis);
                }
            });
    }
    void _IN_RENDER_THREAD SaveAction()
    {
        if (!IBF_Inst_Project.Project.ChangeAfterSave && !IsModProject())
        {
            IBR_ProjectManager::OutputOnSaveAction();
            return;
        }
        //make someone happy
        //SetWaitingPopup();
        IBS_Push([=]()
            {
                Save(IBF_Inst_Project.Project.Path, [](bool OK) {IBRF_CoreBump.SendToR({ [=]()
                {
                    //IBR_PopupManager::ClearCurrentPopup();
                    if (OK)IBR_HintManager::SetHint(loc("GUI_SaveSuccess"), HintStayTimeMillis);
                    else IBR_HintManager::SetHint(loc("GUI_SaveFailure"), HintStayTimeMillis);
                }, nullptr }); });
            });

    }
    void _IN_RENDER_THREAD SaveAsAction()
    {
        SetWaitingPopup();
        AskSavePath([](auto ws)
            {
                if (ws)Save(ws.value(), [ws](bool OK) {IBRF_CoreBump.SendToR({ [=]()
                    {
                        IBR_PopupManager::ClearCurrentPopup();
                        if (OK)
                        {
                            IBF_Inst_Project.Project.Path = ws.value();
                            IBF_Inst_Project.Project.ProjName = FileName(ws.value());
                            IBR_HintManager::SetHint(loc("GUI_SaveSuccess"), HintStayTimeMillis);
                        }
                        else IBR_HintManager::SetHint(loc("GUI_SaveFailure"), HintStayTimeMillis);
                    }, nullptr }); });
                else { IBR_HintManager::SetHint(loc("GUI_ActionCanceled"), HintStayTimeMillis); IBR_PopupManager::ClearPopupDelayed(); }
            });
    }
    void _IN_RENDER_THREAD SaveOptAction()
    {
        if (IBF_Inst_Project.Project.IsNewlyCreated)SaveAsAction();
        else SaveAction();
    }
    void _IN_RENDER_THREAD OutputAction()
    {
        struct IniNameInput
        {
            bool Warning{ false };
            std::string Name;
            std::shared_ptr<BufString> Buf;
            bool Ignore{ false };
            IniNameInput() :Buf(new BufString) {}
        };

        std::vector<IniNameInput> Inis;
        std::shared_ptr<BufString> PathBuffer{ new BufString{} };
        std::shared_ptr<bool> OK{ new bool(false) };
        std::wstring WP;
        {
            IBD_RInterruptF(x);
            Inis.resize(IBF_Inst_Project.Project.Inis.size());
            auto V = IBF_Inst_Project.Project.ProjName;
            auto S = V.find_last_of(L'.');
            if (S != std::wstring::npos)
            {
                V = V.substr(0, S);
            }
            auto U = UnicodetoUTF8(V);
            
            for (size_t i = 0; i < Inis.size(); i++)
            {
                Inis[i].Name = IBF_Inst_Project.Project.Inis[i].Name;
                const auto& T = IBF_Inst_Project.Project.LastOutputIniName[Inis[i].Name];
                if(!T.empty())strcpy(Inis[i].Buf.get(), UnicodetoUTF8(T).c_str());
                else strcpy(Inis[i].Buf.get(), (U + "_" + IBF_Inst_Project.Project.Inis[i].Name + ".ini").c_str());
                Inis[i].Ignore = true;
            }
            for (size_t i = 0; i < Inis.size(); i++)
            {
                if (!IBF_Inst_Project.Project.Inis[i].Secs.empty())
                {
                    Inis[i].Ignore = false;
                    continue;
                }
                for (auto& [N, Q] : IBF_Inst_Project.Project.Inis[i].Secs)
                {
                    const auto& ity = IBB_DefaultRegType::GetRegType(Q.Register).IniType;
                    IBB_Project_Index idx = { ity, "" };
                    if (idx.GetIni(IBF_Inst_Project.Project) != nullptr)
                    {
                        Inis[idx.Ini.Index].Ignore = false;
                    }
                }
            }

            if(!IBF_Inst_Project.Project.LastOutputDir.empty())
                WP = IBF_Inst_Project.Project.LastOutputDir;
            else if (!IBF_Inst_Project.Project.Path.empty())
                WP = RemoveSpec(IBF_Inst_Project.Project.Path);

            strcpy(PathBuffer.get(), UnicodetoUTF8(WP).c_str());
            *OK = IsExistingDir(WP.c_str());
        }
        auto PF{ []() {IBR_HintManager::SetHint(loc("GUI_ActionCanceled"),HintStayTimeMillis); } };
        IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}.CreateModal(loc("GUI_Output_Title"), true, PF)
            .SetSize({ FontHeight * 20.0f,FontHeight * 15.0f })
            .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize).PushMsgBack([=]() mutable
                {
                    if (ImGui::InputText(locc("GUI_Output_Path"), PathBuffer.get(), MAX_STRING_LENGTH))
                    {
                        WP = UTF8toUnicode(PathBuffer.get());
                        *OK = IsExistingDir(WP.c_str());
                        for (auto& I : Inis)I.Warning= PathFileExistsW((WP + UTF8toUnicode(I.Buf.get())).c_str());
                    }
                    if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
                    ImGui::SameLine();
                    if (ImGui::SmallButton("..."))
                    {
                        auto Ret = InsertLoad::SelectFolderName(MainWindowHandle,InsertLoad::SelectFileType{ WP ,locw("GUI_Browse"), L"", L""});
                        if (Ret.Success)
                        {
                            WP = Ret.RetBuf;
                            strcpy(PathBuffer.get(), UnicodetoUTF8(WP).c_str());
                            *OK = IsExistingDir(WP.c_str());
                            for (auto& I : Inis)I.Warning = PathFileExistsW((WP + L"\\" + UTF8toUnicode(I.Buf.get())).c_str());
                        }
                    }

                    for (auto& I : Inis)
                    {
                        if (I.Name == Internal_IniName)continue; //不显示这个，因为不导出
                        if (I.Ignore)continue;

                        ImGui::InputText(("##" + I.Name).c_str(), I.Buf.get(), MAX_STRING_LENGTH);
                        if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
                        I.Warning = PathFileExistsW((WP + L"\\" + UTF8toUnicode(I.Buf.get())).c_str());//Refresh per Frame
                        if (I.Warning)ImGui::Text(locc("GUI_Output_Warning1"));
                    }

                    if (*OK && !WP.empty())
                    {
                        if (ImGui::Button(locc("GUI_OK")))
                        {
                            auto V = GetIgnoredSection();
                            std::vector<std::wstring> TgPath;
                            TgPath.reserve(Inis.size());
                            OutputComplete = false;
                            for (auto& I : Inis)TgPath.push_back(WP + L"\\" + UTF8toUnicode(I.Buf.get()));

                            IBRF_CoreBump.SendToF([=] {
                                IBF_Inst_Setting.OutputDir() = UnicodetoUTF8(WP);
                                IBF_Inst_Setting.SaveSetting(IBR_Inst_Setting.SettingName);
                                if (IBF_Inst_Project.Project.LastOutputDir != WP)
                                {
                                    IBF_Inst_Project.Project.LastOutputDir = WP;
                                    IBF_Inst_Project.Project.ChangeAfterSave = true;
                                }
                                for (auto& I : Inis)
                                {
                                    auto G = UTF8toUnicode(I.Buf.get());
                                    auto& U = IBF_Inst_Project.Project.LastOutputIniName[I.Name];
                                    if (U != G)
                                    {
                                        U = G;
                                        IBF_Inst_Project.Project.ChangeAfterSave = true;
                                    }
                                }
                                Output(WP, TgPath, V, true); });
                            IBRF_CoreBump.SendToR({ [] { IBR_PopupManager::ClearCurrentPopup(); if(!OutputComplete)SetWaitingPopup(); } });
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled(locc("GUI_OK"));
                        ImGui::SameLine();
                        if (WP.empty())
                        {
                            ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_Output_Error1"));
                            ImGui::SameLine();
                        }
                        if (!*OK)
                        {
                            ImGui::TextColored(IBR_Color::ErrorTextColor, locc("GUI_Output_Error2"));
                            ImGui::SameLine();
                        }
                    }
                })));
    }
    void _IN_RENDER_THREAD AutoOutputAction()
    {
        std::wstring WP;
        if (!IBF_Inst_Project.Project.LastOutputDir.empty())
            WP = IBF_Inst_Project.Project.LastOutputDir;
        else if (!IBF_Inst_Project.Project.Path.empty())
            WP = RemoveSpec(IBF_Inst_Project.Project.Path);
        else
        {
            WP = CurrentDirW;
            if (WP.back() == L'\\')WP.pop_back();
        }
        std::vector<std::wstring> TgPath;
        TgPath.reserve(IBF_Inst_Project.Project.Inis.size());
        OutputComplete = false;
        for (auto& I : IBF_Inst_Project.Project.Inis)
        {
            auto& U = IBF_Inst_Project.Project.LastOutputIniName[I.Name];
            if (U.empty())TgPath.push_back(WP + L"\\" + IBF_Inst_Project.Project.ProjName + L"_" + UTF8toUnicode(I.Name) + L".ini");
            else TgPath.push_back(WP + L"\\" + U);
        }
        IBRF_CoreBump.SendToF([=] {Output(WP, TgPath, GetIgnoredSection(), true); });
    }
    void _IN_RENDER_THREAD OutputOnSaveAction()
    {
        if (IsModProject()) return;
        if (IBF_Inst_Setting.OutputOnSave())
        {
            auto AllNotEmpty = std::ranges::all_of(
                IBF_Inst_Project.Project.Inis,
                [](auto& ini) { return !IBF_Inst_Project.Project.LastOutputIniName[ini.Name].empty(); }
            );

            if (AllNotEmpty)IBRF_CoreBump.SendToR({ [=]() {IBR_ProjectManager::AutoOutputAction(); } });
            else IBRF_CoreBump.SendToR({ [=]() {IBR_ProjectManager::OutputAction(); } });
        }
    }

    void _IN_RENDER_THREAD ProjOpen_CreateAction()
    {
        IBRF_CoreBump.SendToR({ []() {CreateAction(); }, &ActionAfterClose });
        CloseAction();
    }
    void _IN_RENDER_THREAD ProjOpen_OpenAction()
    {
        IBRF_CoreBump.SendToR({ []() {OpenAction(); }, &ActionAfterClose });
        CloseAction();
    }
    void _IN_RENDER_THREAD ProjOpen_OpenRecentAction(const std::wstring& Path)
    {
        IBRF_CoreBump.SendToR({ [=]() {OpenRecentAction(Path); }, &ActionAfterClose });
        CloseAction();
    }
    void _IN_RENDER_THREAD OpenRecentOptAction(const std::wstring& Path)
    {
        if (IsOpen())ProjOpen_OpenRecentAction(Path);
        else OpenRecentAction(Path);
    }

    void _IN_RENDER_THREAD OnDropFile(GLFWwindow* window, int argc,const _TEXT_UTF8 char** argv)
    {
        using namespace std::string_literals;
        ((void)window);
        if (IBR_PopupManager::HasPopup)return;
        if (argc == 0)return;
        if (argc == 1)
        {
            std::string ext = ExtName(argv[0]);
            for (auto& c : ext)c = (char)toupper(c);
            if ((ext == ExtensionNameC || ext == "MODPROJ") && !IsModProject())
            {
                ProjOpen_OpenRecentAction(UTF8toUnicode(argv[0]));
                return;
            }
        }


        struct SHPSolution
        {
            std::string Name;
            int Type{ 0 };
            std::wstring FilePath;
        };
        std::vector<SHPSolution> Shapes;
        struct WavEntry { std::string name; std::wstring path; };
        std::vector<WavEntry> WavFiles;
        for (int i = 0; i < argc; i++)
        {
            //get extention name from argv[i] (UTF-8 encoding)
            //get file name without extension from argv[i] (UTF-8 encoding)

            std::string s = FileName(argv[i]);
            for (auto& c : s)c = (char)toupper(c);
            std::string name = FileNameNoExt(s);
            std::string ext = ExtName(s);
            if (ext == ExtensionNameC && IsModProject())
            {
                auto pIproj = IBB_ModuleAltDefault::DefaultIPROJ();
                if (!pIproj) {}
                else
                {
                    auto nameOrig = FileNameNoExt(FileName(argv[i])); // just filename, preserve case
                    auto& var = pIproj->Modules[0].VarList;
                    var.push_back({ "iproj_path", argv[i] });
                    // [LOG] log template data before AddModule
                    {
                        std::string lg = "DBG[Drop_iproj] template: Reg=" + std::string(pIproj->Modules[0].Register)
                            + " DescA=" + pIproj->Modules[0].Desc.A + " DescB=" + pIproj->Modules[0].Desc.B
                            + " EqSize=(" + std::to_string(pIproj->Modules[0].EqSize.x) + "," + std::to_string(pIproj->Modules[0].EqSize.y) + ")"
                            + " EqDelta=(" + std::to_string(pIproj->Modules[0].EqDelta.x) + "," + std::to_string(pIproj->Modules[0].EqDelta.y) + ")"
                            + " VarCount=" + std::to_string(pIproj->Modules[0].VarList.size())
                            + " nameOrig=" + nameOrig + " path=" + argv[i];
//                         GlobalLogB.AddLog_CurTime(false); GlobalLogB.AddLog(lg.c_str());
                    }
                    IBR_Inst_Project.AddModule(*pIproj, nameOrig);
                    var.pop_back();
                    // [LOG] log section count after add
                    {
                        std::string lg = "DBG[Drop_iproj] AFTER_ADD: RevMapSz=" + std::to_string(IBR_Inst_Project.IBR_Rev_SectionMap.size());
//                         GlobalLogB.AddLog(lg.c_str());
                    }
                }
            }
            else if (ext == "VXL")
            {
                auto pVxl = IBB_ModuleAltDefault::DefaultArt_Voxel();
                if (!pVxl)
                    IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::MessageModal(
                    loc("Error_CreateModuleFailed"), loc("Error_NoVXLModule"),
                    { FontHeight * 10.0f, FontHeight * 7.0f }, false, true)));
                else if(IBR_Inst_Project.HasSection({ pVxl->GetFirstINI(), name }))
                    IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::MessageModal(
                    loc("Error_CreateModuleFailed"), loc("Error_UniqueImageModule"),
                    { FontHeight * 10.0f, FontHeight * 7.0f }, false, true)));
                else
                {
                    auto& var = pVxl->Modules[0].VarList;
                    var.clear();
                    std::wstring vxlPath = UTF8toUnicode(argv[i]);
                    var.push_back({ "AssetFile", UnicodetoUTF8(vxlPath) });
                    std::wstring hvaPath = vxlPath;
                    hvaPath.replace(hvaPath.size() - 4, 4, L".hva");
                    var.push_back({ "AssetFile", UnicodetoUTF8(hvaPath) });
                    IBR_Inst_Project.AddModule(*pVxl, name);
                    var.clear();
                }
            }
            else if (ext == "PCX")
            {
                auto pPcx = IBB_ModuleAltDefault::DefaultPCX();
                if (!pPcx) {}
                else if(IBR_Inst_Project.HasSection({ pPcx->GetFirstINI(), s }))
                    IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::MessageModal(
                    loc("Error_CreateModuleFailed"), loc("Error_UniqueImageModule"),
                    { FontHeight * 10.0f, FontHeight * 7.0f }, false, true)));
                else
                {
                    pPcx->Modules[0].VarList.clear();
                    pPcx->Modules[0].VarList.push_back({ "AssetFile", UnicodetoUTF8(UTF8toUnicode(argv[i])) });
                    IBR_Inst_Project.AddModule(*pPcx, s);
                    pPcx->Modules[0].VarList.clear();
                }
            }
            else if (ext == "SHP")
            {
                Shapes.push_back({ name,0,UTF8toUnicode(argv[i]) });
            }
            else if (ext == "WAV" || ext == "WAVE")
            {
                WavFiles.push_back({ name, UTF8toUnicode(argv[i]) });
            }
            else if (ext == "INI")
            {
                IBB_ModuleAlt M;
                M.LoadFromFile(UTF8toUnicode(argv[i]).c_str());
                if (M.Available)
                {
                    IBR_Inst_Project.AddModule(M, GenerateModuleTag());
                    IBR_HintManager::SetHint(loc("GUI_CreateModuleSuccess"), HintStayTimeMillis);
                }
                else
                {
                    auto N = UTF8toUnicode(::FileName(argv[i]));
                    IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::MessageModal(
                        loc("Error_CreateModuleFailed"), UnicodetoUTF8(std::vformat(locw("Error_NotAModule"), std::make_wformat_args(N))),
                        { FontHeight * 10.0f, FontHeight * 7.0f }, false, true)));
                }
            }
            else
            {
                auto N = UTF8toUnicode(ext);
                IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::MessageModal(
                    loc("Error_CreateModuleFailed"), UnicodetoUTF8(std::vformat(locw("Error_LoadUnsupportedType"), std::make_wformat_args(N))),
                    { FontHeight * 10.0f, FontHeight * 7.0f }, false, true)));
            }
        }
        // Process WAV files: create one Sound module with all filenames
        if (!WavFiles.empty())
        {
            auto pSound = IBB_ModuleAltDefault::DefaultSound();
            if (pSound)
            {
                std::string soundsList;
                for (size_t i = 0; i < WavFiles.size(); i++)
                {
                    if (i) soundsList += " ";
                    soundsList += WavFiles[i].name;
                }
                // Set Sounds= and asset paths on template, then AddModule
                auto& clip = pSound->Modules[0];
                for (auto& tok : clip.Lines)
                    if (tok.Key == "Sounds") { tok.Value = soundsList; break; }
                clip.VarList.clear();
                for (auto& w : WavFiles)
                    clip.VarList.push_back({ "AssetFile", UnicodetoUTF8(w.path) });
                IBR_Inst_Project.AddModule(*pSound, GenerateModuleTag());
                clip.VarList.clear();
            }
        }
        auto ShapeCnt = Shapes.size();
        if (!Shapes.empty())
        IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}
            .CreateModal(loc("GUI_LoadFile"), false)
            .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)
            .SetSize({ FontHeight * 20.0f, FontHeight * 6.0f + std::min(ShapeCnt, size_t(6)) * FontHeight * 2.0f })
            .PushMsgBack([SHP=std::move(Shapes)]() mutable
                {
                    ImGui::Text(locc("GUI_SHPToLoad"));

                    auto CreateError = [](const char* Msg)
                        {
                            IBRF_CoreBump.SendToR({ [Msg]
                                {
                                    IBR_PopupManager::ClearCurrentPopup();
                                    IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::MessageModal(
                                    loc("Error_CreateModuleFailed"), Msg,
                                    { FontHeight * 10.0f, FontHeight * 7.0f }, false, true)));
                                } });
                        };

                    //give it up and use scroll bar instead
                    //IBR_ListMenu<SHPSolution> { SHP, u8"SHPSEL",  SelPolicy }.RenderUI();
                    for (size_t Index = 0; Index < SHP.size(); Index++)
                    {
                        auto& S = SHP[Index];
                        ImGui::Text(S.Name.c_str());
                        if (ImGui::RadioButton((loc("GUI_LoadImage_Anim")+std::format(u8"##{}", Index)).c_str(), S.Type == 0, GlobalNodeStyle))S.Type = 0;
                        ImGui::SameLine();
                        if (ImGui::RadioButton((loc("GUI_LoadImage_Building") + std::format(u8"##{}", Index)).c_str(), S.Type == 1, GlobalNodeStyle))S.Type = 1;
                        ImGui::SameLine();
                        if (ImGui::RadioButton((loc("GUI_LoadImage_Infantry") + std::format(u8"##{}", Index)).c_str(), S.Type == 2, GlobalNodeStyle))S.Type = 2;
                        ImGui::SameLine();
                        if (ImGui::RadioButton((loc("GUI_LoadImage_Vehicle") + std::format(u8"##{}", Index)).c_str(), S.Type == 3, GlobalNodeStyle))S.Type = 3;
                    }

                    if (ImGui::Button(locc("GUI_OK")))
                    {
                        for (auto& S : SHP)
                        {
                            switch (S.Type)
                            {
                            case 0://Animation
                            {
                                auto pShp = IBB_ModuleAltDefault::DefaultArt_Animation();
                                if (!pShp)
                                    CreateError(locc("Error_NoAnimModule"));
                                else if(IBR_Inst_Project.HasSection({ pShp->GetFirstINI(), S.Name }))
                                    CreateError(locc("Error_UniqueImageModule"));
                                else
                                {
                                    pShp->Modules[0].VarList.clear();
                                    pShp->Modules[0].VarList.push_back({ "AssetFile", UnicodetoUTF8(S.FilePath) });
                                    IBR_Inst_Project.AddModule(*pShp, S.Name);
                                    pShp->Modules[0].VarList.clear();
                                }
                                break;
                            }
                            case 1://Building
                            {
                                auto pShp = IBB_ModuleAltDefault::DefaultArt_SHPBuilding();
                                if (!pShp)
                                    CreateError(locc("Error_NoBuildingModule"));
                                else if (IBR_Inst_Project.HasSection({ pShp->GetFirstINI(), S.Name }))
                                    CreateError(locc("Error_UniqueImageModule"));
                                else
                                {
                                    pShp->Modules[0].VarList.clear();
                                    pShp->Modules[0].VarList.push_back({ "AssetFile", UnicodetoUTF8(S.FilePath) });
                                    IBR_Inst_Project.AddModule(*pShp, S.Name);
                                    pShp->Modules[0].VarList.clear();
                                }
                                break;
                            }
                            case 2://Infantry
                            {
                                auto pShp = IBB_ModuleAltDefault::DefaultArt_SHPInfantry();
                                if (!pShp)
                                    CreateError(locc("Error_NoInfantryModule"));
                                else if (IBR_Inst_Project.HasSection({ pShp->GetFirstINI(), S.Name }))
                                    CreateError(locc("Error_UniqueImageModule"));
                                else
                                {
                                    pShp->Modules[0].VarList.clear();
                                    pShp->Modules[0].VarList.push_back({ "AssetFile", UnicodetoUTF8(S.FilePath) });
                                    IBR_Inst_Project.AddModule(*pShp, S.Name);
                                    pShp->Modules[0].VarList.clear();
                                }
                                break;
                            }
                            case 3://Vehicle
                            {
                                auto pShp = IBB_ModuleAltDefault::DefaultArt_SHPVehicle();
                                if (!pShp)
                                    CreateError(locc("Error_NoVehicleModule"));
                                else if (IBR_Inst_Project.HasSection({ pShp->GetFirstINI(), S.Name }))
                                    CreateError(locc("Error_UniqueImageModule"));
                                else
                                {
                                    pShp->Modules[0].VarList.clear();
                                    pShp->Modules[0].VarList.push_back({ "AssetFile", UnicodetoUTF8(S.FilePath) });
                                    IBR_Inst_Project.AddModule(*pShp, S.Name);
                                    pShp->Modules[0].VarList.clear();
                                }
                                break;
                            }
                            default:break;
                            }
                        }
                        IBR_PopupManager::ClearPopupDelayed();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(locc("GUI_Cancel")))
                        IBR_PopupManager::ClearPopupDelayed();
                })));
    }
    void _IN_RENDER_THREAD ProjActionByKey()
    {
        if (IsHotKeyPressed(SaveAs))
        {
            SaveAsAction();
        }
        else if (IsHotKeyPressed(Save))
        {
            SaveOptAction();
        }
        else if (IsHotKeyPressed(Open))
        {
            ProjOpen_OpenAction();
        }
        else if (IsHotKeyPressed(Close))
        {
            ProjOpen_CreateAction();
        }
        else if (IsHotKeyPressed(Export))
        {
            OutputAction();
        }
    }

    // ---- ModProject 集中编译 ----

    void _IN_FRONT_THREAD CompileMod(const std::wstring& outputDir); // forward

    void _IN_RENDER_THREAD ShowBuildLog(const std::string& log)
    {
        auto PF{ []() {} };
        IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}.CreateModal(loc("GUI_CompileLog_Title"), true, PF)
            .SetSize({ FontHeight * 30.0f, FontHeight * 18.0f })
            .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize).PushMsgBack([=]() mutable
                {
                    ImGui::TextUnformatted(log.c_str());
                    ImGui::NewLine();
                    if (ImGui::Button(locc("GUI_OK"), { ImGui::GetWindowContentRegionWidth(), FontHeight * 2.0f }))
                        IBR_PopupManager::ClearCurrentPopup();
                })));
    }

    void _IN_RENDER_THREAD CompileModAction()
    {
        struct {
            std::shared_ptr<BufString> dirBuf{ new BufString{} };
            std::shared_ptr<bool> ok{ new bool(false) };
            std::wstring WP;
        } ctx;
        {
            IBD_RInterruptF(x);
            auto& bd = IBF_Inst_ModProject.BuildOutputDir;
            if (!bd.empty()) { ctx.WP = bd; *ctx.ok = IsExistingDir(bd.c_str()); }
            else if (!IBF_Inst_Project.Project.Path.empty()) { ctx.WP = RemoveSpec(IBF_Inst_Project.Project.Path); *ctx.ok = true; }
            else { ctx.WP = CurrentDirW; if (ctx.WP.back() == L'\\') ctx.WP.pop_back(); *ctx.ok = true; }
            strcpy(ctx.dirBuf.get(), UnicodetoUTF8(ctx.WP).c_str());
        }
        auto PF{ []() {IBR_HintManager::SetHint(loc("GUI_ActionCanceled"), HintStayTimeMillis); } };
        IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}.CreateModal(loc("GUI_CompileMod_Title"), true, PF)
            .SetSize({ FontHeight * 20.0f, FontHeight * 8.0f })
            .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize).PushMsgBack([=]() mutable
                {
                    if (ImGui::InputText(locc("GUI_Compile_Path"), ctx.dirBuf.get(), MAX_STRING_LENGTH))
                    {
                        ctx.WP = UTF8toUnicode(ctx.dirBuf.get());
                        *ctx.ok = IsExistingDir(ctx.WP.c_str());
                    }
                    if (ImGui::IsItemActive())IBR_WorkSpace::OperateOnText = true;
                    ImGui::SameLine();
                    if (ImGui::SmallButton("..."))
                    {
                        auto Ret = InsertLoad::SelectFolderName(MainWindowHandle, InsertLoad::SelectFileType{ ctx.WP, locw("GUI_Browse"), L"", L"" });
                        if (Ret.Success)
                        {
                            ctx.WP = Ret.RetBuf;
                            strcpy(ctx.dirBuf.get(), UnicodetoUTF8(ctx.WP).c_str());
                            *ctx.ok = true;
                        }
                    }
                    ImGui::NewLine();
                    if (!*ctx.ok) ImGui::BeginDisabled();
                    if (ImGui::Button(locc("GUI_Compile_Start"), { ImGui::GetWindowContentRegionWidth(), FontHeight * 2.0f }))
                    {
                        auto wdir = ctx.WP;
                        IBF_Inst_ModProject.BuildOutputDir = wdir;
                        IBR_PopupManager::ClearCurrentPopup();
                        IBR_HintManager::SetHint(loc("Log_CompileStarted"), HintStayTimeMillis);
                        IBRF_CoreBump.SendToF({ [wdir]() { CompileMod(wdir); } });
                    }
                    if (!*ctx.ok) ImGui::EndDisabled();
                })));
    }

    struct IprojEntry { std::string name; std::string pathUtf8; };
    struct FlagPackEntry { std::string name; std::vector<std::pair<std::string, std::string>> vars; };

    void _IN_FRONT_THREAD CompileMod(const std::wstring& outputDir)
    {
//         GlobalLogB.AddLog_CurTime(false);
        // GlobalLogB.AddLog((std::string("DBG[Compile] START: ") + UnicodetoUTF8(outputDir)).c_str());
        std::string log;

        // ---- S1: Collect GlobalFlagPacks & iproj paths from modproj ClipData ----
        std::vector<IprojEntry> iprojs;
        std::vector<FlagPackEntry> flagPacks;
        {
            // GlobalLogB.AddLog("DBG[Compile] S1: reading modproj ClipData...");
            IBB_ClipBoardData modClip;
            modClip.SetStream(IBS_Inst_Project.Data, GetClipFormatVersion(IBS_Inst_Project.GetCreateVersionN()));
            // GlobalLogB.AddLog((std::string("DBG[Compile] S1: modClip.Modules.size()=") + std::to_string(modClip.Modules.size())).c_str());
            for (auto& M : modClip.Modules)
            {
                auto ip = std::string{};
                for (auto& v : M.VarList) if (v.A == "iproj_path") { ip = v.B; break; }
                if (!ip.empty())
                {
                    iprojs.push_back({ M.Desc.B, ip });
                    // GlobalLogB.AddLog((std::string("DBG[Compile] S1: iproj_ref: ") + M.Desc.B + " -> " + ip).c_str());
                }
                else
                {
                    FlagPackEntry fp;
                    fp.name = M.Desc.B;
                    for (auto& v : M.VarList) fp.vars.push_back({ v.A, v.B });
                    for (auto& line : M.Lines) fp.vars.push_back({ line.Key, line.Value });
                    flagPacks.push_back(fp);
/*                    GlobalLogB.AddLog((std::string("DBG[Compile] S1: FlagPack: ") + fp.name
                        + " vars_from_VarList=" + std::to_string(M.VarList.size())
                        + " vars_from_Lines=" + std::to_string(M.Lines.size())
                        + " total=" + std::to_string(fp.vars.size())).c_str()); */
                }
            }
        }
/*        GlobalLogB.AddLog((std::string("DBG[Compile] S1: done — iprojs=") + std::to_string(iprojs.size())
            + " flagPacks=" + std::to_string(flagPacks.size())).c_str()); */

        if (iprojs.empty())
        {
            log = loc("Log_CompileEmpty");
            IBF_Inst_ModProject.BuildLog = log;
            IBRF_CoreBump.SendToR({ [log]() { ShowBuildLog(log); } });
            return;
        }

        // ---- S2: Save current modproj state ----
        // GlobalLogB.AddLog("DBG[Compile] S2: saving modproj state...");
        auto savedProject = std::move(IBF_Inst_Project.Project);
        auto savedDisplayNames = std::move(IBF_Inst_Project.DisplayNames);
        auto savedLinkedBy = std::move(IBF_Inst_Project.LinkedBy);
        auto savedIBSData = std::move(IBS_Inst_Project.Data);
        auto savedIBSPath = std::move(IBS_Inst_Project.Path);
        auto savedIBSOutDir = std::move(IBS_Inst_Project.LastOutputDir);
        auto savedIBSOutIni = std::move(IBS_Inst_Project.LastOutputIniName);
        // GlobalLogB.AddLog("DBG[Compile] S2: saved");

        std::vector<std::wstring> allTempFiles;

        int okCount = 0, skipCount = 0, errCount = 0;

        // ---- S3: Compile each iproj ----
        for (auto& entry : iprojs)
        {
            // GlobalLogB.AddLog((std::string("DBG[Compile] S3: ") + entry.name).c_str());
            auto wpath = UTF8toUnicode(entry.pathUtf8);
            if (GetFileAttributesW(wpath.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                log += "[SKIP] " + entry.name + " — " + loc("Log_CompileMissing") + ": " + entry.pathUtf8 + "\n";
                skipCount++;
                continue;
            }

            // S3a: Load iproj
            // GlobalLogB.AddLog("DBG[Compile] S3a: IBS_Load...");
            IBS_Inst_Project.Data.clear();
            IBS_Inst_Project.Path = wpath;
            IBS_Inst_Project.LastOutputDir.clear();
            IBS_Inst_Project.LastOutputIniName.clear();
            if (!IBS_Inst_Project.Load())
            {
                log += "[ERROR] " + entry.name + " — " + loc("Log_CompileLoadFail") + ": " + entry.pathUtf8 + "\n";
                errCount++;
                continue;
            }
            // GlobalLogB.AddLog("DBG[Compile] S3a: loaded OK");

            // Parse iproj ClipData
            IBB_ClipBoardData clip;
            clip.SetStream(IBS_Inst_Project.Data, GetClipFormatVersion(IBS_Inst_Project.GetCreateVersionN()));
            // GlobalLogB.AddLog((std::string("DBG[Compile] S3a: clip.Modules=") + std::to_string(clip.Modules.size())).c_str());

            // Find UseGlobalFlagPack refs and inject FlagPacks into temp clip
            struct Injection { std::string packName; ModuleClipData* srcModule; };
            std::vector<Injection> injections;
            for (auto& M : clip.Modules)
                for (auto& line : M.Lines)
                    if (line.Key == "UseGlobalFlagPack")
                        injections.push_back({ line.Value, &M });
            // GlobalLogB.AddLog((std::string("DBG[Compile] S3a: UseGlobalFlagPack refs=") + std::to_string(injections.size())).c_str());

            std::wstring tempPath = wpath + L"_temp.iproj";
            allTempFiles.push_back(tempPath);

            int fpCounter = 0;
            std::vector<ModuleClipData> flagPackMods;
            for (auto& inj : injections)
            {
                // GlobalLogB.AddLog((std::string("DBG[Compile] S3a: inject ") + inj.packName + " -> " + inj.srcModule->Desc.B).c_str());
                const FlagPackEntry* fp = nullptr;
                for (auto& f : flagPacks) { if (f.name == inj.packName) { fp = &f; break; } }
                if (!fp) { log += "  [WARN] FlagPack '" + inj.packName + "' not found\n"; continue; }

                // Unique name for each FlagPack instance
                std::string fpUniqueName = inj.packName + "_FP" + std::to_string(fpCounter++);

                // Remove UseGlobalFlagPack from source, add UseFlagPack
                for (auto it = inj.srcModule->Lines.begin(); it != inj.srcModule->Lines.end(); )
                {
                    if (it->Key == "UseGlobalFlagPack") it = inj.srcModule->Lines.erase(it);
                    else ++it;
                }
                inj.srcModule->Lines.push_back(IniToken(std::string("UseFlagPack=") + fpUniqueName, false));

                // Create FlagPack module
                ModuleClipData fpMod = *inj.srcModule;
                fpMod.Register = "_FlagPack_";
                fpMod.Desc.B = fpUniqueName;
                fpMod.VarList.clear();
                fpMod.Lines.clear();
                for (auto& kv : fp->vars)
                    if (!kv.first.empty() && kv.first[0] != '_')
                        fpMod.Lines.push_back(IniToken(kv.first + "=" + kv.second, false));
                fpMod.SkipExport = true;
                fpMod.IsComment = false;
                fpMod.Comment.clear();
                flagPackMods.push_back(fpMod);
                // GlobalLogB.AddLog((std::string("DBG[Compile] S3a: created FlagPack ") + fpUniqueName + " lines=" + std::to_string(fpMod.Lines.size())).c_str());
            }
            // Append all FlagPack modules to clip AFTER modifying Lines (avoids dangling ptrs)
            for (auto& fpm : flagPackMods)
                clip.Modules.push_back(fpm);

            // Clone modified clip as tempClip (after Lines were modified in-place)
            IBB_ClipBoardData tempClip = clip;

            // Save temp file
            {
                auto stream = tempClip.GetStream();
                auto origIBSData = std::move(IBS_Inst_Project.Data);
                IBS_Inst_Project.Data = std::move(stream);
                IBS_Inst_Project.Path = tempPath;
                IBS_Inst_Project.Save();
                IBS_Inst_Project.Data = std::move(origIBSData);
                IBS_Inst_Project.Path = wpath;
/*                GlobalLogB.AddLog((std::string("DBG[Compile] S3a: temp saved ") + UnicodetoUTF8(tempPath)
                    + " size=" + std::to_string(tempClip.GetStream().size())).c_str()); */
            }

            // Reload temp file (same flow as normal load+export)
            // GlobalLogB.AddLog("DBG[Compile] S3b: reloading temp file...");
            IBS_Inst_Project.Data.clear();
            IBS_Inst_Project.Path = tempPath;
            IBS_Inst_Project.LastOutputDir.clear();
            IBS_Inst_Project.LastOutputIniName.clear();
            if (!IBS_Inst_Project.Load()) { log += "[ERROR] reload temp failed\n"; errCount++; continue; }
            IBF_Inst_Project.Project = {};
            IBF_Inst_Project.DisplayNames.clear();
            IBF_Inst_Project.LinkedBy.clear();
            IBF_Inst_Project.Load(IBS_Inst_Project);
            IBB_ClipBoardData reloadClip;
            reloadClip.SetStream(IBS_Inst_Project.Data, GetClipFormatVersion(IBS_Inst_Project.GetCreateVersionN()));
            for (auto& M : reloadClip.Modules)
                IBF_Inst_Project.Project.AddModule(M);
            IBF_Inst_Project.UpdateAll();
            // GlobalLogB.AddLog((std::string("DBG[Compile] S3b: Inis=") + std::to_string(IBF_Inst_Project.Project.Inis.size())).c_str());

            // S3e: Build target INI paths
            auto projName = FileNameNoExt(UnicodetoUTF8(IBF_Inst_Project.Project.ProjName));
            std::vector<std::wstring> tgPath;
            tgPath.reserve(IBF_Inst_Project.Project.Inis.size());
            for (auto& Ini : IBF_Inst_Project.Project.Inis)
            {
                if (Ini.Name == Internal_IniName) { tgPath.push_back(L""); continue; }
                auto fname = UTF8toUnicode(projName + "_" + Ini.Name + ".ini");
                tgPath.push_back(outputDir + L"\\" + fname);
                IBF_Inst_ModProject.CompiledIniFiles[Ini.Name].push_back(fname);
            }
            // GlobalLogB.AddLog((std::string("DBG[Compile] S3e: ") + std::to_string(tgPath.size()) + " ini targets").c_str());

            // S3f: Output
            // GlobalLogB.AddLog("DBG[Compile] S3f: Output()...");
            Output(outputDir, tgPath, {}, false);
            // GlobalLogB.AddLog("DBG[Compile] S3f: Output() done");

            log += "[OK] " + entry.name + " → " + UnicodetoUTF8(outputDir) + "\n";
            okCount++;

            // S3g: Copy asset files
            // GlobalLogB.AddLog("DBG[Compile] S3g: copying assets...");
            int assetOk = 0;
            for (auto& M : clip.Modules)
            {
                for (auto& v : M.VarList)
                {
                    if (v.A != "AssetFile") continue;
                    auto src = UTF8toUnicode(v.B);
                    if (GetFileAttributesW(src.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
                    auto fname = FileName(src);
                    auto dst = outputDir + L"\\" + fname;
                    if (CopyFileW(src.c_str(), dst.c_str(), FALSE))
                        { log += "  [ASSET] " + UnicodetoUTF8(fname) + "\n"; assetOk++; }
                    else
                        log += "  [ASSET FAIL] " + v.B + "\n";
                }
            }
            // GlobalLogB.AddLog((std::string("DBG[Compile] S3g: assets copied=") + std::to_string(assetOk)).c_str());
        }

        // ---- S3.5: Generate [#include] sections in md files ----
        // GlobalLogB.AddLog("DBG[Compile] S3_include: generating includes...");
        {
            // Blacklist: crash check
            const wchar_t* blackList[] = { L"Rulesmo.ini", L"Artmo.ini" };
            for (auto* bf : blackList)
            {
                if (GetFileAttributesW((outputDir + L"\\" + bf).c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    MessageBoxW(NULL, L"织网者不能这么用！", L"致命错误", MB_OK | MB_ICONERROR);
                    exit(1);
                }
            }

            // Helper: update [#include] in one md file
            auto updateInclude = [&](const std::wstring& mdPath, const std::string& inType,
                                       const std::vector<std::wstring>& files)
            {
                // GlobalLogB.AddLog((std::string("DBG[Compile] S3_include: ") + inType + " -> " + UnicodetoUTF8(mdPath)).c_str());
                if (GetFileAttributesW(mdPath.c_str()) == INVALID_FILE_ATTRIBUTES)
                {
                    log += std::string("[WARN] ") + UnicodetoUTF8(FileName(mdPath)) + " not found\n";
                    return;
                }
                // Read all lines
                std::vector<std::string> lines;
                {
                    ExtFileClass F;
                    if (F.Open(mdPath.c_str(), L"r"))
                    {
                        std::string line;
                        while (!F.Eof())
                            if (F.ReadLine(line)) lines.push_back(line);
                        F.Close();
                    }
                }
                // Check for blacklisted section headers
                const char* sectionBlackList[] = { "[Projectiles]", "[Projectile]", "[ProjectileTypes]" };
                for (auto& line : lines)
                {
                    for (auto* sbl : sectionBlackList)
                    {
                        if (_strnicmp(line.c_str(), sbl, strlen(sbl)) == 0)
                        {
                            MessageBoxW(NULL, L"织网者不能这么用！", L"致命错误", MB_OK | MB_ICONERROR);
                            exit(1);
                        }
                    }
                }
                // Strip existing [#include] section
                std::vector<std::string> outLines;
                bool inInclude = false;
                for (auto& line : lines)
                {
                    if (!line.empty() && line[0] == '[' && _strnicmp(line.c_str(), "[#include]", 10) == 0 && line.back() == ']')
                    {
                        inInclude = true; continue;
                    }
                    if (inInclude)
                    {
                        if (!line.empty() && line[0] == '[') { inInclude = false; outLines.push_back(line); }
                        continue;
                    }
                    outLines.push_back(line);
                }
                // Append new [#include]
                outLines.push_back("[#include]");
                int idx = 1;
                for (auto& fname : files)
                    outLines.push_back(std::to_string(idx++) + "=" + UnicodetoUTF8(fname));
                outLines.push_back("");
                // Write back
                {
                    ExtFileClass F;
                    if (F.Open(mdPath.c_str(), L"w"))
                    {
                        for (auto& line : outLines) { F.PutStr(line); F.Ln(); }
                        F.Close();
                        log += std::string("[INCLUDE] ") + UnicodetoUTF8(FileName(mdPath)) + " <- " + std::to_string(files.size()) + " entries\n";
                    }
                }
            };

            // For each exported INI type, update its md file(s)
            // Pattern: {lowercase(type)}md.ini
            // Special: Rules → also rulesst.ini, Art → also artst.ini
            for (auto& [iniType, files] : IBF_Inst_ModProject.CompiledIniFiles)
            {
                if (files.empty()) continue;
                std::string lowerType = iniType;
                for (auto& c : lowerType) c = (char)tolower((unsigned char)c);
                updateInclude(outputDir + L"\\" + UTF8toUnicode(lowerType + "md.ini"), iniType, files);
                // Special cases
                if (iniType == "Rules")
                    updateInclude(outputDir + L"\\rulesst.ini", iniType, files);
                else if (iniType == "Art")
                    updateInclude(outputDir + L"\\artst.ini", iniType, files);
            }
            // GlobalLogB.AddLog("DBG[Compile] S3_include: done");
        }

        // ---- S4: Restore modproj state ----
        // GlobalLogB.AddLog("DBG[Compile] S4: restoring modproj...");
        IBF_Inst_Project.Project = std::move(savedProject);
        IBF_Inst_Project.DisplayNames = std::move(savedDisplayNames);
        IBF_Inst_Project.LinkedBy = std::move(savedLinkedBy);
        IBS_Inst_Project.Data = std::move(savedIBSData);
        IBS_Inst_Project.Path = std::move(savedIBSPath);
        IBS_Inst_Project.LastOutputDir = std::move(savedIBSOutDir);
        IBS_Inst_Project.LastOutputIniName = std::move(savedIBSOutIni);
        // GlobalLogB.AddLog("DBG[Compile] S4: restored");

        // Clean up temp files
        for (auto& tf : allTempFiles)
        {
            if (GetFileAttributesW(tf.c_str()) != INVALID_FILE_ATTRIBUTES)
                DeleteFileW(tf.c_str());
        }

        char buf[256];
        snprintf(buf, sizeof(buf), "OK:%d SKIP:%d ERR:%d", okCount, skipCount, errCount);
        log = std::string(buf) + "\n\n" + log;

        // Compiled INI files summary
        log += "\n--- Generated INI Files ---\n";
        for (auto& [iniType, files] : IBF_Inst_ModProject.CompiledIniFiles)
        {
            log += "[" + iniType + "]: " + std::to_string(files.size()) + " file(s)\n";
            for (auto& f : files)
                log += "  " + UnicodetoUTF8(f) + "\n";
        }

        // GlobalLogB.AddLog((std::string("DBG[Compile] DONE: ") + buf).c_str());
        IBF_Inst_ModProject.BuildLog = log;
        IBRF_CoreBump.SendToR({ [log]() { ShowBuildLog(log); } });
    }
};
