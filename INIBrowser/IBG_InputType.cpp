#include "IBG_InputType.h"
#include "imgui_internal.h"
#include "IBRender.h"
#include <fmt/scan.h>


bool InputTextStdString(const char* label, std::string& str,
    ImGuiInputTextFlags flags)
{
    // 定义内联回调
    auto resizeCallback = [](ImGuiInputTextCallbackData* data) -> int {
        if (data->EventFlag & ImGuiInputTextFlags_CallbackResize)
        {
            std::string* str = (std::string*)data->UserData;
            str->resize(data->BufTextLen);
            data->Buf = (char*)str->c_str();
        }
        return 0;
        };

    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputText(label,
        (char*)str.c_str(),
        str.capacity() + 1,
        flags,
        resizeCallback,
        (void*)&str);
}



namespace ImGui
{
    void PushOrderFront(ImGuiWindow* Window);
}

IBB_InputValue::IBB_InputValue(const IBB_InputValue& rhs)
    : Value(rhs.Value), Dirty(rhs.Dirty), LastUpdate(rhs.LastUpdate)
{ }

void IBB_InputValue::NeedsUpdate(IBB_ValueContainer& Cont, IBG_InputComponent& Source)
{
    Dirty = true;
    LastUpdate = { &Cont, &Source };
}

void IBB_InputValue::UpdateValue(const IBB_InputFormat& Format)
{
    Dirty = false;
    if (LastUpdate.Cont && LastUpdate.Source)
    {
        Value = LastUpdate.Source->FormatValue(*LastUpdate.Cont, Format);
    }
}

IBB_InputValue& IBB_ValueContainer::GetValue(int ValueID)
{
    return Values[ValueID];
}

void IBB_ValueContainer::Clear()
{
    Values.clear();
}

IBB_InputValue& IBG_InputForm::GetValue(int ValueID)
{
    return ValueContainer.GetValue(ValueID);
}

IBG_InputFormUIResult IBG_InputForm::RenderUI()
{
    bool Changed = false;
    bool Active = false;

    for (auto& IC : *InputComponents)
    {
        //存在不可缓存的状态
        if (!IC->CanProvideState(ValueContainer))
            GetFormattedString();//确保状态正确

        ImGui::PushID(IC.get());
        auto R = IC->RenderUI(ValueContainer);
        ImGui::PopID();
        Changed |= R.Updated;
        Active |= R.Active;
        if (R.Updated)
            GetValue(R.ValueID).NeedsUpdate(ValueContainer, *IC);
    }
    if (Changed) Dirty = true;
    return { Changed, Active };
}

const std::string& IBG_InputForm::GetFormattedString()
{
    if (Dirty)
    {
        FormattedString.clear();
        for (auto& FC : *FormatComponents)
        {
            const auto& VF = FC->GetFormat();

            switch (VF.Format.Type)
            {
            case IBB_InputFormat::UseFormat:
                FormattedString += VF.Format.String;
                break;
            case IBB_InputFormat::ToString:
            case IBB_InputFormat::PrintF:
            case IBB_InputFormat::StdFormat:
                auto& V = GetValue(VF.ValueID);
                if (V.Dirty)
                    V.UpdateValue(VF.Format);
                FormattedString += V.Value;
            }
        }
        Dirty = false;
    }
    return FormattedString;
}

void IBG_InputForm::ParseFromString(const std::string& Str)
{
    //Clear State
    ResetState();
    auto S = Str;

    //按照*FormatComponents的格式化组件来解析字符串
    //提取全部的固定文本并以此切割文本
    //对固定文本寻找并跳过VF.Format.String
    //如果存在子串为VF.Format.String，则跳到其后
    //否则不做任何动作
    /*
    ResetState()函数之后，ValueContainer的状态：
每个格式化组件需要请求的变量位置，存在一个IBB_InputValue，其中的StateValPtr可能是正确的类型，或者是空的(nullptr)。
如果非空，那么直接调用StateValPtr处理；如果空，那么跳过对这个变量的反序列化。
对一段连续可变文本组件和一段待反序列化的文本：
如果这段文本是空的，跳过对这段的parse；之后，
先从序列中忽略所有对应的变量位置状态为空的组件，再按照规则：
    如果中间夹了<=1个可变文本，则直接parse
    否则通过TryAccept分段parse
    */


    std::string remainingStr = Str;

    // 用于处理连续动态值组件
    std::vector<IBB_ValueFormat> pendingDynamicComponents;
    std::string pendingText;

    auto processPendingDynamicComponents = [&] {
        //如果这段文本是空的，跳过对这段的parse
        if (pendingText.empty())
            return;

        //从序列中忽略所有对应的变量位置状态为空的组件
        std::vector<IBB_ValueFormat> activeComponents;
        for (const auto& VFmt : pendingDynamicComponents)
        {
            auto& V = GetValue(VFmt.ValueID);
            if (V.StateValPtr)
                activeComponents.push_back(VFmt);
        }

        size_t numActive = activeComponents.size();

        if (numActive == 0)
            return;
        else if (numActive == 1)
        {
            auto& fmt = activeComponents[0];
            auto& V = GetValue(fmt.ValueID);
            V.StateValPtr->Parse(fmt.Format, pendingText);
            V.Value = pendingText;
        }
        else
        {
            //多于1个组件，通过TryAccept分段parse
            for (const auto& fmt : activeComponents)
            {
                auto& V = GetValue(fmt.ValueID);
                V.Value = V.StateValPtr->TryAccept(fmt.Format, pendingText);
            }
        }
    };

    auto CleanupDynamicComponents = [&] {
        pendingText.clear();
        pendingDynamicComponents.clear();
    };

    for (auto& FC : *FormatComponents) {
        const auto& VF = FC->GetFormat();

        if (VF.Format.Type == IBB_InputFormat::UseFormat) {
            // 固定文本组件
            auto pos = remainingStr.find(VF.Format.String);
            if (pos == std::string::npos)
            {
                // 未找到固定文本，跳过解析
                continue;
            }
            else
            {
                if (!pendingDynamicComponents.empty())
                {
                    pendingText = remainingStr.substr(0, pos);
                    processPendingDynamicComponents();
                    CleanupDynamicComponents();
                }

                // 移动剩余字符串位置
                remainingStr = remainingStr.substr(pos + VF.Format.String.length());
                pendingText.clear();
            }
        }
        else {
            // 动态值组件（ToString, PrintF, StdFormat）
            // 累积到待处理列表中
            pendingDynamicComponents.push_back(VF);
        }
    }

    if (!pendingDynamicComponents.empty())
    {
        pendingText = remainingStr;
        processPendingDynamicComponents();
        CleanupDynamicComponents();
    }
}

void IBG_InputForm::ResetState()
{
    ValueContainer.Clear();
    for (auto& IC : *InputComponents)
    {
        IC->ResetState(ValueContainer);
    }
    FormattedString.clear();
    Dirty = true;
}











// ========== IBB_ValueFormat ==========
IBB_ValueFormat::IBB_ValueFormat(IBB_InputFormat::_Type type, const std::string& String)
    : Format({type, String}), ValueID(-1)
{ }
IBB_ValueFormat::IBB_ValueFormat(IBB_InputFormat::_Type type, const std::string& String, int id)
    : Format({ type, String }), ValueID(id)
{ }





// ========== IFC_PureText ==========
IFC_PureText::IFC_PureText(const std::string& Text)
    : Format(IBB_InputFormat::UseFormat, Text) {
}

const IBB_ValueFormat& IFC_PureText::GetFormat() {
    return Format;
}


// ========== IFC_LocalizedText ==========
IFC_LocalizedText::IFC_LocalizedText(const std::string& Key, const std::string& Fallback)
    : Format(IBB_InputFormat::UseFormat, ""), Text(Key), FallbackText(Fallback) {
}

const IBB_ValueFormat& IFC_LocalizedText::GetFormat() {
    Format.Format.String = oloc(Text, FallbackText);
    return Format;
}


// ========== IFC_PrintF ==========
IFC_PrintF::IFC_PrintF(int ValueID, const std::string& fmt)
    : Format(IBB_InputFormat::PrintF, fmt, ValueID) {
}

const IBB_ValueFormat& IFC_PrintF::GetFormat() {
    return Format;
}

// ========== IFC_StdFormat ==========
IFC_StdFormat::IFC_StdFormat(int ValueID, const std::string& fmt)
    : Format(IBB_InputFormat::StdFormat, fmt, ValueID) {
}

const IBB_ValueFormat& IFC_StdFormat::GetFormat() {
    return Format;
}


// ========== IFC_ToString ==========
IFC_ToString::IFC_ToString(int ValueID)
    : Format(IBB_InputFormat::ToString, "", ValueID) {
}

const IBB_ValueFormat& IFC_ToString::GetFormat() {
    return Format;
}

// ========== IFC_Error ==========
IFC_Error::IFC_Error(const std::string& Text)
    : Format(IBB_InputFormat::UseFormat, Text), TextW(UTF8toUnicode(Text)) {
}

const IBB_ValueFormat& IFC_Error::GetFormat() {
    Format.Format.String = UnicodetoUTF8(std::vformat(locw("Error_FailedToParseComponent"), std::make_wformat_args(TextW)));
    return Format;
}





// ========== IIC_Formatter ==========
std::string IIC_Formatter(const std::string& Value, const IBB_InputFormat& Format)
{
    switch (Format.Type)
    {
    case IBB_InputFormat::UseFormat:
        return Format.String;
    case IBB_InputFormat::ToString:
        return Value;
    case IBB_InputFormat::PrintF:
    {
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), Format.String.c_str(), Value.c_str());
        return std::string(buffer);
    }
    case IBB_InputFormat::StdFormat:
    {
        try
        {
            return std::vformat(Format.String, std::make_format_args(Value));
        }
        catch (...)
        {
            return Value + "(FORMAT STRING ERROR)";
        }
    }
    }

    std::unreachable();
}

std::string IIC_Formatter(bool Value, const IBB_InputFormat& Format, StrBoolType FmtType)
{
    switch (Format.Type)
    {
    case IBB_InputFormat::UseFormat:
        return Format.String;
    case IBB_InputFormat::ToString:
        return StrBoolImpl(Value, FmtType);
    case IBB_InputFormat::PrintF:
    {
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), Format.String.c_str(), Value);
        return std::string(buffer);
    }
    case IBB_InputFormat::StdFormat:
    {
        try
        {
            return std::vformat(Format.String, std::make_format_args(Value));
        }
        catch (...)
        {
            return Value + "(FORMAT BOOL ERROR)";
        }
    }
    }

    std::unreachable();
}

std::string IIC_Formatter(int Value, const IBB_InputFormat& Format)
{
    switch (Format.Type)
    {
    case IBB_InputFormat::UseFormat:
        return Format.String;
    case IBB_InputFormat::ToString:
        return std::to_string(Value);
    case IBB_InputFormat::PrintF:
    {
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), Format.String.c_str(), Value);
        return std::string(buffer);
    }
    case IBB_InputFormat::StdFormat:
    {
        try
        {
            return std::vformat(Format.String, std::make_format_args(Value));
        }
        catch (...)
        {
            return Value + "(FORMAT INT ERROR)";
        }
    }
    }

    std::unreachable();
}

void IIC_Parser_String(const std::string& StrValue, const IBB_InputFormat& Format, std::string& OutValue)
{
    switch (Format.Type)
    {
    case IBB_InputFormat::UseFormat:
        OutValue = Format.String;
        break;
    case IBB_InputFormat::ToString:
        OutValue = StrValue;
        break;
    case IBB_InputFormat::PrintF:
    {
        char buffer[4096];
        sscanf_s(StrValue.c_str(), Format.String.c_str(), buffer);
        OutValue = std::string(buffer);
        break;
    }
    case IBB_InputFormat::StdFormat:
    {
        auto res = fmt::scan<std::string>(StrValue, Format.String);
        if (res) OutValue = res->value();
        else OutValue = StrValue;
        break;
    }
    }
}

bool Check_A_Format_String_For_Both_Scanf_And_Printf_With_One_Parameter_Is_For_A_String(const std::string& Format)
{
    //忽略%%，之后找到%的位置，看第一个格式符是不是s
    //从%开始，忽略非字母和长度符（h、l、L、j、z、t）寻找

    for (size_t i = 0; i < Format.size(); ++i)
    {
        if (Format[i] == '%')
        {
            if (i + 1 < Format.size() && Format[i + 1] == '%')
            {
                //是%%，跳过
                i++;
                continue;
            }
            //找到格式符
            size_t j = i + 1;
            //while 非字母和长度符（h、l、L、j、z、t）
            while (j < Format.size() && (
                !isalpha(j) || Format[j] == 'h' || Format[j] == 'l' || Format[j] == 'L' ||
                Format[j] == 'j' || Format[j] == 'z' || Format[j] == 't'
                ))
                j++;

            if (j < Format.size())
            {
                //找到了格式符
                if (Format[j] == 's')
                    return true;
                else
                    return false;
            }
        }
    }

    return false;
}

void IIC_Parser_Bool(const std::string& StrValue, const IBB_InputFormat& Format, bool& OutValue)
{
    switch (Format.Type)
    {
    case IBB_InputFormat::UseFormat:
        OutValue = IsTrueString(Format.String);
        break;
    case IBB_InputFormat::ToString:
        OutValue = IsTrueString(StrValue);
        break;
    case IBB_InputFormat::PrintF:
    {
        //决定使用int还是string解析
        //注意这个串本身也是printf用过的
        //所以不会出现scanf有而printf没有的格式

        if (Check_A_Format_String_For_Both_Scanf_And_Printf_With_One_Parameter_Is_For_A_String(Format.String))
        {
            char buffer[4096];
            sscanf_s(StrValue.c_str(), Format.String.c_str(), buffer);
            OutValue = IsTrueString(std::string(buffer));
        }
        else
        {
            int intVal = 0;
            sscanf_s(StrValue.c_str(), Format.String.c_str(), &intVal);
            OutValue = (intVal != 0);
        }

    }
    case IBB_InputFormat::StdFormat:
    {
        //TODO
        // res = fmt::scan<bool>(StrValue, Format.String);
        //if (res) OutValue = res->value();
        //else OutValue = IsTrueString(StrValue);
        break;
    }
    }

}
void IIC_Parser_Int(const std::string& StrValue, const IBB_InputFormat& Format, int& OutValue)
{
    try
    {
        switch (Format.Type)
        {
        case IBB_InputFormat::UseFormat:
            OutValue = std::stoi(Format.String);
            break;
        case IBB_InputFormat::ToString:
            OutValue = std::stoi(StrValue);
            break;
        case IBB_InputFormat::PrintF:
        {
            sscanf_s(StrValue.c_str(), Format.String.c_str(), &OutValue);
            break;
        }
        case IBB_InputFormat::StdFormat:
        {
            auto res = fmt::scan<int>(StrValue, Format.String);
            if (res) OutValue = res->value();
            else OutValue = std::stoi(StrValue);
            break;
        }
        }

    }
    catch (...)
    {
        OutValue = 0;
    }
}

//return 已解析的部分
std::string IIC_Accepter_String(const IBB_InputFormat& Format, std::string& Value, std::string& OutValue)
{
    switch (Format.Type)
    {
    case IBB_InputFormat::UseFormat:
        OutValue = Format.String;
        //Value 不变
        return "";
    case IBB_InputFormat::ToString:
        OutValue.clear();
        OutValue.swap(Value);
        return OutValue;
    case IBB_InputFormat::PrintF:
    {
        char buffer[4096];
        int n = -1;
        auto NewFmt = Format.String + "%n";
        sscanf_s(Value.c_str(), NewFmt.c_str(), buffer, &n);
        buffer[4095] = 0;
        OutValue = std::string(buffer);
        auto Ret = Value.substr(0, n);
        if(n > 0)Value = Value.substr(n);
        return Ret;
    }
    case IBB_InputFormat::StdFormat:
    {
        auto res = fmt::scan<std::string>(Value, Format.String);
        if (res)
        {
            OutValue = res->value();
            auto rg = res->range();
            std::string V(rg.begin(), rg.end());
            auto Ret = Value.substr(0, Value.size() - V.size());
            Value = V;
            return Ret;
        }
        else
        {
            OutValue.clear();
            OutValue.swap(Value);
            return OutValue;
        }
        break;
    }
    }

    std::unreachable();
}
std::string IIC_Accepter_Bool(const IBB_InputFormat& Format, std::string& Value, bool& OutValue)
{
    switch (Format.Type)
    {
    case IBB_InputFormat::UseFormat:
        OutValue = IsTrueString(Format.String);
        //Value 不变
        return "";
    case IBB_InputFormat::ToString:
    {
        auto res = fmt::scan<std::string>(Value, "{}");
        if (res)
        {
            std::string valStr = res->value();
            OutValue = IsTrueString(valStr);
            auto rg = res->range();
            std::string V(rg.begin(), rg.end());
            auto Ret = Value.substr(0, Value.size() - V.size());
            Value = V;
            return Ret;
        }
        return "";
    }
    case IBB_InputFormat::PrintF:
    {
        //决定使用int还是string解析
        //注意这个串本身也是printf用过的
        //所以不会出现scanf有而printf没有的格式

        int n = -1;
        auto NewFmt = Format.String + "%n";
        if (Check_A_Format_String_For_Both_Scanf_And_Printf_With_One_Parameter_Is_For_A_String(Format.String))
        {
            char buffer[4096];
            sscanf_s(Value.c_str(), NewFmt.c_str(), buffer);
            OutValue = IsTrueString(std::string(buffer));
            if (n > 0)
            {
                auto Ret = Value.substr(0, n);
                Value = Value.substr(n);
                return Ret;
            }
            else return "";
        }
        else
        {
            int intVal = 0;
            sscanf_s(Value.c_str(), NewFmt.c_str(), &intVal);
            OutValue = (intVal != 0);
            if (n > 0)
            {
                auto Ret = Value.substr(0, n);
                Value = Value.substr(n);
                return Ret;
            }
            else return "";
        }
    }
    case IBB_InputFormat::StdFormat:
    {
        //auto res = fmt::scan<bool>(Value, Format.String);
        //if (res)
        //{
        //    OutValue = res->value();
        //    auto rg = res->range();
        //    Value.assign(rg.begin(), rg.end());
        //}
        //else
        {
            auto res2 = fmt::scan<std::string>(Value, "{}");
            if (res2)
            {
                std::string valStr = res2->value();
                OutValue = IsTrueString(valStr);
                auto rg = res2->range();
                std::string V(rg.begin(), rg.end());
                auto Ret = Value.substr(0, Value.size() - V.size());
                Value = V;
                return Ret;
            }
            else
            {
                OutValue = false;
                // Value 不变
                return "";
            }
        }
        break;
    }
    }

    std::unreachable();
}
std::string IIC_Accepter_Int(const IBB_InputFormat& Format, std::string& Value, int& OutValue)
{
    try
    {
        switch (Format.Type)
        {
        case IBB_InputFormat::UseFormat:
            OutValue = std::stoi(Format.String);
            //Value 不变
            return "";
        case IBB_InputFormat::ToString:
        {
            size_t idx;
            OutValue = std::stoi(Value, &idx);
            auto Ret = Value.substr(0, idx);
            Value = Value.substr(idx);
            return Ret;
        }
        case IBB_InputFormat::PrintF:
        {
            int n = -1;
            auto NewFmt = Format.String + "%n";
            sscanf_s(Value.c_str(), NewFmt.c_str(), &OutValue);
            if (n > 0)
            {
                auto Ret = Value.substr(0, n);
                Value = Value.substr(n);
                return Ret;
            }
            else return "";
        }
        case IBB_InputFormat::StdFormat:
        {
            auto res = fmt::scan<int>(Value, Format.String);
            if (res)
            {
                OutValue = res->value();
                auto rg = res->range();
                std::string V(rg.begin(), rg.end());
                auto Ret = Value.substr(0, Value.size() - V.size());
                Value = V;
                return Ret;
            }
            else
            {
                size_t idx;
                OutValue = std::stoi(Value, &idx);
                auto Ret = Value.substr(0, idx);
                Value = Value.substr(idx);
                return Ret;
            }
            break;
        }
        }

    }
    catch (...)
    {
        OutValue = 0;
        // Value 不变
        return "";
    }

    std::unreachable();
}



// ========== IIS_String ============
IIS_String::IIS_String(const std::string& initialText)
    : Text(initialText) {
}

std::string IIS_String::Format(const IBB_InputFormat& Format) {
    return IIC_Formatter(Text, Format);
}

void IIS_String::Parse(const IBB_InputFormat& Format, const std::string& Val) {
    return IIC_Parser_String(Val, Format, Text);
}

std::string IIS_String::TryAccept(const IBB_InputFormat& Format, std::string& Value) {
    return IIC_Accepter_String(Format, Value, Text);
}

IISPtr IIS_String::Duplicate() const {
    return std::make_unique<IIS_String>(*this);
}

// ========== IIS_Bool ============
IIS_Bool::IIS_Bool(bool initialValue, StrBoolType fmtType)
    : Value(initialValue), FmtType(fmtType) {
}

std::string IIS_Bool::Format(const IBB_InputFormat& Format) {
    return IIC_Formatter(Value, Format, FmtType);
}

void IIS_Bool::Parse(const IBB_InputFormat& Format, const std::string& Val) {
    return IIC_Parser_Bool(Val, Format, Value);
}

std::string IIS_Bool::TryAccept(const IBB_InputFormat& Format, std::string& Val) {
    return IIC_Accepter_Bool(Format, Val, Value);
}

IISPtr IIS_Bool::Duplicate() const {
    return std::make_unique<IIS_Bool>(*this);
}

// ========== IIS_Int ============
IIS_Int::IIS_Int(int initialValue)
    : Value(initialValue) {
}

std::string IIS_Int::Format(const IBB_InputFormat& Format) {
    return IIC_Formatter(Value, Format);
}

void IIS_Int::Parse(const IBB_InputFormat& Format, const std::string& Val) {
    return IIC_Parser_Int(Val, Format, Value);
}

std::string IIS_Int::TryAccept(const IBB_InputFormat& Format, std::string& Val) {
    return IIC_Accepter_Int(Format, Val, Value);
}

IISPtr IIS_Int::Duplicate() const {
    return std::make_unique<IIS_Int>(*this);
}

// ========== IIC_PureText ==========
IIC_PureText::IIC_PureText(const std::string& InitialText, ImColor color, bool colored, bool disabled, bool wrapped)
    : Text(InitialText), Color(color), Colored(colored), Disabled(disabled), Wrapped(wrapped) {
}

IBB_UpdateResult IIC_PureText::RenderUI(IBB_ValueContainer&) {

    if (Disabled)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    else if (Colored)
        ImGui::PushStyleColor(ImGuiCol_Text, Color.Value);

    if (Wrapped)
        ImGui::TextWrapped("%s", Text.c_str());
    else
        ImGui::Text("%s", Text.c_str());

    if (Disabled || Colored)
        ImGui::PopStyleColor();

    return { false, ImGui::IsItemActive(), -1};
}

std::string IIC_PureText::FormatValue(IBB_ValueContainer& , const IBB_InputFormat& Format) {
    return IIC_Formatter(Text, Format);
}

// ========== IIC_LocalizedText ==========
IIC_LocalizedText::IIC_LocalizedText(const std::string& key, const std::string& fallback, ImColor color, bool colored, bool disabled, bool wrapped)
    : Key(key), FallbackText(fallback), Color(color), Colored(colored), Disabled(disabled), Wrapped(wrapped) {
}

IBB_UpdateResult IIC_LocalizedText::RenderUI(IBB_ValueContainer&) {
    if (Disabled)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    else if (Colored)
        ImGui::PushStyleColor(ImGuiCol_Text, Color.Value);

    if (Wrapped)
        ImGui::TextWrapped("%s", olocc(Key, FallbackText));
    else
        ImGui::Text("%s", olocc(Key, FallbackText));

    if (Disabled || Colored)
        ImGui::PopStyleColor();

    return { false, ImGui::IsItemActive(), -1 };
}

std::string IIC_LocalizedText::FormatValue(IBB_ValueContainer& ,const IBB_InputFormat& Format) {
    return IIC_Formatter(oloc(Key, FallbackText), Format);
}

// ========= IIC_SameLine ==========
IBB_UpdateResult IIC_SameLine::RenderUI(IBB_ValueContainer&)
{
    ImGui::SameLine();
    return { false, false, -1 };
}

std::string IIC_SameLine::FormatValue(IBB_ValueContainer& ,const IBB_InputFormat& )
{
    return "";
}

// ======== IIC_NewLine ==========
IBB_UpdateResult IIC_NewLine::RenderUI(IBB_ValueContainer&)
{
    ImGui::NewLine();
    return { false, false, -1 };
}

std::string IIC_NewLine::FormatValue(IBB_ValueContainer& ,const IBB_InputFormat& )
{
    return "";
}

// ======== IIC_Separator ==========
IBB_UpdateResult IIC_Separator::RenderUI(IBB_ValueContainer&)
{
    ImGui::Separator();
    return { false, false, -1 };
}

std::string IIC_Separator::FormatValue(IBB_ValueContainer& ,const IBB_InputFormat& )
{
    return "";
}

// ========== IIC_InputText ==========

IIC_InputText::IIC_InputText(IBB_ValueContainer& Cont, int valueid, const std::string& InitialText, const std::string& hint)
    : Hint(hint), ValueID(valueid) {
    Hint += "##";
    Hint += RandStr(12);
    Cont.GetValue(ValueID).ResetState<IIS_String>(InitialText);
}

IBB_UpdateResult IIC_InputText::RenderUI(IBB_ValueContainer& Cont) {

    auto& Var = Cont.GetValue(ValueID);
    static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
    auto CurrentValue = Var.Dirty ? Var.StateValPtr->Format(Fmt) : Var.Value;

    auto Size = ImGui::CalcTextSize(Hint.c_str(), NULL, true);
    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX() - Size.x);
    auto Changed = InputTextStdString(Hint.c_str(), CurrentValue);

    if (Changed)
    {
        auto State = Var.StateValue<IIS_String>();
        if (State) State->Text = CurrentValue;
        else Var.ResetState<IIS_String>(CurrentValue);
    }

    return { Changed, ImGui::IsItemActive(), ValueID };
}

std::string IIC_InputText::FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    return Cont.GetValue(ValueID).StateValPtr->Format(Format);
}

void IIC_InputText::ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    auto& Val = Cont.GetValue(ValueID);
    Val.StateValPtr->Parse(Format, Val.Value);
}

bool IIC_InputText::CanProvideState(IBB_ValueContainer&) const
{
    return true;
}

void IIC_InputText::ResetState(IBB_ValueContainer& Cont) const
{
    Cont.GetValue(ValueID).ResetState<IIS_String>();
}

// ========== IIC_EnumCombo ==========
IIC_EnumCombo::IIC_EnumCombo(IBB_ValueContainer& Cont, int valueid, const std::string& InitialValue, const std::string& hint, const std::unordered_map<std::string, std::string>& options)
    : Options(options), Hint(hint), ValueID(valueid) {
    Hint += "##";
    Hint += RandStr(12);
    Cont.GetValue(ValueID).ResetState<IIS_String>(InitialValue);
}

IBB_UpdateResult IIC_EnumCombo::RenderUI(IBB_ValueContainer& Cont) {

    bool Changed = false;

    auto& Var = Cont.GetValue(ValueID);
    static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
    auto CurrentValue = Var.Dirty ? Var.StateValPtr->Format(Fmt) : Var.Value;

    auto Active = false;
    auto Size = ImGui::CalcTextSize(Hint.c_str(), NULL, true);
    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX() - Size.x);
    if (ImGui::BeginCombo(Hint.c_str(),
        (Options.contains(CurrentValue)? Options[CurrentValue] : CurrentValue).c_str()
    ))
    {
        Active = ImGui::IsItemActive();
        ImGui::PushOrderFront(ImGui::GetCurrentWindow());
        for (auto& [Key, DisplayName] : Options)
        {
            auto Equal = (CurrentValue == Key);
            if (ImGui::Selectable(DisplayName.c_str(), Equal))
            {
                if (!Equal)
                {
                    Changed = true;
                    CurrentValue = Key;
                }
            }
            Active |= ImGui::IsItemActive();
        }
        ImGui::EndCombo();
    }

    if (Changed)
    {
        auto State = Var.StateValue<IIS_String>();
        if (State) State->Text = CurrentValue;
        else Var.ResetState<IIS_String>(CurrentValue);
    }

    return { Changed, Active, ValueID };
}

std::string IIC_EnumCombo::FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    return Cont.GetValue(ValueID).StateValPtr->Format(Format);
}

void IIC_EnumCombo::ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    auto& Val = Cont.GetValue(ValueID);
    Val.StateValPtr->Parse(Format, Val.Value);
}

bool IIC_EnumCombo::CanProvideState(IBB_ValueContainer&) const
{
    return true;
}

void IIC_EnumCombo::ResetState(IBB_ValueContainer& Cont) const
{
    Cont.GetValue(ValueID).ResetState<IIS_String>();
}

// ========== IIC_EnumRadio ==========
IIC_EnumRadio::IIC_EnumRadio(IBB_ValueContainer& Cont, int valueid, const std::string& InitialValue, const std::unordered_map<std::string, std::string>& options, bool sameline)
    : Options(options), ValueID(valueid), Hint(RandStr(12)), SameLine(sameline){
    Cont.GetValue(ValueID).ResetState<IIS_String>(InitialValue);
}

IBB_UpdateResult IIC_EnumRadio::RenderUI(IBB_ValueContainer& Cont) {

    bool Changed = false;
    ImGui::PushID(Hint.c_str());

    auto& Var = Cont.GetValue(ValueID);
    static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
    auto CurrentValue = Var.Dirty ? Var.StateValPtr->Format(Fmt) : Var.Value;

    bool Active = false;

    for (auto& [Key, DisplayName] : Options)
    {
        auto Equal = (CurrentValue == Key);
        if (ImGui::RadioButton(DisplayName.c_str(), Equal))
        {
            if (!Equal)
            {
                Changed = true;
                CurrentValue = Key;
            }
        }
        Active |= ImGui::IsItemActive();
        if(SameLine)ImGui::SameLine();
    }

    ImGui::PopID();

    if (Changed)
    {
        auto State = Var.StateValue<IIS_String>();
        if (State) State->Text = CurrentValue;
        else Var.ResetState<IIS_String>(CurrentValue);
    }

    return { Changed, Active, ValueID };
}

std::string IIC_EnumRadio::FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    return Cont.GetValue(ValueID).StateValPtr->Format(Format);
}

void IIC_EnumRadio::ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    auto& Val = Cont.GetValue(ValueID);
    Val.StateValPtr->Parse(Format, Val.Value);
}

bool IIC_EnumRadio::CanProvideState(IBB_ValueContainer&) const
{
    return true;
}

void IIC_EnumRadio::ResetState(IBB_ValueContainer& Cont) const
{
    Cont.GetValue(ValueID).ResetState<IIS_String>();
}

// ========== IIC_Bool ==========
IIC_Bool::IIC_Bool(IBB_ValueContainer& Cont, int valueid, bool InitialValue, StrBoolType fmt, const std::string& hint)
    : ValueID(valueid), Hint(hint), FmtType(fmt) {
    Hint += "##";
    Hint += RandStr(12);
    Cont.GetValue(ValueID).ResetState<IIS_Bool>(InitialValue, FmtType);
}

IBB_UpdateResult IIC_Bool::RenderUI(IBB_ValueContainer& Cont)
{
    auto& Var = Cont.GetValue(ValueID);
    static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
    auto State = Var.StateValue<IIS_Bool>();
    bool Val;
    if (State)Val = State->Value;
    else if (Var.Dirty)Val = IsTrueString(Var.StateValPtr->Format(Fmt));
    else Val = IsTrueString(Var.Value);

    bool Old = Val;

    ImGui::Checkbox(Hint.c_str(), &Val);

    bool Changed = (Old != Val);

    if (Changed)
    {
        if (State) State->Value = Val;
        else Var.ResetState<IIS_Bool>(Val, FmtType);
    }

    return { Changed, ImGui::IsItemActive(), ValueID };
}

std::string IIC_Bool::FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    return Cont.GetValue(ValueID).StateValPtr->Format(Format);
}

void IIC_Bool::ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    auto& Val = Cont.GetValue(ValueID);
    Val.StateValPtr->Parse(Format, Val.Value);
}

bool IIC_Bool::CanProvideState(IBB_ValueContainer&) const
{
    return true;
}

void IIC_Bool::ResetState(IBB_ValueContainer& Cont) const
{
    Cont.GetValue(ValueID).ResetState<IIS_Bool>();
}

// ========== IIC_InputInt ==========
IIC_InputInt::IIC_InputInt(IBB_ValueContainer& Cont, int valueid, int InitialValue, int min, int max, const std::string& hint)
    : Min(min), Max(max), Hint(hint), ValueID(valueid) {
    Hint += "##";
    Hint += RandStr(12);
    Cont.GetValue(ValueID).ResetState<IIS_Int>(InitialValue);
}

IBB_UpdateResult IIC_InputInt::RenderUI(IBB_ValueContainer& Cont) {

    auto& Var = Cont.GetValue(ValueID);
    auto State = Var.StateValue<IIS_Int>();
    int Val;
    if (State) Val = State->Value;
    else
    {
        try { Val = std::stoi(Var.Value); }
        catch (...) { Val = 0; }
    }

    auto Size = ImGui::CalcTextSize(Hint.c_str(), NULL, true);
    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX() - Size.x);
    auto Changed = ImGui::InputInt(Hint.c_str(), &Val, 0, 0);

    if (Changed)
    {
        if (State) State->Value = Val;
        else Var.ResetState<IIS_Int>(Val);
    }

    return { Changed, ImGui::IsItemActive(), ValueID };
}

std::string IIC_InputInt::FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    return Cont.GetValue(ValueID).StateValPtr->Format(Format);
}

void IIC_InputInt::ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    auto& Val = Cont.GetValue(ValueID);
    Val.StateValPtr->Parse(Format, Val.Value);
}

bool IIC_InputInt::CanProvideState(IBB_ValueContainer& Cont) const
{
    return Cont.GetValue(ValueID).CorrectState<IIS_Int>();
}

void IIC_InputInt::ResetState(IBB_ValueContainer& Cont) const
{
    Cont.GetValue(ValueID).ResetState<IIS_Int>();
}

// ========== IIC_SliderInt ==========
IIC_SliderInt::IIC_SliderInt(IBB_ValueContainer& Cont, int valueid, int InitialValue, int min, int max, const std::string& hint, const std::string& slidefmt, bool log)
    : Min(min), Max(max), Hint(hint), SlideFormat(slidefmt), Logarithmic(log), ValueID(valueid) {
    Hint += "##";
    Hint += RandStr(12);
    Cont.GetValue(ValueID).ResetState<IIS_Int>(InitialValue);
}

IBB_UpdateResult IIC_SliderInt::RenderUI(IBB_ValueContainer& Cont) {

    auto& Var = Cont.GetValue(ValueID);
    auto State = Var.StateValue<IIS_Int>();
    int Val;
    if (State) Val = State->Value;
    else
    {
        try { Val = std::stoi(Var.Value); }
        catch (...) { Val = 0; }
    }

    auto Size = ImGui::CalcTextSize(Hint.c_str(), NULL, true);
    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX() - Size.x);
    auto Changed = ImGui::SliderInt(Hint.c_str(), &Val, Min, Max, SlideFormat.c_str(),
        Logarithmic ? ImGuiSliderFlags_Logarithmic : ImGuiSliderFlags_None);

    if (Changed)
    {
        if (State) State->Value = Val;
        else Var.ResetState<IIS_Int>(Val);
    }

    return { Changed, ImGui::IsItemActive(), ValueID };
}

std::string IIC_SliderInt::FormatValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    return Cont.GetValue(ValueID).StateValPtr->Format(Format);
}

void IIC_SliderInt::ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    auto& Val = Cont.GetValue(ValueID);
    Val.StateValPtr->Parse(Format, Val.Value);
}

bool IIC_SliderInt::CanProvideState(IBB_ValueContainer& Cont) const
{
    return Cont.GetValue(ValueID).CorrectState<IIS_Int>();
}

void IIC_SliderInt::ResetState(IBB_ValueContainer& Cont) const
{
    Cont.GetValue(ValueID).ResetState<IIS_Int>();
}

// ======== IIC_Error ==========
IIC_Error::IIC_Error(const std::string& Desc)
    : TextW(UTF8toUnicode(Desc)) {
}

IBB_UpdateResult IIC_Error::RenderUI(IBB_ValueContainer&) {
    ImGui::TextColored(IBR_Color::ErrorTextColor, "%s", UnicodetoUTF8(std::vformat(locw("Error_FailedToParseComponent"), std::make_wformat_args(TextW))).c_str());
    return { false, false, -1 };
}

std::string IIC_Error::FormatValue(IBB_ValueContainer& ,const IBB_InputFormat& ) {
    return UnicodetoUTF8(std::vformat(locw("Error_FailedToParseComponent"), std::make_wformat_args(TextW)));
}

// ======== InputFormComponentFactory ==========
StrBoolType StrBoolTypeFromString(const std::string& str, StrBoolType Default)
{
    /*
    Str_true_false,
    Str_True_False,
    Str_TRUE_FALSE,
    Str_yes_no,
    Str_Yes_No,
    Str_YES_NO,
    Str_t_f,
    Str_T_F,
    Str_y_n,
    Str_Y_N,
    Str_1_0
    */

    if (str == "true_false")return StrBoolType::Str_true_false;
    else if (str == "True_False")return StrBoolType::Str_True_False;
    else if (str == "TRUE_FALSE")return StrBoolType::Str_TRUE_FALSE;
    else if (str == "yes_no")return StrBoolType::Str_yes_no;
    else if (str == "Yes_No")return StrBoolType::Str_Yes_No;
    else if (str == "YES_NO")return StrBoolType::Str_YES_NO;
    else if (str == "t_f")return StrBoolType::Str_t_f;
    else if (str == "T_F")return StrBoolType::Str_T_F;
    else if (str == "y_n")return StrBoolType::Str_y_n;
    else if (str == "Y_N")return StrBoolType::Str_Y_N;
    else if (str == "1_0")return StrBoolType::Str_1_0;
    else return Default;
}

IICPtr InputFormComponentFactory::CreateInputComponent(IBB_ValueContainer& Cont, const JsonObject& Obj)
{
    //【】 is optional
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

    if (oKey && oFallback && oKey.IsTypeString() && oKey.IsTypeString())
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
            else if (oKey && oFallback && oKey.IsTypeString() && oKey.IsTypeString())
            {
                return std::make_unique<IIC_LocalizedText>(oKey.GetString(), oFallback.GetString(), Col, Colored, Disabled, Wrapped);
            }
            else
                return nullptr;

        }
        else if(typeStr == "SameLine")
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
            auto Options = Obj.ItemMapStringOr("Options");

            return std::make_unique<IIC_EnumCombo>(Cont, ValueID, InitValue, Hint, Options);
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
            auto Hint = Obj.ItemStringOr("Hint", "");
            auto Options = Obj.ItemMapStringOr("Options");
            auto SameLine = Obj.ItemBoolOr("SameLine", false);

            return std::make_unique<IIC_EnumRadio>(Cont, ValueID, InitValue, Options, SameLine);

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
            auto FmtString = Obj.ItemStringOr("Fmt", "yes_no");
            auto Hint = Obj.ItemStringOr("Hint", "");

            StrBoolType fmt = StrBoolTypeFromString(FmtString, StrBoolType::Str_yes_no);

            return std::make_unique<IIC_Bool>(Cont, ValueID, InitValue, fmt, Hint);

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

    if(oKey && oFallback && oKey.IsTypeString() && oFallback.IsTypeString())
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

IIFPtr IBG_InputForm::Duplicate() const
{
    auto pif = std::make_unique<IBG_InputForm>(*this);
    return pif;
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

    return Ret;
}

IBG_InputType::IBG_InputType(const IBG_InputType& rhs)
    : Type(rhs.Type), Sidebar(rhs.Sidebar->Duplicate()), WorkSpace(rhs.WorkSpace->Duplicate())
{ }

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
    if (!oSidebar.Available())
        oSidebar = Obj.GetObjectItem("Form");
    if (!oSidebar.Available())
        return false;

    auto oWorkSpace = Obj.GetObjectItem("WorkSpace");
    if (!oWorkSpace.Available())
        oWorkSpace = Obj.GetObjectItem("Form");
    if (!oWorkSpace.Available())
        return false;

    Sidebar.reset(new IBG_InputForm());
    WorkSpace.reset(new IBG_InputForm());

    bool Ret1 = Sidebar->Load(oSidebar);
    bool Ret2 = WorkSpace->Load(oWorkSpace);
    return Ret1 && Ret2;
}


