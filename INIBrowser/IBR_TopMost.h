#pragma once
#include "IBR_Project.h"

namespace IBR_TopMost
{
    extern const char* LayerName;
    using RenderPayload = std::function<void(ImDrawList* DList)>;

    void CommitText(const ImVec2& pos, ImU32 col, const char* text, int Priority = 0);
    void CommitRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f, int Priority = 0);
    void CommitPushClipRect(const ImVec2& clip_rect_min, const ImVec2& clip_rect_max, bool intersect_with_current_clip_rect = false, int Priority = 0);
    void CommitRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, int Priority = 0);
    void CommitPopClipRect(int Priority = 0);
    void CommitPayload(const RenderPayload& Payload, int Priority = 0);
    void CommitDrawOpr(const ImVec2& pos, const StdMessage& Msg);
    void RenderUI();

    bool IsTopMostMenuOn();
    bool MenuMatchesIdent(ImGuiID Identifier);
    bool MenuMatchesMenuID(size_t MenuID);
    bool MenuMatchesSource(size_t MenuID, ImGuiID Identifier);
    void OpenTopMostMenu(ImGuiID Identifier, StdMessage MenuContent);
    void CloseTopMostMenu();
    void OpenTopMostFrom(size_t MenuID, ImGuiID Identifier, StdMessage MenuContent);
    void UpdateTopMostMenu();
}
