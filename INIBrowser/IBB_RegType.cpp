#include "IBB_RegType.h"
#include "Global.h"
#include "IBB_ModuleAlt.h"
#include "IBB_Index.h"

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

namespace IBB_DefaultRegType
{
    std::unordered_map<_TEXT_UTF8 std::string, std::set<_TEXT_UTF8 std::string>>CompoundTypeIndex;
    std::unordered_map<_TEXT_UTF8 std::string, IBB_CompoundRegType>CompoundTypes;
    std::unordered_map<_TEXT_UTF8 std::string, IBB_RegType>RegisterTypes;
    const ImColor DefaultColor{ ImColor(255, 255, 255, 0) };
    const ImColor DefaultColorD{ ImColor(153, 153, 153, 0) };
    IBB_RegType __Default{
        DefaultIniName,
        DefaultColor, DefaultColor, DefaultColor, DefaultColor,
        DefaultColor, DefaultColor, DefaultColor, DefaultColor,
        DefaultColorD, DefaultColorD, DefaultColorD, DefaultColorD,
        false, false, false, u8"模块", 0};

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
        auto S = Obj.GetObjectItem(u8"IniType");
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

        Reg.Export = Obj.ItemBoolOr(u8"Export", false);
        Reg.RegNameAsDisplay = Obj.ItemBoolOr(u8"UseRegName", false);
        Reg.UseOwnName = Obj.ItemBoolOr(u8"UseOwnName", false);
        Reg.DefaultLinks.Value = Obj.ItemMapStringOr(u8"DefaultLinks");

        S = Obj.GetObjectItem(u8"Name");
        if (S.Available())Reg.Name = S.GetString();
        else if (Reg.UseOwnName)Reg.Name = Obj.GetName() + "_";
        else Reg.Name = loc("Back_DefaultModuleName");
        Reg.Count = 0;



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
            CompoundTypeIndex[V].insert(P.Name);
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
        if (S.Available())LoadReg(__Default, S);
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
            auto it = CompoundTypeIndex.find(Name);
            if (it != CompoundTypeIndex.end())
            {
                for (auto& Comp : it->second)
                {
                    auto cit = CompoundTypes.find(Comp);
                    if (cit != CompoundTypes.end())
                    {
                        Reg.DefaultLinks.Merge(cit->second.DefaultLinks, true);
                    }
                }
            }
        }
        for (auto& [Name, Reg] : RegisterTypes)
        {
            std::unordered_map<std::string, std::string> Temp;
            for (auto& [K, V] : Reg.DefaultLinks.Value)
            {
                auto it = CompoundTypes.find(K);
                if (it != CompoundTypes.end())
                {
                    for (auto& R : it->second.Regs)
                        Temp[R] = V;
                }
            }
            Reg.DefaultLinks.Value.insert(Temp.begin(), Temp.end());
        }

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
        for(auto&& File : FindFileVec(FileName))
        {
            JsonFile F;
            auto WFN = ::FileName(File);
            IBR_PopupManager::AddJsonParseErrorPopup(F.ParseFromFileChecked(UnicodetoUTF8(File).c_str(), loc("Error_JsonParseErrorPos"), nullptr),
                UnicodetoUTF8(std::vformat(locw("Error_JsonSyntaxError"), std::make_wformat_args(WFN))));
            if (!F.Available())Available = false;
            else Available &= Load(F);
        }
        return Available;
    }
    bool HasRegType(const _TEXT_UTF8 std::string& Type)
    {
        return RegisterTypes.find(Type) != RegisterTypes.end();
    }
    IBB_RegType& GetRegType(const _TEXT_UTF8 std::string& Type)
    {
        auto it = RegisterTypes.find(Type);
        if (it == RegisterTypes.end())return __Default;
        else return it->second;
    }
    //A 属于 B
    const bool ContainType(const _TEXT_UTF8 std::string& TypeA, const _TEXT_UTF8 std::string& TypeB)
    {
        auto it = CompoundTypeIndex.find(TypeA);
        if (it == CompoundTypeIndex.end())return false;
        return it->second.contains(TypeB);
    }
    const bool MatchType(const _TEXT_UTF8 std::string& TypeA, const _TEXT_UTF8 std::string& TypeB)
    {
        if (TypeA == TypeB)return true;
        return ContainType(TypeA, TypeB) || ContainType(TypeB, TypeA);
    }
    void GenerateDLK(const std::vector<PairClipString>& DLK1, const std::string& Register, IBB_VariableList& DefaultLinkKey)
    {
        for (auto& L : DLK1)
        {
            auto it = CompoundTypes.find(L.A);
            if (it == CompoundTypes.end())DefaultLinkKey.Value[L.A] = L.B;
            else for (auto& V : it->second.Regs)DefaultLinkKey.Value[V] = L.B;
        }
        auto& Reg = GetRegType(Register);
        DefaultLinkKey.UpValue = &Reg.DefaultLinks;
    }
}
