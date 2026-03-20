#include "IBG_InputType_Derived.h"
#include "IBB_CustomBool.h"
#include <ranges>

ImColor LoadColorFromJson(JsonObject Obj, bool& Colored);
ImColor LoadColorFromJson(JsonObject Obj, const ImColor& Default);


// ======== InputTypeFactory ==========

IICPtr InputTypeFactory::CreateInputComponent(IBB_ValueContainer& Cont, const JsonObject& Obj)
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

    auto oConstraint = Obj.GetObjectItem("Constraint");

    if (oConstraint)
    {
        piic->Constraint = CreateValueConstraint(oConstraint);
    }

    piic->Disabled = Obj.ItemBoolOr("Disabled", false);
    piic->UseNodeColorInFrame = Obj.ItemBoolOr("ColoredFrame", false);

    return piic;
}

IICPtr InputTypeFactory::CreateInputComponent_Special(IBB_ValueContainer& Cont, const JsonObject& Obj)
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
    //IIC_MultipleChoice(IBB_ValueContainer& Cont, int valueid, const std::string& InitialText, const std::string& hint, bool sameline, int MaxInOneLine, const std::unordered_map<std::string, IICDescStr>& options, const std::vector<std::string>& OptionOrder);
    // {"Type": "MultipleChoice", "ValueID": <int>, "InitialValue": <string>, "Options": { <AllowedValue1>: <DisplayName1>, <AllowedValue2>: <DisplayName2>, ... } 【, "Hint": <string>】【, "SameLine": <bool>】【, "MaxInOneLine": <int>】}
    //IIC_EnumCombo(int valueid, const std::string& InitialValue, const std::string& hint, const std::unordered_map<std::string, std::string>& options)
    // {"Type": "EnumCombo", "ValueID": <int>, "InitialValue": <string>, "Options": { <AllowedValue1>: <DisplayName1>, <AllowedValue2>: <DisplayName2>, ... } 【, "Hint": <string>】}
    //IIC_EnumRadio(int valueid, const std::string& InitialValue, const std::unordered_map<std::string, std::string>& options, bool sameline, int MaxInOneLine)
    // {"Type": "EnumRadio", "ValueID": <int>, "InitialValue": <string>, "Options": { <AllowedValue1>: <DisplayName1>, <AllowedValue2>: <DisplayName2>, ... } 【, "Hint": <string>】【, "SameLine": <bool>】【, "MaxInOneLine": <int>】}
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
            Col = LoadColorFromJson(oColor, Colored);

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
            auto oHint = Obj.GetObjectItem("Hint");
            IICDescStr Hint = IICDescStr::Load(oHint);

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
            auto oHint = Obj.GetObjectItem("Hint");
            IICDescStr Hint = IICDescStr::Load(oHint);

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
            auto MaxInOneLine = Obj.ItemIntOr("MaxInOneLine", -1);
            auto OptionOrder = Obj.ItemArrayKeyOr("Options");
            auto Delim = Obj.ItemStringOr("Delim", ",");
            auto oHint = Obj.GetObjectItem("Hint");
            IICDescStr Hint = IICDescStr::Load(oHint);

            return std::make_unique<IIC_MultipleChoice>(Cont, ValueID, InitValue, Hint, Delim, SameLine, MaxInOneLine, Options, OptionOrder);
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
            auto oHint = Obj.GetObjectItem("Hint");
            IICDescStr Hint = IICDescStr::Load(oHint);
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
            auto MaxInOneLine = Obj.ItemIntOr("MaxInOneLine", -1);
            auto OptionOrder = Obj.ItemArrayKeyOr("Options");

            return std::make_unique<IIC_EnumRadio>(Cont, ValueID, InitValue, Options, SameLine, MaxInOneLine, OptionOrder);

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
            auto oHint = Obj.GetObjectItem("Hint");
            IICDescStr Hint = IICDescStr::Load(oHint);

            auto oFmt = Obj.GetObjectItem("Fmt");
            StrBoolType fmt = StrBoolTypeFromJSON(oFmt, StrBoolType::Str_yes_no);

            return std::make_unique<IIC_Bool>(Cont, ValueID, InitValue, fmt, Hint);

        }
        else if (typeStr == "ColorPanel")
        {
            //IIC_ColorPanel(IBB_ValueContainer& Cont, int valueid, const std::string& InitialValue, const IICDescStr& hint, ValueMode vm, FormatMode fm);
            // {"Type": "ColorPanel", "ValueID": <int>, "InitialValue": <string>【, "ValueMode": <string>】【, "FormatMode": <string>】【, "Hint": <Desc>】}

            auto oValueID = Obj.GetObjectItem("ValueID");
            if (!oValueID || !oValueID.IsTypeNumber())
                return nullptr;
            int ValueID = oValueID.GetInt();

            auto InitValue = Obj.ItemStringOr("InitialValue", "");
            auto oHint = Obj.GetObjectItem("Hint");
            IICDescStr Hint = IICDescStr::Load(oHint);

            auto VModeStr = Obj.ItemStringOr("ValueMode", "RGB");
            auto FModeStr = Obj.ItemStringOr("FormatMode", "Int");
            IIC_ColorPanel::ValueMode VMode;
            IIC_ColorPanel::FormatMode FMode;

            if (VModeStr == "RGB") VMode = IIC_ColorPanel::RGB;
            else if (VModeStr == "BGR") VMode = IIC_ColorPanel::BGR;
            else if (VModeStr == "HSV") VMode = IIC_ColorPanel::HSV;
            else VMode = IIC_ColorPanel::RGB;

            if (FModeStr == "Int") FMode = IIC_ColorPanel::Int;
            else if (FModeStr == "Float") FMode = IIC_ColorPanel::Float;
            else if (FModeStr == "Hex") FMode = IIC_ColorPanel::Hex;
            else if (FModeStr == "#Hex") FMode = IIC_ColorPanel::Hash_Hex;
            else FMode = IIC_ColorPanel::Int;

            return std::make_unique<IIC_ColorPanel>(Cont, ValueID, InitValue, Hint, VMode, FMode);
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
            auto oHint = Obj.GetObjectItem("Hint");
            IICDescStr Hint = IICDescStr::Load(oHint);

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
            auto oHint = Obj.GetObjectItem("Hint");
            IICDescStr Hint = IICDescStr::Load(oHint);
            auto SlideFmt = Obj.ItemStringOr("ValueFormat", "%d");
            auto Log = Obj.ItemBoolOr("Logarithmic", false);

            return std::make_unique<IIC_SliderInt>(Cont, ValueID, InitValue, Min, Max, Hint, SlideFmt, Log);
        }
        else return nullptr;
    }

    return nullptr;
}

IFCPtr InputTypeFactory::CreateFormatComponent(IBB_ValueContainer& Cont, const JsonObject& Obj)
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
    //IFC_Export_UseKey()
    // "<Export_Key>"
    //IFC_Export_UseKey()
    // {"LineIndexFrom" : <int>}
    //IFC_Export_LineMult(int Start)

    ((void)Cont);

    if (!Obj)return nullptr;

    if (Obj.IsTypeString())
    {
        auto text = Obj.GetString();
        if (text == "<Export_Key>")
            return std::make_unique<IFC_Export_UseKey>();
        return std::make_unique<IFC_PureText>(text);
    }

    if (!Obj.IsTypeObject())return nullptr;

    auto oKey = Obj.GetObjectItem("Key");
    auto oFallback = Obj.GetObjectItem("FallBack");
    auto oValueID = Obj.GetObjectItem("ValueID");
    auto oPrintF = Obj.GetObjectItem("PrintF");
    auto oStdFormat = Obj.GetObjectItem("StdFormat");
    auto oValueIDToString = Obj.GetObjectItem("ValueIDToString");
    auto oRandStr = Obj.GetObjectItem("RandomString");
    auto oLineIndexFrom = Obj.GetObjectItem("LineIndexFrom");
    auto oCollectID = Obj.GetObjectItem("CollectID");
    auto oCollectFormat = Obj.GetObjectItem("CollectFormat");
    auto oFromKey = Obj.GetObjectItem("FromKey");
    auto oNonEmpty = Obj.GetObjectItem("NonEmpty");

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
    else if (oRandStr && oRandStr.IsTypeNumber())
    {
        auto ID = oValueID ? oValueID.GetInt() : -1;
        return std::make_unique<IFC_Export_RandStr>(oRandStr.GetInt(), ID);
    }
    else if (oLineIndexFrom && oLineIndexFrom.IsTypeNumber())
    {
        return std::make_unique<IFC_Export_LineMult>(oLineIndexFrom.GetInt());
    }
    else if (oCollectID && oCollectID.IsTypeNumber())
    {
        std::optional<StrPoolID> FromLine;
        FromLine = oFromKey && oFromKey.IsTypeString() ? std::make_optional(NewPoolStr(oFromKey.GetString())) : std::nullopt;
        bool NonEmpty = oNonEmpty && oNonEmpty.IsTypeBool() ? oNonEmpty.GetBool() : false;
        return std::make_unique<IFC_Export_Collection_ID>(oCollectID.GetInt(), FromLine, NonEmpty);
    }
    else if (oCollectFormat && oCollectFormat.IsTypeArray())
    {
        bool HasError;
        auto pifcv = CreateFormatComponentVector(Cont, oCollectFormat, HasError);
        if (HasError)return nullptr;

        std::optional<StrPoolID> FromLine;
        FromLine = oFromKey && oFromKey.IsTypeString() ? std::make_optional(NewPoolStr(oFromKey.GetString())) : std::nullopt;
        bool NonEmpty = oNonEmpty && oNonEmpty.IsTypeBool() ? oNonEmpty.GetBool() : false;
        return std::make_unique<IFC_Export_Collection_IFCV>(pifcv, FromLine, NonEmpty);
    }
    else if (oFromKey && oFromKey.IsTypeString())
    {
        std::optional<StrPoolID> FromLine;
        FromLine = NewPoolStr(oFromKey.GetString());
        bool NonEmpty = oNonEmpty && oNonEmpty.IsTypeBool() ? oNonEmpty.GetBool() : false;
        return std::make_unique<IFC_Export_Collection_Value>(FromLine, NonEmpty);
    }
    else
        return nullptr;
}

IICPtr InputTypeFactory::GetParseErrorInputComponent(const std::string& Desc)
{
    auto piic = std::make_unique<IIC_Error>(Desc);
    return piic;
}
IFCPtr InputTypeFactory::GetParseErrorFormatComponent(const std::string& Desc)
{
    auto pifc = std::make_unique<IFC_Error>(Desc);
    return pifc;
}

IICVPtr InputTypeFactory::CreateInputComponentVector(IBB_ValueContainer& Cont, const JsonObject& Obj, bool& HasError)
{
    auto InputComponents = std::make_shared<std::vector<IICPtr>>();
    HasError = false;
    auto Objs = Obj.GetArrayObject();
    InputComponents->reserve(Objs.size());
    for (auto& item : Objs)
    {
        auto comp = InputTypeFactory::CreateInputComponent(Cont, item);
        if (comp)
            InputComponents->emplace_back(std::move(comp));
        else
        {
            HasError = true;
            InputComponents->emplace_back(
                InputTypeFactory::GetParseErrorInputComponent(
                    item.PrintUnformatted(
                    )));
        }

    }
    return InputComponents;
}

IFCVPtr InputTypeFactory::CreateFormatComponentVector(IBB_ValueContainer& Cont, const JsonObject& Obj, bool& HasError)
{
    auto FormatComponents = std::make_shared<std::vector<IFCPtr>>();
    HasError = false;
    auto Objs = Obj.GetArrayObject();
    FormatComponents->reserve(Objs.size());
    for (auto& item : Objs)
    {
        if (item.IsTypeObject() && item.HasItem("ValueList"))
        {
            auto List = item.ItemArrayInt("ValueList");
            for (auto& vid : List)
            {
                FormatComponents->push_back(std::make_unique<IFC_ToString>(vid));
                FormatComponents->push_back(std::make_unique<IFC_PureText>(","));
            }
            if(!List.empty())FormatComponents->pop_back();
            continue;
        }

        auto comp = InputTypeFactory::CreateFormatComponent(Cont, item);
        if (comp)
            FormatComponents->emplace_back(std::move(comp));
        else
        {
            HasError = true;
            FormatComponents->emplace_back(
                InputTypeFactory::GetParseErrorFormatComponent(
                    item.PrintUnformatted(
                    )));
        }
    }
    return FormatComponents;
}

ILFVPtr InputTypeFactory::CreateLineFormatVector(IBB_ValueContainer& Cont, const JsonObject& Obj, bool& HasError)
{
    auto LineFormats = std::make_shared<std::vector<IBB_LineFormat>>();
    HasError = false;
    auto Objs = Obj.GetArrayObject();
    LineFormats->reserve(Objs.size());
    for (auto& o : Objs)
    {
        auto oKey = o.GetObjectItem("Key");
        auto oValue = o.GetObjectItem("Value");
        if (oKey && oValue)
        {
            auto pifckey = CreateFormatComponentVector(Cont, oKey, HasError);
            auto pifcvalue = CreateFormatComponentVector(Cont, oValue, HasError);
            LineFormats->emplace_back(pifckey, pifcvalue);
        }
    }
    return LineFormats;
}

StrPoolID AnyTypeID();

IASOpt InputTypeFactory::CreateAcceptorSetting(IBB_ValueContainer& Cont, const JsonObject& Obj, bool& HasError)
{
    if (!Obj || !Obj.IsTypeObject())
    {
        HasError = true;
        return std::nullopt;
    }
    IIT_AcceptorSetting Setting;

    Setting.AcceptRegType = NewPoolStr(Obj.ItemStringOr("Type", ""));
    if (Setting.AcceptRegType == EmptyPoolStr)Setting.AcceptRegType = AnyTypeID();
    
    auto oFormat = Obj.GetObjectItem("ValueFormat");
    if(oFormat)Setting.AcceptFormats = InputTypeFactory::CreateFormatComponentVector(Cont, oFormat, HasError);
    else Setting.AcceptFormats = nullptr;

    ImColor Col;
    bool Colored;
    auto oColor = Obj.GetObjectItem("NodeColor");
    Col = LoadColorFromJson(oColor, Colored);
    Setting.NodeColor = Colored ? std::make_optional(Col) : std::nullopt;

    return Setting;
}

IBB_ValueConstraint InputTypeFactory::CreateValueConstraint(const JsonObject& Obj)
{
    IBB_ValueConstraint ivc;
    if(!Obj || !Obj.IsTypeObject())return ivc;
    for (auto& [k, v] : Obj.GetMapObject())
    {
        int ValueID;
        try { ValueID = std::stoi(k); }
        catch (...) { continue; }
        auto& Cond = ivc.Conditions[ValueID];
        Cond = CreateValueCond(v);
    }
    return ivc;
}

IBB_ValueCond InputTypeFactory::CreateValueCond(const JsonObject& Obj)
{
    IBB_ValueCond ivc;
    ivc.NeedsEmpty = false;
    ivc.Neg = false;
    if (!Obj || !Obj.IsTypeString())return ivc;
    auto Str = Obj.GetString();
    if (Str.empty())return ivc;
    if (Str.front() == '!') { ivc.Neg = true; Str.erase(Str.begin()); }
    if (Str == "<EMPTY>")ivc.NeedsEmpty = true;
    else ivc.Value = Str;
    return ivc;
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

    bool HasError = false;
    InputComponents = InputTypeFactory::CreateInputComponentVector(ValueContainer, oInput, HasError);
    FormatComponents = InputTypeFactory::CreateFormatComponentVector(ValueContainer, oFormat, HasError);
    if (HasError)Ret = false;

    RegenFormattedString();

    return Ret;
}

bool IBG_InputType::Load(const JsonObject& Obj)
{
    auto TypeStr = Obj.ItemStringOr("Type", "Form");
    if (TypeStr == "Link")
        Type = IBG_InputType::Link;
    else if (TypeStr == "Bool")
        Type = IBG_InputType::Bool;
    else if (TypeStr == "Form")
        Type = IBG_InputType::IIF;
    else
        return false;

    Multiple = Obj.ItemBoolOr("Multiple", false);
    NewLineAfterDesc = Obj.ItemBoolOr("NewLineAfterDesc", false);

    auto oLineIDFrom = Obj.GetObjectItem("LineIDFrom");
    if (oLineIDFrom && oLineIDFrom.IsTypeNumber())
    {
        ShowLineID = true;
        LineIDFrom = oLineIDFrom.GetInt();
    }
    else
    {
        ShowLineID = false;
        LineIDFrom = -1;
    }

    auto oForm = Obj.GetObjectItem("Form");
    if (!oForm)
        return false;

    Form.reset(new IBG_InputForm());

    bool Ret1 = Form->Load(oForm);

    bool HasError = false;
    auto oAcceptType = Obj.GetObjectItem("AcceptType");
    AcceptorSetting = InputTypeFactory::CreateAcceptorSetting(Form->GetValues(), oAcceptType, HasError);

    auto oFormatter = Obj.GetObjectItem("ExportMode");
    if (oFormatter)
        KVFmt = KVFormatterFactory::LoadFromJson(oFormatter, *this);
    else
        KVFmt = KVFormatter::Default();

    return Ret1;
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
