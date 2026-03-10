#pragma once
#include "FromEngine/Include.h"
#include "IBB_Components.h"
#include "IBB_Index.h"

struct SectionNode
{
    const IBB_Section* Idx;
    std::vector<const IBB_Section*> To;
};

using Section_Pred = std::function<bool(const IBB_Section* Sec)>;
using Section_LinkGen = std::function<std::vector<const IBB_Section*>(const IBB_Section* Sec)>;

struct OrderCheckResult
{
    bool HasCycle;
    std::vector<const IBB_Section*> Order_Or_Ring;
};

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
