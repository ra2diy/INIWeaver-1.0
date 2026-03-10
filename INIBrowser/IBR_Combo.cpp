#include "IBR_Combo.h"
#include <imgui_internal.h>
#include "Global.h"
#include "IBR_Components.h"
#include "IBB_PropStringPool.h"

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
    auto w = ImGui::GetCurrentWindow();
    auto mx = w->DC.CursorMaxPos;
    bool Clicked = ImGui::BeginCombo(label, preview_value, flags);
    w->DC.CursorMaxPos = mx;
    return Clicked;
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
    IBR_ToolTip(Str.c_str());
}

void IBR_ToolTip(const char* Str)
{
    if (!*Str)return;

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
        ImGui::Text(Str);
        ImGui::EndTooltip();
    }
    else
    {
        auto Wrap = FontHeight * awt * 1.0F;
        auto Pos = ImGui::CalcTextSize(Str, 0, false, Wrap);
        auto PreH = Pos.y + ImGui::GetTextLineHeightWithSpacing();
        auto PosY = ImGui::GetMousePos().y + PreH;
        if (PosY > ScrH)
            ImGui::SetNextWindowPos({ ImGui::GetMousePos().x, ScrH - PreH });

        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(Wrap);
        ImGui::Text(Str);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    
}



// 判断 a 是否在 b 中出现（不区分大小写）
bool contains_ignore_case(const std::string& a, const std::string& b) {
    auto it = std::search(
        b.begin(), b.end(),
        a.begin(), a.end(),
        [](unsigned char c1, unsigned char c2) {
            return std::tolower(c1) == std::tolower(c2);
        }
    );
    return it != b.end();
}

inline bool Matches(std::string& str, const std::string& Name, const IBB_IniLine_Default& opt)
{
    //不分大小写，Name或者DescShort包含str就算匹配
    return contains_ignore_case(str, Name) || contains_ignore_case(str, PoolDesc(opt.DescShort));
}

void EditStringWithOptions(
    bool Active,
    std::string& str)
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

        int Count = 0;
        for (auto& [Name, Line] : IBF_Inst_DefaultTypeList.List.IniLine_Default)
        {
            if (!Line.Known)continue;
            auto NameStr = PoolStr(Name);
            if (!Matches(str, NameStr, Line))continue;
            if(Count++ > 100)
            {
                ImGui::TextDisabled(locc("GUI_TooManyOptions"));
                break;
            }

            if (ImGui::Selectable((NameStr + " : " + PoolDesc(Line.DescShort)).c_str(), false))
                str = NameStr;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                str = NameStr;
            if(ImGui::IsItemHovered())
                IBR_ToolTip(PoolDesc(Line.DescLong));
        }
        ImGui::EndChildFrame();
    }
}
