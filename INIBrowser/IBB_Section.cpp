
#include "FromEngine/Include.h"
#include "FromEngine/global_tool_func.h"

#include "Global.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include "IBG_InputType_Defines.h"
#include <ranges>

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
        LineOrder.clear();
        Inherit = Clip.Inherit;
        Register = NewPoolStr(Clip.Register);
        IBB_DefaultRegType::GenerateDLK(Clip.DefaultLinkKey, Register, DefaultLinkKey, DefaultLinkKey_UpValue);
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
            Register = EmptyPoolStr;
            LineOrder.clear();
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
            LineOrder.clear();
            LinkGroup_NewLinkTo.clear();
            Register = NewPoolStr(Clip.Register);
            Inherit = Clip.Inherit;
            IBB_DefaultRegType::GenerateDLK(Clip.DefaultLinkKey, Register, DefaultLinkKey, DefaultLinkKey_UpValue);
            std::vector<std::string> Order;

            SingleVal = IsTrueString(VarList.GetVariable("SingleVal"));
            VarList.Value.erase("SingleVal");

            SetText(Clip.Lines);
            for (auto& L : Clip.Lines)OnShow[NewPoolStr(L.Key)] = L.Desc;
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
        //IBB_VariableList DD;
        //DefaultLinkKey.Flatten(DD);
        for (auto& [A, B] : DefaultLinkKey)
            Clip.DefaultLinkKey.push_back({ PoolStr(A), PoolStr(B) });
        for (auto& L : LinkGroup_NewLinkTo)
            Clip.LinkGroup_LinkTo.push_back(L.To);
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
            Clip.Register = PoolStr(Register);
            std::unordered_map <std::string, std::vector<IniToken>> Tokens;
            for (auto& sec : SubSecs)
            {
                for (auto& [key, lin] : sec.Lines)
                {
                    if (key == InheritKeyID())
                    {
                        Clip.Inherit = lin.Indexed(0)->GetStringForExport();
                        continue;
                    }
                    auto& Toks = Tokens[PoolStr(key)];
                    lin.ForEach([&](LineData& L) {
                        IniToken Tok;
                        Tok.Empty = false;
                        Tok.HasDesc = !OnShow[key].empty();
                        Tok.IsSection = false;
                        Tok.Key = PoolStr(key);
                        Tok.Value = L->GetStringForExport();
                        Tok.Desc = OnShow[key];
                        Toks.push_back(std::move(Tok));
                    });
                }
            }
            for (auto& s : LineOrder)
            {
                if (auto It = Tokens.find(PoolStr(s)); It != Tokens.end())
                {
                    for(auto&& T : It->second)Clip.Lines.push_back(std::move(T));
                    Tokens.erase(It);
                }
            }
            for (auto& [key, tok] : Tokens)
            {
                for (auto&& T : tok)Clip.Lines.push_back(std::move(T));
            }

            //IBB_VariableList DD;
            //DefaultLinkKey.Flatten(DD);
            for (auto& [A, B] : DefaultLinkKey)
                Clip.DefaultLinkKey.push_back({ PoolStr(A), PoolStr(B) });
            for (auto& [A, B] : VarList.Value)
                Clip.VarList.push_back({ A, B });

            Clip.VarList.push_back({ "SingleVal", SingleVal ? "true" : "false" });
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
    auto l = this->GetLineFromSubSecs(InheritKeyID());
    if (l)Inherit = l->Indexed(0)->GetString();//继承暂时禁止可重
    SubSecs.clear();
    LineOrder.clear();
    OnShow.clear();
    std::unordered_map<IBB_SubSec_Default*, int> SubSecList;
    std::unordered_set<StrPoolID> UsedKeys;
    auto u = [&](const IniToken& tok) {
        //if (tok.Value.empty())MessageBoxA(MainWindowHandle, tok.Key.c_str(), "fff1", MB_OK);

        auto ptr = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(tok.Key);
        auto It = SubSecList.find(ptr);
        if (It == SubSecList.end())
        {
            It = SubSecList.insert({ ptr,SubSecs.size() }).first;
            SubSecs.emplace_back(ptr, this);
        }

        auto TokKeyID = NewPoolStr(tok.Key);

        auto& Sub = SubSecs.at(It->second);
        {
            if (!Sub.MergeLine(TokKeyID, Index_AlwaysNew, tok.Value, IBB_IniMergeMode::Replace, false))Ret = false;
        }

        if (tok.HasDesc)
        {
            if (tok.Desc.empty())OnShow[TokKeyID] = EmptyOnShowDesc;
            else OnShow[TokKeyID] = tok.Desc;
        }
        else OnShow.erase(TokKeyID);

        if (!UsedKeys.contains(TokKeyID))
        {
            LineOrder.push_back(TokKeyID);
            UsedKeys.insert(TokKeyID);
        }
    };

    {
        IniToken i;
        i.Key = InheritKeyName;
        i.Value = Inherit;
        i.HasDesc = false;
        i.IsSection = false;
        i.Empty = false;
        u(i);
    }

    if (SingleVal)
    {
        bool NoSingleVal = std::ranges::all_of(Tokens, [](const IniToken& Tok) { return Tok.Key != SingleValName; });
        if (NoSingleVal)
        {
            IniToken i;
            i.Key = SingleValName;
            i.Value = "";
            i.HasDesc = true;
            i.Desc = EmptyOnShowDesc;
            i.IsSection = false;
            i.Empty = false;
            u(i);
        }
    }

    for (auto& tok : Tokens)
    {
        if (tok.Empty || tok.IsSection)continue;
        u(tok);
    }

    

    UpdateAll();

    return Ret;
}

/*
bool IBB_Section::GenerateLines(const IBB_VariableList& Par, const std::vector<std::string>& Order, bool InitOnShow)
{
    bool Ret = true;
    SubSecs.clear();
    bool RemakeOrder = Order.empty();
    LineOrder = Order |
        std::views::transform([](auto ID) {return NewPoolStr(ID); }) |
        std::ranges::to<std::vector>();
    std::unordered_map<IBB_SubSec_Default*, int> SubSecList;
    for (const auto& [Key, Value] : Par.Value)
    {
        auto ptr = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(Key);
        auto It = SubSecList.find(ptr);
        if (It == SubSecList.end())
        {
            It = SubSecList.insert({ ptr,SubSecs.size() }).first;
            SubSecs.emplace_back(ptr, this);
        }
        auto& Sub = SubSecs.at(It->second);
        auto KeyID = NewPoolStr(Key);
        if (!Sub.MergeLine(KeyID, Value, InitOnShow, IBB_IniMergeMode::Replace))Ret = false;
        if (RemakeOrder)LineOrder.push_back(KeyID);
    }
    return Ret;
}
*/

std::vector<std::string> IBB_Section::GetLineOrderString() const
{
    return LineOrder |
        std::views::transform([](auto ID) { return PoolStr(ID); }) |
        std::ranges::to<std::vector>();
}

std::vector<StrPoolID> IBB_Section::GetKeys(bool PrintExtraData) const
{
    std::vector<StrPoolID> Ret;
    if (IsLinkGroup)
    {
        return Ret;
    }
    else
    {
        for (const auto& Sub : SubSecs)
        {
            auto K = Sub.GetKeys(PrintExtraData);
            Ret.insert(Ret.end(), K.begin(), K.end());
        }
        if (PrintExtraData)
        {
            Ret.push_back(NewPoolStr("_SECTION_NAME"));
            Ret.push_back(NewPoolStr("_IS_LINKGROUP"));
            IBB_VariableList VL;
            VarList.Flatten(VL);
            for (const auto& V : VL.Value)
                Ret.push_back(NewPoolStr(V.first));
        }
    }
    return Ret;
}
IBB_VariableMultiList IBB_Section::GetLineList(bool PrintExtraData, bool FromExport, std::vector<std::string>* TmpLineOrder) const
{
    IBB_VariableMultiList Ret;
    if (IsLinkGroup)
    {
        for (const auto& L : LinkGroup_NewLinkTo)
        {
            auto pf = L.From.GetSec(*(Root->Root)), pt = L.To.GetSec(*(Root->Root));
            if (pf != nullptr && pt != nullptr)
                Ret.Push("_LINK_FROM_" + pf->Name, "_LINK_TO_" + pt->Name);
        }
        if (PrintExtraData)
        {
            Ret.Push("_SECTION_NAME", Name);
            Ret.Push("_IS_LINKGROUP", "true");
            IBB_VariableList VL;
            VarList.Flatten(VL);
            Ret.Merge(VL, false);
        }
    }
    else
    {
        for (const auto& Sub : SubSecs)
            Ret.Merge(Sub.GetLineList(PrintExtraData, FromExport, TmpLineOrder));
        if (PrintExtraData)
        {
            Ret.Push("_SECTION_NAME", Name);
            Ret.Push("_IS_LINKGROUP", "false");
            IBB_VariableList VL;
            VarList.Flatten(VL);
            Ret.Merge(VL, false);
        }
    }

    if (FromExport)
        Ret.Value = Ret.Value |
        std::views::filter([&](auto& p) { return !p.second.empty(); }) |
        std::ranges::to<std::unordered_map>();

    return Ret;
}

std::string IBB_Section::GetText(bool PrintExtraData, bool FromExport, bool ForEdit) const
{
    std::string Text;
    if (IsLinkGroup)
    {
        auto LineList = GetLineList(PrintExtraData, FromExport);
        for (auto& [K, Vals] : LineList.Value)
        {
            for (auto& V : Vals)
                Text += K + "=" + V + "\n";
        }
    }
    else
    {
        auto TmpLineOrder = GetLineOrderString();
        auto LineList = GetLineList(PrintExtraData, FromExport, &TmpLineOrder);
        for (auto& s : TmpLineOrder)
        {
            if (LineList.HasValue(s))
            {
                auto& Vals = LineList.GetVars(s);
                for (auto& Val : Vals)
                {
                    if (ForEdit && OnShow.find(NewPoolStr(s)) != OnShow.end())
                    {
                        auto& ons = OnShow.at(NewPoolStr(s));
                        if (ons.empty()) Text += s + "=" + Val + "\n";
                        else if (ons == EmptyOnShowDesc) Text += "#" + s + "=" + Val + "\n";
                        else Text += ons + "#" + s + "=" + Val + "\n";
                    }
                    else Text += s + "=" + Val + "\n";
                    LineList.Value.erase(s);
                }
            }
        }
        for (auto& [K, Vals] : LineList.Value)
        {
            for (auto& V : Vals)
                Text += K + "=" + V + "\n";
        }
    }

    
    return Text;
}

std::string IBB_Section::GetTextForEdit() const
{
    std::string Ret;
    if (!GetLineFromSubSecs(InheritKeyID()) && !Inherit.empty())
    {
        Ret += InheritKeyName;
        Ret += "=";
        Ret += Inherit;
        Ret += "\n";
    }
    Ret += GetText(false, false, true);
    return Ret;
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


bool IBB_Section::HasLine(StrPoolID Key) const
{
    if (IsLinkGroup || IsComment()) return false;
    if (SubSecs.empty())return false;
    if (GetLineFromSubSecs(Key) != nullptr)return true;
    return false;
}

bool IBB_Section::IsOnShow(StrPoolID Key) const
{
    auto _F = this->OnShow.find(Key);
    if (_F == this->OnShow.end() || _F->second.empty())return false;
    else return true;
}

const std::string& IBB_Section::GetOnShow(StrPoolID Key) const
{
    const static std::string Empty{};
    auto _F = this->OnShow.find(Key);
    return _F == this->OnShow.end() ? Empty : _F->second;
}

StrPoolID IBB_Section::GetDLK(StrPoolID Reg) const
{
    if(DefaultLinkKey.contains(Reg))
        return DefaultLinkKey.at(Reg);
    if (DefaultLinkKey_UpValue->contains(Reg))
        return DefaultLinkKey_UpValue->at(Reg);

    if (Reg == Register && DefaultLinkKey.contains(MyTypeID()))
        return DefaultLinkKey.at(MyTypeID());
    if (Reg == Register && DefaultLinkKey_UpValue->contains(MyTypeID()))
        return DefaultLinkKey_UpValue->at(MyTypeID());

    if (DefaultLinkKey.contains(AnyTypeID()))
        return DefaultLinkKey.at(AnyTypeID());
    if (DefaultLinkKey_UpValue->contains(AnyTypeID()))
        return DefaultLinkKey_UpValue->at(AnyTypeID());

    return EmptyPoolStr;
}

void IBB_Section::SetOnShow(StrPoolID Key, const std::string& Value, bool AllowReapply)
{
    if (!IsOnShow(Key))OnShow[Key] = Value;
    else if(AllowReapply)OnShow[Key] = Value;
}

void IBB_Section::SetOnShow(StrPoolID Key)
{
    if (!IsOnShow(Key))OnShow[Key] = EmptyOnShowDesc;
}

void IBB_Section::PushLineOrder(StrPoolID Key)
{
    if (std::find(LineOrder.begin(), LineOrder.end(), Key) == LineOrder.end())
        LineOrder.push_back(Key);
}

void IBB_Section::RecheckLineOrder()
{
    std::unordered_set<StrPoolID> LineOrderKeys;
    std::vector<StrPoolID> NewLineOrder;
    for (auto& s : LineOrder)
    {
        if (LineOrderKeys.insert(s).second)
        {
            NewLineOrder.push_back(s);
        }
    }
    LineOrder = std::move(NewLineOrder);
}

const IBB_IniLine* IBB_Section::GetLineFromSubSecs(StrPoolID KeyName) const
{
    if (SubSecs.empty())return nullptr;
    for (auto& Sub : SubSecs)
    {
        auto It = Sub.Lines.find(KeyName);
        if (It != Sub.Lines.end())
        {
            return std::addressof(It->second);
        }
    }
    return nullptr;
}

IBB_IniLine* IBB_Section::GetLineFromSubSecs(StrPoolID KeyName)
{
    if (SubSecs.empty())return nullptr;
    for (auto& Sub : SubSecs)
    {
        auto It = Sub.Lines.find(KeyName);
        if (It != Sub.Lines.end())
        {
            return std::addressof(It->second);
        }
    }
    return nullptr;
}

std::pair <IBB_IniLine*, IBB_SubSec*> IBB_Section::GetLineFromSubSecsEx2(StrPoolID KeyName)
{
    if (SubSecs.empty())return { nullptr, nullptr };
    for (auto& Sub : SubSecs)
    {
        auto It = Sub.Lines.find(KeyName);
        if (It != Sub.Lines.end())
        {
            return {
                std::addressof(It->second),
                std::addressof(Sub)
            };
        }
    }
    return { nullptr, nullptr };
}

std::pair<IBB_IniLine*, size_t> IBB_Section::GetLineFromSubSecsEx(StrPoolID KeyName)
{
    if (SubSecs.empty())return { nullptr, 0 };
    for (auto& Sub : SubSecs)
    {
        auto It = Sub.Lines.find(KeyName);
        if (It != Sub.Lines.end())
        {
            return {
                std::addressof(It->second),
                std::ranges::find(Sub.Lines_ByName, KeyName) - Sub.Lines_ByName.begin()
            };
        }
    }
    return { nullptr, 0 };
}

IBB_Project_Index IBB_Section::GetThisIndex() const
{
    return IBB_Project_Index{ Root->Name,Name };
}
IBB_Section_Desc IBB_Section::GetThisDesc() const
{
    return IBB_Section_Desc{ Root->Name,Name };
}
IBB_SectionID IBB_Section::GetThisID() const
{
    return ID;
}

std::vector<IBB_NewLink>& IBB_Section::GetLinkedBy_Cached() const
{
    return IBF_Inst_Project.GetLinkedBy_Cached(GetThisDesc());
}

std::vector<IBB_NewLink>& IBB_Section::GetLinkedBy_NoCached() const
{
    return IBF_Inst_Project.GetLinkedBy_NoCached(GetThisDesc());
}


const std::vector<std::string>& SplitParamCached(const std::string& Text);

//返回Def对应的SubSec，若没有则构造一个新的SubSec并返回
IBB_SubSec& IBB_Section::GetSubSecByDef(IBB_SubSec_Default* Def)
{
    auto It = std::find_if(SubSecs.begin(), SubSecs.end(), [&](const IBB_SubSec& su) {return su.Default == Def; });
    if (It == SubSecs.end())
    {
        SubSecs.emplace_back(Def, this);
        It = std::prev(SubSecs.end());
    }
    auto& Sub = *It;
    return Sub;
}

//返回包含Key的SubSec，若没有则构造一个新的SubSec并返回
IBB_SubSec& IBB_Section::GetSubSecByLine(const std::string& Key)
{
    auto ptr = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(Key);
    return GetSubSecByDef(ptr);
}

bool IBB_Section::RemoveLine(StrPoolID Key)
{
    auto [pLine, pSub] = GetLineFromSubSecsEx2(Key);
    if (!pLine || !pSub)return false;
    OnShow.erase(Key);
    LineOrder.erase(std::remove(LineOrder.begin(), LineOrder.end(), Key), LineOrder.end());
    auto& LB = pSub->Lines_ByName;
    LB.erase(std::remove(LB.begin(), LB.end(), Key), LB.end());
    pSub->Lines.erase(Key);
    return UpdateAll();
}

bool IBB_Section::MergeLine(StrPoolID Key, size_t Index, const std::string& Value, IBB_IniMergeMode Mode, bool NoUpdate)
{
    switch (Mode)
    {
    case IBB_IniMergeMode::Replace:
    {
        auto ptr = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(Key);
        PushLineOrder(Key);
        auto& Sub = GetSubSecByDef(ptr);
        return Sub.MergeLine(Key, Index, Value, IBB_IniMergeMode::Replace, NoUpdate);
    }
    case IBB_IniMergeMode::Merge:
    {
        auto ptr = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(Key);
        const auto MergeVal = [](const std::string & Orig, const std::string & Value)->std::string
        {
            auto & pv = SplitParamCached(Orig);
            bool Unique = std::ranges::all_of(pv, [&](const auto& elem) { return elem != Value; });
            auto str = pv |
                std::views::join_with(',') |
                std::ranges::to<std::string>();
            if (Unique)
            {
                if (!str.empty())str += ",";
                str += Value;
            }
            return str;
        };
        PushLineOrder(Key);
        auto& Sub = GetSubSecByDef(ptr);
        auto LineIt = Sub.Lines.find(Key);
        std::string NewVal;
        if (LineIt == Sub.Lines.end())NewVal = Value;
        else NewVal = MergeVal(LineIt->second.Indexed(Index)->GetString(), Value);
        return Sub.MergeLine(Key, Index, NewVal, IBB_IniMergeMode::Replace, NoUpdate);
        return true;
    }
    case IBB_IniMergeMode::Reserve:
    {
        if (HasLine(Key))return true;
        return MergeLine(Key, Index, Value, IBB_IniMergeMode::Replace, NoUpdate);
    }
    }

    std::unreachable();
}



void IBB_Section::OrderKey(StrPoolID Key, size_t NewOrder)
{
    if (NewOrder > LineOrder.size())NewOrder = LineOrder.size();
    auto it = std::find(LineOrder.begin(), LineOrder.end(), Key);
    if (it != LineOrder.end())
    {
        LineOrder.erase(it);
        LineOrder.insert(LineOrder.begin() + NewOrder, Key);
    }
}

void IBB_Section::CheckSubsecOrder()
{
    if (this->SubSecOrder.size() != this->SubSecs.size() + 1)
    {
        this->SubSecOrder.clear();
        for (size_t i = 0; i < this->SubSecs.size(); i++)this->SubSecOrder.push_back(i);
        std::ranges::sort(this->SubSecOrder, [&](size_t a, size_t b) {
            return this->SubSecs[a].Default->Name < this->SubSecs[b].Default->Name;
            });
    }
}

IBB_Section::IBB_Section(const std::string& N, IBB_Ini* R) :
    Name(N),
    Root(R),
    IsLinkGroup(false),
    DefaultLinkKey_UpValue(nullptr),
    ID(Root->Name, N)
{
    this->VarList.Value["_InitialSecName"] = N;
}


IBB_Section::IBB_Section(IBB_Section&& S) noexcept :
    Root(S.Root),
    IsLinkGroup(S.IsLinkGroup),
    Name(std::move(S.Name)),
    SubSecs(std::move(S.SubSecs)),
    VarList(std::move(S.VarList)),
    LinkGroup_NewLinkTo(std::move(S.LinkGroup_NewLinkTo)),
    LineOrder(std::move(S.LineOrder)),
    ID(S.ID),
    DefaultLinkKey(std::move(S.DefaultLinkKey)),
    DefaultLinkKey_UpValue(S.DefaultLinkKey_UpValue)
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
    ID = IBB_SectionID{ Root->Name,Name };
    return true;
}

bool IBB_Section::AcceptNewNameInLinkTo(IBB_SectionID NewID, IBB_Section* Target, const std::string& NewName)
{
    //现在this的有些Link的To指向Target，处理他们。
    //同时，修改里面的值。
    //只能由Rename调用，故Target非空，且非自连。
    auto& Proj = *Root->Root;
    bool Ret = true;
    if (IsLinkGroup)
    {
        //直接修改链接本身
        for (auto& Link : LinkGroup_NewLinkTo)
            if (Link.To.GetSec(Proj) == Target)
                Link.To = { Link.To.Ini(), NewName };
    }
    else
    {
        for (auto& Sub : SubSecs)
            for (auto&& [idx, Link] : std::views::zip(std::views::iota(0u), Sub.NewLinkTo))
                if (Link.To.GetSec(Proj) == Target)
                {
                    //按照这个Link，找到所有from的地方并修改to到新name
                    Ret &= Sub.RenameInLinkTo(idx, Target->Name, NewName);
                    //然后修改链接本身
                    Link.To = NewID;
                }
    }
    return Ret;
}

bool IBB_Section::RemoveNameInLinkTo(IBB_Section* Target)
{
    //现在this的有些Link的To指向Target，处理他们。
    //同时，清除里面的值。
    //只能由Isolate调用，故Target非空，且非自连。
    auto& Proj = *Root->Root;
    bool Ret = true;
    if (IsLinkGroup)
    {
        std::vector<IBB_NewLink> NL;
        //直接修改链接本身
        for (auto& Link : LinkGroup_NewLinkTo)
            if (Link.To.GetSec(Proj) != Target)
                NL.push_back(Link);
    }
    else
    {
        for (auto& Sub : SubSecs)
        {
            std::vector<IBB_NewLink> NL;
            std::map<size_t, size_t> OldToNew;
            for (auto&& [idx, Link] : std::views::zip(std::views::iota(0u), Sub.NewLinkTo))
                if (Link.To.GetSec(Proj) == Target)
                    //按照这个Link，找到所有from的地方并修改to到新name
                    Ret &= Sub.RenameInLinkTo(idx, Target->Name, "");
                else if(!Link.Empty())
                {
                    NL.push_back(Link);
                    OldToNew[idx] = NL.size() - 1;
                }
            Sub.UpdateNewLinkTo(std::move(NL));
            Sub.LinkSrc = Sub.LinkSrc |
                std::views::filter([&](auto& p) { return OldToNew.contains(p.second); }) |
                std::views::transform([&](auto& p) { return std::make_pair(p.first, OldToNew[p.second]); }) |
                std::ranges::to<std::multimap>();
        }
    }
    return Ret;
}

bool IBB_Section::AcceptNewNameInLinkedBy(IBB_SectionID OldIndex, const std::string& NewName)
{
    //LinkedBy已经无辣！
    IM_UNUSED(OldIndex);
    IM_UNUSED(NewName);
    return true;
}

bool IBB_Section::Rename(const std::string& NewName)
{
    bool Ret = true;
    auto& Proj = *Root->Root;
    IBB_SectionID NewID = IBB_SectionID(Root->Name, NewName);
    if (IsLinkGroup)
    {
        for (auto& Link : LinkGroup_NewLinkTo)
        {
            //找到连到的对象，改他们linkedby
            Ret &= AcceptNewNameInLinkedBy(Link.To, NewName);
            //然后修改链接本身From
            Link.From = NewID;
            //自连则也要修改To
            if (Link.To == ID)
                Link.To = NewID;
        }
    }
    else
    {
        for (auto& Sub : SubSecs)
            for (auto&& [idx, Link] : std::views::zip(std::views::iota(0u), Sub.NewLinkTo))
            {
                //按照这个Link，找到所有from的地方并修改to到新name
                Ret &= Sub.RenameInLinkTo(idx, Name, NewName);
                //找到连到的对象，改他们linkedby
                Ret &= AcceptNewNameInLinkedBy(Link.To, NewName);
                //然后修改链接本身
                Link.From = NewID;
                //自连则也要修改To
                if (Link.To == ID)
                    Link.To = NewID;
            }
    }
    for (auto& Link : GetLinkedBy_NoCached())
    {
        //按照From追溯到来源
        auto Src = Link.From.GetSec(Proj);
        //空挂则跳过
        if (!Src)
            continue;
        //自连则也要修改From
        else if (Src == this)
            Link.From = NewID;
        //非自连则修改LinkTo的情况
        else
            Ret &= Src->AcceptNewNameInLinkTo(NewID, this, NewName);
        //然后修改链接本身
        Link.To = NewID;
        
    }
    Name = NewName;
    ID = NewID;
    return Ret;
}

bool IBB_Section::ChangeAddress()
{
    bool Ret = true;
    for (auto& ss : SubSecs) if (!ss.ChangeRoot(this))Ret = false;
    return Ret;
}

bool IBB_Section::Isolate()
{
    auto& Proj = *Root->Root;
    bool Ret = true;
    for (auto& Link : GetLinkedBy_NoCached())
    {
        //按照From追溯到来源
        auto Src = Link.From.GetSec(Proj);
        if (Src && Src != this)
            Ret &= Src->RemoveNameInLinkTo(this);
    }
    return Ret;
}

bool IBB_Section::UpdateAll()
{
    bool Ret = true;
    auto PProj = Root->Root;
    if (IsLinkGroup)
    {
        std::vector<IBB_NewLink> NewGroup;
        NewGroup.reserve(LinkGroup_NewLinkTo.size());
        for (size_t i = 0; i < LinkGroup_NewLinkTo.size(); i++)
        {
            auto& NL = LinkGroup_NewLinkTo.at(i);
            if (NL.To.GetSec(*PProj) != nullptr)NewGroup.push_back(NL);
        }
        for (auto& L : NewGroup)
            L.FromKey = EmptyPoolStr;
        LinkGroup_NewLinkTo = NewGroup;
    }
    else
    {
        for (auto& ss : SubSecs)
        {
            ss.Root = this;
            if (!ss.UpdateAll())Ret = false;
        }

        Ret &= UpdateLineOrder();

        //移除不存在的键的OnShow
        OnShow = GetKeys(false) |
            std::views::transform([&](auto&& k) {return std::make_pair(k, OnShow.contains(k) ? OnShow.at(k) : ""); }) |
            std::views::filter([&](auto&& s) {return !s.second.empty(); }) |
            std::ranges::to<std::unordered_map>();
    }
    return Ret;
}

bool IBB_Section::UpdateLineOrder()
{
    if (IsLinkGroup)return false;
    if (IsComment())return true;

    //update LineOrder
    //add new lines and remove deleted lines
    std::unordered_set<StrPoolID> NewLineOrder;
    for (const auto& ss : SubSecs)
    {
        for (const auto& [K, V] : ss.Lines)
            if (!NewLineOrder.contains(K))NewLineOrder.insert(K);
    }

    // remove deleted lines
    for (auto it = LineOrder.begin(); it != LineOrder.end(); )
    {
        if (!NewLineOrder.contains(*it))
            it = LineOrder.erase(it);
        else
            ++it;
    }
    // add new lines to the back
    for (auto& K : LineOrder)
        NewLineOrder.erase(K);
    for (const auto& K : NewLineOrder)
        LineOrder.push_back(K);

    return true;
}
