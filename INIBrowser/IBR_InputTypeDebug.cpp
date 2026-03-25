#include "IBR_Misc.h"
#include "IBR_Project.h"
#include "IBB_RegType.h"
#include "IBR_Components.h"

void ITD_Init();
void ITD_Column1();
void ITD_Column2();

extern int FontHeight;

/*

左：
原表单列表
新表单列表
>添加表单
原语句列表
新语句列表
>添加语句
预览模块
- >文字/模块
- >测试导出
右：

*/

enum class ITD_CurRightCol {
    None,
    LoadedForm,
    NewForm,
    LinePreview,
    MiscTools
}RightCol;

namespace IBB_DefaultRegType
{
    extern std::unordered_map<_TEXT_UTF8 std::string, IBG_InputType> InputTypes;

    void RTTPT(const std::wstring& wcs, const std::string& Info);
}

namespace ITDContext
{
    std::unordered_map<_TEXT_UTF8 std::string, IBG_InputType> NewInputTypes;
    std::unordered_map<_TEXT_UTF8 std::string, std::string> OrigInputStr;

    const IBG_InputType* CurrentInputType;
    std::string CurrentInputName;
    bool FromNew;
    IIFPtr CurrentInputForm;
    bool EditingJSON;
    std::shared_ptr<BufString> EditFormBuf;

    std::string NewFormName;
    std::shared_ptr<BufString> NewFormBuf;
}

void ITD_Init()
{
    RightCol = ITD_CurRightCol::None;
}

void ITD_Column1()
{
    if (ImGui::TreeNode(u8"已载入的输入类型"))
    {
        for(auto& [k,v] : IBB_DefaultRegType::InputTypes)
            if (ImGui::Selectable(k.c_str(), ITDContext::CurrentInputName == k && !ITDContext::FromNew))
            {
                ITDContext::CurrentInputType = &v;
                ITDContext::CurrentInputName = k;
                ITDContext::FromNew = false;
                ITDContext::CurrentInputForm.reset();
                RightCol = ITD_CurRightCol::LoadedForm;
            }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode(u8"临时的输入类型"))
    {
        for (auto& [k, v] : ITDContext::NewInputTypes)
            if (ImGui::Selectable(k.c_str(), ITDContext::CurrentInputName == k && ITDContext::FromNew))
            {
                ITDContext::CurrentInputType = &v;
                ITDContext::CurrentInputName = k;
                ITDContext::FromNew = true;
                ITDContext::CurrentInputForm.reset();
                RightCol = ITD_CurRightCol::LoadedForm;
            }
        ImGui::TreePop();
    }

    if (ImGui::Button(u8"新建输入类型"))
    {
        if (!ITDContext::NewFormBuf)
            ITDContext::NewFormBuf = std::make_shared<BufString>();
        RightCol = ITD_CurRightCol::NewForm;
    }

    ImGui::BeginDisabled();
    if (ImGui::Button(u8"预览语句"))
    {
        RightCol = ITD_CurRightCol::LinePreview;
    }

    
    if (ImGui::Button(u8"其他工具"))
    {
        RightCol = ITD_CurRightCol::MiscTools;
    }
    ImGui::EndDisabled();
}

void ITD_Column2_LoadedForm();
void ITD_Column2_NewForm();
void ITD_Column2_LinePreview();

void ITD_Column2()
{
    switch (RightCol)
    {
    case ITD_CurRightCol::None:
        break;
    case ITD_CurRightCol::LoadedForm:
        ITD_Column2_LoadedForm();
        break;
    case ITD_CurRightCol::NewForm:
        ITD_Column2_NewForm();
        break;
    case ITD_CurRightCol::LinePreview:
        ITD_Column2_LinePreview();
        break;
    case ITD_CurRightCol::MiscTools:
        break;
    default:
        break;
    }
}

void ITD_Column2_LoadedForm()
{
    /*
    const IBG_InputType* CurrentInputType;
    std::string CurrentInputName;
    bool FromNew;
    */
    auto CurInput = ITDContext::CurrentInputType;
    if (!CurInput)return;

    auto& FromNew = ITDContext::FromNew;
    ImGui::Text(u8"输入类型名称：%s", ITDContext::CurrentInputName.c_str());

    ImGui::Text(u8"表单预览");

    auto& iif = ITDContext::CurrentInputForm;
    if (!iif)
    {
        iif = CurInput->Form->Duplicate();
        iif->EnableLinkNode();
        ITDContext::EditingJSON = false;
    }

    iif->RenderUI(LinkNodeSetting());

    ImGui::Text(u8"当前文本：%s", iif->GetFormattedString().c_str());

    if (FromNew)
    {
        if (!ITDContext::EditingJSON)
        {
            if (ImGui::Button(u8"编辑JSON文本"))
            {
                ITDContext::EditingJSON = true;
                ITDContext::EditFormBuf = std::make_shared<BufString>();
                strcpy_s(ITDContext::EditFormBuf.get(), MAX_STRING_LENGTH, ITDContext::OrigInputStr[ITDContext::CurrentInputName].c_str());
            }
            if (ImGui::TreeNode(u8"JSON文本"))
            {
                if (ImGui::Button(u8"复制文本"))
                    ImGui::SetClipboardText(ITDContext::OrigInputStr[ITDContext::CurrentInputName].c_str());
                ImGui::TextWrapped(ITDContext::OrigInputStr[ITDContext::CurrentInputName].c_str());
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::Text(u8"更新类型json");
            ImGui::TextDisabled(u8"写成形如\"{...}\"的形式");
            ImGui::InputTextMultiline(u8"##NewFormInput", ITDContext::EditFormBuf.get(), MAX_STRING_LENGTH,
                ImVec2(0.0F, ImGui::GetWindowHeight() / 2.5F));

            bool Close = false;
            bool Update = false;
            if (ImGui::Button(u8"更新JSON"))
            {
                Update = true;
                Close = false;
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"取消编辑"))
            {
                Close = true;
                Update = false;
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"放弃修改"))
            {
                Close = true;
                Update = false;
            }

            if(Close)ITDContext::EditingJSON = false;
            if (Update)
            {
                JsonFile F;
                auto WFN = UTF8toUnicode(ITDContext::CurrentInputName);
                IBR_PopupManager::AddJsonParseErrorPopup(F.ParseTmpChecked(ITDContext::EditFormBuf.get(), loc("Error_JsonParseErrorPos"), nullptr),
                    UnicodetoUTF8(std::vformat(locw("Error_JsonSyntaxError"), std::make_wformat_args(WFN))));
                if (F.Available())
                {
                    if (!ITDContext::NewInputTypes[ITDContext::CurrentInputName].Load(F))
                    {
                        IBB_DefaultRegType::RTTPT(WFN, u8"JSON语义错误！");
                        ITDContext::NewInputTypes.erase(ITDContext::CurrentInputName);
                    }
                    else
                    {
                        IBR_HintManager::SetHint(u8"成功更新临时的输入类型！", HintStayTimeMillis);
                        ITDContext::OrigInputStr[ITDContext::CurrentInputName] = ITDContext::EditFormBuf.get();
                        CurInput = &ITDContext::NewInputTypes[ITDContext::CurrentInputName];
                        iif.reset();
                        iif = CurInput->Form->Duplicate();
                        iif->EnableLinkNode();
                    }
                }
            }
        }
        ImGui::NewLine(); ImGui::NewLine();
        if (ImGui::Button(u8"删除输入类型"))
        {
            IBR_PopupManager::SetCurrentPopup(std::move(
                IBR_PopupManager::Popup{}
                .CreateModal(loc("GUI_WaitingTitle"), false, []()
                    {
                        IBR_HintManager::SetHint(loc("GUI_ActionCanceled"), HintStayTimeMillis);
                    }
                )
                .SetFlag(
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoNav |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize)
                .SetSize({ FontHeight * 15.0F,FontHeight * 7.0F })
                .PushTextBack(u8"是否删除此输入类型？")
                .PushMsgBack([]()
                    {
                        if (ImGui::Button(locc("GUI_Yes"), { FontHeight * 6.0f,FontHeight * 2.0f }))
                        {
                            ITDContext::NewInputTypes.erase(ITDContext::CurrentInputName);
                            ITDContext::OrigInputStr.erase(ITDContext::CurrentInputName);
                            ITDContext::CurrentInputType = nullptr;
                            ITDContext::CurrentInputName = "";
                            ITDContext::FromNew = false;
                            RightCol = ITD_CurRightCol::None;
                            IBR_PopupManager::ClearPopupDelayed();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(locc("GUI_No"), { FontHeight * 6.0f,FontHeight * 2.0f }))
                        {
                            IBR_PopupManager::ClearPopupDelayed();
                        }
                    }))
            );
        }
    }
}

void ITD_Column2_NewForm()
{
    ImGui::Text(u8"输入类型名称");
    InputTextStdString(u8"##NewFormName", ITDContext::NewFormName);
    ImGui::Text(u8"输入类型json");
    ImGui::TextDisabled(u8"写成形如\"{...}\"的形式");
    ImGui::InputTextMultiline(u8"##NewFormInput", ITDContext::NewFormBuf.get(), MAX_STRING_LENGTH,
        ImVec2(0.0F, ImGui::GetWindowHeight() / 2.5F));
    bool SameName = ITDContext::NewInputTypes.contains(ITDContext::NewFormName);
    if (SameName)
    {
        ImGui::BeginDisabled();
        ImGui::Button(u8"添加");
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(IBR_Color::ErrorTextColor, u8"已存在同名输入类型");
    }
    else if (ITDContext::NewFormName.empty() || !*ITDContext::NewFormBuf.get())
    {
        ImGui::BeginDisabled();
        ImGui::Button(u8"添加");
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(IBR_Color::ErrorTextColor, u8"内容不可为空");
    }
    else
    {
        if (ImGui::Button(u8"添加"))
        {
            JsonFile F;
            auto WFN = UTF8toUnicode(ITDContext::NewFormName);
            IBR_PopupManager::AddJsonParseErrorPopup(F.ParseTmpChecked(ITDContext::NewFormBuf.get(), loc("Error_JsonParseErrorPos"), nullptr),
                UnicodetoUTF8(std::vformat(locw("Error_JsonSyntaxError"), std::make_wformat_args(WFN))));
            if (F.Available())
            {
                if (!ITDContext::NewInputTypes[ITDContext::NewFormName].Load(F))
                {
                    IBB_DefaultRegType::RTTPT(WFN, u8"JSON语义错误！");
                    ITDContext::NewInputTypes.erase(ITDContext::NewFormName);
                }
                else
                {
                    IBR_HintManager::SetHint(u8"成功创建临时的输入类型！", HintStayTimeMillis);
                    ITDContext::OrigInputStr[ITDContext::NewFormName] = ITDContext::NewFormBuf.get();

                    ITDContext::CurrentInputType = &ITDContext::NewInputTypes[ITDContext::NewFormName];
                    ITDContext::CurrentInputName = ITDContext::NewFormName;
                    ITDContext::FromNew = true;
                    ITDContext::CurrentInputForm.reset();
                    RightCol = ITD_CurRightCol::LoadedForm;
                }
            }
        }
    }
}

void ITD_Column2_LinePreview()
{

}
