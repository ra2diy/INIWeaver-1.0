#pragma once
#include "FromEngine/Include.h"

void PushComboRect();
bool IBR_Combo_Stage_I(const char* label, const char* preview_value, ImGuiComboFlags flags);
void IBR_Combo_Stage_II();
void IBR_Combo_Stage_III();

template <typename T>
bool IBR_Combo(const char* label, const char* preview_value, ImGuiComboFlags flags, const T& body)
{
    if (IBR_Combo_Stage_I(label, preview_value, flags))
    {
        IBR_Combo_Stage_II();
        body();
        IBR_Combo_Stage_III();
        return true;
    }
    return false;
}

struct ImRect;
std::vector<ImRect>& GetComboRects();

void IBR_ToolTip(const char* Str);
void IBR_ToolTip(const std::string& Str);
void IBR_ToolTip(const std::wstring& Str);

void EditStringWithOptions(
    bool Active,
    std::string& str
);
