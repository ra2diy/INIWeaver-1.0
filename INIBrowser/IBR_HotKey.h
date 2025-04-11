#pragma once
#include "FromEngine/Include.h"
#include "Global.h"

namespace IBR_HotKey
{
    struct HotKey
    {
        bool Ctrl{ false },
            Shift{ false },
            Alt{ false },
            Super{ false };
        std::vector<ImGuiKey> Keys{};

        bool Match();
        void Load(JsonObject J);
    };
    void InitFromJson(JsonObject J);
    HotKey GetHotKey(const std::string& Name);
}

#define IsHotKeyPressedDec(KeyID) bool Is ## KeyID ## Pressed();
    
#define IsHotKeyPressedDef(KeyID) \
bool Is ## KeyID ## Pressed() \
{\
    static IBR_HotKey::HotKey K;\
    if(K.Keys.empty())\
    {\
        K = IBR_HotKey::GetHotKey(#KeyID);\
    }\
    return K.Match();\
}

#define IsHotKeyPressed(KeyID) (Is ## KeyID ## Pressed())

IsHotKeyPressedDec(Copy);
IsHotKeyPressedDec(Paste);
IsHotKeyPressedDec(Cut);
IsHotKeyPressedDec(Undo);
IsHotKeyPressedDec(Redo);
IsHotKeyPressedDec(SelectAll);
IsHotKeyPressedDec(SelectNone);//Ctrl+Shift+A temporarily unused
IsHotKeyPressedDec(SelectInvert);//Ctrl+Shift+I temporarily unused
IsHotKeyPressedDec(Save);
IsHotKeyPressedDec(Open);
IsHotKeyPressedDec(Close);
IsHotKeyPressedDec(SaveAs);
IsHotKeyPressedDec(Export);
IsHotKeyPressedDec(Delete);
IsHotKeyPressedDec(DeleteAll);
IsHotKeyPressedDec(Center);
IsHotKeyPressedDec(Refresh);
IsHotKeyPressedDec(SwitchDisplayMode);
IsHotKeyPressedDec(RenameModule);
IsHotKeyPressedDec(RenameRegister);
