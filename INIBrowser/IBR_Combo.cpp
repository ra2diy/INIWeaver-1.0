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

void PushComboRect(const ImRect& R)
{
    ComboRects.push_back(R);
}

void PushComboRect()
{
    ComboRects.push_back(ImGui::GetCurrentWindow()->Rect());
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

bool Matches(std::string& str, const InputTextOption& opt)
{
    //不分大小写，Text或者Desc包含str就算匹配
    auto StrLower = str;
    for (auto& c : StrLower)c = (char)tolower(c);
    return opt.Pattern.contains(StrLower);
}

bool InputTextStdStringWithOption(
    const char* label,
    std::string& str,
    ImGuiInputTextFlags flags,
    const std::vector<InputTextOption>& options)
{
    const ImVec2 Cursor = ImGui::GetCursorScreenPos();
    auto Ret = InputTextStdString(label, str, flags);

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::BeginComboEx("##TYPEALT_DICT", str.c_str(), ImGuiComboFlags_NoPreview, false, Cursor))
    {
        ComboRects.push_back(ImGui::GetCurrentWindow()->Rect());
        ImGui::PushOrderFront(ImGui::GetCurrentWindow());

        for (auto& opt : options)
        {
            if (!Matches(str, opt))
                continue;
            if (ImGui::Selectable((opt.Text + " : " + opt.Desc).c_str(), false))
                str = opt.Text;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                str = opt.Text;
            if (ImGui::IsItemHovered())
                IBR_ToolTip(opt.Hint);
        }
        ImGui::EndCombo();
    }

    return Ret;
}

void EditStringWithOptions(
    bool Active,
    std::string& str,
    const std::vector<InputTextOption>& options)
{
    static bool LastActive = false;
    static bool LastActive2 = false;
    if(Active || LastActive || LastActive2)
    {
        LastActive2 = LastActive;
        LastActive = Active;

        //Scrollbar cannot work properly here
        ImGui::BeginChildFrame(ImGui::GetID("##TYPEALT_DICT"), { 0, FontHeight * 10.0f },
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
        for (auto& opt : options)
        {
            if (!Matches(str, opt))
                continue;
            if (ImGui::Selectable((opt.Text + " : " + opt.Desc).c_str(), false))
                str = opt.Text;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                str = opt.Text;
            if(ImGui::IsItemHovered())
                IBR_ToolTip(opt.Hint);
        }
        ImGui::EndChildFrame();
    }
}
