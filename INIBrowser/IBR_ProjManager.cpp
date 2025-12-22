#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBB_RegType.h"
#include "IBB_ModuleAlt.h"
#include <imgui_internal.h>
#include <shlwapi.h>
#include <format>
#include "IBR_HotKey.h"

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

std::wstring RemoveSpec(std::wstring W)
{
    PathRemoveFileSpecW(W.data());
    W.resize(wcslen(W.data()));
    return W;
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
            IBR_SelectMode::CancelSelectMode();
            IBR_EditFrame::Clear();
            IBR_PopupManager::ClearRightClickMenu();
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
            auto L1 = wcslen(L"*" ExtensionNameW);
            Filter.resize(T2.size() + T3.size() + 64);
            wcscpy(Filter.data(), T3.c_str());
            wcscpy(Filter.data() + T3.size() + 1, L"*" ExtensionNameW);
            wcscpy(Filter.data() + T3.size() + L1 + 2, T2.c_str());
            wcscpy(Filter.data() + T3.size() + L1 + T2.size() + 3, L"*.*");
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
        if (!Path.ends_with(ExtensionNameW))Path += ExtensionNameW;
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
                    if (ImGui::Button((loc("GUI_AskIfSave_Yes") + u8"##AskIfSave").c_str()))
                    {
                        IBRF_CoreBump.SendToR({ [=]() {  Next(true); IBR_PopupManager::ClearCurrentPopup(); }, nullptr });
                    }ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FontHeight * 0.7f);
                    if (ImGui::Button((loc("GUI_AskIfSave_No") + u8"##AskIfSave").c_str()))
                    {
                        IBRF_CoreBump.SendToR({ [=]() {  Next(false); IBR_PopupManager::ClearCurrentPopup(); }, nullptr});
                    }ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FontHeight * 0.7f);
                    if (ImGui::Button((loc("GUI_AskIfSave_Cancel") + u8"##AskIfSave").c_str()))
                    {
                        IBRF_CoreBump.SendToR({ [=]() {IBR_PopupManager::ClearCurrentPopup();
                        GotoCloseShellLoop = false; glfwSetWindowShouldClose(PreLink::window, false); }, nullptr });
                    }
                })));
    }

    void _IN_FRONT_THREAD OutputRegister(std::vector<std::vector<std::string>>& TextPieces)
    {
        auto& Proj = IBF_Inst_Project.Project;
        std::unordered_map<std::string, std::vector<std::string>> Reg;
        for (auto& Ini : Proj.Inis)
            for (auto& [SN, Sec] : Ini.Secs)
                Reg[Sec.Register].push_back(Sec.Name);
        for (auto& [N, R] : Reg)
        {
            auto& RegType = IBB_DefaultRegType::GetRegType(N);
            if (!RegType.Export)continue;
            IBB_Project_Index Idx(RegType.IniType);
            Idx.GetIni(Proj);
            //MessageBoxA(NULL, Idx.GetIni(Proj)->Name.c_str(), std::to_string(Idx.Ini.Index).c_str(), MB_OK);
            std::string V;
            V += '[';V += N;V += "]\n";
            for (auto& v : R)
            {
                V += v; V += '='; V += v; V += '\n';
            }
            TextPieces[Idx.Ini.Index].push_back(std::move(V));
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

        std::map<IBB_Section_Desc, std::string> DisplayRev;
        for (auto& [K, V] : IBF_Inst_Project.DisplayNames)DisplayRev[V] = K;

        size_t N = TargetIniPath.size();
        std::vector<std::vector<std::string>>TextPieces;
        TextPieces.resize(N);



        OutputRegister(TextPieces);
        for (size_t I = 0; I < N; I++)
        {
            //MessageBoxW(NULL, TargetIniPath[I].c_str(), L"dsfsd", MB_OK);
            for (auto& [SN, Sec] : Inis[I].Secs)
            {
                for (auto& Sub : Sec.SubSecs)for (auto& L : Sub.Lines)
                    if (L.first == InheritKeyName)Sec.Inherit = L.second.Data->GetString();
                IBB_Section_Desc Desc = { Inis[I].Name, Sec.Name };
                std::string V;
                V += ';'; V += DisplayRev[Desc];  V += '\n';
                if (IgnoredSection.contains(Desc) || Sec.IsLinkGroup || Sec.IsComment())continue;
                V += '['; V += Sec.Name;
                if(!Sec.Inherit.empty())
                { V+="]:["; V+=Sec.Inherit; }
                V += "]\n";
                V += Sec.GetText(false, true);
                TextPieces[I].push_back(std::move(V));
            }
        }

        for (size_t I = 0; I < N; I++)
        {
            if (Inis[I].Name == Internal_IniName)continue;//不导出这个
            if (TextPieces[I].empty())continue;

            ExtFileClass F;
            if (F.Open(TargetIniPath[I].c_str(), L"w"))
            {
                auto cwa = locw("AppName");
                auto cwb = UTF8toUnicode(TimeNowU8());
                F.PutStr(";" + UnicodetoUTF8(std::vformat(locw("Back_OutputINILine1"), std::make_wformat_args(cwa, VersionW)))); F.Ln();
                F.PutStr(";" + UnicodetoUTF8(std::vformat(locw("Back_OutputINILine2"), std::make_wformat_args(IBF_Inst_Project.Project.ProjName)))); F.Ln();
                F.PutStr(";" + UnicodetoUTF8(std::vformat(locw("Back_OutputINILine3"), std::make_wformat_args(cwb)))); F.Ln();
                for (auto& V : TextPieces[I])
                {
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
                else { IBR_HintManager::SetHint(loc("GUI_ActionCanceled"), HintStayTimeMillis); IBR_PopupManager::ClearPopupDelayed(); }
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

        if (!IBF_Inst_Project.Project.ChangeAfterSave)return;
        SetWaitingPopup();
        IBS_Push([=]()
            {
                Save(IBF_Inst_Project.Project.Path, [](bool OK) {IBRF_CoreBump.SendToR({ [=]()
                {
                    IBR_PopupManager::ClearCurrentPopup();
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
            if (I.Name == Internal_IniName)continue; //不导出这个
            auto& U = IBF_Inst_Project.Project.LastOutputIniName[I.Name];
            if (U.empty())TgPath.push_back(WP + L"\\" + IBF_Inst_Project.Project.ProjName + L"_" + UTF8toUnicode(I.Name) + L".ini");
            else TgPath.push_back(WP + L"\\" + U);
        }
        IBRF_CoreBump.SendToF([=] {Output(WP, TgPath, GetIgnoredSection(), true); });
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
            if (ext == ExtensionNameC)
            {
                ProjOpen_OpenRecentAction(UTF8toUnicode(argv[0]));
                return;
            }
        }


        struct SHPSolution
        {
            std::string Name;
            int Type{ 0 };
        };
        std::vector<SHPSolution> Shapes;
        for (int i = 0; i < argc; i++)
        {
            //get extention name from argv[i] (UTF-8 encoding)
            //get file name without extension from argv[i] (UTF-8 encoding)

            std::string s = FileName(argv[i]);
            for (auto& c : s)c = (char)toupper(c);
            std::string name = FileNameNoExt(s);
            std::string ext = ExtName(s);
            if (ext == "VXL")
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
                else IBR_Inst_Project.AddModule(*pVxl, name);
            }
            else if (ext == "SHP")
            {
                Shapes.push_back({ name,0 });
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
        if (!Shapes.empty())
        IBR_PopupManager::SetCurrentPopup(std::move(IBR_PopupManager::Popup{}
            .CreateModal(loc("GUI_LoadFile"), false)
            .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)
            .SetSize({ FontHeight * 20.0f, FontHeight * 6.0f + std::min(Shapes.size(), size_t(6)) * FontHeight * 2.0f })
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
                                else IBR_Inst_Project.AddModule(*pShp, S.Name);
                                break;
                            }
                            case 1://Building
                            {
                                auto pShp = IBB_ModuleAltDefault::DefaultArt_SHPBuilding();
                                if (!pShp)
                                    CreateError(locc("Error_NoBuildingModule"));
                                else if (IBR_Inst_Project.HasSection({ pShp->GetFirstINI(), S.Name }))
                                    CreateError(locc("Error_UniqueImageModule"));
                                else IBR_Inst_Project.AddModule(*pShp, S.Name);
                                break;
                            }
                            case 2://Infantry
                            {
                                auto pShp = IBB_ModuleAltDefault::DefaultArt_SHPInfantry();
                                if (!pShp)
                                    CreateError(locc("Error_NoInfantryModule"));
                                else if (IBR_Inst_Project.HasSection({ pShp->GetFirstINI(), S.Name }))
                                    CreateError(locc("Error_UniqueImageModule"));
                                else IBR_Inst_Project.AddModule(*pShp, S.Name);
                                break;
                            }
                            case 3://Vehicle
                            {
                                auto pShp = IBB_ModuleAltDefault::DefaultArt_SHPVehicle();
                                if (!pShp)
                                    CreateError(locc("Error_NoVehicleModule"));
                                else if (IBR_Inst_Project.HasSection({ pShp->GetFirstINI(), S.Name }))
                                    CreateError(locc("Error_UniqueImageModule"));
                                else IBR_Inst_Project.AddModule(*pShp, S.Name);
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
};
