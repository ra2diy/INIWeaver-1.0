#include "IBB_RegType.h"
#include "Global.h"
#include "IBB_ModuleAlt.h"
#include "IBB_Index.h"
#include "IBG_InputType.h"
#include "IBR_Components.h"
#include "IBB_CustomBool.h"
#include "IBR_LinkNode.h"
#include "IBB_FileChecker.h"

std::string IBB_RegType::GetNoName()
{
    auto NewName = Name + std::to_string(++Count);
    while (IBF_Inst_Project.HasDisplayName(NewName))NewName = Name + std::to_string(++Count);
    return NewName;
}

std::string IBB_RegType::GetNoName(const std::string& Reg)
{
    int NCount = 0;
    if (!IBF_Inst_Project.HasDisplayName(Name + Reg))return Name + Reg;
    auto NewName = Name + Reg + std::to_string(++NCount);
    while (IBF_Inst_Project.HasDisplayName(NewName))NewName = Name + Reg + std::to_string(++NCount);
    return NewName;
}

std::wstring FileName(const std::wstring& ss);
std::vector<std::wstring> FindFileVec(const std::wstring& pattern);
void subreplace(std::string& dst_str, const std::string& sub_str, const std::string& new_str);

const char* AnyTypeName = "_AnyType";
const char* MyTypeName = "_MyType";
StrPoolID AnyTypeID()
{
    static StrPoolID ID = NewPoolStr(AnyTypeName);
    return ID;
}
StrPoolID MyTypeID()
{
    static StrPoolID ID = NewPoolStr(MyTypeName);
    return ID;
}

namespace IBB_DefaultRegType
{
    std::unordered_map<StrPoolID, std::set<StrPoolID>>CompoundTypeIndex;
    std::unordered_map<_TEXT_UTF8 std::string, IBB_CompoundRegType>CompoundTypes;
    std::unordered_map<_TEXT_UTF8 std::string, IBB_RegType>RegisterTypes;
    const ImColor DefaultColor{ ImColor(255, 255, 255, 0) };
    const ImColor DefaultColorD{ ImColor(153, 153, 153, 0) };
    IBB_RegType __Default{
        DefaultIniName,
        DefaultColor, DefaultColor, DefaultColor, DefaultColor,
        DefaultColor, DefaultColor, DefaultColor, DefaultColor,
        DefaultColorD, DefaultColorD, DefaultColorD, DefaultColorD,
        false, false, false, false, u8"模块", "", 0};
    StrBoolType DefaultStrBoolType;
    ImColor DefaultNodeColor;

    std::unordered_map<_TEXT_UTF8 std::string, IBG_InputType> InputTypes;

    auto rttpt = [](const std::wstring& wcs, const std::string& Info)
        {
            auto ws = std::vformat(locw("Error_CannotLoadPresetType"), std::make_wformat_args(wcs));
            auto wss = UnicodetoUTF8(std::vformat(locw("Log_LoadConfigErrorInfo"), std::make_wformat_args(ws)));
            IBR_PopupManager::AddLoadConfigErrorPopup(wss + "\n" + loc("Log_PresetTypeInfo") + "\n" + Info, "");
        };

    void InitInputTypes(const std::string& S_StrBool)
    {
        static bool CALLED = false;
        if (!CALLED) CALLED = true;
        else return;

        static std::string StrTypeJSON =
R"({
    "Type" : "Link",
    "Form" : {
        "Input" : [
            {"Type": "InputText", "ValueID": 0 }
        ],
        "Format" : [
            {"ValueIDToString": 0}
        ]
    }
})";
        static std::string BoolTypeJSON =
R"({
    "Type" : "Bool",
    "Form" : {
        "Input" : [
            {"Type": "Bool", "ValueID": 0, "InitialValue": false, "Fmt": <DEFAULT_FORMAT> }
        ],
        "Format" : [
            {"ValueIDToString": 0}
        ]
    }
})";
        static std::string LinkTypeJSON =
R"({
    "Type" : "Link",
    "Form" : {
        "Input" : [
            {"Type": "Link", "ValueID": 0}
        ],
        "Format" : [
            {"ValueIDToString": 0}
        ]
    }
})";
        static std::string ImportTypeJSON =
R"({
    "Type" : "Link",
    "Form": {
        "Input" : [
            {"Type": "Link", "ValueID": 0}
        ],
        "Format": [
            { "ValueIDToString": 0 }
        ]
    },
    "ExportMode": {
        "Type": "Import",
        "IniType": "_MyType",
        "MergeTarget": false
    }
 })";
                static std::string ImportAndMergeTypeJSON =
R"({
    "Type" : "Link",
    "Form": {
        "Input" : [
            {"Type": "Link", "ValueID": 0}
        ],
        "Format": [
            { "ValueIDToString": 0 }
        ]
    },
    "ExportMode": {
        "Type": "Import",
        "IniType": "_MyType",
        "MergeTarget": true
    }
 })";

        subreplace(BoolTypeJSON, "<DEFAULT_FORMAT>", S_StrBool);

        JsonFile StrObj; StrObj.Parse(StrTypeJSON);
        JsonFile BoolObj; BoolObj.Parse(BoolTypeJSON);
        JsonFile LinkObj; LinkObj.Parse(LinkTypeJSON);
        JsonFile ImportObj; ImportObj.Parse(ImportTypeJSON);
        JsonFile IAMObj; IAMObj.Parse(ImportAndMergeTypeJSON);

        

        if (!InputTypes[u8"String"].Load(StrObj))
            rttpt(L"String", StrTypeJSON);
        if (!InputTypes[u8"Bool"].Load(BoolObj))
            rttpt(L"Bool", BoolTypeJSON);
        if (!InputTypes[u8"Link"].Load(LinkObj))
            rttpt(L"Link", LinkTypeJSON);
        if (!InputTypes[u8"Import"].Load(ImportObj))
            rttpt(L"Import", ImportTypeJSON);
        if (!InputTypes[u8"ImportAndMerge"].Load(IAMObj))
            rttpt(L"ImportAndMerge", ImportAndMergeTypeJSON);

    }

    void ClearModuleCount()
    {
        for (auto& [S, V] : RegisterTypes)
            V.Count = 0;
    }
    void SwitchDarkColor()
    {
        for (auto& [S, V] : RegisterTypes)
        {
            V.FrameColor = V.FrameColorD;
            V.FrameColorPlus1 = V.FrameColorDPlus1;
            V.FrameColorPlus2 = V.FrameColorDPlus2;
            V.FrameColorH = V.FrameColorDH;
        }
    }
    void SwitchLightColor()
    {
        for (auto& [S, V] : RegisterTypes)
        {
            V.FrameColor = V.FrameColorL;
            V.FrameColorPlus1 = V.FrameColorLPlus1;
            V.FrameColorPlus2 = V.FrameColorLPlus2;
            V.FrameColorH = V.FrameColorLH;
        }
    }
    void LoadReg(IBB_RegType& Reg, JsonObject Obj)
    {
        auto S = Obj.GetObjectItem(u8"ExportToINI");
        if (S.Available())Reg.IniType = S.GetString();
        else Reg.IniType = DefaultIniName;
        S = Obj.GetObjectItem(u8"FrameColor");
        if (S.Available())
        {
            auto V = S.GetArrayInt();
            if (V.size() == 3)Reg.FrameColorL = ImColor(V[0], V[1], V[2]);
            else if (V.size() >= 4)Reg.FrameColorL = ImColor(V[0], V[1], V[2], V[3]);
            else Reg.FrameColorL = DefaultColor;
        }
        else Reg.FrameColorL = DefaultColor;

        Reg.Export = Obj.ItemBoolOr(u8"NeedRegisterList", false);
        Reg.RegNameAsDisplay = Obj.ItemBoolOr(u8"UseRegName", false);
        Reg.UseOwnName = Obj.ItemBoolOr(u8"UseOwnName", false);
        Reg.ValidateOptions = Obj.ItemBoolOr(u8"ValidateOptions", false);

        auto Val = Obj.ItemMapStringOr(u8"DefaultLinks");
        for (auto&& [k, v] : Val)Reg.DefaultLinks.emplace(NewPoolStr(k), NewPoolStr(v));

        Reg.Options = Obj.ItemMapStringOr(u8"Options");

        S = Obj.GetObjectItem(u8"Name");
        if (S.Available())Reg.Name = S.GetString();
        else if (Reg.UseOwnName)Reg.Name = Obj.GetName() + "_";
        else Reg.Name = loc("Back_DefaultModuleName");
        Reg.Count = 0;

        Reg.ExportName = Obj.ItemStringOr(u8"RegisterListName", "");
        



        Reg.FrameColorD = Reg.FrameColorL;
        Reg.FrameColorLPlus1 = Reg.FrameColorL;
        Reg.FrameColorLPlus2 = Reg.FrameColorL;
        Reg.FrameColorLH = Reg.FrameColorL;
        Reg.FrameColorDPlus1 = Reg.FrameColorD;
        Reg.FrameColorDPlus2 = Reg.FrameColorD;
        Reg.FrameColorDH = Reg.FrameColorD;

        ImVec4 X;
        ImGui::ColorConvertRGBtoHSV(
            Reg.FrameColorL.Value.x, Reg.FrameColorL.Value.y, Reg.FrameColorL.Value.z, X.x, X.y, X.z);
        ImVec4 LightBaseHSV = X;
        X.z *= 0.7f; X.y *= 0.7f;
        ImVec4 DarkBaseHSV = X;
        ImGui::ColorConvertHSVtoRGB(X.x, X.y, X.z, Reg.FrameColorD.Value.x, Reg.FrameColorD.Value.y, Reg.FrameColorD.Value.z);

        LightBaseHSV.z *= 1.2f;
        ImGui::ColorConvertHSVtoRGB(LightBaseHSV.x, LightBaseHSV.y, LightBaseHSV.z, Reg.FrameColorLPlus1.Value.x, Reg.FrameColorLPlus1.Value.y, Reg.FrameColorLPlus1.Value.z);
        LightBaseHSV.z *= 1.2f;
        ImGui::ColorConvertHSVtoRGB(LightBaseHSV.x, LightBaseHSV.y, LightBaseHSV.z, Reg.FrameColorLPlus2.Value.x, Reg.FrameColorLPlus2.Value.y, Reg.FrameColorLPlus2.Value.z);
        LightBaseHSV.y *= 0.5f;
        ImGui::ColorConvertHSVtoRGB(LightBaseHSV.x, LightBaseHSV.y, LightBaseHSV.z, Reg.FrameColorLH.Value.x, Reg.FrameColorLH.Value.y, Reg.FrameColorLH.Value.z);

        DarkBaseHSV.z *= 1.2f;
        ImGui::ColorConvertHSVtoRGB(DarkBaseHSV.x, DarkBaseHSV.y, DarkBaseHSV.z, Reg.FrameColorDPlus1.Value.x, Reg.FrameColorDPlus1.Value.y, Reg.FrameColorDPlus1.Value.z);
        DarkBaseHSV.z *= 1.2f;
        ImGui::ColorConvertHSVtoRGB(DarkBaseHSV.x, DarkBaseHSV.y, DarkBaseHSV.z, Reg.FrameColorDPlus2.Value.x, Reg.FrameColorDPlus2.Value.y, Reg.FrameColorDPlus2.Value.z);
        DarkBaseHSV.y *= 0.5f;
        ImGui::ColorConvertHSVtoRGB(DarkBaseHSV.x, DarkBaseHSV.y, DarkBaseHSV.z, Reg.FrameColorDH.Value.x, Reg.FrameColorDH.Value.y, Reg.FrameColorDH.Value.z);


        if (IBF_Inst_Setting.IsDarkMode())
        {
            Reg.FrameColor = Reg.FrameColorD;
            Reg.FrameColorPlus1 = Reg.FrameColorDPlus1;
            Reg.FrameColorPlus2 = Reg.FrameColorDPlus2;
            Reg.FrameColorH = Reg.FrameColorDH;
        }
        else
        {
            Reg.FrameColor = Reg.FrameColorL;
            Reg.FrameColorPlus1 = Reg.FrameColorLPlus1;
            Reg.FrameColorPlus2 = Reg.FrameColorLPlus2;
            Reg.FrameColorH = Reg.FrameColorLH;
        }
    }
    void RegisterCompoundType(IBB_CompoundRegType&& Com)
    {
        auto& P = CompoundTypes[Com.Name];
        P = std::move(Com);
        for (auto& V : P.Regs)
            CompoundTypeIndex[NewPoolStr(V)].insert(NewPoolStr(P.Name));
    }
    void EnsureRegType(const _TEXT_UTF8 std::string& Type)
    {
        IBB_RegType* pp;
        auto it = RegisterTypes.find(Type);
        if (it == RegisterTypes.end())
        {
            auto& P = RegisterTypes[Type];
            P = __Default;
            pp = &P;
        }
        else pp = &it->second;

        auto pIni = IBF_Inst_Project.Project.GetIni(IBB_Project_Index(pp->IniType));
        if (!pIni)IBF_Inst_Project.Project.CreateIni(pp->IniType);
    }
    bool Load(JsonObject Obj)
    {
        auto S = Obj.GetObjectItem(u8"Default");
        std::string S_StrBool = "yes_no";
        if (S.Available())
        {
            LoadReg(__Default, S);

            auto oBoolFmt = S.GetObjectItem("BoolFmt");
            DefaultStrBoolType = StrBoolTypeFromJSON(oBoolFmt, StrBoolType::Str_yes_no);
            if (oBoolFmt) S_StrBool = oBoolFmt.PrintUnformatted();
            else S_StrBool = "\"yes_no\"";

            auto oDNC = S.GetObjectItem(u8"DefaultNodeColor");
            if (oDNC.Available())
            {
                auto V = oDNC.GetArrayInt();
                if (V.size() == 3)DefaultNodeColor = ImColor(V[0], V[1], V[2]);
                else if (V.size() >= 4)DefaultNodeColor = ImColor(V[0], V[1], V[2], V[3]);
                else DefaultNodeColor = 0;
            }
        }

        S = Obj.GetObjectItem(u8"RegisterTypes");
        if (S.Available())
            for (auto& [N, V] : S.GetMapObject())
                LoadReg(RegisterTypes[N], V);
        S = Obj.GetObjectItem(u8"CompoundTypes");
        if (S.Available())
            for (auto& [N,V] : S.GetMapObject())
            {
                IBB_CompoundRegType Com;
                Com.Name = N;
                Com.DisplayName = V.ItemStringOr(u8"Name", N);
                Com.Regs = V.ItemArrayStringOr(u8"Types");
                Com.DefaultLinks.Value = V.ItemMapStringOr(u8"DefaultLinks");
                RegisterCompoundType(std::move(Com));
            }

        for (auto& [Name, Reg] : RegisterTypes)
        {
            auto it = CompoundTypeIndex.find(NewPoolStr(Name));
            if (it != CompoundTypeIndex.end())
            {
                for (auto& Comp : it->second)
                {
                    auto cit = CompoundTypes.find(PoolStr(Comp));
                    if (cit != CompoundTypes.end())
                    {
                        for(auto& [k, v] : cit->second.DefaultLinks.Value)
                            Reg.DefaultLinks[NewPoolStr(k)] = NewPoolStr(v);
                    }
                }
            }
        }
        for (auto& [Name, Reg] : RegisterTypes)
        {
            std::unordered_map<StrPoolID, StrPoolID> Temp;
            for (auto& [K, V] : Reg.DefaultLinks)
            {
                auto it = CompoundTypes.find(PoolStr(K));
                if (it != CompoundTypes.end())
                {
                    for (auto& R : it->second.Regs)
                        Temp[NewPoolStr(R)] = V;
                }
            }
            Reg.DefaultLinks.insert(Temp.begin(), Temp.end());
        }


        InitInputTypes(S_StrBool);
        S = Obj.GetObjectItem(u8"InputTypes");
        if (S.Available())
            for (auto& [N, V] : S.GetMapObject())
                if (!InputTypes[N].Load(V))
                    rttpt(UTF8toUnicode(N), V.PrintData());

        return true;
    }
    bool LoadFromFile(const wchar_t* FileName)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog((u8"IBB_DefaultRegType::LoadFromFile ： " + loc("Log_LoadRegType")).c_str());
        }
        bool Available = true;
        auto&& Vec = FindFileVec(FileName);
        for(auto&& File : Vec)
        {
            IBB_FileCheck(File, false, true, false);
            JsonFile F;
            auto WFN = ::FileName(File);
            IBR_PopupManager::AddJsonParseErrorPopup(F.ParseFromFileChecked(UnicodetoUTF8(File).c_str(), loc("Error_JsonParseErrorPos"), nullptr),
                UnicodetoUTF8(std::vformat(locw("Error_JsonSyntaxError"), std::make_wformat_args(WFN))));
            if (!F.Available())Available = false;
            else Available &= Load(F);
        }

        if (Vec.empty())
        {
            auto S_StrBool = "\"yes_no\"";
            InitInputTypes(S_StrBool);
        }

        return Available;
    }
    bool HasRegType(const _TEXT_UTF8 std::string& Type)
    {
        return RegisterTypes.find(Type) != RegisterTypes.end();
    }
    bool HasRegType(StrPoolID Type)
    {
        return RegisterTypes.find(PoolStr(Type)) != RegisterTypes.end();
    }
    IBB_RegType& GetRegType(const _TEXT_UTF8 std::string& Type)
    {
        auto it = RegisterTypes.find(Type);
        if (it == RegisterTypes.end())return __Default;
        else return it->second;
    }
    IBB_RegType& GetRegType(StrPoolID Type)
    {
        auto it = RegisterTypes.find(PoolStr(Type));
        if (it == RegisterTypes.end())return __Default;
        else return it->second;
    }
    const _TEXT_UTF8 std::string& GetIniTypeOfReg(const _TEXT_UTF8 std::string& Type)
    {
        if (CompoundTypes.contains(Type))
        {
            auto& Com = CompoundTypes.at(Type);
            if (!Com.Regs.empty())
            {
                auto it = RegisterTypes.find(Com.Regs[0]);
                if (it != RegisterTypes.end())
                    return it->second.IniType;
            }
        }
        return GetRegType(Type).IniType;
    }
    const _TEXT_UTF8 std::string& GetIniTypeOfReg(StrPoolID Type)
    {
        return GetIniTypeOfReg(PoolStr(Type));
    }

    bool HasInputType(const _TEXT_UTF8 std::string& Type)
    {
        return InputTypes.find(Type) != InputTypes.end();
    }
    IBG_InputType& GetInputType(const _TEXT_UTF8 std::string& Type)
    {
        auto it = InputTypes.find(Type);
        if (it == InputTypes.end())return InputTypes[u8"Link"];
        else return it->second;
    }
    IBG_InputType& GetDefaultInputType()
    {
        return InputTypes[u8"String"];
    }
    ImColor GetDefaultNodeColor()
    {
        return DefaultNodeColor;
    }
    LinkNodeSetting GetDefaultLinkNodeSetting()
    {
        return LinkNodeSetting{
            AnyTypeID(), -1, DefaultNodeColor
        };
    }
    StrBoolType GetDefaultStrBoolType()
    {
        return DefaultStrBoolType;
    }
    IBG_InputType& SelectInputTypeByValue(const _TEXT_UTF8 std::string& Value)
    {
        if (AcceptStrAsBool(Value.c_str(), DefaultStrBoolType))
            return GetInputType("Bool");
        else
            return GetDefaultInputType();
    }
    //A 属于 B
    const bool ContainType(StrPoolID TypeA, StrPoolID TypeB)
    {
        if (TypeB == AnyTypeID())return true;
        auto it = CompoundTypeIndex.find(TypeA);
        if (it == CompoundTypeIndex.end())return false;
        return it->second.contains(TypeB);
    }
    const bool MatchType(StrPoolID TypeA, StrPoolID TypeB)
    {
        if (TypeA == TypeB)return true;
        return ContainType(TypeA, TypeB) || ContainType(TypeB, TypeA);
    }
    void GenerateDLK(const std::vector<PairClipString>& DLK1, StrPoolID Register, std::unordered_map<StrPoolID, StrPoolID>& DefaultLinkKey, std::unordered_map<StrPoolID, StrPoolID>*& UpValue)
    {
        for (auto& L : DLK1)
        {
            auto it = CompoundTypes.find(L.A);
            if (it == CompoundTypes.end())DefaultLinkKey[NewPoolStr(L.A)] = NewPoolStr(L.B);
            else for (auto& V : it->second.Regs)DefaultLinkKey[NewPoolStr(V)] = NewPoolStr(L.B);
        }
        auto& Reg = GetRegType(Register);
        UpValue = &Reg.DefaultLinks;
    }
}
