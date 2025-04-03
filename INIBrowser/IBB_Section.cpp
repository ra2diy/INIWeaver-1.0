
#include "FromEngine/Include.h"
#include "FromEngine/global_tool_func.h"
#include "IBBack.h"
#include "Global.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"

bool IBB_Section::Generate(const ModuleClipData& Clip)
{
    /*
    IsLinkGroup=true
    Desc
    DefaultLinkKey
    EqSize
    EqDelta
    VarList
    LinkTo
    */
    for (auto& L : Clip.VarList)VarList.Value[L.A] = L.B;
    if (Clip.IsLinkGroup)
    {
        IsLinkGroup = true;
        SubSecs.clear();
        Comment.clear();
        OnShow.clear();
        Inherit = Clip.Inherit;
        Register = Clip.Register;
        IBB_DefaultRegType::GenerateDLK(Clip.DefaultLinkKey, DefaultLinkKey);
        for (auto& L : Clip.LinkGroup_LinkTo)
            Root->Root->AddNewLinkToLinkGroup({ Root->Name,Name }, { L.A, L.B });
    }
    else
    {
        IsLinkGroup = false;
   /*
   IsLinkGroup=false
   IsComment=true
   Desc
   EqSize
   EqDelta
   Comment
   */
        if (Clip.IsComment)
        {
            SubSecs.clear();
            OnShow.clear();
            Inherit.clear();
            Register.clear();
            Comment = Clip.Comment;
        }
    /*
    IsLinkGroup=false
    IsComment=false
    Ignore
    Desc
    Lines
    Inherit
    Register
    DefaultLinkKey
    DisplayName
    EqSize
    EqDelta
    VarList
    */
        else
        {
            Comment.clear();
            OnShow.clear();
            LinkedBy.clear();
            LinkGroup_LinkTo.clear();
            Register = Clip.Register;
            Inherit = Clip.Inherit;
            IBB_DefaultRegType::GenerateDLK(Clip.DefaultLinkKey, DefaultLinkKey);
            IBB_VariableList VL;
            for (auto& L : Clip.Lines)
            {
                VL.Value[L.Key] = L.Value;
                OnShow[L.Key] = L.Desc;
            }
            GenerateLines(VL);
        }
    }
    return true;
}

void IBB_Section::GetClipData(ModuleClipData& Clip)
{
    /*
    IsLinkGroup=true
    Desc
    DefaultLinkKey
    EqSize
    EqDelta
    VarList
    LinkTo
    */
    if (IsLinkGroup)
    {
        Clip.IsLinkGroup = true;
        Clip.IsComment = false;
        Clip.Desc.A = Root->Name;
        Clip.Desc.B = Name;
        for (auto& [A, B] : DefaultLinkKey)
            Clip.DefaultLinkKey.push_back({ A, B });
        for (auto& L : LinkGroup_LinkTo)
            Clip.LinkGroup_LinkTo.push_back({ L.To.Ini.GetText(), L.To.Section.GetText() });
        for (auto& [A, B] : VarList.Value)
            Clip.VarList.push_back({ A, B });
    }
    else
    {
    /*
    IsLinkGroup=false
    IsComment=true
    Desc
    EqSize
    EqDelta
    Comment
    */
        if (IsComment())
        {
            Clip.IsLinkGroup = false;
            Clip.IsComment = true;
            Clip.Desc.A = Root->Name;
            Clip.Desc.B = Name;
            Clip.Comment = Comment;
        }
    /*
    IsLinkGroup=false
    IsComment=false
    Ignore
    Desc
    Lines
    Inherit
    Register
    DefaultLinkKey
    DisplayName
    EqSize
    EqDelta
    VarList
    */
        else
        {
            Clip.IsLinkGroup = false;
            Clip.IsComment = false;
            Clip.Desc.A = Root->Name;
            Clip.Desc.B = Name;
            Clip.Inherit = Inherit;
            Clip.Register = Register;
            for (auto& sec : SubSecs)
            {
                for (auto& [key, lin] : sec.Lines)
                {
                    Clip.Lines.emplace_back();
                    auto& Tok = Clip.Lines.back();
                    Tok.Empty = false;
                    Tok.HasDesc = !OnShow[key].empty();
                    Tok.IsSection = false;
                    Tok.Key = key;
                    Tok.Value = lin.Data->GetString();
                    Tok.Desc = OnShow[key];
                }
            }
            for (auto& [key, val] : UnknownLines.Value)
            {
                Clip.Lines.emplace_back();
                auto& Tok = Clip.Lines.back();
                Tok.Empty = false;
                Tok.HasDesc = !OnShow[key].empty();
                Tok.IsSection = false;
                Tok.Key = key;
                Tok.Value = val;
                Tok.Desc = OnShow[key];
            }
            for (auto& [A, B] : DefaultLinkKey)
                Clip.DefaultLinkKey.push_back({ A, B });
            for (auto& [A, B] : VarList.Value)
                Clip.VarList.push_back({ A, B });
        }
    }
}

bool IBB_Section::SetText(char* Text)
{
    return SetText(GetTokens(GetLines(Text)));
}

bool IBB_Section::SetText(const std::vector<IniToken>& Tokens)
{
    IsLinkGroup = false;
    bool Ret = true;
    SubSecs.clear();
    UnknownLines.Value.clear();
    std::unordered_map<IBB_SubSec_Default*, int> SubSecList;
    for (auto& tok : Tokens)
    {
        //if (tok.Value.empty())MessageBoxA(MainWindowHandle, tok.Key.c_str(), "fff1", MB_OK);
        if (tok.Empty || tok.IsSection)continue;
        auto ptr = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(tok.Key);
        if (ptr == nullptr)UnknownLines.Value[tok.Key] = tok.Value;
        else
        {
            auto It = SubSecList.find(ptr);
            if (It == SubSecList.end())
            {
                It = SubSecList.insert({ ptr,SubSecs.size() }).first;
                SubSecs.emplace_back(ptr, this);
            }
            auto& Sub = SubSecs.at(It->second);
            {
                if (!Sub.AddLine({tok.Key,tok.Value}))Ret = false;
            }
        }
    }
    return Ret;
}

std::vector<IBB_Link> IBB_Section::GetLinkTo() const
{
    if (IsLinkGroup)
    {
        return LinkGroup_LinkTo;
    }
    else
    {
        std::vector<IBB_Link> Ret;
        for (const auto& Sub : SubSecs)
            Ret.insert(Ret.end(), Sub.LinkTo.begin(), Sub.LinkTo.end());
        return Ret;
    }
}
std::vector<std::string> IBB_Section::GetKeys(bool PrintExtraData) const
{
    std::vector<std::string> Ret;
    if (IsLinkGroup)
    {
        for (const auto& L : LinkGroup_LinkTo)
        {
            auto pf = L.From.GetSec(*(Root->Root)), pt = L.To.GetSec(*(Root->Root));
            if (pf != nullptr && pt != nullptr)
                Ret.push_back("_LINK_FROM_" + pf->Name);
        }
        if (PrintExtraData)
        {
            Ret.push_back("_SECTION_NAME");
            Ret.push_back("_IS_LINKGROUP");
            IBB_VariableList VL;
            VarList.Flatten(VL);
            for (const auto& V : VL.Value)
                Ret.push_back(V.first);
        }
    }
    else
    {
        for (const auto& Sub : SubSecs)
        {
            auto K = Sub.GetKeys(PrintExtraData);
            Ret.insert(Ret.end(), K.begin(), K.end());
        }
        for (const auto& V : UnknownLines.Value)
            Ret.push_back(V.first);
        if (PrintExtraData)
        {
            Ret.push_back("_SECTION_NAME");
            Ret.push_back("_IS_LINKGROUP");
            IBB_VariableList VL;
            VarList.Flatten(VL);
            for (const auto& V : VL.Value)
                Ret.push_back(V.first);
            for (const auto& L : LinkedBy)
            {
                auto pf = L.From.GetSec(*(Root->Root)), pt = L.To.GetSec(*(Root->Root));
                if (pf != nullptr && pt != nullptr)
                    Ret.push_back("_LINK_FROM_" + pf->Name);
            }
        }
    }
    return Ret;
}
IBB_VariableList IBB_Section::GetLineList(bool PrintExtraData) const
{
    IBB_VariableList Ret;
    if (IsLinkGroup)
    {
        for (const auto& L : LinkGroup_LinkTo)
        {
            auto pf = L.From.GetSec(*(Root->Root)), pt = L.To.GetSec(*(Root->Root));
            if (pf != nullptr && pt != nullptr)
                Ret.Value["_LINK_FROM_" + pf->Name] = "_LINK_TO_" + pt->Name;
        }
        if (PrintExtraData)
        {
            Ret.Value["_SECTION_NAME"] = Name;
            Ret.Value["_IS_LINKGROUP"] = "true";
            IBB_VariableList VL;
            VarList.Flatten(VL);
            Ret.Merge(VL, false);
        }
    }
    else
    {
        for (const auto& Sub : SubSecs)
            Ret.Merge(Sub.GetLineList(PrintExtraData), false);
        Ret.Merge(UnknownLines, false);
        if (PrintExtraData)
        {
            Ret.Value["_SECTION_NAME"] = Name;
            Ret.Value["_IS_LINKGROUP"] = "false";
            IBB_VariableList VL;
            VarList.Flatten(VL);
            Ret.Merge(VL, false);
            for (auto L : LinkedBy)
            {
                auto pf = L.From.GetSec(*(Root->Root)), pt = L.To.GetSec(*(Root->Root));
                if (pf != nullptr && pt != nullptr)
                    Ret.Value["_LINK_FROM_" + pf->Name] = "_LINK_TO_" + pt->Name;
            }
        }
    }
    return Ret;
}
IBB_VariableList IBB_Section::GetSimpleLines() const
{
    IBB_VariableList Ret;
    if (!IsLinkGroup)
    {
        for (const auto& Sub : SubSecs)
            Ret.Merge(Sub.GetLineList(false), false);
        Ret.Merge(UnknownLines, false);
    }
    return Ret;
}
std::string IBB_Section::GetText(bool PrintExtraData, bool FromExport) const
{
    std::string Text;
    if (IsLinkGroup)
    {
        Text += ";LinkGroup - Links\n";
        for (const auto& L : LinkGroup_LinkTo)
        {
            auto pf = L.From.GetSec(*(Root->Root)), pt = L.To.GetSec(*(Root->Root));
            if (pf != nullptr && pt != nullptr)
            {
                Text += pf->Name;
                Text.push_back('=');
                Text += pt->Name;
                Text.push_back('\n');
            }
            else Text += "_ERROR=\n";
        }
        if (PrintExtraData)
        {
            Text += "\n;ExtraData - Name\n";
            Text += "_SECTION_NAME=" + Name;
            Text += "\n\n;ExtraData - IsLinkGroup\n_IS_LINKGROUP=true\n;ExtraData - Variables\n";
            Text += VarList.GetText(true);
        }
    }
    else
    {
        for (const auto& Sub : SubSecs)
            Text += Sub.GetText(PrintExtraData, FromExport);
        //Text.push_back('\n');
        Text += UnknownLines.GetText(false);
        if (PrintExtraData)
        {
            Text += "\n;ExtraData - Name\n";
            Text += "_SECTION_NAME=" + Name;
            Text += "\n\n;ExtraData - IsLinkGroup\n_IS_LINKGROUP=false\n;ExtraData - Variables\n";
            Text += VarList.GetText(true);
            Text += "\n;ExtraData - Linked By\n";
            for (const auto& L : LinkedBy)
                Text += L.GetText(*(Root->Root));
        }
    }
    return Text;
}

std::vector<size_t> IBB_Section::GetRegisteredPosition() const
{
    std::vector<size_t> Ret;
    auto& proj = *(Root->Root);
    for (size_t i = 0; i < proj.RegisterLists.size(); i++)
    {
        auto& li = proj.RegisterLists.at(i);
        for (size_t j = 0; j < li.List.size(); j++)
            if (li.List.at(j) == this) { Ret.push_back(i); break; }
    }
    return Ret;
}
std::vector<std::pair<size_t, size_t>> IBB_Section::GetRegisteredPositionAlt() const
{
    std::vector<std::pair<size_t, size_t>> Ret;
    auto& proj = *(Root->Root);
    for (size_t i = 0; i < proj.RegisterLists.size(); i++)
    {
        auto& li = proj.RegisterLists.at(i);
        for (size_t j = 0; j < li.List.size(); j++)
            if (li.List.at(j) == this) { Ret.push_back({ i,j }); break; }
    }
    return Ret;
}

IBB_Section_NameType IBB_Section::GetNameType() const
{
    IBB_Section_NameType Ret;
    Ret.IsLinkGroup = IsLinkGroup;
    Ret.IniType = Root->Name;
    Ret.Name = Name;
    Ret.VarList = VarList;
    Ret.Lines = GetSimpleLines();
    return Ret;
}

std::string IBB_Section::GetFullVariable(const std::string& _Name) const
{
    if (VarList.HasValue(_Name))return VarList.GetVariable(_Name);
    if (_Name == "_SECTION_NAME")return Name;
    if (UnknownLines.HasValue(_Name))return UnknownLines.GetVariable(_Name);
    for (const auto& ss : SubSecs)
    {
        auto u = ss.GetFullVariable(_Name);
        if (!u.empty())return u;
    }
    return "";
}

void IBB_Section::RedirectLinkAsDupicate()
{
    /*
    IBB_Ini* Root;

    std::string Name;
    std::vector<IBB_SubSec> SubSecs;
    std::vector<IBB_Link> LinkedBy;

    */
    //LinkedBy.clear();
    for (auto& [k, s] : VarList.Value)
    {
        auto it = IBR_Inst_Project.CopyTransform.find(s);
        if (it != IBR_Inst_Project.CopyTransform.end())
            s = it->second;
    }
    for (auto& [k, s] : UnknownLines.Value)
    {
        auto it = IBR_Inst_Project.CopyTransform.find(s);
        if (it != IBR_Inst_Project.CopyTransform.end())
            s = it->second;
    }
    for (auto& sub : SubSecs)
    {
        for (auto& lin : sub.Lines)
        {
            lin.second.Data->UpdateAsDuplicate();
        }
        //sub.LinkTo.clear();
    }
}

IBB_IniLine* IBB_Section::GetLineFromSubSecs(const std::string& KeyName) const
{
    if (SubSecs.empty())return nullptr;
    for (auto& Sub : SubSecs)
    {
        auto It = Sub.Lines.find(KeyName);
        if (It != Sub.Lines.end())
        {
            return const_cast<IBB_IniLine*>(std::addressof(It->second));
        }
    }
    return nullptr;
}

IBB_Project_Index IBB_Section::GetThisIndex() const
{
    return IBB_Project_Index{ Root->Name,Name };
}
IBB_Section_Desc IBB_Section::GetThisDesc() const
{
    return IBB_Section_Desc{ Root->Name,Name };
}

bool IBB_Section::GenerateLines(const IBB_VariableList& Par)
{
    bool Ret = true;
    SubSecs.clear();
    std::unordered_map<IBB_SubSec_Default*, int> SubSecList;
    for (const auto& L : Par.Value)
    {
        auto ptr = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(L.first);
        if (ptr == nullptr)UnknownLines.Value[L.first] = L.second;
        else
        {
            auto It = SubSecList.find(ptr);
            if (It == SubSecList.end())
            {
                It = SubSecList.insert({ ptr,SubSecs.size() }).first;
                SubSecs.emplace_back(ptr, this);
            }
            auto& Sub = SubSecs.at(It->second);
            if (!Sub.AddLine(L))Ret = false;
        }
    }
    return Ret;
}

bool IBB_Section::Generate(const IBB_Section_NameType& Par)
{
    Name = Par.Name;
    IsLinkGroup = Par.IsLinkGroup;
    VarList = Par.VarList;
    if (IsLinkGroup)
    {
        if (!Par.Lines.Value.empty())
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("IBB_Section::Generate ： 错误：Section作为一个LinkGroup却具有非空的Lines。");
            }
            return false;
        }
        else return true;
    }
    else
    {
        return GenerateLines(Par.Lines);
    }
}

bool IBB_Section::Merge(const IBB_Section& Another, const IBB_VariableList& MergeType, bool IsDuplicate)
{
    auto pproj = Root->Root; (void)pproj;
    if (IsLinkGroup)
    {
        LinkGroup_LinkTo.insert(LinkGroup_LinkTo.end(), Another.LinkGroup_LinkTo.begin(), Another.LinkGroup_LinkTo.end());
        return true;
    }
    else
    {
        UnknownLines.Merge(Another.UnknownLines, false);
        VarList.Merge(Another.VarList, false);
        for (const auto& ss : Another.SubSecs)
        {
            if (ss.Default == nullptr)continue;
            IBB_TDIndex<IBB_SubSec_Default*> idx(ss.Default);
            auto It = idx.Search<IBB_SubSec>(SubSecs, true, [](const IBB_SubSec& su)->IBB_SubSec_Default* {return su.Default; });
            if (It == SubSecs.end())
            {
                if (IsDuplicate)
                {
                    SubSecs.emplace_back(); SubSecs.back().GenerateAsDuplicate(ss); SubSecs.back().ChangeRoot(this);
                }
                else { SubSecs.push_back(ss); SubSecs.back().ChangeRoot(this); }
            }
            else It->Merge(ss, MergeType, IsDuplicate);
        }
        return true;
    }
    //LinkedBy : Refresh from Update !
}
bool IBB_Section::Merge(const IBB_Section& Another, const std::string& MergeType, bool IsDuplicate)
{
    auto pproj = Root->Root; (void)pproj;
    if (IsLinkGroup)
    {
        LinkGroup_LinkTo.insert(LinkGroup_LinkTo.end(), Another.LinkGroup_LinkTo.begin(), Another.LinkGroup_LinkTo.end());
        return true;
    }
    else
    {
        UnknownLines.Merge(Another.UnknownLines, false);
        VarList.Merge(Another.VarList, false);
        for (const auto& ss : Another.SubSecs)
        {
            if (ss.Default == nullptr)continue;
            IBB_TDIndex<IBB_SubSec_Default*> idx(ss.Default);
            auto It = idx.Search<IBB_SubSec>(SubSecs, true, [](const IBB_SubSec& su)->IBB_SubSec_Default* {return su.Default; });
            if (It == SubSecs.end())
            {
                if (IsDuplicate)
                {
                    SubSecs.emplace_back(); SubSecs.back().GenerateAsDuplicate(ss); SubSecs.back().ChangeRoot(this);
                }
                else { SubSecs.push_back(ss); SubSecs.back().ChangeRoot(this); }
            }
            else It->Merge(ss, MergeType, IsDuplicate);
        }
        return true;
    }
    //LinkedBy : Refresh from Update !
}

bool IBB_Section::GenerateAsDuplicate(const IBB_Section& Src)
{
    auto pproj = Root->Root;
    Merge(Src, "Replace", true);
    for (auto s : Src.GetRegisteredPosition())
    {
        pproj->RegisterSection(s, *this);
    }
    return true;
}

IBB_Section::IBB_Section(IBB_Section&& S) :
    Root(S.Root),
    IsLinkGroup(S.IsLinkGroup),
    Name(std::move(S.Name)),
    SubSecs(std::move(S.SubSecs)),
    LinkedBy(std::move(S.LinkedBy)),
    VarList(std::move(S.VarList)),
    UnknownLines(std::move(S.UnknownLines)),
    LinkGroup_LinkTo(std::move(S.LinkGroup_LinkTo))
{
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_Section::IBB_Section MOVE CTOR <- IBB_Section&& Src=%p(Name=%s)", &S, Name.c_str()); GlobalLogB.AddLog(LogBufB);
    }
    ChangeAddress();
}

bool IBB_Section::ChangeRoot(const IBB_Ini* NewRoot)
{
    Root = const_cast<IBB_Ini*>(NewRoot);
}

bool IBB_Section::Rename(const std::string& NewName)
{
    if (EnableLogEx)
    {
        GlobalLogB.AddLog_CurTime(false);
        sprintf_s(LogBufB, "IBB_Section::Rename (OldName=%s)<- std::string NewName=%s", Name.c_str(), NewName.c_str()); GlobalLogB.AddLog(LogBufB);
    }
    Name = NewName;
    if (IsLinkGroup)
    {
        for (auto& p : LinkGroup_LinkTo)
        {
            if (p.Another != nullptr)p.Another->From.Section.Assign(NewName);
            p.From.Section.Assign(NewName);
        }

        for (auto& p : LinkedBy)
        {
            if (p.Another != nullptr)p.Another->To.Section.Assign(NewName);
            p.To.Section.Assign(NewName);
        }
    }
    else
    {
        for (auto& s : SubSecs)
        {
            for (auto& p : s.LinkTo)
            {
                auto it = s.Lines.find(p.FromKey);
                if (it != s.Lines.end())
                {
                    if (it->second.Default != nullptr)
                    {
                        if (it->second.Default->Property.Type == "Link")
                            it->second.Data->SetValue(NewName);
                        //TODO:LinkList
                    }
                }
                if (p.Another != nullptr)p.Another->From.Section.Assign(NewName);
                p.From.Section.Assign(NewName);
            }
        }

        for (auto& p : LinkedBy)
        {
            if (p.Another != nullptr)p.Another->To.Section.Assign(NewName);
            p.To.Section.Assign(NewName);
        }
    }
    return true;
}

bool IBB_Section::ChangeAddress()
{
    bool Ret = true;
    for (auto& L : LinkedBy) if (!L.ChangeAddress())Ret = false;
    if (IsLinkGroup)
    {
        for (auto& L : LinkGroup_LinkTo)if (!L.ChangeAddress())Ret = false;
    }
    else
    {
        for (auto& ss : SubSecs) if (!ss.ChangeRoot(this))Ret = false;
    }
    return Ret;
}

bool IBB_Section::Isolate()
{
    auto pproj = Root->Root;
    //*
    for (auto& L : LinkedBy)
    {
        auto ps = L.From.GetSec(*pproj);
        if (L.Another != nullptr && ps != nullptr)
        {
            if (ps->IsLinkGroup)
            {
                if (L.Another->Order < ps->LinkGroup_LinkTo.size() - 1)
                {
                    ps->LinkGroup_LinkTo.at(L.Another->Order) = ps->LinkGroup_LinkTo.back();
                    ps->LinkGroup_LinkTo.pop_back();
                }
                if (L.Another->Order == ps->LinkGroup_LinkTo.size() - 1)
                {
                    ps->LinkGroup_LinkTo.pop_back();
                }
            }
            else
            {
                if (L.Another->OrderEx < ps->SubSecs.size())
                {
                    auto& ss = ps->SubSecs.at(L.Another->OrderEx);
                    auto it = ss.Lines.find(L.Another->FromKey);
                    if (it != ss.Lines.end())if (it->second.Default != nullptr)
                        it->second.Data->Clear();
                    if (L.Another->Order < ss.LinkTo.size() - 1)
                    {
                        ss.LinkTo.at(L.Another->Order) = ss.LinkTo.back();
                        ss.LinkTo.pop_back();
                    }
                    if (L.Another->Order == ss.LinkTo.size() - 1)
                    {
                        ss.LinkTo.pop_back();
                    }
                }

            }
        }
    }

    if (IsLinkGroup)
    {
        for (auto& L : LinkGroup_LinkTo)
        {
            auto ps = L.To.GetSec(*pproj);
            if (L.Another != nullptr && ps != nullptr)
            {
                if (L.Another->Order < ps->LinkedBy.size() - 1)
                {
                    ps->LinkedBy.back().Order = L.Another->Order;
                    ps->LinkedBy.at(L.Another->Order) = ps->LinkedBy.back();
                    ps->LinkedBy.pop_back();
                }
                else if (L.Another->Order == ps->LinkedBy.size() - 1)
                {
                    ps->LinkedBy.pop_back();
                }
            }
        }
    }
    else
    {
        //TODO
        for (auto& ss : SubSecs)
        {
            for (auto& L : ss.LinkTo)
            {
                auto ps = L.To.GetSec(*pproj);
                if (L.Another != nullptr && ps != nullptr)
                {
                    if (L.Another->Order < ps->LinkedBy.size() - 1)
                    {
                        ps->LinkedBy.back().Order = L.Another->Order;
                        ps->LinkedBy.at(L.Another->Order) = ps->LinkedBy.back();
                        ps->LinkedBy.pop_back();
                    }
                    else if (L.Another->Order == ps->LinkedBy.size() - 1)
                    {
                        ps->LinkedBy.pop_back();
                    }
                }
            }
        }
    }
    //*/
    return true;
}

bool IBB_Section::UpdateAll()
{
    bool Ret = true;
    auto PProj = Root->Root;
    if (IsLinkGroup)
    {
        std::vector<IBB_Link> NewGroup;
        NewGroup.reserve(LinkGroup_LinkTo.size());
        for (size_t i = 0; i < LinkGroup_LinkTo.size(); i++)
        {
            if (LinkGroup_LinkTo.at(i).To.GetSec(*PProj) != nullptr)NewGroup.push_back(LinkGroup_LinkTo.at(i));
        }
        for (auto& L : NewGroup)
        {
            L.FromKey.clear();
        }
        LinkGroup_LinkTo = NewGroup;
        if (LinkGroup_LinkTo.size() < (LinkGroup_LinkTo.capacity() >> 1))LinkGroup_LinkTo.shrink_to_fit();//TEST
    }
    else
    {
        for (auto& ss : SubSecs)
        {
            if (!ss.UpdateAll())Ret = false;
            for (auto& [K, V] : ss.Lines)
            {
                if (OnShow.find(K) == OnShow.end())
                    OnShow[K] = V.Default->IsLinkAlt() ? EmptyOnShowDesc : "";
            }
        }
    }
    return Ret;
}

