
#include "FromEngine/Include.h"
#include "FromEngine/global_tool_func.h"

#include "Global.h"
#include "IBB_ModuleAlt.h"
#include "IBB_RegType.h"
#include "IBG_InputType.h"
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
        Register = Clip.Register;
        IBB_DefaultRegType::GenerateDLK(Clip.DefaultLinkKey, Register, DefaultLinkKey);
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
            LinkedBy.clear();
            LineOrder.clear();
            LinkGroup_LinkTo.clear();
            Register = Clip.Register;
            Inherit = Clip.Inherit;
            IBB_DefaultRegType::GenerateDLK(Clip.DefaultLinkKey, Register, DefaultLinkKey);
            IBB_VariableList VL;
            std::vector<std::string> Order;
            VL.Value[InheritKeyName] = Inherit;
            OnShow[InheritKeyName] = "\n";
            for (auto& L : Clip.Lines)
            {
                VL.Value[L.Key] = L.Value;
                OnShow[L.Key] = L.Desc;
                std::string dsc;
                Order.push_back(L.Key); 
            }
            
            GenerateLines(VL, Order, false);
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
        for (auto& [A, B] : DefaultLinkKey.Value)
            Clip.DefaultLinkKey.push_back({ A, B });
        for (auto& L : LinkGroup_LinkTo)
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
            Clip.Register = Register;
            std::unordered_map <std::string, IniToken> Tokens;
            for (auto& sec : SubSecs)
            {
                for (auto& [key, lin] : sec.Lines)
                {
                    if (key == InheritKeyName)
                    {
                        Clip.Inherit = lin.Data->GetStringForExport();
                        continue;
                    }
                    auto& Tok = Tokens[key];
                    Tok.Empty = false;
                    Tok.HasDesc = !OnShow[key].empty();
                    Tok.IsSection = false;
                    Tok.Key = key;
                    Tok.Value = lin.Data->GetStringForExport();
                    Tok.Desc = OnShow[key];
                }
            }
            for (auto& [key, val] : UnknownLines.Value)
            {
                auto& Tok = Tokens[key];
                Tok.Empty = false;
                Tok.HasDesc = !OnShow[key].empty();
                Tok.IsSection = false;
                Tok.Key = key;
                Tok.Value = val;
                Tok.Desc = OnShow[key];
            }
            for (auto& s : LineOrder)
            {
                if (auto It = Tokens.find(s); It != Tokens.end())
                {
                    Clip.Lines.push_back(std::move(It->second));
                    Tokens.erase(It);
                }
            }
            for (auto& [key, tok] : Tokens)
            {
                Clip.Lines.push_back(std::move(tok));
            }

            //IBB_VariableList DD;
            //DefaultLinkKey.Flatten(DD);
            for (auto& [A, B] : DefaultLinkKey.Value)
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
    auto l = this->GetLineFromSubSecs(InheritKeyName);
    if (l)Inherit = l->Data->GetString();
    SubSecs.clear();
    LineOrder.clear();
    OnShow.clear();
    UnknownLines.Value.clear();
    std::unordered_map<IBB_SubSec_Default*, int> SubSecList;
    auto u = [&](const IniToken& tok) {
        //if (tok.Value.empty())MessageBoxA(MainWindowHandle, tok.Key.c_str(), "fff1", MB_OK);

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
                if (!Sub.AddLine({ tok.Key,tok.Value }, false, IBB_IniMergeMode::Replace))Ret = false;
                if (tok.HasDesc)
                {
                    if (tok.Desc.empty())OnShow[tok.Key] = EmptyOnShowDesc;
                    else OnShow[tok.Key] = tok.Desc;
                }
                else OnShow.erase(tok.Key);
            }
        }
        LineOrder.push_back(tok.Key);
    };

    IniToken i;
    i.Key = InheritKeyName;
    i.Value = Inherit;
    i.HasDesc = false;
    i.IsSection = false;
    i.Empty = false;
    u(i);
    for (auto& tok : Tokens)
    {
        if (tok.Empty || tok.IsSection)continue;
        u(tok);
    }

    return Ret;
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
IBB_VariableList IBB_Section::GetLineList(bool PrintExtraData, bool FromExport) const
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
            Ret.Merge(Sub.GetLineList(PrintExtraData, FromExport), false);
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
            Ret.Merge(Sub.GetLineList(false, false), false);
        Ret.Merge(UnknownLines, false);
    }
    return Ret;
}

std::string IBB_Section::GetText(bool PrintExtraData, bool FromExport, bool ForEdit) const
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
            Text += VarList.GetText(true, FromExport);
        }
    }
    else
    {
        auto LineList = GetLineList(PrintExtraData, FromExport);
        for (auto& s : LineOrder)
        {
            if (LineList.HasValue(s))
            {
                if (ForEdit && OnShow.find(s) != OnShow.end())
                {
                    auto& ons = OnShow.at(s);
                    if (ons.empty()) Text += s + "=" + LineList.GetVariable(s) + "\n";
                    else if (ons == EmptyOnShowDesc) Text += "#" + s + "=" + LineList.GetVariable(s) + "\n";
                    else Text += ons + "#" + s + "=" + LineList.GetVariable(s) + "\n";
                }
                else Text += s + "=" + LineList.GetVariable(s) + "\n";
                LineList.Value.erase(s);
            }
        }
        for (auto& [K, V] : LineList.Value)
        {
            Text += K + "=" + V + "\n";
        }
    }

    
    return Text;
}

std::string IBB_Section::GetTextForEdit() const
{
    std::string Ret;
    if (!Inherit.empty())
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

IIFWrapper_Wrapper IBB_Section::GetLineIIF(const std::string& Key) const
{
    auto NewIIF = [&] () -> IIFWrapper {
        auto pLine = GetLineFromSubSecs(Key);
        if (pLine)
        {
            if (pLine->Default)
            {
                auto& piif = pLine->Default->GetInputType().Sidebar;
                if (piif) {
                    auto dup = piif->Duplicate();
                    dup->ParseFromString(pLine->Data->GetString());
                    return dup;
                }
                else return std::monostate{};
            }
            else return std::monostate{};
        }
        else if (UnknownLines.HasValue(Key))
        {
            auto& Value = UnknownLines.GetVariable(Key);
            auto& piif = IBB_DefaultRegType::SelectInputTypeByValue(Value).Sidebar;
            if (piif) {
                auto dup = piif->Duplicate();
                dup->ParseFromString(Value);
                return dup;
            }
            else return std::monostate{};
        }
        else return std::monostate{};
    };

    auto pbk = IBR_EditFrame::CurSection.GetBack();
    if (pbk == this)
    {
        auto it = IBR_EditFrame::EditLines.find(Key);
        if (it == IBR_EditFrame::EditLines.end())
            return { std::monostate{} };// FUCK YOU IF IT HAPPENS
        if (it->second.Edit.Input)
            return { &(it->second.Edit.Input->Form) };
        else
            return { NewIIF() };
    }
    auto Rsec = IBR_Inst_Project.GetSection(GetThisDesc());
    auto pSD = Rsec.GetSectionData();
    if (pSD)
    {
        auto it = pSD->ActiveLines.find(Key);
        if (it == pSD->ActiveLines.end())
            return { NewIIF() };
        if (it->second.Edit.Input)
            return { &(it->second.Edit.Input->Form) };
        else
            return { NewIIF() };
    }
    return { NewIIF() };
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

bool IBB_Section::HasLine(const std::string& Key) const
{
    if (IsLinkGroup || IsComment()) return false;
    if (UnknownLines.HasValue(Key))return true;
    if (SubSecs.empty())return false;
    if (GetLineFromSubSecs(Key) != nullptr)return true;
    return false;
}

bool IBB_Section::IsOnShow(const std::string& Key) const
{
    auto _F = this->OnShow.find(Key);
    if (_F != this->OnShow.end() && _F->second.empty())return false;
    else return true;
}

const std::string& IBB_Section::GetOnShow(const std::string& Key) const
{
    const static std::string Empty{};
    auto _F = this->OnShow.find(Key);
    return _F == this->OnShow.end() ? Empty : _F->second;
}

const IBB_IniLine* IBB_Section::GetLineFromSubSecs(const std::string& KeyName) const
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

IBB_IniLine* IBB_Section::GetLineFromSubSecs(const std::string& KeyName)
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

std::pair <IBB_IniLine*, IBB_SubSec*> IBB_Section::GetLineFromSubSecsEx2(const std::string& KeyName)
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

std::pair<IBB_IniLine*, size_t> IBB_Section::GetLineFromSubSecsEx(const std::string& KeyName)
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

bool IBB_Section::GenerateLines(const IBB_VariableList& Par, const std::vector<std::string>& Order, bool InitOnShow)
{
    bool Ret = true;
    SubSecs.clear();
    bool RemakeOrder = Order.empty();
    LineOrder = Order;
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
            if (!Sub.AddLine(L, InitOnShow, IBB_IniMergeMode::Replace))Ret = false;
            if (RemakeOrder)LineOrder.push_back(L.first);
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
                GlobalLogB.AddLog((u8"IBB_Section::Generate ： " + loc("Log_GenerateInvalidLines")).c_str());
            }
            return false;
        }
        else return true;
    }
    else
    {
        return GenerateLines(Par.Lines, {}, true);
    }
}

bool IBB_Section::Merge(const IBB_Section& Another, const std::unordered_map<std::string, IBB_IniMergeMode>& MergeType, bool IsDuplicate)
{
    auto pproj = Root->Root; (void)pproj;
    if (IsLinkGroup)
    {
        LinkGroup_LinkTo.insert(LinkGroup_LinkTo.end(), Another.LinkGroup_LinkTo.begin(), Another.LinkGroup_LinkTo.end());
        return true;
    }
    else
    {
        for (const auto& key : Another.LineOrder)
        {
            if (std::find(LineOrder.begin(), LineOrder.end(), key) == LineOrder.end())
                LineOrder.push_back(key);
        }

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
bool IBB_Section::Merge(const IBB_Section& Another, IBB_IniMergeMode MergeType, bool IsDuplicate)
{
    auto pproj = Root->Root; (void)pproj;
    if (IsLinkGroup)
    {
        LinkGroup_LinkTo.insert(LinkGroup_LinkTo.end(), Another.LinkGroup_LinkTo.begin(), Another.LinkGroup_LinkTo.end());
        return true;
    }
    else
    {
        for (const auto& key : Another.LineOrder)
        {
            if (std::find(LineOrder.begin(), LineOrder.end(), key) == LineOrder.end())
                LineOrder.push_back(key);
        }

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

const std::vector<std::string>& SplitParamCached(const std::string& Text);

bool IBB_Section::MergeLine(const std::string& Key, const std::string& Value, IBB_IniMergeMode Mode)
{
    switch (Mode)
    {
    case IBB_IniMergeMode::Replace:
    {
        auto ptr = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(Key);
        bool Unique = std::ranges::all_of(LineOrder, [&](const auto& elem) { return elem != Key; });
        if (Unique)LineOrder.push_back(Key);
        if (ptr == nullptr)
        {
            UnknownLines.Value[Key] = Value;
            IBRF_CoreBump.SendToR({ [=] {IBR_EditFrame::UpdateLine(Key, Value); } });
            return true;
        }
        else
        {
            auto It = std::find_if(SubSecs.begin(), SubSecs.end(), [&](const IBB_SubSec& su) {return su.Default == ptr; });
            if (It == SubSecs.end())
            {
                SubSecs.emplace_back(ptr, this);
                It = std::prev(SubSecs.end());
            }
            auto& Sub = *It;
            IBRF_CoreBump.SendToR({ [=] {IBR_EditFrame::UpdateLine(Key, Value); } });
            return Sub.AddLine({ Key, Value }, true, IBB_IniMergeMode::Replace);
        }
    }
    case IBB_IniMergeMode::Merge:
    {
        auto ptr = IBF_Inst_DefaultTypeList.List.KeyBelongToSubSec(Key);
        bool Unique = std::ranges::all_of(LineOrder, [&](const auto& elem) { return elem != Key; });
        if (Unique)LineOrder.push_back(Key);

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

        if (ptr == nullptr)
        {
            UnknownLines.Value[Key] = MergeVal(UnknownLines.Value[Key], Value);
            IBRF_CoreBump.SendToR({ [=] {IBR_EditFrame::UpdateLine(Key, Value); } });
            return true;
        }
        else
        {
            auto It = std::find_if(SubSecs.begin(), SubSecs.end(), [&](const IBB_SubSec& su) {return su.Default == ptr; });
            if (It == SubSecs.end())
            {
                SubSecs.emplace_back(ptr, this);
                It = std::prev(SubSecs.end());
            }
            auto& Sub = *It;
            auto LineIt = Sub.Lines.find(Key);
            std::string NewVal;
            if (LineIt == Sub.Lines.end())NewVal = Value;
            else NewVal = MergeVal(LineIt->second.Data->GetString(), Value);
            IBRF_CoreBump.SendToR({ [=] {IBR_EditFrame::UpdateLine(Key, NewVal); } });
            return Sub.AddLine({ Key, NewVal }, true, IBB_IniMergeMode::Replace);
        }
        return true;
    }
    case IBB_IniMergeMode::Reserve:
    {
        if (HasLine(Key))return true;
        return MergeLine(Key, Value, IBB_IniMergeMode::Replace);
    }
    }

    std::unreachable();
}


bool IBB_Section::GenerateAsDuplicate(const IBB_Section& Src)
{
    auto pproj = Root->Root;
    Merge(Src, IBB_IniMergeMode::Replace, true);
    for (auto s : Src.GetRegisteredPosition())
    {
        pproj->RegisterSection(s, *this);
    }
    return true;
}

void IBB_Section::OrderKey(const std::string& Key, size_t NewOrder)
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
    if (this->SubSecOrder.size() != this->SubSecs.size())
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
    IsLinkGroup(false)
{
    this->VarList.Value["_InitialSecName"] = N;
}


IBB_Section::IBB_Section(IBB_Section&& S) noexcept :
    Root(S.Root),
    IsLinkGroup(S.IsLinkGroup),
    Name(std::move(S.Name)),
    SubSecs(std::move(S.SubSecs)),
    LinkedBy(std::move(S.LinkedBy)),
    VarList(std::move(S.VarList)),
    UnknownLines(std::move(S.UnknownLines)),
    LinkGroup_LinkTo(std::move(S.LinkGroup_LinkTo)),
    LineOrder(std::move(S.LineOrder))
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
    auto OldName = Name;
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
                if (p.Another != nullptr)p.Another->From.Section.Assign(NewName);
                p.From.Section.Assign(NewName);
            }
        }

        for (auto& p : LinkedBy)
        {
            if (p.Another != nullptr)p.Another->To.Section.Assign(NewName);
            p.To.Section.Assign(NewName);
            auto U = p.From.GetSec(*Root->Root);
            if (U && !U->IsLinkGroup)
            {
                for (auto& sub : U->SubSecs)
                {
                    for (auto& [N, L] : sub.Lines)
                    {
                        auto q = L.GetData<IBB_IniLine_DataList>();
                        if(q)q->ReplaceValue(OldName, NewName);
                        else L.Data->SetValue(NewName);
                    }
                }
            }
        }

        for (auto& sub : SubSecs)
        {
            for (auto& [N, L] : sub.Lines)
            {
                auto q = L.GetData<IBB_IniLine_DataList>();
                if(q)q->ReplaceValue(OldName, NewName);
                else L.Data->SetValue(NewName);
            }
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
                    {
                        //it->second.Data->Clear();
                        auto p = it->second.GetData<IBB_IniLine_DataList>();
                        if (p)p->RemoveValue(Name);
                        else p->Clear();
                    }
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
    /*
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
            ss.Root = this;
            if (!ss.UpdateAll())Ret = false;
            /*
            for (auto& [K, V] : ss.Lines)
            {
                if (OnShow.find(K) == OnShow.end())
                    OnShow[K] = V.Default->IsLinkAlt() ? EmptyOnShowDesc : "";
            }
            */
        }

        Ret &= UpdateLineOrder();
    }
    return Ret;
}

bool IBB_Section::UpdateLineOrder()
{
    if (IsLinkGroup)return false;
    if (IsComment())return true;

    //update LineOrder
    //add new lines and remove deleted lines
    std::unordered_set<std::string> NewLineOrder;
    for (const auto& ss : SubSecs)
    {
        for (const auto& [K, V] : ss.Lines)
            if (!NewLineOrder.contains(K))NewLineOrder.insert(K);
    }
    for (const auto& [K, V] : UnknownLines.Value)
    {
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
    for (const auto& K : NewLineOrder)
    {
        if (std::find(LineOrder.begin(), LineOrder.end(), K) == LineOrder.end())
            LineOrder.push_back(K);
    }

    return true;
}
