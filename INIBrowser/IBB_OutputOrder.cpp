#include "IBB_OutputOrder.h"
#include "IBB_Project.h"
#include "IBB_Ini.h"
#include "Global.h"
#include <queue>

OrderCheckResult IBB_OrderChecker::GenerateOrder()
{
    //Kahn's algorithm
    //尝试生成拓扑序，如果成功则返回拓扑序，否则说明存在环，返回nullopt
    std::vector<const IBB_Section*> Ret;
    std::unordered_map<const IBB_Section*, int> InDegree;
    for (auto& Node : Nodes)
        InDegree[Node.Idx] = 0;
    for (auto& Node : Nodes)
        for (auto& To : Node.To)
            InDegree[To]++;
    std::queue<const IBB_Section*> Q;
    for (auto& [Sec, Deg] : InDegree)
        if (Deg == 0)Q.push(Sec);
    while (!Q.empty())
    {
        auto Cur = Q.front();
        Q.pop();
        Ret.push_back(Cur);
        for (auto& Node : Nodes)
            if (Node.Idx == Cur)
                for (auto& To : Node.To)
                {
                    InDegree[To]--;
                    if (InDegree[To] == 0)Q.push(To);
                }
    }
    if (Ret.size() == Nodes.size())
    {
        std::reverse(Ret.begin(), Ret.end());//反转一下
        return { false, std::move(Ret) };
    }
    else
    {
        //从入度不为0的节点中随便选一个开始DFS，直到回到这个节点，记录路径即为环
        std::unordered_map<const IBB_Section*, bool> Visited;
        std::vector<const IBB_Section*> Path;
        std::function<bool(const IBB_Section*)> Dfs = [&](const IBB_Section* Cur)->bool
            {
                Visited[Cur] = true;
                Path.push_back(Cur);
                for (auto& Node : Nodes)
                    if (Node.Idx == Cur)
                        for (auto& To : Node.To)
                        {
                            if (!Visited[To])
                            {
                                if (Dfs(To))return true;
                            }
                            else
                            {
                                //找到了环
                                auto It = std::find(Path.begin(), Path.end(), To);
                                if (It != Path.end())
                                {
                                    std::vector<const IBB_Section*> Ring(It, Path.end());
                                    return true;
                                }
                            }
                        }
                Path.pop_back();
                return false;
            };
        for (auto& [Sec, Deg] : InDegree)
            if (Deg != 0)
            {
                if (Dfs(Sec))break;
            }
        return { true, std::move(Path) };
    }
}

IBB_OrderChecker::IBB_OrderChecker(const IBB_Project& Proj, const Section_Pred& Pred, const Section_LinkGen& LinkGen)
{
    pProj = &Proj;
    for (auto& Ini : Proj.Inis)
        for (auto& [Name, Sec] : Ini.Secs)
            if (Pred(&Sec))
                Nodes.push_back({ &Sec, LinkGen(&Sec) });
}


OrderCheckResult TopoSortByInherit(const IBB_Project& Proj)
{
    IBB_OrderChecker Checker(Proj,
        [](const IBB_Section*) { return true; },
        [&](const IBB_Section* Sec) {
            std::vector<const IBB_Section*> Ret;
            if (!Sec->Inherit.empty())
            {
                auto pSec = Proj.GetSecIndex(Sec->Inherit, Sec->Root->Name).GetSec(Proj);
                if (pSec)Ret.push_back(pSec);
            }
            return Ret;
        });
    return Checker.GenerateOrder();
}

OrderCheckResult TopoSortByImport(const IBB_Project& Proj)
{
    IBB_OrderChecker Checker(Proj,
        [](const IBB_Section*) { return true; },
        [&](const IBB_Section* Sec) {
            std::vector<const IBB_Section*> Ret;
            if (!Sec->IsLinkGroup)
            {
                for (auto& Sub : Sec->SubSecs)
                    for(auto& Link : Sub.NewLinkTo)
                        if (Link.FromKey == ImportKeyName)
                        {
                            auto pSec = Link.To.GetSec(Proj);
                            if (pSec)Ret.push_back(pSec);
                        }
            }
            return Ret;
        });
    return Checker.GenerateOrder();
}
