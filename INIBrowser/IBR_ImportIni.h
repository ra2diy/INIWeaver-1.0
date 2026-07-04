#pragma once
#include "FromEngine/Include.h"
#include "IBB_IniImport.h"
#include <functional>

#ifndef _TEXT_UTF8
#define _TEXT_UTF8
#endif

// ---------- 导入预览结果回调 ----------
struct IBR_ImportResult
{
    bool Confirmed{ false };                    // 用户是否确认
    ImportedIniFile File;                       // 最终的 section 列表（含用户修正后的类型）
    std::string INIType;
};

// ---------- 导入预览弹窗 ----------
namespace IBR_ImportPreview
{
    // 打开导入预览弹窗（异步，用户确认后通过 Callback 通知）
    // Callback 会在渲染线程被调用
    void Open(ImportedIniFile&& File, const std::function<void(const IBR_ImportResult&)>& Callback);

    // 渲染弹窗（在渲染循环中调用）
    void RenderUI();
}
