#include "IBG_InputType_Derived.h"
#include "IBB_CustomBool.h"
#include "imgui_internal.h"
#include "IBRender.h"
#include "IBR_Combo.h"
#include <fmt/scan.h>
#include <ranges>

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
    : Value(rhs.Value), Dirty(rhs.Dirty), LastUpdate(rhs.LastUpdate),
    StateValPtr(rhs.StateValPtr ? rhs.StateValPtr->Duplicate() : nullptr)
{ }

void IBB_InputValue::NeedsUpdate(IBB_ValueContainer& Cont, IBG_InputComponent& Source)
{
    Dirty = true;
    LastUpdate = { &Cont, &Source };
}

void IBB_InputValue::UpdateValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    Dirty = false;
    if (LastUpdate.Cont && LastUpdate.Source)
    {
        Value = LastUpdate.Source->FormatValue(Cont, *this, Format);
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

void IBG_InputForm::CheckStatus()
{
    if (ComponentStatus.size() < InputComponents->size())
        for (size_t i = ComponentStatus.size(); i < InputComponents->size(); i++)
            ComponentStatus.push_back(InputComponents->at(i)->InitialStatus);

    bool TryUpdateLink = LinkNodeContext::CurSub;
    if (!TryUpdateLink)
    {

    }
    else if (LinkNodeContext::CurLineChangeCompStatus)
    {
        for (auto& CS : ComponentStatus)
        {
            if (CS.InputMethod == IICStatus::Link)CS.InputMethod = IICStatus::Input;
            else if (CS.InputMethod == IICStatus::Input)CS.InputMethod = IICStatus::Link;
        }
    }
    
}

#include "Global.h"

IBG_InputFormUIResult IBG_InputForm::RenderUI(const LinkNodeSetting& Default)
{
    bool Changed = false;
    bool Active = false;

    CheckStatus();

    bool TryUpdateLink = LinkNodeEnabled && LinkNodeContext::CurSub;
    std::unordered_set<uint64_t> UsedLinks;
    bool NeedsUpdateLink = IBR_LinkNode::UpdateLinkInitial();
    IICStatus StatusAlwaysInput;

    for (auto&& [CompIdx, IC, CSOrig] : std::views::zip(std::views::iota(0u), *InputComponents, ComponentStatus))
    {
        LinkNodeContext::CompIndex = CompIdx;
        //存在不可缓存的状态
        if (!IC->CanProvideState(ValueContainer))
            GetFormattedString();//确保状态正确

        StatusAlwaysInput.InputMethod = IICStatus::Input;
        auto& CS = LinkNodeEnabled ? CSOrig : StatusAlwaysInput;

        if (CS.InputMethod == IICStatus::Link && !IC->UseCustomSetting)
            IC->NodeSetting = Default;

        ImGui::PushID(IC.get());
        auto R = IC->RenderUI(ValueContainer, CS);
        ImGui::PopID();

        if (TryUpdateLink)
            IBR_LinkNode::UpdateLink(*LinkNodeContext::CurSub, LinkNodeContext::LineIndex, CompIdx, &UsedLinks);

        Changed |= R.Updated;
        Active |= R.Active;
        if (R.Updated)
        {
            GetValue(R.ValueID).NeedsUpdate(ValueContainer, *IC);
            if (
                !NeedsUpdateLink && (
                    IC->UseCustomSetting ||
                    IC->InitialStatus.InputMethod == IICStatus::Link ||
                    CS.InputMethod == IICStatus::Link
                    )
                )NeedsUpdateLink = true;
        }
    }

    LinkNodeContext::CompIndex = UINT_MAX;

    if (TryUpdateLink)
    {
        IBR_LinkNode::PushRestLinkForDraw(
            *LinkNodeContext::CurSub,
            UsedLinks,
            LinkNodeContext::LineIndex,
            IBR_LinkNode::DefaultCenterInWindow()
        );
    }

    if (Changed)
    {
        Dirty = true;
        IBG_Undo.SomethingShouldBeHere();
        if (TryUpdateLink && NeedsUpdateLink)
        {
            //auto& KeyName = LinkNodeContext::CurSub->Lines_ByName[LinkNodeContext::LineIndex];
            //auto Snapshot =
            //GetFormattedString();
            //LinkNodeContext::CurSub->Root->MergeLine(KeyName, Snapshot, IBB_IniMergeMode::Replace, true);
            //ParseFromString(Snapshot);
        }
    }
    return { Changed, Active };
}

const std::string& IBG_InputForm::RegenFormattedString()
{
    Dirty = true;
    auto TmpFmt = IBB_InputFormat{ IBB_InputFormat::ToString, "" };
    //强制更新所有值，确保状态正确
    //实际上没使用出来的值，所以直接入TmpFmt即可。
    for (auto& IC : *InputComponents)
        IC->FormatValue(ValueContainer, ValueContainer.GetValue(IC->GetCurrentTargetValueID()), TmpFmt);
    return GetFormattedString();
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
                    V.UpdateValue(ValueContainer, VF.Format);
                FormattedString += V.Value;
            }
        }
        Dirty = false;
    }
    return FormattedString;
}

void IBG_InputForm::ParseFromString(const std::string& Str)
{
    //Clear State & Save Status
    auto CC = std::move(ComponentStatus);
    ResetState();
    ComponentStatus = std::move(CC);

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
    ComponentStatus.clear();
    FormattedString.clear();
    Dirty = true;
    for (auto& IC : *InputComponents)
    {
        IC->ResetState(ValueContainer);
        ComponentStatus.push_back(IC->InitialStatus);
    }
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


// ========== IFC_Export_UseKey =========
IFC_Export_UseKey::IFC_Export_UseKey()
    : Format(IBB_InputFormat::ToString, "") {
}

const IBB_ValueFormat& IFC_Export_UseKey::GetFormat() {
    Format.Format.String = PoolStr(ExportContext::Key);
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

IBB_UpdateResult IIC_PureText::RenderUI(IBB_ValueContainer&, IICStatus&) {

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

std::string IIC_PureText::FormatValue(IBB_ValueContainer&, IBB_InputValue&, const IBB_InputFormat& Format) {
    return IIC_Formatter(Text, Format);
}

// ========== IIC_LocalizedText ==========
IIC_LocalizedText::IIC_LocalizedText(const std::string& key, const std::string& fallback, ImColor color, bool colored, bool disabled, bool wrapped)
    : Key(key), FallbackText(fallback), Color(color), Colored(colored), Disabled(disabled), Wrapped(wrapped) {
}

IBB_UpdateResult IIC_LocalizedText::RenderUI(IBB_ValueContainer&, IICStatus&) {
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

std::string IIC_LocalizedText::FormatValue(IBB_ValueContainer&, IBB_InputValue&, const IBB_InputFormat& Format) {
    return IIC_Formatter(oloc(Key, FallbackText), Format);
}

// ========= IIC_SameLine ==========
IBB_UpdateResult IIC_SameLine::RenderUI(IBB_ValueContainer&, IICStatus&)
{
    ImGui::SameLine();
    return { false, false, -1 };
}

std::string IIC_SameLine::FormatValue(IBB_ValueContainer&, IBB_InputValue&, const IBB_InputFormat&)
{
    return "";
}

// ======== IIC_NewLine ==========
IBB_UpdateResult IIC_NewLine::RenderUI(IBB_ValueContainer&, IICStatus&)
{
    ImGui::NewLine();
    return { false, false, -1 };
}

std::string IIC_NewLine::FormatValue(IBB_ValueContainer&, IBB_InputValue&, const IBB_InputFormat&)
{
    return "";
}

// ======== IIC_Separator ==========
IBB_UpdateResult IIC_Separator::RenderUI(IBB_ValueContainer&, IICStatus&)
{
    ImGui::Separator();
    return { false, false, -1 };
}

std::string IIC_Separator::FormatValue(IBB_ValueContainer&, IBB_InputValue&,  const IBB_InputFormat&)
{
    return "";
}

// ========== IIC_InputText ==========

IIC_InputText::IIC_InputText(IBB_ValueContainer& Cont, int valueid, const std::string& InitialText, const IICDescStr& hint)
    : Hint(hint), ValueID(valueid)
{
    Hint.Short += "##";
    Hint.Short += RandStr(12);
    auto& Val = Cont.GetValue(ValueID);
    Val.ResetState<IIS_String>(InitialText);
    Val.NeedsUpdate(Cont, *this);
}

IBB_UpdateResult RenderIICInputText(
    IIC_InputText* pIn,
    IICStatus& Status,
    std::string& InitialValue,
    const LinkNodeSetting& LinkNode,
    const std::function<IBB_UpdateResult(const std::string& NewValue, bool Active)>& ModifyFunc
)
{
    if (Status.InputMethod == IICStatus::Link)
    {
        IBB_UpdateResult def = { false, false, -1 };
        return IBR_LinkNode::RenderUI_Node(pIn->Hint.Short, pIn->Hint.Long, def, LinkNode, ModifyFunc);
    }
    else
    {
        //auto Size = ImGui::CalcTextSize(Hint.c_str(), NULL, true);
        //ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX() - Size.x);
        auto Changed = InputTextStdString(pIn->Hint.Short.c_str(), InitialValue);
        if (ImGui::IsItemHovered())IBR_ToolTip(pIn->Hint.Long);


        auto Active = ImGui::IsItemActive();
        if (Changed)return ModifyFunc(InitialValue, Active);
        else return { false, Active, -1 };

    }
}

IBB_UpdateResult IIC_InputText::RenderUI(IBB_ValueContainer& Cont, IICStatus& Status)
{
    auto& Var = Cont.GetValue(ValueID);
    auto mf = [&Var, vid = ValueID](const std::string& NewValue, bool Active) ->IBB_UpdateResult
        {
            auto State = Var.StateValue<IIS_String>();
            if (State) State->Text = NewValue;
            else Var.ResetState<IIS_String>(NewValue);
            return { true, Active, vid };
        };
    static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
    auto CurrentValue = Var.Dirty ? Var.StateValPtr->Format(Fmt) : Var.Value;

    return RenderIICInputText(this, Status, CurrentValue, NodeSetting, mf);
}

std::string IIC_InputText::FormatValue(IBB_ValueContainer& Cont, IBB_InputValue& Val, const IBB_InputFormat& Format)
{
    auto V = Val.StateValPtr->Format(Format);
    if (ExportContext::OnExport)
    {
        auto pSec = IBF_Inst_Project.Project.GetSecIndex(V, "").GetSec(IBF_Inst_Project.Project);
        if (pSec && pSec->SingleVal)
        {
            auto pLine = pSec->GetLineFromSubSecs(SingleValID());
            if (pLine)V = pLine->Data->GetStringForExport();
        }
        Val.NeedsUpdate(Cont, *this);
    }
    return V;
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

// ========== IIC_MultipleChoice ==========

IIC_MultipleChoice::IIC_MultipleChoice(IBB_ValueContainer& Cont, int valueid, const std::string& InitialText, const std::string& DelimStr, bool sameline, int maxInOneLine, const std::unordered_map<std::string, IICDescStr>& options, const std::vector<std::string>& order)
    : Hint(RandStr(12)), ValueID(valueid), SameLine(sameline), MaxInOneLine(maxInOneLine), Options(options), OptionOrder(order), Delim(DelimStr)
{
    auto& Val = Cont.GetValue(ValueID);
    Val.ResetState<IIS_String>(InitialText);
    Val.NeedsUpdate(Cont, *this);
}

IBB_UpdateResult IIC_MultipleChoice::RenderUI(IBB_ValueContainer& Cont, IICStatus&)
{
    auto& Var = Cont.GetValue(ValueID);
    static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
    auto CurrentValue = Var.Dirty ? Var.StateValPtr->Format(Fmt) : Var.Value;
    bool Changed = false;
    bool Active = false;


    auto SelectedValue = CurrentValue |
        std::views::split(Delim) |
        std::ranges::to<std::unordered_set<std::string>>();
    auto SelectedCond = OptionOrder |
        std::views::transform([&](auto& Val) { return (uint8_t)SelectedValue.contains(Val); }) |
        std::ranges::to<std::vector>();
    ImGui::PushID(Hint.c_str());

    for (auto&& [idx, Selected, Choice] : std::views::zip(std::views::iota(1), SelectedCond, OptionOrder))
    {
        auto& [DisplayName, DescLong] = Options[Choice];

        bool NewSel = Selected;
        auto& Desc = IBR_WorkSpace::ShowRegName ? Choice : DisplayName;

        ImGui::Checkbox(Desc.c_str(), &NewSel);
        if ((uint8_t)NewSel != Selected)
        {
            Changed = true;
            Selected = (uint8_t)NewSel;
        }

        Active |= ImGui::IsItemActive();
        if (ImGui::IsItemHovered() && !DescLong.empty())
            IBR_ToolTip(DescLong.c_str());
        if (SameLine)
        {
            if (MaxInOneLine <= 0 || (idx % MaxInOneLine))
                ImGui::SameLine();
        }
    }
    ImGui::PopID();


    if (Changed)
    {
        CurrentValue = std::views::zip(SelectedCond, OptionOrder) |
            std::views::filter([&](auto&& t) { return std::get<0>(t); }) |
            std::views::transform([&](auto&& t) { return std::get<1>(t); }) |
            std::views::join_with(Delim) |
            std::ranges::to<std::string>();

        auto State = Var.StateValue<IIS_String>();
        if (State) State->Text = CurrentValue;
        else Var.ResetState<IIS_String>(CurrentValue);
        return { true, Active, ValueID };
    }
    else return { false, Active, -1 };
}

std::string IIC_MultipleChoice::FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format)
{
    return Val.StateValPtr->Format(Format);
}

void IIC_MultipleChoice::ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    auto& Val = Cont.GetValue(ValueID);
    Val.StateValPtr->Parse(Format, Val.Value);
}

bool IIC_MultipleChoice::CanProvideState(IBB_ValueContainer&) const
{
    return true;
}

void IIC_MultipleChoice::ResetState(IBB_ValueContainer& Cont) const
{
    Cont.GetValue(ValueID).ResetState<IIS_String>();
}

// ========== IIC_EnumCombo ==========
IIC_EnumCombo::IIC_EnumCombo(IBB_ValueContainer& Cont, int valueid, const std::string& InitialValue, const std::string& hint, const std::unordered_map<std::string, IICDescStr>& options, const std::vector<std::string>& order)
    : Options(options), Hint(hint), ValueID(valueid), OptionOrder(order) {
    Hint += "##";
    Hint += RandStr(12);
    auto& Val = Cont.GetValue(ValueID);
    Val.ResetState<IIS_String>(InitialValue);
    Val.NeedsUpdate(Cont, *this);
}

IBB_UpdateResult IIC_EnumCombo::RenderUI(IBB_ValueContainer& Cont, IICStatus&) {

    //TODO STATUS

    bool Changed = false;

    auto& Var = Cont.GetValue(ValueID);
    static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
    auto CurrentValue = Var.Dirty ? Var.StateValPtr->Format(Fmt) : Var.Value;

    auto Active = false;
    auto Size = ImGui::CalcTextSize(Hint.c_str(), NULL, true);
    ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX() - Size.x);
    IBR_Combo(Hint.c_str(), (Options.contains(CurrentValue) ? Options[CurrentValue].Short : CurrentValue).c_str(), 0,
        [&] {
            Active = ImGui::IsItemActive();
            for (auto& Key : OptionOrder)
            {
                auto& [DisplayName, DescLong] = Options[Key];
                auto& Desc = IBR_WorkSpace::ShowRegName ? Key : DisplayName;
                auto Equal = (CurrentValue == Key);
                if (ImGui::Selectable(Desc.c_str(), Equal))
                {
                    if (!Equal)
                    {
                        Changed = true;
                        CurrentValue = Key;
                    }
                }
                if (ImGui::IsItemHovered() && !DescLong.empty())
                    IBR_ToolTip(DescLong.c_str());
                Active |= ImGui::IsItemActive();
            }
        }
    );

    if (Changed)
    {
        auto State = Var.StateValue<IIS_String>();
        if (State) State->Text = CurrentValue;
        else Var.ResetState<IIS_String>(CurrentValue);
    }

    return { Changed, Active, ValueID };
}

std::string IIC_EnumCombo::FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format)
{
    return Val.StateValPtr->Format(Format);
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
IIC_EnumRadio::IIC_EnumRadio(IBB_ValueContainer& Cont, int valueid, const std::string& InitialValue, const std::unordered_map<std::string, IICDescStr>& options, bool sameline, int maxInOneLine, const std::vector<std::string>& order)
    : Options(options), ValueID(valueid), Hint(RandStr(12)), SameLine(sameline), MaxInOneLine(maxInOneLine), OptionOrder(order) {
    auto& Val = Cont.GetValue(ValueID);
    Val.ResetState<IIS_String>(InitialValue);
    Val.NeedsUpdate(Cont, *this);
}

IBB_UpdateResult IIC_EnumRadio::RenderUI(IBB_ValueContainer& Cont, IICStatus&) {

    //TODO STATUS

    bool Changed = false;
    ImGui::PushID(Hint.c_str());

    auto& Var = Cont.GetValue(ValueID);
    static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
    auto CurrentValue = Var.Dirty ? Var.StateValPtr->Format(Fmt) : Var.Value;

    bool Active = false;

    for (auto&& [idx, Key] : std::views::zip(std::views::iota(1), OptionOrder))
    {
        auto& [DisplayName, DescLong] = Options[Key];
        auto& Desc = IBR_WorkSpace::ShowRegName ? Key : DisplayName;
        auto Equal = (CurrentValue == Key);
        if (ImGui::RadioButton(Desc.c_str(), Equal))
        {
            if (!Equal)
            {
                Changed = true;
                CurrentValue = Key;
            }
        }
        if (ImGui::IsItemHovered() && !DescLong.empty())
            IBR_ToolTip(DescLong.c_str());
        Active |= ImGui::IsItemActive();
        if (SameLine)
        {
            if (MaxInOneLine <= 0 || (idx % MaxInOneLine))
                ImGui::SameLine();
        }
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

std::string IIC_EnumRadio::FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format)
{
    return Val.StateValPtr->Format(Format);
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
IIC_Bool::IIC_Bool(IBB_ValueContainer& Cont, int valueid, bool InitialValue, StrBoolType fmt, const IICDescStr& hint)
    : ValueID(valueid), Hint(hint), FmtType(fmt) {
    Hint.Short += "##";
    Hint.Short += RandStr(12);
    auto& Val = Cont.GetValue(ValueID);
    Val.ResetState<IIS_Bool>(InitialValue, FmtType);
    Val.NeedsUpdate(Cont, *this);
}

void RenderIICBool(IIC_Bool* pBool, bool& Val)
{
    ImGui::Checkbox(pBool->Hint.Short.c_str(), &Val);
    if (ImGui::IsItemHovered())IBR_ToolTip(pBool->Hint.Long);
}

IBB_UpdateResult IIC_Bool::RenderUI(IBB_ValueContainer& Cont, IICStatus&)
{
    //TODO STATUS

    auto& Var = Cont.GetValue(ValueID);
    static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
    auto State = Var.StateValue<IIS_Bool>();
    bool Val;
    if (State)Val = State->Value;
    else if (Var.Dirty)Val = IsTrueString(Var.StateValPtr->Format(Fmt));
    else Val = IsTrueString(Var.Value);

    bool Old = Val;

    RenderIICBool(this, Val);

    bool Changed = (Old != Val);

    if (Changed)
    {
        if (State) State->Value = Val;
        else Var.ResetState<IIS_Bool>(Val, FmtType);
    }

    return { Changed, ImGui::IsItemActive(), ValueID };
}

std::string IIC_Bool::FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format)
{
    auto& Ptr = Val.StateValPtr;
    auto StateVal = Val.StateValue<IIS_Bool>();
    if (StateVal)StateVal->FmtType = FmtType;
    return Ptr->Format(Format);
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
IIC_InputInt::IIC_InputInt(IBB_ValueContainer& Cont, int valueid, int InitialValue, int min, int max, const IICDescStr& hint)
    : Min(min), Max(max), Hint(hint), ValueID(valueid) {
    Hint.Short += "##";
    Hint.Short += RandStr(12);
    auto& Val = Cont.GetValue(ValueID);
    Val.ResetState<IIS_Int>(InitialValue);
    Val.NeedsUpdate(Cont, *this);
}

IBB_UpdateResult IIC_InputInt::RenderUI(IBB_ValueContainer& Cont, IICStatus& Status)
{
    auto& Var = Cont.GetValue(ValueID);
    auto mf = [&Var, vid = ValueID](const std::string& NewValue, bool Active) ->IBB_UpdateResult
        {
            auto State = Var.StateValue<IIS_String>();
            if (State) State->Text = NewValue;
            else Var.ResetState<IIS_String>(NewValue);
            return { true, Active, vid };
        };

    if (Status.InputMethod == IICStatus::Link)
    {
        IBB_UpdateResult def = { false, false, -1 };
        return IBR_LinkNode::RenderUI_Node(Hint.Short, Hint.Long, def, NodeSetting, mf);
    }
    else
    {
        static const IBB_InputFormat Fmt = { IBB_InputFormat::ToString, "" };
        auto CurrentValue = Var.Dirty ? Var.StateValPtr->Format(Fmt) : Var.Value;

        //auto Size = ImGui::CalcTextSize(Hint.c_str(), NULL, true);
        //ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX() - Size.x);
        auto Changed = InputTextStdString(Hint.Short.c_str(), CurrentValue);
        if (ImGui::IsItemHovered())IBR_ToolTip(Hint.Long);
        if (Changed)
        {
            char* ss;
            int Val = strtol(CurrentValue.c_str(), &ss, 10);
            if (*ss == 0)
            {
                Val = std::clamp(Val, Min, Max);
                CurrentValue = std::to_string(Val);
            }
        }

        auto Active = ImGui::IsItemActive();
        if (Changed)return mf(CurrentValue, Active);
        else return { false, Active, -1 };

    }
}


std::string IIC_InputInt::FormatValue(IBB_ValueContainer& Cont, IBB_InputValue& Val, const IBB_InputFormat& Format)
{
    auto V = Val.StateValPtr->Format(Format);
    if (ExportContext::OnExport)
    {
        auto pSec = IBF_Inst_Project.Project.GetSecIndex(V, "").GetSec(IBF_Inst_Project.Project);
        if (pSec && pSec->SingleVal)
        {
            auto pLine = pSec->GetLineFromSubSecs(SingleValID());
            if (pLine)V = pLine->Data->GetStringForExport();
        }
        Val.NeedsUpdate(Cont, *this);
    }
    return V;
}

void IIC_InputInt::ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    auto& Val = Cont.GetValue(ValueID);
    Val.StateValPtr->Parse(Format, Val.Value);
}

bool IIC_InputInt::CanProvideState(IBB_ValueContainer&) const
{
    return true;
}

void IIC_InputInt::ResetState(IBB_ValueContainer& Cont) const
{
    Cont.GetValue(ValueID).ResetState<IIS_String>();
}

// ========== IIC_ColorPanel ==========

IBB_UpdateResult IIC_ColorPanel::RenderUI(IBB_ValueContainer& Cont, IICStatus& Status)
{
    const auto GetCol = [&](int v1, int v2, int v3) {
        switch (Mode)
        {
        case IIC_ColorPanel::RGB:
            return ImColor(v1, v2, v3);
            break;
        case IIC_ColorPanel::BGR:
            return ImColor(v3, v2, v1);
            break;
        case IIC_ColorPanel::HSV:
            return ImColor::HSV(float(v1) / 360.0f, float(v2) / 255.0f, float(v3) / 255.0f);
            break;
        default:
            break;
        }
        return ImColor{};
     };
    const auto GetVal = [&](int vid) {
        auto& Var = Cont.GetValue(vid);
        auto State = Var.StateValue<IIS_Int>();
        int Val;
        if (State) Val = State->Value;
        else
        {
            try { Val = std::stoi(Var.Value); }
            catch (...) { Val = 0; }
        }
        return Val;
    };
    int Val1 = GetVal(ValueID1);
    int Val2 = GetVal(ValueID1);
    int Val3 = GetVal(ValueID1);
    auto Col = GetCol(Val1, Val2, Val3);

    if (Status.InputMethod == IICStatus::Link)
    {
        if (ImGui::ColorButton(u8"阿巴", Col))
            Status.InputMethod = IICStatus::Input;

        return { false, ImGui::IsItemActive(), -1 };

    }
    else
    {
        ImGui::PushID(114514);
        if (ImGui::Button(u8"⇔"))Status.InputMethod = IICStatus::Link;
        ImGui::ColorEdit3(Hint.c_str(), &Col.Value.x);
        ImGui::PopID();
        ImGui::ColorPicker3(Hint.c_str(), &Col.Value.x);
    }

}

std::string IIC_ColorPanel::FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format)
{
    return Val.StateValPtr->Format(Format);
}

void IIC_ColorPanel::ParseValue(IBB_ValueContainer& Cont, const IBB_InputFormat& Format)
{
    auto& Val1 = Cont.GetValue(ValueID1);
    Val1.StateValPtr->Parse(Format, Val1.Value);
    auto& Val2 = Cont.GetValue(ValueID2);
    Val2.StateValPtr->Parse(Format, Val2.Value);
    auto& Val3 = Cont.GetValue(ValueID3);
    Val3.StateValPtr->Parse(Format, Val3.Value);
}

bool IIC_ColorPanel::CanProvideState(IBB_ValueContainer& Cont) const
{
    return
        Cont.GetValue(ValueID1).CorrectState<IIS_Int>() &&
        Cont.GetValue(ValueID2).CorrectState<IIS_Int>() &&
        Cont.GetValue(ValueID3).CorrectState<IIS_Int>();
}

void IIC_ColorPanel::ResetState(IBB_ValueContainer& Cont) const
{
    Cont.GetValue(ValueID1).ResetState<IIS_Int>();
    Cont.GetValue(ValueID2).ResetState<IIS_Int>();
    Cont.GetValue(ValueID3).ResetState<IIS_Int>();
}

// ========== IIC_SliderInt ==========
IIC_SliderInt::IIC_SliderInt(IBB_ValueContainer& Cont, int valueid, int InitialValue, int min, int max, const IICDescStr& hint, const std::string& slidefmt, bool log)
    : Min(min), Max(max), Hint(hint), SlideFormat(slidefmt), Logarithmic(log), ValueID(valueid) {
    Hint.Short += "##";
    Hint.Short += RandStr(12);
    auto& Val = Cont.GetValue(ValueID);
    Val.ResetState<IIS_Int>(InitialValue);
    Val.NeedsUpdate(Cont, *this);
}

IBB_UpdateResult IIC_SliderInt::RenderUI(IBB_ValueContainer& Cont, IICStatus&) {

    //TODO STATUS

    auto& Var = Cont.GetValue(ValueID);
    auto State = Var.StateValue<IIS_Int>();
    int Val;
    if (State) Val = State->Value;
    else
    {
        try { Val = std::stoi(Var.Value); }
        catch (...) { Val = 0; }
    }

    //auto Size = ImGui::CalcTextSize(Hint.c_str(), NULL, true);
    //ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX() - Size.x);
    auto Changed = ImGui::SliderInt(Hint.Short.c_str(), &Val, Min, Max, SlideFormat.c_str(),
        Logarithmic ? ImGuiSliderFlags_Logarithmic : ImGuiSliderFlags_None);
    if (ImGui::IsItemHovered())IBR_ToolTip(Hint.Long);

    if (Changed)
    {
        if (State) State->Value = Val;
        else Var.ResetState<IIS_Int>(Val);
    }

    return { Changed, ImGui::IsItemActive(), ValueID };
}

std::string IIC_SliderInt::FormatValue(IBB_ValueContainer&, IBB_InputValue& Val, const IBB_InputFormat& Format)
{
    return Val.StateValPtr->Format(Format);
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

IBB_UpdateResult IIC_Error::RenderUI(IBB_ValueContainer&, IICStatus&) {
    ImGui::TextColored(IBR_Color::ErrorTextColor, "%s", UnicodetoUTF8(std::vformat(locw("Error_FailedToParseComponent"), std::make_wformat_args(TextW))).c_str());
    return { false, false, -1 };
}

std::string IIC_Error::FormatValue(IBB_ValueContainer&, IBB_InputValue&, const IBB_InputFormat&) {
    return UnicodetoUTF8(std::vformat(locw("Error_FailedToParseComponent"), std::make_wformat_args(TextW)));
}


// ---------- DUPLICATION ----------

IBG_InputType::IBG_InputType(const IBG_InputType& rhs)
    : Type(rhs.Type), Form(rhs.Form->Duplicate())
{
}

IIFPtr IBG_InputForm::Duplicate() const
{
    auto pif = std::make_unique<IBG_InputForm>(*this);
    return pif;
}
