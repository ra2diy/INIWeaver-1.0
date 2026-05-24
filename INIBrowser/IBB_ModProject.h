#pragma once
#include "FromEngine/Include.h"
#include "IBB_Project.h"

// ModProject 子项目引用
struct IBB_SubProjectRef
{
    std::wstring iproj_path;    // .iproj 文件绝对路径
    std::string  export_prefix; // 编译输出文件名前缀
    std::string  display_name;  // 画布上显示名称
};

// ModProject 数据模型 —— .modproj 是 .iproj 的超集
// Canvas 复用 IBB_Project（画布内容主存储）
// SubProjects 记录被引用的子 .iproj 列表
struct IBB_ModProject
{
    IBB_Project Canvas;                         // 画布（复用现有项目结构）
    std::vector<IBB_SubProjectRef> SubProjects; // 子项目映射
    std::wstring Path;                          // .modproj 文件路径
    std::wstring Name;                          // 项目名称
    bool IsNewlyCreated = true;

    void Clear();
    void AddSubProject(const std::wstring& iproj_path,
                       const std::string& display_name,
                       const std::string& export_prefix);

    // 初始化 iproj_ref / global_module 注册类型
    static void InitTypes();
};

// 注册类型名宏（编译期常量，用于 ModuleClipData 等需要 const char* 的场合）
#define RegTypeName_iproj_ref    "iproj_ref"
#define RegTypeName_global_module "global_module"

// 运行时 const char* 指针（供 EnsureRegType 等使用）
extern const char* RegType_iproj_ref;
extern const char* RegType_global_module;
