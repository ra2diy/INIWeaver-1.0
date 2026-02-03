#include "IBRender.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include<imgui_internal.h>
#include <ranges>

namespace ImGui
{
    ImVec2 GetLineEndPos();
    ImVec2 GetLineBeginPos();
    bool IsWindowClicked(ImGuiMouseButton Button);
    void PushOrderFront(ImGuiWindow* Window);
}

extern float MWWidth, MWWidth2;
std::tuple<bool, ImVec2, ImVec2> RectangleCross(ImVec2 UL1, ImVec2 DR1, ImVec2 UL2, ImVec2 DR2);

namespace IBR_DynamicData
{
    ExtFileClass DynamicData;
    int DefaultX{ -1 }, DefaultY{ -1 };
    void SetDefaultWidth(int W)
    {
        DefaultX = W;
    }
    void SetDefaultHeight(int H)
    {
        DefaultY = H;
    }

    void Read(int DefaultResX, int DefaultResY)
    {
        if (DynamicData.Open(".\\Resources\\dynamic.dat", "rb"))
        {
            int64_t ScrXR, ScrYR;
            DynamicData.ReadData(ScrXR);
            DynamicData.ReadData(ScrYR);
            double SectionTextScaleR;
            int32_t MWSR;
            DynamicData.ReadData(MWSR);
            DynamicData.ReadData(SectionTextScaleR);
            if (!SectionTextScaleR)SectionTextScaleR = 1.0f;
            IBR_FullView::Ratio = IBR_WorkSpace::RatioPrev = (float)SectionTextScaleR;
            MWWidth2 = (float)MWSR;
            //DynamicData.ReadData(IBR_FullView::EqCenter.x);
            //DynamicData.ReadData(IBR_FullView::EqCenter.y);
            IBR_WorkSpace::EqCenterPrev = IBR_FullView::EqCenter;

            ScrX = (int)ScrXR;
            ScrY = (int)ScrYR;
            if (ScrX <= 0 || ScrY <= 0)
            {
                ScrX = DefaultResX;
                ScrY = DefaultResY;
            }
            DynamicData.Close();
        }
        else //缺省值
        {
            ScrX = DefaultResX;
            ScrY = DefaultResY;
            IBR_FullView::Ratio = IBR_WorkSpace::RatioPrev = 1.0f;
        }
        if (DefaultX > 0)ScrX = DefaultX;
        if (DefaultY > 0)ScrY = DefaultY;
        IBR_UICondition::CurrentScreenWidth = ScrX;
        IBR_UICondition::CurrentScreenHeight = ScrY;
    }
    void SetRandom()
    {
        ::srand((unsigned)::time(NULL));
        GlobalRnd = std::default_random_engine{ (unsigned)::time(NULL) };
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            GlobalLog.AddLog(locc("Log_InitRandom"));
        }
    }
    void Open()
    {
        DynamicData.Open(L".\\Resources\\dynamic.dat", L"wb");
        const int Retry = 5;
        for (int i = 0; i < Retry && (!DynamicData.Available()); i++)
        {
            Sleep(5);
            DynamicData.Open(L".\\Resources\\dynamic.dat", L"wb");
        }
        if (!DynamicData.Available())
        {
            if (GetLastError() == ERROR_SHARING_VIOLATION)
            {
                glfwHideWindow(PreLink::window);
                MessageBoxW(NULL, locwc("Error_AnotherInstanceIsRunning"), _AppNameW, MB_OK);
                ExitProcess(0);
            }
            auto ls = std::to_wstring(::GetLastError());
            MessageBoxW(NULL, (std::vformat(locw("Error_LastErrorCode"), std::make_wformat_args(ls))).c_str(),
                (L"IBR_DynamicData::Open " + locw("Error_ErrorOccurred")).c_str(), MB_OK);
        }
    }
    void Save()
    {
        DynamicData.Rewind();
        DynamicData.WriteData((int64_t)(IBR_UICondition::CurrentScreenWidth));
        DynamicData.WriteData((int64_t)(IBR_UICondition::CurrentScreenHeight));
        DynamicData.WriteData((int32_t)MWWidth);
        DynamicData.WriteData((double)IBR_FullView::Ratio);
        //DynamicData.WriteData(IBR_FullView::EqCenter.x);
        //DynamicData.WriteData(IBR_FullView::EqCenter.y);
        DynamicData.Flush();
    }
    void Close()
    {
        DynamicData.Close();
    }
}

extern char CurrentDirA[5000];
extern wchar_t CurrentDirW[5000];

namespace IBR_RecentManager
{
    ExtFileClass RecentFile;
    std::vector<_TEXT_UTF8 std::string> RecentName;
    std::wstring Path;

    const int MaxRecent = 10;

    IBR_ListMenu<_TEXT_UTF8 std::string> RecentList{ RecentName,u8"Recent",[](_TEXT_UTF8 std::string& Name,int,int)
        {
            if (ImGui::SmallButton((loc("GUI_Open") + u8"##" + Name).c_str()))
                IBR_ProjectManager::OpenRecentOptAction(UTF8toUnicode(Name));
            ImGui::SameLine();
            ImGui::TextWrapped(Name.c_str());
        } };
    void RenderUI()
    {
        ImGui::Text(locc("GUI_Recent"));
        if (ImGui::Button(locc("GUI_ClearRecent")))
        {
            IBR_PopupManager::SetCurrentPopup(std::move(
                IBR_PopupManager::Popup{}
                .CreateModal(loc("GUI_WaitingTitle"), false, []()
                    {
                            IBR_HintManager::SetHint(loc("GUI_ActionCanceled"), HintStayTimeMillis);
                    }
                )
                .SetFlag(
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoNav |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize)
                .SetSize({ FontHeight * 15.0F,FontHeight * 7.0F })
                .PushTextBack(loc("GUI_AskIfClearRecent"))
                .PushMsgBack([]()
                    {
                        if (ImGui::Button(locc("GUI_Yes"), { FontHeight * 6.0f,FontHeight * 2.0f }))
                        {
                            RecentName.clear();
                            IBR_RecentManager::Save();
                            IBR_PopupManager::ClearPopupDelayed();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(locc("GUI_No"), { FontHeight * 6.0f,FontHeight * 2.0f }))
                        {
                            IBR_PopupManager::ClearPopupDelayed();
                        }
                    }))
            );
        }
        RecentList.RenderUI();
    }
    void Load()
    {
        if (RecentFile.Open(Path.c_str(), L"rb"))
        {
            if (!RecentFile.Available())
            {
                auto ls = std::to_wstring(::GetLastError());
                MessageBoxW(NULL, (std::vformat(locw("Error_LastErrorCode"), std::make_wformat_args(ls))).c_str(),
                    (L"IBR_RecentManager::Load " + locw("Error_ErrorOccurred")).c_str(), MB_OK);
            }
            RecentFile.ReadVector(RecentName);
            RecentFile.Close();
        }
    }

    void WanDuZiLe()
    {
        if(!RecentName.empty())RecentName.erase(RecentName.begin());
    }

    void Push(const std::wstring& _Path)
    {
        _TEXT_UTF8 std::string up = UnicodetoUTF8(_Path);
        if (RecentName.empty())
        {
            RecentName.push_back(up);
        }
        else if (RecentName.front() != up)
        {
            RecentName.erase(std::remove_if(RecentName.begin(), RecentName.end(), [&](const auto& ws)->bool {return ws == up; }), RecentName.end());
            RecentName.insert(RecentName.begin(), up);
            while (RecentName.size() > MaxRecent)RecentName.pop_back();
        }
    }
    void Save()
    {
        if (RecentFile.Open(Path.c_str(), L"wb"))
        {
            RecentFile.WriteVector(RecentName);
            RecentFile.Close();
        }
    }
}

std::vector<std::string_view> GetLines(std::string&& Text);

namespace IBR_PopupManager
{
    Popup CurrentPopup;
    bool HasPopup = false;
    Popup RightClickMenu;
    bool HasRightClickMenu = false;
    bool FirstRightClick = false;
    ImVec2 RightClickMenuPos{ 0,0 };
    bool IsMouseOnPopupCond{ false };

    std::vector<StdMessage> DelayedPopupAction;

    struct JsonParseErrorPopup
    {
        std::string Title;
        std::string ErrorStr;
        std::string Info;
    };
    std::vector<JsonParseErrorPopup> JsonParseErrorList;
    size_t JsonParseErrorListShown = 0;
    void ShowJsonParseErrorImpl()
    {
        if (JsonParseErrorListShown < JsonParseErrorList.size())
        {
            auto& _W = JsonParseErrorList[JsonParseErrorListShown];
            SetCurrentPopup(
                std::move(Popup{}
                    .CreateModal(_W.Title, true, []() { {
                            JsonParseErrorListShown++;
                            IBRF_CoreBump.SendToR({ [] {ShowJsonParseErrorImpl(); } });
                        }})
                .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)
                .PushMsgBack([]()
                    {
                        auto& W = JsonParseErrorList[JsonParseErrorListShown];
                        ImGui::Text(W.Info.c_str());
                        auto V = W.ErrorStr | std::views::split('\n');
                        int TgIdx = 0;
                        int Idx = 1;
                        for (auto L : V)
                        {
                            auto R = std::string_view{ L.data(), L.size() };
                            if (R.find(u8"【出错位置】") != R.npos)
                            {
                                TgIdx = Idx;
                                ImGui::Text(u8"第%d行：", Idx);
                                break;
                            }
                            Idx++;
                        }
                        Idx = 1;
                        for (auto L : V)
                        {
                            if (Idx >= TgIdx - 4 && Idx <= TgIdx + 4)
                                ImGui::Text(std::string(L.data(), L.size()).c_str());
                            Idx++;
                        }
                        ImGui::Text(locc("GUI_SeeLogForDetails"));
                    }))
                );

        }
        else ClearPopupDelayed();
    }
    void AddErrorPopup(std::string&& ErrorStr, const std::string& Title, const std::string& Info, const std::wstring& LogFormat)
    {
        if (ErrorStr.empty())return;
        if (JsonParseErrorListShown != 0)
        {
            JsonParseErrorList.erase(JsonParseErrorList.begin(), JsonParseErrorList.begin() + JsonParseErrorListShown);
            JsonParseErrorListShown = 0;
        }
        if (JsonParseErrorListShown == JsonParseErrorList.size())IBRF_CoreBump.SendToR({ [] {ShowJsonParseErrorImpl(); } });
        if (EnableLog)
        {
            GlobalLog.AddLog_CurTime(false);
            auto V = UTF8toUnicode(ErrorStr);
            GlobalLog.AddLog(std::vformat(LogFormat, std::make_wformat_args(V)));
        }
        JsonParseErrorList.push_back({ Title, std::move(ErrorStr), Info });
    }
    void AddJsonParseErrorPopup(std::string&& ErrorStr, const std::string& Info)
    {
        AddErrorPopup(std::move(ErrorStr), loc("GUI_JsonParseError"), Info, locw("Log_JsonParseErrorInfo"));
    }
    void AddModuleParseErrorPopup(std::string&& ErrorStr, const std::string& Info)
    {
        AddErrorPopup(std::move(ErrorStr), loc("GUI_ModuleParseError"), Info, locw("Log_ModuleParseErrorInfo"));
    }

    bool IsMouseOnPopup()
    {
        return IsMouseOnPopupCond;
    }
    Popup BasicPopupA{
        false,false, false, false, "",ImGuiWindowFlags_None,[]() {},[]() {}
    };
    void ClearPopupDelayed()
    {
        JsonParseErrorList.clear();
        JsonParseErrorListShown = 0;
        IBRF_CoreBump.SendToR({ [] {IBR_PopupManager::ClearCurrentPopup(); } });
    }
    Popup& Popup::Create(const _TEXT_UTF8 std::string& title)
    {
        Title = title;
        CanClose = false;
        Show = []() {};
        Flag = ImGuiWindowFlags_AlwaysAutoResize;
        return *this;
    }
    Popup& Popup::CreateModal(const _TEXT_UTF8 std::string& title, bool canclose, StdMessage close)
    {
        Title = title;
        Modal = true;
        CanClose = canclose;
        Close = close;
        Show = []() {};
        Flag = ImGuiWindowFlags_AlwaysAutoResize;
        return *this;
    }
    Popup& Popup::SetTitle(const _TEXT_UTF8 std::string& title)
    {
        Title = title;
        return *this;
    }
    Popup& Popup::SetFlag(ImGuiWindowFlags flag)
    {
        Flag |= flag;
        return *this;
    }
    Popup& Popup::UnsetFlag(ImGuiWindowFlags flag)
    {
        Flag &= ~flag;
        return *this;
    }
    Popup& Popup::ClearFlag()
    {
        Flag = 0;
        return *this;
    }
    Popup& Popup::UseMyStyle()
    {
        HasOwnStyle = true;
        return *this;
    }
    Popup& Popup::PushTextPrev(const _TEXT_UTF8 std::string& Text)//TODO:优化，试图砍掉prev
    {
        StdMessage ShowPrev{ std::move(Show) };
        Show = [=]() {ImGui::TextWrapped(Text.c_str()); ShowPrev(); };
        return *this;
    }
    Popup& Popup::PushTextBack(const _TEXT_UTF8 std::string& Text)
    {
        StdMessage ShowPrev{ std::move(Show) };
        Show = [=]() {ShowPrev(); ImGui::TextWrapped(Text.c_str()); };
        return *this;
    }
    Popup& Popup::PushMsgPrev(StdMessage Msg)
    {
        StdMessage ShowPrev{ std::move(Show) };
        Show = [=]() {Msg(); ShowPrev(); };
        return *this;
    }
    Popup& Popup::PushMsgBack(StdMessage Msg)
    {
        StdMessage ShowPrev{ std::move(Show) };
        Show = [=]() {ShowPrev(); Msg(); };
        return *this;
    }
    Popup& Popup::SetSize(ImVec2 NewSize)
    {
        Size = NewSize;
        return *this;
    }
    Popup SingleText(const _TEXT_UTF8 std::string& StrId, const _TEXT_UTF8 std::string& Text, bool Modal)
    {
        return (Modal ? Popup{}.CreateModal(StrId, false) : Popup{}.Create(StrId))
            .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar )
            .PushTextBack(Text);
    }
    Popup MessageModal(const _TEXT_UTF8 std::string& Title, const _TEXT_UTF8 std::string& Text, ImVec2 Size , bool CanClose, bool UseDefaultOK, StdMessage Close)
    {
        auto P = Popup{}
            .CreateModal(Title, CanClose, std::move(Close))
            .SetFlag(ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)
            .SetSize(Size)
            .PushTextBack(Text);
        if(UseDefaultOK)P.PushMsgBack([X = Size.x]()
            {
                ImGui::NewLine();
                ImGui::SetCursorPosX(X / 2 - FontHeight);
                if (ImGui::Button(locc("GUI_OK")))
                    ClearPopupDelayed();
            });
        return P;
    }
    void RenderUI()
    {
        IsMouseOnPopupCond = false;
        bool AboutToCloseRight = false;
        if (HasPopup)
        {
            static bool pp = true;
            ImGui::OpenPopup(CurrentPopup.Title.c_str());
            DelayedPopupAction.clear();
            bool HPPrev = HasPopup;
            if (CurrentPopup.Size.x >= 1.0F && CurrentPopup.Size.y >= 1.0F)ImGui::SetNextWindowSize(CurrentPopup.Size);
            if (CurrentPopup.Modal)
            {
                if (ImGui::BeginPopupModal(CurrentPopup.Title.c_str(), CurrentPopup.CanClose ? (&HasPopup) : nullptr), CurrentPopup.Flag)
                {
                    IsMouseOnPopupCond |= ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
                    CurrentPopup.Show();
                    ImGui::EndPopup();
                }
            }
            else
            {
                if (ImGui::BeginPopup(CurrentPopup.Title.c_str(), CurrentPopup.Flag))
                {
                    IsMouseOnPopupCond |= ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
                    CurrentPopup.Show();
                    ImGui::EndPopup();
                }
            }
            if (CurrentPopup.CanClose && HPPrev && (!HasPopup))CurrentPopup.Close();
        }
        if (HasRightClickMenu)
        {
            if (!FirstRightClick && !ImGui::IsPopupOpen(RightClickMenu.Title.c_str()))
            {
                HasRightClickMenu = false;
            }
            else
            {
                FirstRightClick = false;
                ImGui::OpenPopup(RightClickMenu.Title.c_str());
                DelayedPopupAction.clear();
                if (RightClickMenu.Size.x >= 1.0F && RightClickMenu.Size.y >= 1.0F)ImGui::SetNextWindowSize(RightClickMenu.Size);
                ImGui::SetNextWindowPos(RightClickMenuPos);
                float W = -1.0F, H = -1.0F;
                if (ImGui::BeginPopup(RightClickMenu.Title.c_str(),
                    RightClickMenu.Flag
                    | ImGuiWindowFlags_NoTitleBar
                    | ImGuiWindowFlags_NoScrollbar
                    | ImGuiWindowFlags_AlwaysAutoResize
                    ))
                {
                    IsMouseOnPopupCond |= ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
                    if(!RightClickMenu.HasOwnStyle)ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_PopupBg));
                    RightClickMenu.Show();
                    ImGui::PushOrderFront(ImGui::GetCurrentWindow());
                    if (!RightClickMenu.HasOwnStyle)ImGui::PopStyleColor();
                    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
                        && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                        && RightClickMenu.InstantClose)
                    {
                        AboutToCloseRight = true;
                    }
                    H = ImGui::GetWindowHeight();
                    W = ImGui::GetWindowWidth();
                    ImGui::EndPopup();

                    //Adjust position if out of screen
                    ImVec2 NewPos = RightClickMenuPos;
                    auto ScrH = IBR_UICondition::CurrentScreenHeight - IBR_HintManager::GetHeight();
                    auto ScrW = IBR_UICondition::CurrentScreenWidth;
                    if (NewPos.y + H > ScrH )
                    {
                        NewPos.y = ScrH - H;
                        if (NewPos.y < 0)NewPos.y = 0;
                    }
                    if (NewPos.x + W > ScrW)
                    {
                        NewPos.x = ScrW - W;
                        if (NewPos.x < 0)NewPos.x = 0;
                    }
                    RightClickMenuPos = NewPos;
                }
            }
            
        }
        if (HasPopup || HasRightClickMenu)
        {
            ImGui::CloseCurrentPopup();
            int i = 0;
            while (!DelayedPopupAction.empty())
            {
                std::vector<StdMessage> DelayedPopupActionTemp;
                std::swap(DelayedPopupAction, DelayedPopupActionTemp);
                for (auto& Msg : DelayedPopupActionTemp)
                {
                    if (ImGui::Begin((CurrentPopup.Title + std::to_string(i)).c_str(), nullptr,
                        ImGuiWindowFlags_AlwaysAutoResize
                        | ImGuiWindowFlags_NoFocusOnAppearing
                        | ImGuiWindowFlags_NoTitleBar
                        | ImGuiWindowFlags_NoCollapse))
                    {
                        IsMouseOnPopupCond |= ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
                        Msg();
                        ImGui::PushOrderFront(ImGui::GetCurrentWindow());
                        ImGui::End();
                    }
                    i++;
                }
            }
            
        }
        if (AboutToCloseRight)HasRightClickMenu = false;
    }
}



void Browse_ShowList_Impl(const std::string& suffix, int* Page, BrowseParamList& List)
{
    if (List.HasPrev || List.HasNext)
    {
        if (List.HasPrev)
        {
            if (ImGui::ArrowButton(("prev_" + suffix).c_str(), ImGuiDir_Left))
            {
                (*Page)--;
                if (EnableLog)
                {
                    GlobalLog.AddLog_CurTime(false);
                    GlobalLog.AddLog(locc("Log_PrevPage"));
                }
            }
            ImGui::SameLine();
            ImGui::Text(locc("GUI_PrevPage"));
            ImGui::SameLine();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, { 0,0,0,0 });
            ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0,0,0,0 });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0,0,0,0 });
            ImGui::ArrowButton(("prev_" + suffix).c_str(), ImGuiDir_Left);
            ImGui::SameLine();
            ImGui::PopStyleColor(4);
        }
        if ((*Page) + 1 >= 1000)ImGui::SetCursorPosX(FontHeight * 13.0f);
        if ((*Page) + 1 >= 100)ImGui::SetCursorPosX(FontHeight * 12.5f);
        else ImGui::SetCursorPosX(FontHeight * 12.0f);
        if (List.HasNext)
        {
            ImGui::Text(locc("GUI_NextPage"));
            ImGui::SameLine();
            if (ImGui::ArrowButton(("next_" + suffix).c_str(), ImGuiDir_Right))
            {
                (*Page)++;
                if (EnableLog)
                {
                    GlobalLog.AddLog_CurTime(false);
                    GlobalLog.AddLog(locc("Log_NextPage"));
                }
            }
            ImGui::SameLine();

        }
        ImGui::NewLine();

        if (*Page != 0)
        {
            if (ImGui::ArrowButton(("fpg_" + suffix).c_str(), ImGuiDir_Left))
            {
                (*Page) = 0;
                if (EnableLog)
                {
                    GlobalLog.AddLog_CurTime(false);
                    GlobalLog.AddLog(locc("Log_FirstPage"));
                }
            }
            ImGui::SameLine();
            ImGui::Text(locc("GUI_FirstPage"));
            ImGui::SameLine();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, { 0,0,0,0 });
            ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0,0,0,0 });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0,0,0,0 });
            ImGui::ArrowButton("OBSOLETE_BUTTON", ImGuiDir_Left);
            ImGui::SameLine();
            ImGui::PopStyleColor(4);
        }
        if ((*Page) + 1 >= 1000)ImGui::SetCursorPosX(FontHeight * 5.5f);
        else ImGui::SetCursorPosX(FontHeight * 6.0f);
        auto PosYText = ImGui::GetCursorPosY();
        ImGui::SetCursorPosY(PosYText - FontHeight * 0.5f);
        ImGui::Text(locc("GUI_PageInfo"), (*Page) + 1, List.PageN);
        ImGui::SetCursorPosY(PosYText);
        ImGui::SameLine();
        if ((*Page) + 1 >= 1000)ImGui::SetCursorPosX(FontHeight * 13.0f);
        if ((*Page) + 1 >= 100)ImGui::SetCursorPosX(FontHeight * 12.5f);
        else ImGui::SetCursorPosX(FontHeight * 12.0f);
        if ((*Page) + 1 != List.PageN)
        {
            ImGui::Text(locc("GUI_LastPage"));
            ImGui::SameLine();
            if (ImGui::ArrowButton(("lpg_" + suffix).c_str(), ImGuiDir_Right))
            {
                (*Page) = List.PageN - 1;
                if (EnableLog)
                {
                    GlobalLog.AddLog_CurTime(false);
                    GlobalLog.AddLog(locc("Log_LastPage"));
                }
            }
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }
}

inline int GetPage(int u)
{
    return (u - 1) / KeyPerPage + 1;
}

BrowseParamList MakeParamList(size_t size, int Page)
{
    BrowseParamList L;
    L.RenderF = (Page) * KeyPerPage;
    L.RenderN = (1 + (Page)) * KeyPerPage;
    L.Sz = (int)size;
    L.HasPrev = ((Page) != 0);
    L.HasNext = (L.RenderN < L.Sz);
    L.RealRF = std::max(L.RenderF, 0);
    L.RealNF = std::min(L.RenderN, L.Sz);
    L.PageN = GetPage(L.Sz);
    return L;
}




namespace SearchModuleAlt
{
    extern IBR_ListMenu<IBB_ModuleAlt*> DoubleClickTable;
}

void DDCC()
{
    SearchModuleAlt::DoubleClickTable.RenderUI();
}

namespace IBR_HintManager
{
    std::vector<_TEXT_UTF8 std::string> Hint;
    int HintChangeMillis = 5000;
    int UseHint = 0;
    ETimer::TimerClass HintTimer;

    _TEXT_UTF8 std::string CustomHint;
    bool HasCustom = false, HasSet = false, IsCountDown = false;
    std::function<bool(_TEXT_UTF8 std::string&)> CustomFn;
    ETimer::TimerClass CountDownTimer;
    int CountDownMillis;

    void Clear()
    {
        HasSet = HasCustom = IsCountDown = false;
    }
    void Load()
    {
        ExtFileClass GetHint;
        if (GetHint.Open(".\\Resources\\hint.txt", "r"))
        {
            char str[MAX_STRING_LENGTH];
            while (!GetHint.Eof())
            {
                GetHint.GetStr(str, MAX_STRING_LENGTH);
                Hint.push_back(str);
            }
            GetHint.Close();
            if (!Hint.empty())Hint.erase(Hint.begin());//第一个不要！
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(std::vformat(locwc("Log_LoadHint"), std::make_wformat_args(L".\\Resources\\hint.txt")));
            }
        }
        else
        {
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                GlobalLog.AddLog(std::vformat(locwc("Log_LoadHintFailed"), std::make_wformat_args(L".\\Resources\\hint.txt")));
            }
        }
    }

    const float ExtHeight = 0.4f;
    const float TotHeight = 1.0f + 2 * ExtHeight;

    float GetHeight()
    {
        return FontHeight * TotHeight;
    }
    void RenderUI()
    {
        ImDrawList* DList = ImGui::GetForegroundDrawList();
        if (!Hint.empty())
        {
            if (HintTimer.GetMilli() > HintChangeMillis)
            {
                UseHint = rand() % Hint.size();
                HintTimer.Set();
            }
        }
        if (HasSet && IsCountDown && CountDownTimer.GetMilli() > CountDownMillis)Clear();
        if (HasSet && HasCustom)
            if (!CustomFn(CustomHint))Clear();

        DList->AddRectFilled({ 0.0f,(float)IBR_UICondition::CurrentScreenHeight - GetHeight() },
            { (float)IBR_UICondition::CurrentScreenWidth ,(float)IBR_UICondition::CurrentScreenHeight },
            ImColor(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg)));
        DList->AddLine({ 0.0f,(float)IBR_UICondition::CurrentScreenHeight - GetHeight() },
            { (float)IBR_UICondition::CurrentScreenWidth,(float)IBR_UICondition::CurrentScreenHeight - GetHeight() },
            ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), 1.0f);
        /*
        if (_TempSelectLink::IsInLink())
        {
            const char* Tx = u8"左键单击选择，右键取消";
            DList->AddText({ FontHeight * 0.8f,(float)IBR_UICondition::CurrentScreenHeight - FontHeight * 1.25f },
                ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Text)),
                Tx, Tx + strlen(Tx));
        }
        else*/

        if (!Hint.empty() || HasSet)
        {
            const std::string& HintStr = GetHint();
            DList->AddText({ FontHeight * 0.8f,(float)IBR_UICondition::CurrentScreenHeight - FontHeight * (1.0f + ExtHeight) },
                ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Text)),
                HintStr.c_str(), HintStr.c_str() + HintStr.size());
        }
    }
    void SetHint(_TEXT_UTF8 const std::string& Str, int TimeLimitMillis)
    {
        HasCustom = false;
        HasSet = true;
        CustomHint = Str;
        if (TimeLimitMillis != -1)
        {
            IsCountDown = true;
            CountDownMillis = TimeLimitMillis;
            CountDownTimer.Set();
        }
        else IsCountDown = false;

    }
    void SetHintCustom(const std::function<bool(_TEXT_UTF8 std::string&)>& Fn)
    {
        HasCustom = true;
        HasSet = true;
        CustomHint = "";
        IsCountDown = false;
        CustomFn = Fn;
    }
    const std::string& GetHint()
    {
        return HasSet ? CustomHint : Hint[UseHint];
    }
}

namespace IBR_WorkSpace
{
    extern ImVec2 DragStartMouse, DragStartEqCenter, DragStartReCenter;
    extern ImVec2 ExtraMove, MousePosExt, DragStartEqMouse, DragCurMouse, DragCurEqMouse;
    extern int UpdatePrevResult;
    extern float NewRatio;
    extern bool NeedChangeRatio, HoldingModules, InitHolding, IsMassSelecting, IsMassAfter, HasRightDownToWait, HasLefttDownToWait, MoveAfterMass;
}

namespace IBR_SelectMode
{
    Mode CurrentMode;
    bool InSelectProcess;
    std::vector<IBR_Project::id_t> MassSelectWindows;
    std::set<IBB_Section_Desc> MassSelectWindowsDesc;
    const std::vector<IBR_Project::id_t>& GetMassSelected()
    {
        return MassSelectWindows;
    }
    bool IsWindowMassSelected(const IBB_Section_Desc& Desc)
    {
        return (IBR_WorkSpace::IsMassSelecting || IBR_WorkSpace::IsMassAfter) && MassSelectWindowsDesc.count(Desc) != 0;
    }
    void EnterSelectMode(const Mode& Mode)
    {
        if (!InSelectProcess)
        {
            InSelectProcess = true;
            CurrentMode = Mode;
        }
    }
    void ExitSelectMode(IBR_Section Section, ImVec2 ClickRePos)
    {
        if (InSelectProcess)
        {
            InSelectProcess = false;
            CurrentMode.Exit(Section, ClickRePos);
        }
    }
    void CancelSelectMode()
    {
        if (InSelectProcess)
        {
            InSelectProcess = false;
            CurrentMode.Cancel();
        }
    }
    void UpdateMassSelect()
    {
        if (IBR_WorkSpace::IsMassSelecting)
        {
            MassSelectWindows.clear();
            MassSelectWindowsDesc.clear();
            auto [MinX, MaxX] = std::minmax(IBR_WorkSpace::DragStartEqMouse.x, IBR_WorkSpace::DragCurEqMouse.x);
            auto [MinY, MaxY] = std::minmax(IBR_WorkSpace::DragStartEqMouse.y, IBR_WorkSpace::DragCurEqMouse.y);
            ImRect SelectRect{ {MinX, MinY}, {MaxX ,MaxY} };
            for (auto& [id, ds] : IBR_Inst_Project.IBR_SectionMap)
            {
                if (!IBR_Inst_Project.GetSectionFromID(id).HasBack())continue;
                ImRect EqRect{ ds.EqPos,ds.EqPos + ds.EqSize };
                if (SelectRect.Contains(EqRect))
                {
                    MassSelectWindows.push_back(id);
                    MassSelectWindowsDesc.insert(ds.Desc);
                }
            }
        }
    }
    void RenderUI_MassSelect()
    {
        if (!IBR_PopupManager::HasPopup && IBR_WorkSpace::IsMassSelecting)
        {
            ImDrawList* DList = ImGui::GetForegroundDrawList();
            DList->PushClipRect(IBR_RealCenter::WorkSpaceUL, IBR_RealCenter::WorkSpaceDR);
            auto [MinX, MaxX] = std::minmax(IBR_WorkSpace::DragStartEqMouse.x, IBR_WorkSpace::DragCurEqMouse.x);
            auto [MinY, MaxY] = std::minmax(IBR_WorkSpace::DragStartEqMouse.y, IBR_WorkSpace::DragCurEqMouse.y);
            ImRect SelectRect{ {MinX, MinY}, {MaxX, MaxY} };
            for (auto& [id, ds] : IBR_Inst_Project.IBR_SectionMap)
            {
                if (!IBR_Inst_Project.GetSectionFromID(id).HasBack())continue;
                if (ds.IsIncluded())continue;
                ImRect EqRect{ ds.EqPos,ds.EqPos + ds.EqSize };
                if (SelectRect.Contains(EqRect))
                {
                    auto ReUL = IBR_WorkSpace::EqPosToRePos(ds.EqPos), ReDR = IBR_WorkSpace::EqPosToRePos(ds.EqPos + ds.EqSize);
                    DList->AddRectFilled(ReUL, ReDR, IBR_Color::ForegroundCoverColor);
                }
            }
            auto S = IBR_WorkSpace::EqPosToRePos(IBR_WorkSpace::DragStartEqMouse), T = IBR_WorkSpace::EqPosToRePos(IBR_WorkSpace::DragCurEqMouse);
            DList->AddRect(S, T, IBR_Color::ForegroundMarkColor);
            DList->PopClipRect();
        }
        if (!IBR_PopupManager::HasPopup && IBR_WorkSpace::IsMassAfter)
        {
            ImDrawList* DList = ImGui::GetForegroundDrawList();
            DList->PushClipRect(IBR_RealCenter::WorkSpaceUL, IBR_RealCenter::WorkSpaceDR);
            for (auto& id : IBR_WorkSpace::MassTarget)
            {
                auto R = IBR_Inst_Project.GetSectionFromID(id);
                auto ds = R.GetSectionData();
                if (!ds || !R.HasBack())continue;
                if (ds->IsIncluded())continue;
                auto ReUL = IBR_WorkSpace::EqPosToRePos(ds->EqPos), ReDR = IBR_WorkSpace::EqPosToRePos(ds->EqPos + ds->EqSize);
                DList->AddRectFilled(ReUL, ReDR, IBR_Color::ForegroundCoverColor);
            }
            DList->PopClipRect();
        }
    }
    void RenderUI_SelectProcess()
    {
        if (InSelectProcess)
        {
            auto& WUL = IBR_RealCenter::WorkSpaceUL, WDR = IBR_RealCenter::WorkSpaceDR;
            ImDrawList* DList = ImGui::GetForegroundDrawList();
            IBR_Project::id_t HoverId = ULLONG_MAX;
            if (CurrentMode.RestrictedToSections)
            {
                for (auto& sp : IBR_Inst_Project.IBR_SectionMap)
                {
                    if (!IBR_Inst_Project.GetSectionFromID(sp.first).HasBack())continue;
                    auto ReUL = IBR_WorkSpace::EqPosToRePos(sp.second.EqPos), ReDR = IBR_WorkSpace::EqPosToRePos(sp.second.EqPos + sp.second.EqSize);
                    auto [ok, NUL, NDR] = RectangleCross(ReUL, ReDR, WUL, WDR);
                    if (ok)DList->AddRectFilled(NUL, NDR, IBR_Color::ForegroundCoverColor);

                    if (sp.second.Hovered)HoverId = sp.first;
                }
            }
            else
            {
                DList->AddRectFilled(WUL, WDR, IBR_Color::ForegroundCoverColor);
            }
            auto& io = ImGui::GetIO();
            if (IBR_WorkSpace::InWorkSpace(io.MousePos))
            {
                DList->AddCircle(io.MousePos, 5.0f, IBR_Color::LinkingLineColor, 0, 2.0f);

                bool RC = ImGui::IsMouseClicked(ImGuiMouseButton_Right), LC = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                if (RC && (!LC))CancelSelectMode();
                if (LC && (!RC))
                {
                    if (CurrentMode.RestrictedToSections)
                    {
                        if (HoverId != ULLONG_MAX)ExitSelectMode(IBR_Inst_Project.GetSectionFromID(HoverId), io.MousePos);
                    }
                    else ExitSelectMode(IBR_Inst_Project.GetSectionFromID(ULLONG_MAX), io.MousePos);
                }
            }
        }
    }
    void RenderUI()
    {
        UpdateMassSelect();
        RenderUI_MassSelect();
        RenderUI_SelectProcess();
    }
    bool InSelectMode()
    {
        return InSelectProcess;
    }
    const Mode& CurrentSelectMode()
    {
        return CurrentMode;
    }
}
