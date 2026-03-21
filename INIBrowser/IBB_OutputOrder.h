#pragma once
#include "FromEngine/Include.h"
#include "IBB_Components.h"
#include "IBB_Index.h"

template<typename T>
struct SectionTNode
{
    T Idx;
    std::vector<T> To;
};

using SectionNode = SectionTNode<const IBB_Section*>;

using Section_Pred = std::function<bool(const IBB_Section* Sec)>;
using Section_LinkGen = std::function<std::vector<const IBB_Section*>(const IBB_Section* Sec)>;

template<typename T>
struct OrderCheckTResult
{
    bool HasCycle;
    std::vector<T> Order_Or_Ring;
};
using OrderCheckResult = OrderCheckTResult<const IBB_Section*>;

struct IBB_OrderChecker
{
    std::vector<SectionNode> Nodes;
    const IBB_Project* pProj;
    OrderCheckResult GenerateOrder();
    IBB_OrderChecker() = delete;
    IBB_OrderChecker(const IBB_Project& Proj, const Section_Pred& Pred, const Section_LinkGen& LinkGen);
};

OrderCheckResult TopoSortByInherit(const IBB_Project& Proj);
OrderCheckResult TopoSortByImport(const IBB_Project& Proj);
OrderCheckTResult<IBB_LineLocation> TopoSortByKeyLink(const IBB_Project& Proj);
