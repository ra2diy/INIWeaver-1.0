#include "IBR_Combo.h"
#include <imgui_internal.h>

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
    ImGui::PushOrderFront(ImGui::GetCurrentWindow());
}

void IBR_Combo_Stage_III()
{
    ComboRects.push_back(ImGui::GetCurrentWindow()->Rect());
    ImGui::EndCombo();
}
