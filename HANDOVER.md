# INIWeaver (INI织网者) 交接文档

> 项目根: `I:\WeaverAI\`
> 版本: 1.0.9-dev-b7 → 开发中 (ModProject 分支)
> 构建输出: `x64\Release\INIWeaver.exe`
> 备份: `backup_v1.0.9-dev-b6\`

---

## 一、项目概述

**INI织网者** 是红警2 YR Mod 开发用的可视化 INI 编辑器。将传统手写 INI 文本转为节点-连线图，自动处理注册、继承、素材引入。

- **开发者**: 钢铁之锤（主程序员）
- **设计者**: 艾木魁/Kenosis
- **语言**: C++23
- **平台**: Windows (Win32/x64)
- **GUI**: Dear ImGui + GLFW + OpenGL 2
- **构建**: Visual Studio 2022 (v143 toolset)
- **线程**: 三线程（渲染 / 前端逻辑 / 保存I/O）

---

## 二、目录结构

```
I:\WeaverAI\
├── INIBrowser.sln                    # VS 解决方案
├── INIBrowser\                       # 主项目（所有源码在此）
│   ├── INIBrowser.vcxproj            # MSBuild 项目文件
│   ├── INIBrowser.cpp                # ★ 程序入口 wWinMain
│   ├── Initialize.h/.cpp             # ★ 四阶段初始化 + ShellLoop 主循环
│   ├── MainStage.h                   # ★ 主舞台 UI (ControlPanel)
│   ├── SaveFile.h / IBS_File.cpp     # 通用二进制文件读写框架
│   ├── IBFront.h/.cpp                # ★ 前端线程循环
│   ├── IBSave.h/.cpp                 # ★ 保存线程 (项目序列化)
│   ├── Global.h/.cpp                 # 全局状态/版本号/线程通信
│   ├── Resource.h/.rc / icon1.ico    # Windows 资源
│   │
│   ├── FromEngine\                   # 引擎工具层
│   │   ├── Include.h                 # 聚合包含 (imgui/GLFW/STL/Win32)
│   │   ├── types.h                   # 基础类型: BufString/StrPoolID/DescPoolOffset
│   │   ├── InfoStack.h               # 线程安全消息栈模板
│   │   ├── RFBump.h/.cpp             # ★ Render-Front 双向消息通信核心
│   │   ├── external_file.h           # 文件读写封装 ExtFileClass
│   │   ├── external_log.h/.cpp       # 日志类 LogClass
│   │   ├── global_timer.h            # 高精度计时器
│   │   └── global_tool_func.h/.cpp   # 工具函数: JSON/CSV/编码/正则/随机字符串
│   │
│   ├── IBG_*                         # 全局通用数据层
│   │   ├── IBG_Ini.h                 # INI Token 解析器 + Base64
│   │   ├── IBG_InputType.h/.cpp      # 输入类型虚基类系统
│   │   ├── IBG_InputType_Defines.h   # 内建输入类型宏定义
│   │   ├── IBG_InputType_Derived.h   # 派生输入类型定义
│   │   ├── IBG_InputType_Factory.cpp # 输入类型工厂函数
│   │   └── IBG_UndoTree.h            # 撤销/重做栈
│   │
│   ├── IBB_*                         # ★ 后端模型层 (核心数据模型)
│   │   ├── IBB_Components.h/.cpp     # 核心组件: VariableList/Section_Desc/SectionID
│   │   ├── IBB_Ini.h/.cpp            # ★ 主 INI 模型: IniLine/SubSec/Section/Ini
│   │   ├── IBB_IniLine.h/.cpp        # INI 行数据: LineData variant 类型系统
│   │   ├── IBB_Index.h/.cpp          # Project_Index / SectionID 管理
│   │   ├── IBB_Project.h/.cpp        # ★ 项目根模型: IBB_Project/RegisterList
│   │   ├── IBB_Setting.h/.cpp        # 设置系统: SettingPack/SettingType
│   │   ├── IBB_Section.cpp           # Section 完整模型
│   │   ├── IBB_SubSec.cpp            # Sub-Section 模型
│   │   ├── IBB_PropStringPool.h/.cpp # 属性字符串池 (字符串去重/ID化)
│   │   ├── IBB_RegType.h/.cpp        # 注册类型加载 (RegisterTypes)
│   │   ├── IBB_ModuleAlt.h/.cpp      # 模块/模板系统 (预制 Section)
│   │   ├── IBB_OutputFormat.h/.cpp   # 导出格式控制
│   │   ├── IBB_OutputOrder.h/.cpp    # 导出键顺序控制
│   │   ├── IBB_CustomBool.h/.cpp     # 自定义布尔类型
│   │   ├── IBB_FileChecker.h/.cpp    # 文件存在性检查 (带弹窗)
│   │   └── IBB_FileAssoc.cpp         # Windows 文件关联 (.iproj)
│   │
│   ├── IBR_*                         # ★ 渲染/UI 层 (所有 ImGui 交互)
│   │   ├── IBR_Project.h/.cpp        # ★ 渲染层核心: SectionData/Link/Module 管理
│   │   ├── IBR_Components.h/.cpp     # UI 组件: Popup/SelectMode/Recent/DynamicData
│   │   ├── IBR_Combo.h/.cpp          # 自定义下拉框组件
│   │   ├── IBR_Debug.h/.cpp          # 调试 UI (-debugmenu)
│   │   ├── IBR_Font.h/.cpp           # 字体搜索/加载
│   │   ├── IBR_HotKey.h/.cpp         # 快捷键系统
│   │   ├── IBR_IniLine.h             # INI 行渲染: WorkSpaceLine/NodeSession
│   │   ├── IBR_LinkNode.h/.cpp       # 链接节点渲染
│   │   ├── IBR_ListView.h/.cpp       # 列表视图组件
│   │   ├── IBR_Localization.h/.cpp   # 多语言本地化
│   │   ├── IBR_Misc.h/.cpp           # 颜色/主题/提示管理器
│   │   ├── IBR_FullView.cpp          # 全视图缩放/平移
│   │   ├── IBR_Panel.cpp             # 面板布局渲染
│   │   ├── IBR_ProjManager.cpp       # 项目管理器 (新建/打开/保存)
│   │   ├── IBR_Section.cpp           # Section 渲染命令接口
│   │   ├── IBR_SectionData.cpp       # ★ Section UI 主体渲染 (最复杂)
│   │   ├── IBR_Setting.cpp           # 设置窗口渲染
│   │   ├── IBR_TopMost.h/.cpp        # 窗口置顶控制
│   │   ├── IBR_WorkSpace.cpp         # ★ 工作区 (节点画布) 渲染
│   │   └── IBR_InputTypeDebug.cpp    # 输入类型调试 UI
│   │
│   ├── cjson\                        # 第三方: cJSON 库
│   └── fmt\                          # 第三方: fmt 库
│
├── ImGui\                            # 第三方: Dear ImGui + 后端 (GLFW/OpenGL2/Win32)
└── libs\                             # 第三方库: GLFW / uSynergy
```

---

## 三、架构核心概念

### 3.1 三线程模型

| 线程 | 函数 | 职责 |
|------|------|------|
| **渲染线程** (主) | `ShellLoop()` | ImGui 渲染循环，处理所有用户交互 |
| **前端线程** | `IBF_Thr_FrontLoop()` | 执行后端逻辑：设置读写、模块加载、数据操作 |
| **保存线程** | `IBS_Thr_SaveLoop()` | 异步处理 .iproj 文件的保存和加载 |

线程间通信通过 `RFBump` 消息系统（`InfoStack<T>` 线程安全队列 + 互斥锁）。

### 3.2 数据模型层级

```
IBB_Project                    # 项目根
  └── IBB_Ini[]                # 多个 INI 文件 (如 Rules, Art)
        └── IBB_Section[]      # 多个 Section ([SOMETYPE])
              └── IBB_SubSec[] # SubSection (如 AresInherit 创建的子块)
                    └── IBB_IniLine[]  # 键值行 (Damage=100, Warhead=...)
                          └── LineData (std::variant)
                                ├── IniLine_Data_String  # 普通文本值
                                ├── IniLine_Data_Link    # 链接到其他 Section
                                └── ...                  # 其他 variant 类型
```

### 3.3 MVC 分层

| 层 | 前缀 | 角色 |
|----|------|------|
| **Model** | `IBB_*` | 数据模型，不依赖 UI |
| **View+Controller** | `IBR_*` | ImGui 渲染 + 用户交互处理 |
| **Backend Logic** | `IBFront` | 命令执行、异步操作 |
| **Engine** | `FromEngine/` | 基础工具（JSON/CSV/INIO解析/编码） |

### 3.4 命令通信模式

渲染层不直接操作后端模型。修改数据流程：
1. `IBR_*` 创建命令数据
2. 通过 `IBRF_Bump::Send_ToFront()` 发送给前端线程
3. 前端线程执行命令，修改 `IBB_*` 模型
4. 前端线程通过 `IBRF_Bump::Send_ToRender()` 返回结果/通知 UI 刷新

### 3.5 关键类型

| 类型 | 用途 |
|------|------|
| `StrPoolID` | 字符串池 ID，所有字符串去重后用 ID 引用 |
| `SectionID` | Section 的唯一标识符（注册名 + INI 类型） |
| `DescPoolOffset` | 描述字符串池偏移 |
| `BufString` | 缓冲区字符串（固定大小栈字符串） |
| `LineData` | `std::variant` 多态 INI 行值 |

### 3.6 运行时配置文件位置

| 相对路径 | 格式 | 用途 |
|----------|------|------|
| `.\Global\RegisterTypes.json` | JSON | 注册大类声明（WeaponTypes/Warheads/Animations...） |
| `.\Global\RegisterTypesSpecial.json` | JSON | 特殊注册类型（SubSec 等） |
| `.\Global\RegisterTypesCompound.json` | JSON | 复合输入类型配置 |
| `.\Global\TypeAlt*.csv` | CSV | 语句字典（Key 的注释、链接种类、输入类型等） |
| `.\Global\TypeAlt8Compound.csv` | CSV | 特殊输入格式定义 |
| `.\Global\TypeAlt9Links.csv` | CSV | 精简链接字典 |
| `.\Global\TypeAltSpecial.csv` | CSV | 特殊 Key 定义（1.0.8+） |
| `.\Global\Modules\` | .ini 文件 | 模块库（预制 Section 模板） |
| `.\Resources\language.ini` | INI | 多语言字符串 |
| `.\Resources\config.json` | JSON | 字体/主题/快捷键配置 |
| `.\Resources\setting.dat` | 二进制 | 用户设置持久化 |
| `.\Resources\recent.dat` | 二进制 | 最近文件列表 |
| `.\Resources\hint.txt` | TXT | 提示文本 |
| `.\Resources\load.txt` | TXT | 预加载字形范围 |

### 3.7 命令行参数

| 参数 | 用途 |
|------|------|
| `-debugmenu` | 显示调试菜单 |
| `-logconfig` | 输出加载的配置文件顺序 |

---

## 四、文件详细索引

> ★ 标记 = 核心入口/控制流文件，修改功能时常从这里入手

### 入口与控制流

| 文件 | 用途 | 关键函数 |
|------|------|----------|
| `INIBrowser.cpp` | 程序入口 `wWinMain` | 调度四阶段初始化 + ShellLoop |
| `Initialize.h` | 初始化声明 | `Initialize_Stage_I~IV`, `ShellLoop` |
| `Initialize.cpp` | 四阶段实现 | GLFW/ImGui 初始化、主循环、CleanUp |
| `MainStage.h` | 主舞台 UI | `ControlPanel()` — 整个应用的主 UI 函数 |
| `Global.h` | 全局声明 | 版本号 `IB_VERSION`、全局变量 extern 声明 |
| `Global.cpp` | 全局定义 | 所有全局变量实例的定义 |

### 线程与通信

| 文件 | 用途 | 关键函数/类 |
|------|------|-------------|
| `IBFront.h` | 前端线程接口 | `IBF_Setting`, `IBF_DefaultTypeList`, `IBF_Project` |
| `IBFront.cpp` | 前端线程循环 | `IBF_Thr_FrontLoop()` — 处理来自渲染线程的命令 |
| `IBSave.h` | 保存线程接口 | `IBS_Project` — 项目序列化/反序列化 |
| `IBSave.cpp` | 保存线程循环 | `IBS_Thr_SaveLoop()` — 异步文件 I/O |
| `SaveFile.h` | 二进制文件读写 | `SaveFile` 类 — 分块读/写 |
| `IBS_File.cpp` | SaveFile 实现 | `SaveFile::Read()`, `SaveFile::Write()` |
| `RFBump.h` | R↔F 通信核心 | `IBRF_Bump` — 消息发送/接收/延迟/中断 |
| `RFBump.cpp` | 通信实现 | 互斥锁、消息队列管理 |

### 引擎工具层 (FromEngine/)

| 文件 | 用途 |
|------|------|
| `Include.h` | 聚合包含: imgui.h, GLFW, STL 容器/算法, Win32 API, 字符串流 |
| `types.h` | 基础类型: `BufString<N>`, `StrPoolID`, `DescPoolOffset`, `callback_t` |
| `InfoStack.h` | 线程安全消息栈 `InfoStack<T>` (push/peek/pop) |
| `external_file.h` | `ExtFileClass`: ANSI/UTF8/Unicode 文件读写, `PointerArray`, `BytePointerArray` |
| `external_log.h/.cpp` | `LogClass` — 日志输出到文件 |
| `global_timer.h` | `GetSysTimeMicros()` — QueryPerformanceCounter 封装 |
| `global_tool_func.h/.cpp` | 核心工具函数: `AnsiToUnicode`, `Utf8ToUnicode`, `JsonFile`, `CSVReader`, 正则匹配, `RandomString` |

### 全局通用层 (IBG_*)

| 文件 | 用途 |
|------|------|
| `IBG_Ini.h` | INI Token 解析: `IniToken` 结构, `GetLines()`, `GetTokens()`, Base64 编解码 |
| `IBG_InputType.h` | 输入类型系统基类 `IBG_InputType` + 注册宏 |
| `IBG_InputType.cpp` | 输入类型实现 |
| `IBG_InputType_Defines.h` | 内建输入类型的宏定义 (InputText/Bool/EnumCombo/SliderInt...) |
| `IBG_InputType_Derived.h` | 派生输入类型定义 |
| `IBG_InputType_Factory.cpp` | `IBG_CreateInput()` 工厂函数 |
| `IBG_UndoTree.h` | `IBG_UndoStack` — 撤销/重做栈 |

### 后端模型层 (IBB_*)

| 文件 | 用途 | 关键类/结构 |
|------|------|------------|
| `IBB_Components.h/.cpp` | 核心组件 | `VariableList`, `VariableMultiList`, `Section_Desc`, `SectionID`, `Project_Index` |
| `IBB_Ini.h` | INI 主模型头 | `IniLine_Default`, `SubSec_Default`, `IniLine`, `SubSec`, `Section`, `Ini`, `NewLink` |
| `IBB_Ini.cpp` | INI 模型实现 | Section/SubSec 的创建/修改/删除/导出 |
| `IBB_IniLine.h` | INI 行数据层 | `IniLine_Data_Base`, `LineLocation`, `IniLine` 类 (含 variant LineData) |
| `IBB_IniLine.cpp` | INI 行实现 | 行数据操作、链接管理 |
| `IBB_Section.cpp` | Section 完整实现 | Section 的 SubSec 管理、导出逻辑 |
| `IBB_SubSec.cpp` | Sub-Section 实现 | SubSec 的 Line 管理、继承/AresInherit |
| `IBB_Index.h/.cpp` | 索引管理 | `Project_Index` — 全局 SectionID 索引, `LockIndex`, `UnlockIndex` |
| `IBB_Project.h/.cpp` | 项目根模型 | `IBB_Project` — 管理 Inis[], `RegisterList`, `DefaultTypeList` |
| `IBB_Setting.h/.cpp` | 设置系统 | `SettingPack`, `SettingType`, `SettingTypeList` — 设置项注册/读写/序列化 |
| `IBB_PropStringPool.h/.cpp` | 字符串池 | 字符串去重、ID化，减少内存占用 |
| `IBB_RegType.h/.cpp` | 注册类型 | 加载 RegisterTypes*.json，管理 INI 大类类型 |
| `IBB_ModuleAlt.h/.cpp` | 模块模板 | 从 `.\Global\Modules\` 加载预制 Section 模板 |
| `IBB_OutputFormat.h/.cpp` | 导出格式 | 控制 INI 输出时的格式/排列 |
| `IBB_OutputOrder.h/.cpp` | 导出顺序 | 控制键值的输出顺序（按继承顺序排列等） |
| `IBB_CustomBool.h/.cpp` | 自定义布尔 | 非标准 true/false 值映射 (yeaaaaaaah_fuuuuuuuck 等) |
| `IBB_FileChecker.h/.cpp` | 文件检查 | 检查素材文件是否存在，弹窗提示 |
| `IBB_FileAssoc.cpp` | 文件关联 | Windows 注册表 .iproj 文件关联 |

### 渲染/UI 层 (IBR_*)

| 文件 | 用途 | 关键类/函数 |
|------|------|------------|
| `IBR_Project.h` | 渲染层核心头 | `IBR_SectionData`, `IBR_Section`, `IBR_Project` — 渲染层项目状态 |
| `IBR_Project.cpp` | 渲染层核心实现 | 模块创建/删除/复制、链接管理 |
| `IBR_Components.h/.cpp` | UI 组件 | `SelectMode` (框选模式), `DynamicData`, `RecentManager`, `PopupManager` |
| `IBR_Combo.h/.cpp` | 下拉框 | 自定义 ImGui Combo 组件 (支持搜索/过滤) |
| `IBR_Debug.h/.cpp` | 调试 UI | 调试菜单、性能统计、内部状态查看 |
| `IBR_Font.h/.cpp` | 字体 | 字体搜索、加载、字形范围管理 |
| `IBR_HotKey.h/.cpp` | 快捷键 | 快捷键注册/触发/自定义 |
| `IBR_IniLine.h` | INI 行渲染 | `WorkSpaceLine` — 画布上显示的 INI 行, `NodeSession` |
| `IBR_LinkNode.h/.cpp` | 链接节点 | 链接线的渲染、交互 (拖拽/连接/断开) |
| `IBR_ListView.h/.cpp` | 列表视图 | 模块列表 (顶部"列表"菜单) 渲染 |
| `IBR_Localization.h/.cpp` | 本地化 | `IBR_Loc()` 函数、多语言字符串加载 |
| `IBR_Misc.h/.cpp` | 杂项 | 颜色方案、主题、`HintManager` (鼠标悬停提示) |
| `IBR_FullView.cpp` | 全视图 | 画布缩放/平移/视图变换 (ImGui DrawList 坐标变换) |
| `IBR_Panel.cpp` | 面板布局 | 左/右/底部面板的布局渲染 |
| `IBR_ProjManager.cpp` | 项目管理器 | 新建/打开/保存/另存为/最近文件对话框 |
| `IBR_Section.cpp` | Section 命令 | `IBR_Section` 类 — 渲染层 Section 的命令接口 |
| `IBR_SectionData.cpp` | Section UI 主体 | **最大的 UI 文件** — Section 的编辑栏、文本编辑模式、所有 UI 交互 |
| `IBR_Setting.cpp` | 设置窗口 | 设置界面的 ImGui 渲染 |
| `IBR_TopMost.h/.cpp` | 窗口置顶 | 保持窗口在最前 |
| `IBR_WorkSpace.cpp` | 工作区画布 | **节点画布渲染** — 绘制所有 Section、连线、背景网格 |
| `IBR_InputTypeDebug.cpp` | 输入类型调试 | 调试模式下的输入类型可视化 |

### 第三方库

| 路径 | 库 | 版本 | 用途 |
|------|-----|------|------|
| `cjson/` | cJSON | 1.x | JSON 解析 |
| `fmt/include/` `fmt/src/` | {fmt} | - | 字符串格式化 (Header-only) |
| `ImGui/` | Dear ImGui | - | GUI 渲染核心 (in-tree) |
| `ImGui/backends/` | imgui_impl_glfw/opengl2/win32 | - | ImGui 后端 |
| `libs/glfw/` | GLFW | 3.x | 窗口管理 + OpenGL 上下文 |
| `libs/usynergy/` | uSynergy | 1.0.0 | Synergy 共享剪贴板客户端 |

---

## 五、构建说明

### 环境要求
- **Visual Studio 2022** (v143 toolset)
- **Windows SDK 10.0**
- C++23 支持 (/std:c++23)
- C11 支持 (/std:c11)

### 构建步骤
1. 打开 `I:\WeaverAI\INIBrowser.sln`
2. 选择配置: **Release | x64**
3. 生成 → 生成解决方案
4. 输出: `x64\Release\INIWeaver.exe`

### 预处理器定义
- `FMT_HEADER_ONLY` — fmt 库 header-only 模式
- `IMGUI_USE_WCHAR32` — ImGui 宽字符模式
- `_CRT_SECURE_NO_WARNINGS` — 禁用 CRT 安全警告

### 运行时结构
```
INIWeaver.exe
├── Resources\
│   ├── config.json
│   ├── language.ini
│   ├── setting.dat       (自动生成)
│   ├── recent.dat        (自动生成)
│   ├── hint.txt
│   └── load.txt
└── Global\
    ├── Modules\           (模块库 .ini 文件)
    ├── RegisterTypes*.json
    └── TypeAlt*.csv/json
```

---

## 六、常见修改场景定位

| 需求 | 入手文件 | 说明 |
|------|----------|------|
| 增加新的输入组件类型 | `IBG_InputType_Defines.h` → `IBG_InputType_Factory.cpp` → `IBG_InputType_Derived.h` | 定义宏 → 注册工厂 → 实现类 |
| 添加菜单项/工具栏按钮 | `MainStage.h` → `ControlPanel()` | 主 UI 布局在这里 |
| 修改导出 INI 逻辑 | `IBB_OutputFormat.cpp` → `IBB_OutputOrder.cpp` → `IBB_Ini.cpp` | 格式 → 顺序 → 导出主逻辑 |
| 修改 Section 编辑栏 UI | `IBR_SectionData.cpp` | 最复杂的 UI 文件 |
| 修改画布交互 (框选/拖拽) | `IBR_WorkSpace.cpp` → `IBR_Components.cpp` (SelectMode) | 画布渲染 + 选择逻辑 |
| 修改快捷键 | `IBR_HotKey.cpp` → `Resources\config.json` | 代码注册 + 配置文件 |
| 修改多语言文本 | `Resources\language.ini` → `IBR_Localization.cpp` | INI 文件 + 加载逻辑 |
| 增加注册类型 | `Global\RegisterTypes.json` → `IBB_RegType.cpp` | 配置 + 解析逻辑 |
| 修改模块库模板 | `Global\Modules\*.ini` → `IBB_ModuleAlt.cpp` | 模板文件 + 加载逻辑 |
| 修改 .iproj 文件格式 | `IBSave.cpp` (序列化) + `IBFront.cpp` (加载) | 两端都要改 |
| 修改设置项 | `IBB_Setting.cpp` (注册) → `IBR_Setting.cpp` (UI) | 后端注册 + 前端渲染 |
| 修改链接行为 | `IBR_LinkNode.cpp` (UI) → `IBB_IniLine.cpp` (数据) | 交互 + 数据模型 |

---

## 七、关键术语

| 术语 | 含义 |
|------|------|
| **Section** | INI 中的一个块 `[TypeName]`，如武器/单位/弹头 |
| **SubSec** | 子 Section（如 AresInherit 创建的继承块） |
| **IniLine** | INI 中的一行键值对 `Key=Value` |
| **LineData** | IniLine 的值 — `std::variant` 多态类型 |
| **RegisterList** | 注册表 — 需要汇总导出的类型列表 (如 `[InfantryTypes]`) |
| **RegisterType** | 注册类型大类 — WeaponTypes/Warheads/Animations... |
| **Module** | 预制模板 — 从模块库拖入画布的 Section 蓝图 |
| **语句包 (FlagPack)** | 特殊模块，连线后直接粘贴语句到目标，不创建新 Section |
| **AresInherit** | `[NewType]:[BaseType]` 格式的继承语法 |
| **Node / Link** | 画布上可拖拽连线的节点和连线 |
| **Import 导出模式** | 从其他 INI 搜索并合并语句的导出方式 |
| **Recompose 导出模式** | 根据值和输入表单重组键值对的导出方式 |
| **iproj** | 工程文件格式（自定义二进制），只有织网者可识别 |

---
## 八、当前改动 (2026-05-24 模块库侧栏重构)

### 已完成

#### 1. 模块库侧栏
- **IBR_Panel.cpp**: 新增 `ControlPanel_Modules()` 函数，左侧面板显示资源管理器式模块库树
- **IBB_ModuleAlt.h/.cpp**: 新增 `Tree_RenderUISidebar()` 用于侧栏树形渲染（带文件夹图标 `DrawFolderIcon`），与原有右键菜单 `SpecialTree_RenderUI` 分离
- **MainStage.h**: 保持不变（模块库入口由 IBR_Panel 的菜单按钮控制）
- **"文件"菜单**: 保存/打开/导出/最近打开归位到文件下拉菜单

#### 2. MenuItemID 重编号
- **IBR_Misc.h**: 新增 `MenuItemID_MODULES = 1`，后续 `VIEW/LIST/EDIT/SETTING/ABOUT/DEBUG` 各+1
- **IBR_WorkSpace.cpp**: 点击模块退出编辑后切换回模块库（`ChooseMenu(MenuItemID_MODULES)`）

#### 3. 构建适配
- **INIBrowser.vcxproj**: VS 2025 (v18) toolset 适配，`MSBuildStartupDirectory` 改为 `SolutionDir`

### 文件改动清单

| 文件 | 改动性质 |
|------|----------|
| `IBB_ModuleAlt.h` | +绘制文件夹图标 (`DrawFolderIcon`), +声明 `Tree_RenderUISidebar`, `IsModuleTreeEmpty` |
| `IBB_ModuleAlt.cpp` | 同上实现, +`Tree_RenderUISidebar` 带 `BeginChild` 滚动 |
| `IBR_Panel.cpp` | +`include IBB_ModuleAlt.h`, +`ControlPanel_Modules`, +模块库按钮到 ItemList |
| `IBR_Misc.h` | `MenuItemID` 全部+1 偏移 (FILE=0, MODULES=1, VIEW=2, ...) |
| `IBR_WorkSpace.cpp` | `ChooseMenu(MenuItemID_FILE)` → `ChooseMenu(MenuItemID_MODULES)` |
| `INIBrowser.vcxproj` | VS2025 工具集适配 |

### PR 状态
- 对应分支已推送 [KenosisM/INIWeaver-AI](https://github.com/KenosisM/INIWeaver-AI)
- 待上游 [ra2diy/INIWeaver-1.0](https://github.com/ra2diy/INIWeaver-1.0) 合并

---
## 九、ModProject 功能设计 (未实现 / 待重新开始)

> 以下为分析阶段的设计思路，实际代码已回滚。

### 核心理念

`.modproj` 文件作为 `.iproj` 的超集，复用 99% 的现有交互逻辑。唯一特殊行为：拖入 `.iproj` 到 `.modproj` 画布时不打开而创建 `iproj_ref` 引用模块。

**不使用独立模式变量** (`IsModProjectMode`)，改为检查 `IBF_Inst_Project.Project.Path.ends_with(".modproj")`（`IsModProject()`）。

### 数据结构

#### IBB_ModProject
```
Canvas          IBB_Project       // 画布（复用现有项目结构）
SubProjects[]   IBB_SubProjectRef // 子项目映射 (iproj_path → export_prefix)
GlobalModules[] IBB_ModuleAlt     // 全局模块（编译时注入子项目）
MainIniRefs     map<string,wstring> // 主 INI 文件路径引用
BuildOutputDir  wstring           // 编译输出目录
```

#### IBB_SubProjectRef
```
iproj_path:    wstring  // .iproj 文件绝对路径
export_prefix: string   // 编译输出文件名前缀
display_name:  string   // 画布上显示名称
```

#### 内部注册类型
```
RegType_iproj_ref    = "iproj_ref"      // 画布上的 iproj 引用模块
RegType_global_module = "global_module"  // 全局模块类型
UseGlobalModule       // RegisterTypesSpecial.json (Import + RingCheck)
```

### 实现分期

| 阶段 | 内容 | 涉及文件 |
|------|------|----------|
| P1 | IBB_ModProject 数据模型 + RegType 注册 | `IBB_ModProject.h/.cpp`, `Global.h/.cpp`, `Initialize.cpp` |
| P2 | 新建/保存/打开 .modproj, 拖入 iproj 创建引用模块, 删除时清理双向引用 | `IBR_Panel.cpp`, `IBR_ProjManager.cpp`, `IBR_Project.cpp` |
| P3 | ModProjPath 序列化 (IBSave/IBS_Project), 双向引用写入 .iproj | `IBSave.h/.cpp`, `IBFront.cpp`, `IBB_Project.h` |
| P4 | 资产路径记录 (AssetFiles) | `IBB_Ini.h`, `IBR_ProjManager.cpp` |
| P5 | 编译管道：子项目加载 → 全局注入 → INI 导出 → #include 生成 | `IBR_ProjManager.cpp` (新增 OutputModProject) |
| P6 | 主 INI 复制 + 资产文件拷贝 | 同上 |

### 序列化兼容性分析

#### ModProjPath (IBS_Project 层)
- 需在 `FVersion >= 10010` 分支读写
- 插入位置：`PersistentID` 之后、`LastOutputIniName` 之前
- 旧 .iproj 无此字段 → 默认空字符串（向后兼容）
- 旧 Weaver 打开新 .iproj → 提示"文件版本过高"

#### AssetFiles (ModuleClipData 层)
- `ClipWriteStream` / `ClipReadStream` 独立版本系统
- 保留区 (8 bool + 8 int) 不修改大小
- AssetFiles 追加在保留区之后，`VersionAtLeast(10010)` 门控

### 编译管道设计

```
OutputModProject(modproj_path, output_dir):
  for each SubProject in modproj.SubProjects:
    iproj = LoadProject(subproject.iproj_path)
    // 注入全局模块（本地 Key 优先，fallback）
    for each global in modproj.GlobalModules:
      for each section in iproj:
        section.InsertIfNotExists(global.Lines)
    // 运行现有 Output 管道
    texts = RunExistingOutputPipeline(iproj)
    all_texts[ini_type].push_back({prefix, texts})
  
  // 合并写入
  for each ini_type:
    WriteFile(output_dir / prefix_ini_type.ini, merge(texts))
  
  // 生成 [#include] 追加到主 INI
  CopyFile(MainIniPath, output_dir / "RA2MD.INI")
  AppendIncludeBlock(output_dir / "RA2MD.INI", [include_list])
  
  // 拷贝资产文件
  for each section.AssetFiles:
    CopyFile(source, output_dir / Assets / filename(source))
```

### 已知坑点

1. **空 Data 向量崩溃**: `IBB_ClipBoardData::SetStream` 对空 `Data` 向量会创建 `Cursor = nullptr`，需 early return
2. **异步打开间隙**: `ProjOpen_OpenRecentAction` 内部 `CloseAction` 同步 + `SendToR(OpenRecentAction)` 异步，关闭-打开间 `Path` 为空导致 `IsModProject()` 返回 false
3. **ImGui 上下文破坏**: modproj 右键菜单中 `SpecialTree_RenderUI` 的 `DelayedPopupAction` 在 modproj 状态下破坏 ImGui `DrawList::_Data`
4. **线程安全**: `IBF_Inst_Project.Project.Path` 被渲染线程直接读取，需确保前端线程写操作原子性

---

## 十、当前进度 (2026-05-24)

### 已推 PR

| 分支 | 内容 |
|------|------|
| `KenosisM:master` | 模块库侧栏重构 + 系统模块（MenuItemID 重编号） |
| `KenosisM:asset-track` | WAV/PCX 拖入 + VarList 资产路径追溯 + Palette .pal 解析 |
| `KenosisM:wav-pcx` | 早期 WAV+PCX PR（已关闭） |

### 本地开发中（未推）

#### ModProject P1+P2 — iproj_ref 模块生命周期
- 拖入 .iproj 创建 iproj_ref 模块（DefaultIPROJ 模板，保留原始大小写）
- 保存时 ModProjPath 尾追写入 .iproj（sentinel 去重）
- 重命名：两阶段收集映射 → 传播到其他 modproj → MoveFileW 靠后
- 传播时同步更新其他 modproj 的 iproj_path + 模块名（Desc.B）
- 删除模块：保存时对比前次磁盘版本差分，定向剥离标记
- VarList 去重、.pal 调色板资产路径解析
- `IBR_Project::Save()` 核心顺序：去重 → 差分剥离 → 追加 → .pal → 两阶段重命名
- IBB_ModProject.h/.cpp：数据模型，InitTypes() 注册 RegType
- Global.h/.cpp：`IsModProject()` 内联，`IBF_Inst_ModProject`
- IBR_ProjManager.cpp：PathFilter +.modproj，Save/OnDropFile/Output 适配
- IBSave.cpp：SaveExtHookProc 实时替换后缀，SelectFileName 自动后缀
- language.ini：+GUI_OutputModule_Type5
- iproj_ref 缺失文件标记：`IBR_SectionData.Missing` 字段 + Load 时检查 + WorkSpace/SectionData 红色遮罩渲染

#### 缺失文件标记机制（2026-05-24）
- **数据模型**: `IBR_SectionData` 新增 `bool Missing{ false }`（参照 Frozen 机制）
- **检查时机**: `IBR_Project::Load()` 在第一个 SendToR 回调中遍历所有 section，检查 `bk->VarList["iproj_path"]` → `PathFileExistsW`
- **渲染**: 
  - `IBR_SectionData.cpp`: 简化为 `if (Data->Missing) → AddRectFilled(MissingFileMaskColor)`
  - `IBR_WorkSpace.cpp`: 同样绘制红色半透明遮罩（与 Frozen 遮罩并列）
- **不参与序列化**: Missing 是渲染层运行时状态，不在 ModuleClipData 中持久化（每次加载重新检查）
- **颜色**: `MissingFileMaskColor` — Light: (224,37,37,60), Dark: (224,60,60,60)
- **文件检查**: 使用 `GetFileAttributesW() == INVALID_FILE_ATTRIBUTES`（避免 Shlwapi.h 引入 windows.h 与 imgui.h 的 min/max 宏冲突）
- **状态**: ✅ 已实现并验证通过 (2026-05-24)
- **备份**: `backup_v1.0.9-dev-b5\`

#### 关键技术点
- `IBS_LoadProject` / `IBS_SaveProject` 硬编码写入 `IBS_Inst_Project.Data`（全局），跨 modproj 操作需 Data/Path 保存恢复
- `ProjSL` 是全局单例（`IBSave.cpp:246`），禁止跨线程并发使用
- 后端 `VarList["iproj_path"]` 与临时 ClipData.VarList 必须同步写回

### 项目准则（已写入 SOUL.md）
- 严格复用旧函数，不得重写流程
- 版本号锁定，非明令不修改
- 新字段走已有序列化通道（VarList）
- 流程设计由用户全权负责，AI 仅执行代码实现
