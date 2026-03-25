#include "IBR_TopMost.h"
#include "IBR_Misc.h"
#include <imgui_internal.h>

namespace IBR_TopMost
{
    const char* LayerName = "##IBR_TopMost_Popup";
    std::map<int, std::vector<RenderPayload>> Payloads;
    std::vector<std::pair<ImVec2, StdMessage>> DrawOprPayloads;

    void CommitText(const ImVec2& pos, ImU32 col, const char* text, int Priority)
    {
        CommitPayload([=, T = std::string(text)](ImDrawList* DList)
            {
                DList->AddText(pos, col, T.c_str());
            }, Priority);
    }
    void CommitRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags, float thickness, int Priority)
    {
        CommitPayload([=](ImDrawList* DList)
            {
                DList->AddRect(p_min, p_max, col, rounding, flags, thickness);
            }, Priority);
    }
    void CommitPushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect, int Priority)
    {
        CommitPayload([=](ImDrawList* DList)
            {
                DList->PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect);
            }, Priority);
    }
    void CommitRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags flags, int Priority)
    {
        CommitPayload([=](ImDrawList* DList)
            {
                DList->AddRectFilled(p_min, p_max, col, rounding, flags);
            }, Priority);
    }
    void CommitPopClipRect(int Priority)
    {
        CommitPayload([](ImDrawList* DList)
            {
                DList->PopClipRect();
            }, Priority);
    }
    void CommitPayload(const RenderPayload& Payload, int Priority)
    {
        Payloads[Priority].push_back(Payload);
    }
    void CommitDrawOpr(const ImVec2& pos, const StdMessage& Msg)
    {
        DrawOprPayloads.push_back({ pos, Msg });
    }

    void RenderUI_MenuOff()
    {
        if (Payloads.empty() && DrawOprPayloads.empty())return;

        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::SetNextWindowPos(IBR_RealCenter::WorkSpaceUL);
        ImGui::SetNextWindowSize(IBR_RealCenter::WorkSpaceDR - IBR_RealCenter::WorkSpaceUL);
        ImGui::Begin(LayerName, nullptr,
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoMouseInputs
        );
        ImGui::SetWindowFontScale(IBR_FullView::Ratio);
        auto Window = ImGui::GetCurrentWindow();

        //ImGui::Text("Hello World");
        //ImGui::Text("This is a message!");

        for (auto& [Pos, Msg] : DrawOprPayloads)
        {
            ImGui::SetCursorScreenPos(Pos);
            ImGui::BeginGroup();
            Msg();
            ImGui::EndGroup();
        }

        ImDrawList* DList = ImGui::GetWindowDrawList();//ImGui::GetForegroundDrawList();
        for (auto& [Pri, PayloadList] : Payloads)
            for (auto& Payload : PayloadList)
                Payload(DList);



        ImGui::End();
        Payloads.clear();
        DrawOprPayloads.clear();

        ImGui::GetCurrentContext()->ExtraTopMostWindow2 = Window;
    }

    bool TopMostMenuOn = false;
    size_t OpenMenuID;
    size_t MenuContentIdentifier;
    size_t InvalidMenuID = size_t(-1);
    StdMessage MenuContent;
    bool IsTopMostMenuOn()
    {
        return TopMostMenuOn;
    }
    bool MenuMatchesIdent(ImGuiID Identifier)
    {
        return TopMostMenuOn && MenuContentIdentifier == Identifier;
    }
    bool MenuMatchesMenuID(size_t MenuID)
    {
        return TopMostMenuOn && OpenMenuID == MenuID;
    }
    void OpenTopMostMenu(ImGuiID Identifier, StdMessage Content)
    {
        OpenTopMostFrom(InvalidMenuID, Identifier, Content);
    }
    bool MenuMatchesSource(size_t MenuID, ImGuiID Identifier)
    {
        return TopMostMenuOn && MenuContentIdentifier == Identifier && OpenMenuID == MenuID;
    }
    void CloseTopMostMenu()
    {
        TopMostMenuOn = false;
        OpenMenuID = InvalidMenuID;
    }
    void OpenTopMostFrom(size_t MenuID, ImGuiID Identifier, StdMessage Content)
    {
        TopMostMenuOn = true;
        OpenMenuID = MenuID;
        MenuContent = Content;
        MenuContentIdentifier = Identifier;
    }
    void UpdateTopMostMenu()
    {
        auto Item = IBR_Inst_Menu.GetMenuItem();
        if (Item != OpenMenuID && OpenMenuID != -1)
            CloseTopMostMenu();
    }

    void RenderUI_MenuOn()
    {
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::SetNextWindowPos(IBR_RealCenter::WorkSpaceUL);
        ImGui::SetNextWindowSize(IBR_RealCenter::WorkSpaceDR - IBR_RealCenter::WorkSpaceUL);
        ImGui::Begin(LayerName, nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoResize
        );
        //auto Window = ImGui::GetCurrentWindow();

        PushComboRect();
        MenuContent();

        ImGui::End();
        ImGui::GetCurrentContext()->ExtraTopMostWindow2 = nullptr;
    }


    void RenderUI()
    {
        UpdateTopMostMenu();
        if (TopMostMenuOn)RenderUI_MenuOn();
        else RenderUI_MenuOff();
    }
}
