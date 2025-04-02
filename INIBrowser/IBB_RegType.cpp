#include "IBB_RegType.h"
#include "Global.h"
#include "IBB_ModuleAlt.h"

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

namespace IBB_DefaultRegType
{
    std::unordered_map<_TEXT_UTF8 std::string, std::set<_TEXT_UTF8 std::string>>CompoundTypeIndex;
    std::unordered_map<_TEXT_UTF8 std::string, IBB_CompoundRegType>CompoundTypes;
    std::unordered_map<_TEXT_UTF8 std::string, IBB_RegType>RegisterTypes;
    const ImColor DefaultColor{ ImColor(255, 255, 255, 0) };
    const ImColor DefaultColorD{ ImColor(153, 153, 153, 0) };
    IBB_RegType __Default{ DefaultIniName, DefaultColor, DefaultColor, DefaultColorD, false, false, false, u8"模块", 0};

    void ClearModuleCount()
    {
        for (auto& [S, V] : RegisterTypes)
            V.Count = 0;
    }
    void SwitchDarkColor()
    {
        for (auto& [S, V] : RegisterTypes)
            V.FrameColor = V.FrameColorD;
    }
    void SwitchLightColor()
    {
        for (auto& [S, V] : RegisterTypes)
            V.FrameColor = V.FrameColorL;
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

        S = Obj.GetObjectItem(u8"Export");
        if (S.Available())Reg.Export = S.GetBool();
        else Reg.Export = false;

        S = Obj.GetObjectItem(u8"UseRegName");
        if (S.Available())Reg.RegNameAsDisplay = S.GetBool();
        else Reg.RegNameAsDisplay = false;

        S = Obj.GetObjectItem(u8"UseOwnName");
        if (S.Available())Reg.UseOwnName = S.GetBool();
        else Reg.UseOwnName = false;

        S = Obj.GetObjectItem(u8"Name");
        if (S.Available())Reg.Name = S.GetString();
        else if (Reg.UseOwnName)Reg.Name = Obj.GetName() + "_";
        else Reg.Name = u8"模块";
        Reg.Count = 0;

        Reg.FrameColorD = Reg.FrameColorL;
        ImVec4 X;
        ImGui::ColorConvertRGBtoHSV(
            Reg.FrameColorL.Value.x, Reg.FrameColorL.Value.y, Reg.FrameColorL.Value.z, X.x, X.y, X.z);
        X.z *= 0.7f; X.y *= 0.7f;
        ImGui::ColorConvertHSVtoRGB(X.x, X.y, X.z, Reg.FrameColorD.Value.x, Reg.FrameColorD.Value.y, Reg.FrameColorD.Value.z);
        if (IBF_Inst_Setting.IsDarkMode())Reg.FrameColor = Reg.FrameColorD;
        else Reg.FrameColor = Reg.FrameColorL;
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
                Com.Regs = V.GetArrayString();
                RegisterCompoundType(std::move(Com));
            }
        return true;
    }
    bool LoadFromFile(const char* FileName)
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog("IBB_DefaultRegType::LoadFromFile ： 开始读取注册配置。");
        }
        JsonFile F;
        F.ParseFromFile(FileName);
        if (!F.Available())return false;
        return Load(F);
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
    void GenerateDLK(const std::vector<PairClipString>& DLK1, std::unordered_map<std::string, std::string>& DefaultLinkKey)
    {
        for (auto& L : DLK1)
        {
            auto it = CompoundTypes.find(L.A);
            if (it == CompoundTypes.end())DefaultLinkKey[L.A] = L.B;
            else for (auto& V : it->second.Regs)DefaultLinkKey[V] = L.B;
        }
    }
}
