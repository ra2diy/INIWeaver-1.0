#include "IBG_InputType_Derived.h"
#include "IBB_CustomBool.h"
#include <ranges>


// ======== InputFormComponentFactory ==========

IICPtr InputFormComponentFactory::CreateInputComponent(IBB_ValueContainer& Cont, const JsonObject& Obj)
{
    //【】 is optional
    // 
    // General Setting
    //
    // { "InitialStatus" : <IICStatus> }
    // <IICStatus> :
    // "InitialLink" / "InitialInput"
    // { "LinkNode" : <LinkNodeSetting> }
    // <LinkNodeSetting> :
    // { "Type": <string>, "Limit": <int>, "Color": <Color> }

    auto piic = CreateInputComponent_Special(Cont, Obj);

    if (!piic) return nullptr;

    auto oInitialStatus = Obj.GetObjectItem("InitialStatus");

    if (oInitialStatus)
    {
        if (!piic->InitialStatus.Load(oInitialStatus))
            return nullptr;
    }

    auto oLinkNode = Obj.GetObjectItem("LinkNode");

    if (oLinkNode)
    {
        piic->NodeSetting.Load(oLinkNode, &piic->UseCustomSetting);
    }
    else
    {
        piic->UseCustomSetting = false;
    }

    return piic;
}

IICPtr InputFormComponentFactory::CreateInputComponent_Special(IBB_ValueContainer& Cont, const JsonObject& Obj)
{
    //【】 is optional
    // Special Setting
    // 
    //IIC_PureText(const std::string& InitialText, ImColor color, bool colored, bool disabled, bool wrapped)
    // <string>
    // {"Type": "Text", "Text": <string>【, "Color": <Color>】【, "Disabled": <bool>】【, "Wrapped": <bool>】}
    //IIC_LocalizedText(const std::string& key, const std::string& Fallback, ImColor color, bool colored, bool disabled, bool wrapped);
    // {"Key": <string>, "FallBack": <string>}
    // {"Type": "Text", "Key": <string>, "FallBack": <string>,【, "Color": <Color>】【, "Disabled": <bool>】【, "Wrapped": <bool>】}
    //IIC_SameLine
    // "<SameLine>"
    // {"Type": "SameLine"}
    //IIC_NewLine
    // "<NewLine>"
    // {"Type": "NewLine"}
    //IIC_Separator
    // "<Separator>"
    // {"Type": "Separator"}
    //IIC_InputText(int valueid, const std::string& InitialText, const std::string& hint)
    // {"Type": "InputText", "ValueID": <int> 【, "InitialValue": <string>】【, "Hint": <string>】}
    // While Link shares the class but with preset initial status
    // {"Type": "Link", "ValueID": <int> 【, "InitialValue": <string>】【, "Hint": <string>】}
    //IIC_MultipleChoice(IBB_ValueContainer& Cont, int valueid, const std::string& InitialText, const std::string& hint, bool sameline, const std::unordered_map<std::string, IICDescStr>& options, const std::vector<std::string>& OptionOrder);
    // {"Type": "MultipleChoice", "ValueID": <int>, "InitialValue": <string>, "Options": { <AllowedValue1>: <DisplayName1>, <AllowedValue2>: <DisplayName2>, ... } 【, "Hint": <string>】【, "SameLine": <bool>】}
    //IIC_EnumCombo(int valueid, const std::string& InitialValue, const std::string& hint, const std::unordered_map<std::string, std::string>& options)
    // {"Type": "EnumCombo", "ValueID": <int>, "InitialValue": <string>, "Options": { <AllowedValue1>: <DisplayName1>, <AllowedValue2>: <DisplayName2>, ... } 【, "Hint": <string>】}
    //IIC_EnumRadio(int valueid, const std::string& InitialValue, const std::unordered_map<std::string, std::string>& options, bool sameline)
    // {"Type": "EnumRadio", "ValueID": <int>, "InitialValue": <string>, "Options": { <AllowedValue1>: <DisplayName1>, <AllowedValue2>: <DisplayName2>, ... } 【, "Hint": <string>】【, "SameLine": <bool>】}
    //IIC_Bool(int valueid, bool InitialValue, StrBoolType fmt, const std::string& hint)
    // {"Type": "Bool", "ValueID": <int>, "InitialValue": <bool> 【, "Fmt": <string>】【, "Hint": <string>】}
    //IIC_InputInt(int valueid, int InitialValue, int min, int max, const std::string& hint)
    // {"Type": "InputInt", "ValueID": <int>, "InitialValue": <int>【, "Min": <int>, 】【"Max": <int> 】【, "Hint": <string>】}
    //IIC_SliderInt(int valueid, int InitialValue, int min, int max, const std::string& hint, const std::string& slidefmt, bool log)
    // {"Type": "SliderInt", "ValueID": <int>, "InitialValue": <int>, "Min": <int>, "Max": <int> 【, "Hint": <string>】【, "ValueFormat": <string>】【, "Logarithmic": <bool>】}

    if (!Obj)return nullptr;

    //PROCESS SHORTCUTS

    if (Obj.IsTypeString())
    {
        auto typeStr = Obj.GetString();
        if (typeStr == "<SameLine>")
            return std::make_unique<IIC_SameLine>();
        else if (typeStr == "<NewLine>")
            return std::make_unique<IIC_NewLine>();
        else if (typeStr == "<Separator>")
            return std::make_unique<IIC_Separator>();
        else
            return std::make_unique<IIC_PureText>(typeStr, ImColor(), false, false, false);
    }

    if (!Obj.IsTypeObject())return nullptr;

    auto oKey = Obj.GetObjectItem("Key");
    auto oFallback = Obj.GetObjectItem("FallBack");

    if (oKey && oFallback && oKey.IsTypeString() && oFallback.IsTypeString())
    {
        return std::make_unique<IIC_LocalizedText>(oKey.GetString(), oFallback.GetString(), ImColor(), false, false, false);
    }

    //PROCESS OBJECT

    auto oType = Obj.GetObjectItem("Type");
    if (oType && oType.IsTypeString())
    {
        auto typeStr = oType.GetString();
        if (typeStr == "Text")
        {
            auto oText = Obj.GetObjectItem("Text");

            //【, "Color": <Color>】【, "Colored": <bool>】
            ImColor Col;
            bool Colored = false;
            auto oColor = Obj.GetObjectItem("Color");
            auto V = oColor.GetArrayInt();
            if (V.size() == 3)
            {
                Col = ImColor(V[0], V[1], V[2]);
                Colored = true;
            }
            else if (V.size() >= 4)
            {
                Col = ImColor(V[0], V[1], V[2], V[3]);
                Colored = true;
            }


            //【, "Disabled": <bool>】【, "Wrapped": <bool>】
            bool Disabled = Obj.ItemBoolOr("Disabled", false);
            bool Wrapped = Obj.ItemBoolOr("Wrapped", false);

            if (oText && oText.IsTypeString())
            {
                return std::make_unique<IIC_PureText>(oText.GetString(), Col, Colored, Disabled, Wrapped);
            }
            else if (oKey && oFallback && oKey.IsTypeString() && oFallback.IsTypeString())
            {
                return std::make_unique<IIC_LocalizedText>(oKey.GetString(), oFallback.GetString(), Col, Colored, Disabled, Wrapped);
            }
            else
                return nullptr;

        }
        else if (typeStr == "SameLine")
            return std::make_unique<IIC_SameLine>();
        else if (typeStr == "NewLine")
            return std::make_unique<IIC_NewLine>();
        else if (typeStr == "Separator")
            return std::make_unique<IIC_Separator>();
        else if (typeStr == "InputText")
        {
            //IIC_InputText(int valueid, const std::string& InitialText, const std::string& hint)
            // {"Type": "InputText", "ValueID": <int> 【, "InitialValue": <string>】【, "Hint": <string>】}
            auto oValueID = Obj.GetObjectItem("ValueID");
            if (!oValueID || !oValueID.IsTypeNumber())
                return nullptr;
            int ValueID = oValueID.GetInt();

            auto InitValue = Obj.ItemStringOr("InitialValue", "");
            auto Hint = Obj.ItemStringOr("Hint", "");

            return std::make_unique<IIC_InputText>(Cont, ValueID, InitValue, Hint);
        }
        else if (typeStr == "Link")
        {
            //Another IIC_InputText
            //IIC_InputText(int valueid, const std::string& InitialText, const std::string& hint)
            // While Link shares the class but with preset initial status
            // {"Type": "Link", "ValueID": <int> 【, "InitialValue": <string>】【, "Hint": <string>】}
            auto oValueID = Obj.GetObjectItem("ValueID");
            if (!oValueID || !oValueID.IsTypeNumber())
                return nullptr;
            int ValueID = oValueID.GetInt();

            auto InitValue = Obj.ItemStringOr("InitialValue", "");
            auto Hint = Obj.ItemStringOr("Hint", "");

            auto q = std::make_unique<IIC_InputText>(Cont, ValueID, InitValue, Hint);

            if (q)q->InitialStatus.InputMethod = IICStatus::Link;

            return q;
        }
        else if (typeStr == "MultipleChoice")
        {
            //IIC_MultipleChoice(IBB_ValueContainer& Cont, int valueid, const std::string& InitialText, const std::string& hint, bool sameline, const std::unordered_map<std::string, IICDescStr>& options, const std::vector<std::string>& OptionOrder);
            // {"Type": "MultipleChoice", "ValueID": <int>, "InitialValue": <string>, "Options": { <AllowedValue1>: <DisplayName1>, <AllowedValue2>: <DisplayName2>, ... } 【, "Hint": <string>】【, "SameLine": <bool>】}

            auto oValueID = Obj.GetObjectItem("ValueID");
            if (!oValueID || !oValueID.IsTypeNumber())
                return nullptr;
            int ValueID = oValueID.GetInt();

            auto InitValue = Obj.ItemStringOr("InitialValue", "");
            auto Options = Obj.ItemMapObjectOr("Options") |
                std::views::transform([](auto&& p) {return std::make_pair(p.first, IICDescStr::Load(p.second)); }) |
                std::ranges::to<std::unordered_map<std::string, IICDescStr>>();
            auto SameLine = Obj.ItemBoolOr("SameLine", false);
            auto OptionOrder = Obj.ItemArrayKeyOr("Options");

            return std::make_unique<IIC_MultipleChoice>(Cont, ValueID, InitValue, SameLine, Options, OptionOrder);
        }
        else if (typeStr == "EnumCombo")
        {
            //IIC_EnumCombo(int valueid, const std::string& InitialValue, const std::string& hint, const std::unordered_map<std::string, std::string>& options)
            // {"Type": "EnumCombo", "ValueID": <int>, "InitialValue": <string>, "Options": { <AllowedValue1>: <DisplayName1>, <AllowedValue2>: <DisplayName2>, ... } 【, "Hint": <string>】}

            auto oValueID = Obj.GetObjectItem("ValueID");
            if (!oValueID || !oValueID.IsTypeNumber())
                return nullptr;
            int ValueID = oValueID.GetInt();

            auto InitValue = Obj.ItemStringOr("InitialValue", "");
            auto Hint = Obj.ItemStringOr("Hint", "");
            auto Options = Obj.ItemMapObjectOr("Options") |
                std::views::transform([](auto&& p) {return std::make_pair(p.first, IICDescStr::Load(p.second)); }) |
                std::ranges::to<std::unordered_map<std::string, IICDescStr>>();
            auto OptionOrder = Obj.ItemArrayKeyOr("Options");

            return std::make_unique<IIC_EnumCombo>(Cont, ValueID, InitValue, Hint, Options, OptionOrder);
        }
        else if (typeStr == "EnumRadio")
        {
            //IIC_EnumRadio(int valueid, const std::string& InitialValue, const std::unordered_map<std::string, std::string>& options, bool sameline)
            // {"Type": "EnumRadio", "ValueID": <int>, "InitialValue": <string>, "Options": { <AllowedValue1>: <DisplayName1>, <AllowedValue2>: <DisplayName2>, ... } 【, "Hint": <string>】【, "SameLine": <bool>】}

            auto oValueID = Obj.GetObjectItem("ValueID");
            if (!oValueID || !oValueID.IsTypeNumber())
                return nullptr;
            int ValueID = oValueID.GetInt();

            auto InitValue = Obj.ItemStringOr("InitialValue", "");
            auto Options = Obj.ItemMapObjectOr("Options") |
                std::views::transform([](auto&& p) {return std::make_pair(p.first, IICDescStr::Load(p.second)); }) |
                std::ranges::to<std::unordered_map<std::string, IICDescStr>>();
            auto SameLine = Obj.ItemBoolOr("SameLine", false);
            auto OptionOrder = Obj.ItemArrayKeyOr("Options");

            return std::make_unique<IIC_EnumRadio>(Cont, ValueID, InitValue, Options, SameLine, OptionOrder);

        }
        else if (typeStr == "Bool")
        {
            //IIC_Bool(int valueid, bool InitialValue, StrBoolType fmt, const std::string& hint)
            // {"Type": "Bool", "ValueID": <int>, "InitialValue": <bool> 【, "Fmt": <string>】【, "Hint": <string>】}

            auto oValueID = Obj.GetObjectItem("ValueID");
            if (!oValueID || !oValueID.IsTypeNumber())
                return nullptr;
            int ValueID = oValueID.GetInt();

            auto InitValue = Obj.ItemBoolOr("InitialValue", false);
            auto Hint = Obj.ItemStringOr("Hint", "");

            auto oFmt = Obj.GetObjectItem("Fmt");
            StrBoolType fmt = StrBoolTypeFromJSON(oFmt, StrBoolType::Str_yes_no);

            return std::make_unique<IIC_Bool>(Cont, ValueID, InitValue, fmt, Hint);

        }
        else if (typeStr == "ColorPanel")
        {
            return std::make_unique<IIC_ColorPanel>();
        }
        else if (typeStr == "InputInt")
        {
            //IIC_InputInt(int valueid, int InitialValue, int min, int max, const std::string& hint)
            // {"Type": "InputInt", "ValueID": <int>, "InitialValue": <int>, "Min": <int>, "Max": <int> 【, "Hint": <string>】}

            auto oValueID = Obj.GetObjectItem("ValueID");
            if (!oValueID || !oValueID.IsTypeNumber())
                return nullptr;
            int ValueID = oValueID.GetInt();

            auto oMin = Obj.GetObjectItem("Min");
            auto oMax = Obj.GetObjectItem("Max");
            int Min, Max;

            if (!oMin || !oMin.IsTypeNumber())Min = -INT_MAX;
            else Min = oMin.GetInt();

            if (!oMax || !oMax.IsTypeNumber())Max = INT_MAX;
            else Max = oMax.GetInt();

            auto InitValue = Obj.ItemIntOr("InitialValue", 0);
            auto Hint = Obj.ItemStringOr("Hint", "");

            return std::make_unique<IIC_InputInt>(Cont, ValueID, InitValue, Min, Max, Hint);

        }
        else if (typeStr == "SliderInt")
        {
            //IIC_SliderInt(int valueid, int InitialValue, int min, int max, const std::string& hint, const std::string& slidefmt, bool log)
            // {"Type": "SliderInt", "ValueID": <int>, "InitialValue": <int>, "Min": <int>, "Max": <int> 【, "Hint": <string>】【, "ValueFormat": <string>】【, "Logarithmic": <bool>】}

            auto oValueID = Obj.GetObjectItem("ValueID");
            if (!oValueID || !oValueID.IsTypeNumber())
                return nullptr;
            int ValueID = oValueID.GetInt();

            auto oMin = Obj.GetObjectItem("Min");
            auto oMax = Obj.GetObjectItem("Max");
            if (!oMin || !oMin.IsTypeNumber() || !oMax || !oMax.IsTypeNumber())
                return nullptr;
            int Min = oMin.GetInt();
            int Max = oMax.GetInt();

            auto InitValue = Obj.ItemIntOr("InitialValue", 0);
            auto Hint = Obj.ItemStringOr("Hint", "");
            auto SlideFmt = Obj.ItemStringOr("ValueFormat", "%d");
            auto Log = Obj.ItemBoolOr("Logarithmic", false);

            return std::make_unique<IIC_SliderInt>(Cont, ValueID, InitValue, Min, Max, Hint, SlideFmt, Log);
        }
        else return nullptr;
    }

    return nullptr;
}
IFCPtr InputFormComponentFactory::CreateFormatComponent(IBB_ValueContainer& Cont, const JsonObject& Obj)
{
    //IFC_PureText(const std::string& Text)
    // <string>
    //IFC_LocalizedText(const std::string& Key, const std::string& Fallback)
    // {"Key": <string>, "FallBack": <string>}
    //IFC_PrintF(int ValueID, const std::string& Format)
    // {"ValueID": <int>, "PrintF": <string>}
    //IFC_StdFormat(int ValueID, const std::string& Format)
    // {"ValueID": <int>, "StdFormat": <string>}
    //IFC_ToString(int ValueID)
    // {"ValueIDToString": <int>}

    ((void)Cont);

    if (!Obj)return nullptr;

    if (Obj.IsTypeString())
    {
        auto text = Obj.GetString();
        return std::make_unique<IFC_PureText>(text);
    }

    if (!Obj.IsTypeObject())return nullptr;

    auto oKey = Obj.GetObjectItem("Key");
    auto oFallback = Obj.GetObjectItem("FallBack");
    auto oValueID = Obj.GetObjectItem("ValueID");
    auto oPrintF = Obj.GetObjectItem("PrintF");
    auto oStdFormat = Obj.GetObjectItem("StdFormat");
    auto oValueIDToString = Obj.GetObjectItem("ValueIDToString");

    if (oKey && oFallback && oKey.IsTypeString() && oFallback.IsTypeString())
    {
        return std::make_unique<IFC_LocalizedText>(oKey.GetString(), oFallback.GetString());
    }
    else if (oValueID && oPrintF && oValueID.IsTypeNumber() && oPrintF.IsTypeString())
    {
        return std::make_unique<IFC_PrintF>(oValueID.GetInt(), oPrintF.GetString());
    }
    else if (oValueID && oStdFormat && oValueID.IsTypeNumber() && oStdFormat.IsTypeString())
    {
        return std::make_unique<IFC_StdFormat>(oValueID.GetInt(), oStdFormat.GetString());
    }
    else if (oValueIDToString && oValueIDToString.IsTypeNumber())
    {
        return std::make_unique<IFC_ToString>(oValueIDToString.GetInt());
    }
    else
        return nullptr;
}

IICPtr InputFormComponentFactory::GetParseErrorInputComponent(const std::string& Desc)
{
    auto piic = std::make_unique<IIC_Error>(Desc);
    return piic;
}
IFCPtr InputFormComponentFactory::GetParseErrorFormatComponent(const std::string& Desc)
{
    auto pifc = std::make_unique<IFC_Error>(Desc);
    return pifc;
}


// ======== LOADING ==========

const std::unordered_map<std::string, IICStatus> SpecialStatus{
    {"InitialLink", {IICStatus::Link}},
    {"InitialInput", {IICStatus::Input}}
};


bool IICStatus::Load(const JsonObject& Obj)
{
    if (Obj.IsTypeString())
    {
        auto cit = SpecialStatus.find(Obj.GetString());
        if (cit == SpecialStatus.end())
        {
            InputMethod = IICStatus::Input;
        }
    }
    else if (Obj.IsTypeObject())
    {
        auto MethodStr = Obj.ItemStringOr("Method", "Input");
        if (MethodStr == "Input")InputMethod = IICStatus::Input;
        else if (MethodStr == "Link")InputMethod = IICStatus::Link;
        else InputMethod = IICStatus::Input;
    }
    else return false;
    return true;
}


bool IBG_InputForm::Load(const JsonObject& Obj)
{
    InputComponents = std::make_shared<std::vector<IICPtr>>();
    FormatComponents = std::make_shared<std::vector<IFCPtr>>();
    ValueContainer.Clear();
    Dirty = true;
    FormattedString.clear();

    auto oInput = Obj.GetObjectItem("Input");
    if (!oInput.Available() || !oInput.IsTypeArray())
        return false;

    auto oFormat = Obj.GetObjectItem("Format");
    if (!oFormat.Available() || !oFormat.IsTypeArray())
        return false;

    bool Ret = true;

    for (auto& item : oInput.GetArrayObject())
    {
        auto comp = InputFormComponentFactory::CreateInputComponent(ValueContainer, item);
        if (comp)
            InputComponents->emplace_back(std::move(comp));
        else
        {
            Ret = false;
            InputComponents->emplace_back(
                InputFormComponentFactory::GetParseErrorInputComponent(
                    item.PrintUnformatted(
                    )));
        }

    }

    for (auto& item : oFormat.GetArrayObject())
    {
        auto comp = InputFormComponentFactory::CreateFormatComponent(ValueContainer, item);
        if (comp)
            FormatComponents->emplace_back(std::move(comp));
        else
        {
            Ret = false;
            InputComponents->emplace_back(
                InputFormComponentFactory::GetParseErrorInputComponent(
                    item.PrintUnformatted(
                    )));
        }
    }

    ResetState();

    return Ret;
}

bool IBG_InputType::Load(const JsonObject& Obj)
{
    auto TypeStr = Obj.ItemStringOr("Type", "Form");
    if (TypeStr == "Link")
        Type = IBG_InputType::Link;
    else if (TypeStr == "Form")
        Type = IBG_InputType::Form;
    else
        return false;

    //if (Type == IBG_InputType::Link)
    //    return true;

    auto oSidebar = Obj.GetObjectItem("Sidebar");
    if (!oSidebar)
        oSidebar = Obj.GetObjectItem("Form");
    if (!oSidebar)
        return false;

    auto oWorkSpace = Obj.GetObjectItem("WorkSpace");
    if (!oWorkSpace)
        oWorkSpace = Obj.GetObjectItem("Form");
    if (!oWorkSpace)
        return false;

    Sidebar.reset(new IBG_InputForm());
    WorkSpace.reset(new IBG_InputForm());

    bool Ret1 = Sidebar->Load(oSidebar);
    bool Ret2 = WorkSpace->Load(oWorkSpace);

    WorkSpace->EnableLinkNode();

    auto oFormatter = Obj.GetObjectItem("Formatter");
    if (oFormatter)
        KVFmt = KVFormatterFactory::LoadFromJson(oFormatter);
    else
        KVFmt = KVFormatter::Default();

    return Ret1 && Ret2;
}

IICDescStr IICDescStr::Load(JsonObject Obj)
{
    if (!Obj)return { "", "" };
    else if (Obj.IsTypeString())return { Obj.GetString(), "" };
    else if (Obj.IsTypeArray())
    {
        auto V = Obj.GetArrayString();
        if (V.size() >= 2)return { V[0], V[1] };
        else if (V.size() == 1)return { V[0], "" };
        else return { "", "" };
    }
    else return { "", "" };
}
