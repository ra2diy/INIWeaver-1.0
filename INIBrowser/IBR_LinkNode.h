#pragma once
#include "FromEngine/Include.h"
#include "IBB_Index.h"
#include <functional>



enum class ValidateResult
{
    Normal,//值通过校验
    Abnormal,//值未通过校验，但可以接受
    Refused,//拒绝这个值
    Unknown//校验无法进行
};

struct IBB_ValidateComponent;
struct IBB_ValueContainer;

using IVCPtr = std::unique_ptr<IBB_ValidateComponent>;

struct IBB_ValidateComponent : std::enable_shared_from_this<IBB_ValidateComponent>
{
    virtual ValidateResult Validate(IBB_ValueContainer& Cont) = 0;
};

struct IICStatus;
struct IBB_UpdateResult;
struct IBR_SectionData;
struct IBB_SubSec;
struct IBB_Section_Desc;

namespace IBR_NodeSession
{
    struct SessionKey
    {
        std::string Ini, Sec, Sub;
        size_t Line, Comp;
    };

    struct SessionLinkList
    {
        std::string Display, Section;
        bool UseLink;
    };

    struct SessionValue
    {
        bool NotifyNewValue;
        std::string NewValue;
        bool NotifyValueToMerge;
        std::string ValueToMerge;
        std::vector<SessionLinkList> LinkList;
        ImVec2 LastCenter;
        bool Collapsed;

        void Renew()
        {
            NotifyNewValue = false;
            NotifyValueToMerge = false;
            LinkList.clear();
        }
    };

    struct SourceNodeKey
    {
        std::string Ini, Sec, Line;
        size_t Comp;
        size_t ID() const;
    };

    struct SourceNodeValue
    {
        ImVec2 Center;
    };

    uint64_t GetSessionIdx(const std::string& Ini, const std::string& Sec, const std::string& Sub, size_t Line, size_t Comp);
    uint64_t GetSessionIdx(IBB_SectionID SecID, const std::string& Sub, size_t Line, size_t Comp);
    SessionValue& GetSessionValue(uint64_t SessionID);
    void SetSessionStatus(uint64_t SessionID, ImVec2 Center, bool Collapsed);
}

namespace IBR_LinkNode
{
    ImU32 AdjustLineCol(ImU32 Color);

    ImColor AdjustNodeCol(ImU32 Color, bool Empty, bool Inherit);

    IBB_UpdateResult RenderUI_Node(
        IBR_SectionData& Data,
        IBB_SubSec& FromSub,
        size_t LineIdx,
        size_t CompIdx,
        const std::string& Hint,
        const std::string& DescLong,
        const IBB_UpdateResult& DefaultResult,
        const LinkNodeSetting& LinkNode,
        const std::function<IBB_UpdateResult(const std::string& NewValue, bool Active)>& ModifyFunc
    );

    IBB_UpdateResult RenderUI_Node(
        const std::string& Hint,
        const std::string& DescLong,
        const IBB_UpdateResult& DefaultResult,
        const LinkNodeSetting& LinkNode,
        const std::function<IBB_UpdateResult(const std::string& NewValue, bool Active)>& ModifyFunc
    );

    void PushLinkForDraw(
        ImVec2 Center,
        IBB_SectionID Dest,
        StrPoolID DestKey,
        uint64_t SessionID,
        ImU32 LineCol,
        bool FromImport,
        bool SelfLink,
        bool Collapsed,
        bool SrcDragging = false
    );

    void UpdateLink(
        IBB_SubSec& FromSub,
        size_t LineIdx,
        size_t CompIdx,
        std::unordered_set<uint64_t>* Pushed = nullptr
    );

    void PushRestLinkForDraw(
        IBB_SubSec& FromSub,
        const std::unordered_set<uint64_t>& Pushed,
        size_t LineIdx,
        ImVec2 Center,
        bool SrcDragging = false
    );

    void PushInactiveLines(
        IBB_SubSec& FromSub,
        ImVec2 Center
    );

    ImVec2 DefaultCenter();
    ImVec2 DefaultCenterInWindow();

    bool UpdateLinkInitial();
}

struct LineDragData;

//Use do not check
namespace LinkNodeContext
{
    extern IBB_SubSec* CurSub;
    extern size_t LineIndex;
    extern size_t CompIndex;
    extern ImVec2 CollapsedCenter;
    extern bool CurLineChangeCompStatus;
    extern LineDragData CurDragData;
    extern ImVec2 CurDragStart;
    extern ImVec2 CurDragStartEqCenter;
    extern ImU32 CurDragCol;
    extern bool HasDragNow;
}
