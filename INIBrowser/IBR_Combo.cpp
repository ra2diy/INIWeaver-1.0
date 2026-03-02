#include "IBR_Combo.h"
#include <imgui_internal.h>
#include "Global.h"
#include "IBR_Components.h"

namespace ImGui
{
    void PushOrderFront(ImGuiWindow* Window);
}

std::vector<ImRect> ComboRects;

std::vector<ImRect>& GetComboRects()
{
    return ComboRects;
}

void ClearComboRects()
{
    ComboRects.clear();
}


bool IBR_Combo_Stage_I(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
    return ImGui::BeginCombo(label, preview_value, flags);
}

void IBR_Combo_Stage_II()
{
    ComboRects.push_back(ImGui::GetCurrentWindow()->Rect());
    ImGui::PushOrderFront(ImGui::GetCurrentWindow());
}

void IBR_Combo_Stage_III()
{
    ImGui::EndCombo();
}

extern int FontHeight;

void IBR_ToolTip(const std::wstring& Str)
{
    IBR_ToolTip(UnicodetoUTF8(Str));
}

void IBR_ToolTip(const std::string& Str)
{
    //Autowrap & Use String
    auto awt = IBF_Inst_Setting.AutoWrapThreshold();
    auto ScrH = IBR_UICondition::CurrentScreenHeight - IBR_HintManager::GetHeight();
    if (awt == -1)
    {
        auto PreH = ImGui::GetTextLineHeightWithSpacing();
        auto PosY = ImGui::GetMousePos().y + PreH;
        if (PosY > ScrH)
            ImGui::SetNextWindowPos({ ImGui::GetMousePos().x, ScrH - PreH });
        ImGui::BeginTooltip();
        ImGui::Text(Str.c_str());
        ImGui::EndTooltip();
    }
    else
    {
        auto Wrap = FontHeight * awt * 1.0F;
        auto Pos = ImGui::CalcTextSize(Str.c_str(), 0, false, Wrap);
        auto PreH = Pos.y + ImGui::GetTextLineHeightWithSpacing();
        auto PosY = ImGui::GetMousePos().y + PreH;
        if (PosY > ScrH)
            ImGui::SetNextWindowPos({ ImGui::GetMousePos().x, ScrH - PreH });

        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(Wrap);
        ImGui::Text(Str.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    
}

bool tf(const char* label, std::string& str, ImGuiInputTextFlags flags)
{
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL);
    const float iw = ImGui::CalcItemWidth();
    const ImVec2 Cursor = ImGui::GetCursorScreenPos();
    const ImVec2 Pad = ImGui::GetCurrentContext()->Style.FramePadding;
    const ImRect bb(Cursor, { Cursor.x + iw, Cursor.y + label_size.y + Pad.y * 2.0f });
    const ImGuiID popup_id = ImHashStr("##TYPEALT_DICT");
    bool popup_open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);

    auto Ret = InputTextStdString(label, str, flags);
    if(ImGui::IsItemActive() && !popup_open)
    {
        popup_open = true;
        ImGui::OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        ComboRects.push_back(ImGui::GetCurrentWindow()->Rect());
        ImGui::BeginComboPopup(popup_id, bb, 0);
        //ComboRects.push_back(ImGui::GetCurrentWindow()->Rect());
        //ImGui::PushOrderFront(ImGui::GetCurrentWindow());
        //if (ImGui::Selectable("1", false))IBR_HintManager::SetHint("1", HintStayTimeMillis);
        //if (ImGui::Selectable("2", false))IBR_HintManager::SetHint("2", HintStayTimeMillis);
        //if (ImGui::Selectable("3", false))IBR_HintManager::SetHint("3", HintStayTimeMillis);
        //if (ImGui::Selectable("4", false))IBR_HintManager::SetHint("4", HintStayTimeMillis);
        ImGui::EndCombo();
    }
    return Ret;
}
