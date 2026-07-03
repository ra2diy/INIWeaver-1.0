#pragma once
#include "FromEngine/Include.h"
#include "cjson/cJSON.h"
#include "IBB_Components.h"
#include "IBB_RegType.h"
#include "IBB_Ini.h"
#include "IBB_ModuleAlt.h"

#ifndef _TEXT_UTF8
#define _TEXT_UTF8
#endif

// ---------- 匹配状态 ----------
enum class IniImportMatchStatus
{
    Matched,     // 精确匹配到注册表类型（注册表列表或直接名称匹配）
    LinkMatched, // 通过链接关系匹配（某个链接键的值指向此块）
    Unmatched    // 未能匹配
};

// ---------- 解析后的一节 ----------
struct ImportedIniSection
{
    std::string SectionName;                        // 原始 section 名（不含括号）
    std::vector<IniToken> KeyValues;       // 键值对列表
    IniImportMatchStatus MatchStatus{ IniImportMatchStatus::Unmatched };
    std::string MatchedRegType;                     // 匹配到的注册表类型名（空表示未匹配）

    // 布局结果
    ImVec2 EqPos{ 0.0F, 0.0F };
    ImVec2 EqSize{ 0.0F, 0.0F };

    // 是否是注册表列表块（如 [ArmorTypes]），导入时应跳过
    bool IsRegistryList{ false };

    // 用户是否勾选了该块（预览弹窗中可取消）
    bool Selected{ true };

    // 链接匹配来源描述（如 "SomeSection.Weapon"），仅 LinkMatched 时有意义
    std::string LinkMatchSource;

    // 继承自（如 [Etd7pDv9g9H4]:[CIV1] 时此值为 "CIV1"）
    std::string Inherit;

    // 用于链接检测的内部索引
    size_t Index{ 0 };
};

// ---------- 解析后的整个 INI 文件 ----------
struct ImportedIniFile
{
    std::wstring FilePath;                          // 原始文件路径
    std::vector<ImportedIniSection> Sections;       // 所有 section
};

// ---------- 导入选项（预留扩展点） ----------
struct IniImportOptions
{
    std::string TargetIniName;  // 目标 Ini 名，空字符串 = 默认 Ini
    // 未来可扩展: MatchStrategy, OverwriteMode 等
};

// ---------- 链接关系 ----------
struct IniImportLinkRelation
{
    size_t FromSectionIdx;
    size_t ToSectionIdx;
    std::string FromKey;
    std::string ToKey;
};

// ========== 函数声明 ==========

// 解析 INI 文件
ImportedIniFile ParseIniFile(const std::wstring& FilePath);

// 精确匹配注册表类型（遍历 RegisterTypes）
void MatchSectionToRegType(ImportedIniFile& File);

// 手动设置某个 section 的注册表类型（供 UI 层调用后修正）
void SetSectionRegType(ImportedIniSection& Section, const std::string& RegType);

// 检测跨 section 的键级链接关系（基于 DefaultLinks）
std::vector<IniImportLinkRelation> DetectLinkRelations(const ImportedIniFile& File);

// 计算布局（同类型竖向，有链接横向）
void CalculateLayout(ImportedIniFile& File, const std::vector<IniImportLinkRelation>& Links);

// 将导入结果转换为 ModuleClipData 列表，准备 AddModule
std::vector<ModuleClipData> ImportedSectionsToModuleClipData(
    const ImportedIniFile& File,
    const IniImportOptions& Options = IniImportOptions{}
);
