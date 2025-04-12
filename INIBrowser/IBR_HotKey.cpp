#include "IBR_HotKey.h"

namespace IBR_HotKey
{
    std::unordered_map<std::string, ImGuiKey> KeyNames;
    std::optional<ImGuiKey> GetKey(const std::string& Name)
    {
        if (KeyNames.empty())
        {
            //ImGui::GetKeyName(ImGuiKey key)
            for (ImGuiKey I = ImGuiKey_Tab; I < ImGuiKey_GamepadStart; I++)
            {
                KeyNames[ImGui::GetKeyName(I)] = I;
            }
        }
        auto it = KeyNames.find(Name);
        if (it != KeyNames.end())return it->second;
        return std::nullopt;
    }

    bool HotKey::Match()
    {
        if (IBR_WorkSpace::LastOperateOnText)return false;

        auto& IO = ImGui::GetIO();

        if (Ctrl ^ IO.KeyCtrl)return false;
        if (Shift ^ IO.KeyShift)return false;
        if (Alt ^ IO.KeyAlt)return false;
        if (Super ^ IO.KeySuper)return false;

        if (Keys.empty())return false;
        for (auto& k : Keys)
            if (!ImGui::IsKeyPressed(k))
                return false;
        return true;
    }

    void HotKey::Load(JsonObject J)
    {
        if (!J.Available())return;
        if (!J.IsTypeArray())
        {
            if (EnableLog)
            {
                GlobalLog.AddLog_CurTime(false);
                auto w = UTF8toUnicode(J.PrintData());
                GlobalLog.AddLog(std::vformat(locw("Error_HotKeyFormat"), std::make_wformat_args(w)));
            }
        }
        for (const auto& w : J.GetArrayString())
        {
            if (w.empty())continue;
            if (w == "Ctrl")Ctrl = true;
            else if (w == "Shift")Shift = true;
            else if (w == "Alt")Alt = true;
            else if (w == "Super")Super = true;
            else
            {
                auto k = GetKey(w);
                if (k)
                {
                    Keys.push_back(k.value());
                }
                else
                {
                    if (EnableLog)
                    {
                        GlobalLog.AddLog_CurTime(false);
                        auto q = UTF8toUnicode(w);
                        GlobalLog.AddLog(std::vformat(locw("Error_NotAHotKey"), std::make_wformat_args(q)));
                    }
                }
            }
        }
    }

    std::unordered_map<std::string, HotKey> HotKeys;

    void InitFromJson(JsonObject J)
    {
        if (!J.Available())return;
        for (auto& [k, v] : J.GetMapObject())
        {
            HotKeys[k].Load(v);
        }
    }

    HotKey GetHotKey(const std::string& Name)
    {
        auto it = HotKeys.find(Name);
        if (it != HotKeys.end())return it->second;
        return {};
    }
}


IsHotKeyPressedDef(Copy);
IsHotKeyPressedDef(Paste);
IsHotKeyPressedDef(Cut);
IsHotKeyPressedDef(Undo);
IsHotKeyPressedDef(Redo);
IsHotKeyPressedDef(SelectAll);
IsHotKeyPressedDef(SelectNone);
IsHotKeyPressedDef(SelectInvert);
IsHotKeyPressedDef(Save);
IsHotKeyPressedDef(Open);
IsHotKeyPressedDef(Close);
IsHotKeyPressedDef(SaveAs);
IsHotKeyPressedDef(Export);
IsHotKeyPressedDef(Delete);
IsHotKeyPressedDef(DeleteAll);
IsHotKeyPressedDef(Center);
IsHotKeyPressedDef(Refresh);
IsHotKeyPressedDef(SwitchDisplayMode);
IsHotKeyPressedDef(RenameModule);
IsHotKeyPressedDef(RenameRegister);
