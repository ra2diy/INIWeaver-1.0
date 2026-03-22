#include "IBB_OutputOrder.h"
#include "IBB_Project.h"
#include "IBB_Ini.h"
#include "Global.h"
#include <queue>

template<typename T>
OrderCheckTResult<T> GenerateOrderT(const std::vector<SectionTNode<T>>& Nodes)
{
    //Kahn's algorithm
    //尝试生成拓扑序，如果成功则返回拓扑序，否则说明存在环，返回nullopt
    std::vector<T> Ret;
    std::unordered_map<T, int> InDegree;
    for (auto& Node : Nodes)
        InDegree[Node.Idx] = 0;
    for (auto& Node : Nodes)
        for (auto& To : Node.To)
            InDegree[To]++;
    std::queue<T> Q;
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
        std::unordered_map<T, bool> Visited;
        std::vector<T> Path;
        std::function<bool(T)> Dfs = [&](T Cur)->bool
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
                                    std::vector<T> Ring(It, Path.end());
                                    Path = std::move(Ring);
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

OrderCheckResult IBB_OrderChecker::GenerateOrder()
{
    return GenerateOrderT(Nodes);
}

IBB_OrderChecker::IBB_OrderChecker(const IBB_Project& Proj, const Section_Pred& Pred, const Section_LinkGen& LinkGen)
{
    pProj = &Proj;
    for (auto& Ini : Proj.Inis)
        for (auto& [Name, Sec] : Ini.Secs)
            if (Pred(&Sec))
                Nodes.push_back({ &Sec, LinkGen(&Sec) });
}


OrderCheckResult TopoSortByKeyID(const IBB_Project& Proj, StrPoolID Key)
{
    IBB_OrderChecker Checker(Proj,
        [](const IBB_Section*) { return true; },
        [&](const IBB_Section* Sec) {
            std::vector<const IBB_Section*> Ret;
            if (!Sec->Inherit.empty())
            {
                for (auto& Sub : Sec->SubSecs)
                    for (auto& Link : Sub.NewLinkTo)
                        if (Link.FromLoc.Key == Key)
                        {
                            auto pSec = Link.ToLoc.Sec.GetSec(Proj);
                            if (pSec)Ret.push_back(pSec);
                        }
            }
            return Ret;
        });
    return Checker.GenerateOrder();
}

size_t KeyLinkHash(IBB_SectionID ID, StrPoolID Key, size_t Mult)
{
    size_t Seed = 0;
    std::hash<IBB_SectionID> IDHash;
    std::hash<StrPoolID> StrHash;
    std::hash<size_t> SizeTHash;
    Seed ^= IDHash(ID) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
    Seed ^= StrHash(Key) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
    Seed ^= SizeTHash(Mult) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
    return Seed;
}

OrderCheckTResult<IBB_LineLocation> TopoSortByKeyLink(const IBB_Project& Proj)
{
    std::vector<SectionTNode<IBB_LineLocation>> Nodes;
    std::unordered_map<IBB_LineLocation, std::vector<IBB_LineLocation>> Conn;
    for (auto& Ini : Proj.Inis)
        for (auto& [Name, Sec] : Ini.Secs)
            for (auto& Sub : Sec.SubSecs)
                for (auto& Link : Sub.NewLinkTo)
                {
                    Conn[Link.FromLoc].push_back(Link.ToLoc);
                    Conn[Link.ToLoc];
                }
    for(auto& [From, To] : Conn)
        Nodes.push_back({ From, To });
    return GenerateOrderT(Nodes);
}
