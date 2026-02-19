#include "IBR_Combo.h"
#include <imgui_internal.h>
#include "Global.h"

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
    ImGui::BeginTooltip();
    auto awt = IBF_Inst_Setting.AutoWrapThreshold();
    if (awt != -1)ImGui::PushTextWrapPos(FontHeight * awt * 1.0F);
    ImGui::Text(Str.c_str());
    if (awt != -1)ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}
