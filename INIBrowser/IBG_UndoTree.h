#pragma once
#include "IBB_ModuleAlt.h"
#include "IBSave.h"
#include "IBB_Index.h"
#include "IBB_Components.h"

struct IBG_UndoStack
{
    struct _Item
    {
        _TEXT_ANSI std::string Id;
        std::function<void()> UndoAction, RedoAction;
        std::function<std::any()> Extra;
    };
    std::vector<_Item> Stack;
    //直接赋值得同时改Cursor
    int Cursor{ -1 };
    bool Undo();
    bool Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    void SomethingShouldBeHere();
    void Release();
    void Push(const _Item& a);
    void RenderUI();
    void Clear();
    _Item* Top();
};
/*
struct IBG_UndoNode_Global
{
    uint64_t Version;
    std::wstring LastOutputDir;
    std::vector<std::pair<std::string, std::wstring>> LastOutputIniName;
    float FullView_Ratio;
    ImVec2 FullView_EqCenter;
};

struct IBG_UndoNode_Section
{
    ModuleClipData Clip;
};

using IUGPtr = std::shared_ptr<IBG_UndoNode_Global>;
using IUSPtr = std::shared_ptr<IBG_UndoNode_Section>;


struct IBG_UndoDiff_Section
{
    IBB_SectionID ID;
    std::optional<IUSPtr> OldClip;
    std::optional<IUSPtr> NewClip;
};

struct IBG_UndoDiff_Global
{
    std::optional<IUGPtr> Diff;
};

struct IBG_UndoDiff
{
    IBG_UndoDiff_Global GlobalData;
    std::vector<IBG_UndoDiff_Section> SectionData;
};

struct IBG_UndoStack
{
    std::vector<IBG_UndoDiff> Stack;
    //直接赋值得同时改Cursor
    int Cursor{ -1 };
    bool Undo();
    bool Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    void SomethingShouldBeHere();
    void Release();
    void Push(const IBG_UndoDiff& a);
    void Push(const IBG_UndoNode_Global& Global);
    void Push(const IBB_SectionID& ID, const std::optional<ModuleClipData>& NewClip);
    void Push(const std::vector<IBG_UndoDiff_Section>& Sections);

    void RenderUI();
    void Clear();
    IBG_UndoDiff* Top();
};
*/
