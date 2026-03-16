#include "IBR_Project.h"
#include "IBFront.h"
#include "Global.h"
#include "FromEngine/RFBump.h"
#include "FromEngine/global_timer.h"
#include "IBR_Debug.h"
#include <imgui_internal.h>
#include "IBR_Misc.h"

namespace IBR_FullView
{
    ImVec2 CurrentEqMax;

    ImVec2 ViewSize;//Set in Initialize Stage IV
    double ViewScale = 20;

    ImVec2 GetDefaultEqMax() { return (ImVec2)(ViewSize * ViewScale * (0.5)); }
    ImVec2 GetEqMax() { return CurrentEqMax; }
    ImVec2 GetEqMin() { auto Max = GetEqMax(); return { -Max.x, -Max.y }; }

    void UpdateCurrentEqMax()
    {
        auto EqMax = GetDefaultEqMax();
        for (auto& sp : IBR_Inst_Project.IBR_SectionMap)
        {
            if (!IBR_Inst_Project.GetSectionFromID(sp.first).HasBack())continue;
            auto& sd = sp.second;
            if (sd.IsIncluded())continue;
            EqMax.x = std::max({ EqMax.x, fabsf(sd.EqPos.x), fabsf(sd.EqPos.x + sd.EqSize.x) });
            EqMax.y = std::max({ EqMax.y, fabsf(sd.EqPos.y), fabsf(sd.EqPos.y + sd.EqSize.y) });
        }
        CurrentEqMax = EqMax;
    }

    int EqXRange(const ImVec2& V)
    {
        auto MaxXY = GetEqMax();
        if (V.x < -MaxXY.x)return -1;
        if (V.x > MaxXY.x)return 1;
        return 0;
    }

    int EqYRange(const ImVec2& V)
    {
        auto MaxXY = GetEqMax();
        if (V.y < -MaxXY.y)return -1;
        if (V.y > MaxXY.y)return 1;
        return 0;
    }

    void EqPosFixRange(ImVec2& V)
    {
        auto MaxXY = GetEqMax();
        V.x = std::min(V.x, MaxXY.x);
        V.x = std::max(V.x, -MaxXY.x);
        V.y = std::min(V.y, MaxXY.y);
        V.y = std::max(V.y, -MaxXY.y);
    }

    bool EqPosInRange(ImVec2 V)
    {
        auto MaxXY = GetEqMax();
        return V.x <= MaxXY.x && V.x >= -MaxXY.x && V.y <= MaxXY.y && V.y >= -MaxXY.y;
    }

    ImVec2 ViewSizeOnDraw()
    {
        return GetEqMax() * 2.0 / ViewScale;
    }

    void DrawView(ImDrawList* dl, ImVec2 Pos)
    {
        auto VS = ViewSizeOnDraw();
        ImVec2 CPos = Pos + (VS * 0.5);
        dImVec2 VOffset = dImVec2(IBR_FullView::EqCenter) / ViewScale;
        dl->AddRectFilled(Pos, Pos + VS, IBR_Color::BackgroundColor);
        dImVec2 CC = Pos + (VS * 0.5);//中心
        /*
        for (int i = 0; i < CurrentNSec; i++)
        {
            ImVec2 VCI = WindowZoomDirection[i] / (double)IBR_FullView::Ratio / ViewScale ;
            for (int j : Linkto[i])
            {
                if (j == -1)continue;
                ImVec2 VCJ = WindowZoomDirection[j] / (double)IBR_FullView::Ratio / ViewScale ;
                dl->AddBezierCurve(
                    VCI + CPos + VOffset,
                    ImVec2{ (3 * VCJ.x + VCI.x) / 4 ,VCI.y } + CPos + VOffset,
                    ImVec2{ (3 * VCI.x + VCJ.x) / 4 ,VCJ.y } + CPos + VOffset,
                    VCJ + CPos + VOffset,
                     _TempSelectLink::LegalLineColor,1.0f);
            }
        }*/
        IBR_SectionData* psd = nullptr;
        for (auto& sp : IBR_Inst_Project.IBR_SectionMap)
        {
            if (!IBR_Inst_Project.GetSectionFromID(sp.first).HasBack())continue;
            auto& sd = sp.second;
            if (sd.IsIncluded())continue;
            if (sp.first == IBR_EditFrame::CurSection.ID)
            {
                psd = &sp.second; continue;
            }
            ImVec2 WUL = CC + sd.EqPos / ViewScale;
            ImVec2 WDR = CC + (sd.EqPos + sd.EqSize) / ViewScale;
            //MessageBoxA(NULL, sp.second.Desc.GetText().c_str(), "!!!FFF!!!", MB_OK);
            auto RSec = IBR_Inst_Project.GetSectionFromID(sp.first);
            if (sd.Ignore)
            {
                auto C = ImColor(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
                if (IBF_Inst_Setting.IsDarkMode())
                {
                    C.Value.x += 0.3f;
                    C.Value.y += 0.3f;
                    C.Value.z += 0.3f;
                }
                else
                {
                    C.Value.x *= 0.8f;
                    C.Value.y *= 0.8f;
                    C.Value.z *= 0.8f;
                }
                dl->AddRectFilled(WUL, WDR, C);
            }
            else
            {
                dl->AddRectFilled(WUL, WDR, RSec.GetRegTypeColor());
            }
            dl->AddRect(WUL, WDR, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg)), 0.0f, 0, 1.0f);
        }
        if (psd != nullptr)
        {
            auto& sd = *psd;
            ImVec2 WUL = CC + sd.EqPos / ViewScale;
            ImVec2 WDR = CC + (sd.EqPos + sd.EqSize) / ViewScale;
            dl->AddRectFilled(WUL, WDR, IBR_Color::ViewFocusWindowColor);
            dl->AddRect(WUL, WDR, ImColor(ImGui::GetStyleColorVec4(IBR_Color::FocusLineColor)), 0.0f, 0, 2.0f);
        }

        /*
        {
            ImVec2 WUL = CC + IBR_WorkSpace::DragStartEqMouse / ViewScale;
            ImVec2 WDR = CC + IBR_WorkSpace::DragCurEqMouse / ViewScale;
            dl->AddRectFilled(WUL, WDR, IBR_Color::FocusWindowColor);
            dl->AddRect(WUL, WDR, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), 0.0f, 0, 1.0f);
        }
        */

        dImVec2 ClipSize = (IBR_RealCenter::WorkSpaceDR - IBR_RealCenter::WorkSpaceUL) / (double)IBR_FullView::Ratio;
        dImVec2 ClipVSize = ClipSize / ViewScale;
        dImVec2 ClipVUL = VOffset - (ClipVSize * 0.5);
        volatile auto X1 = ClipSize;
        volatile auto X2 = ClipVUL;
        volatile auto X3 = CPos;
        (void)X1; (void)X2; (void)X3;
        //sprintf(LogBuf, "X1=(%f,%f) X2=(%f,%f) X3=(%f,%f)", X1.x, X1.y, X2.x, X2.y, X3.x, X3.y);
        dImVec2 CCPS = ClipVUL + (ClipVSize / 2) + CPos;
        dl->AddLine(CCPS - ImVec2{ 3, 0 }, CCPS + ImVec2{ 4, 0 }, IBR_Color::CenterCrossColor, 1.0f);
        dl->AddLine(CCPS - ImVec2{ 0, 3 }, CCPS + ImVec2{ 0, 4 }, IBR_Color::CenterCrossColor, 1.0f);
        dl->AddRect(ClipVUL + CPos, ClipVUL + ClipVSize + CPos, IBR_WorkSpace::IsBgDragging ? IBR_Color::CenterCrossColor : IBR_Color::ClipFrameLineColor, 0.0f, 0, 1.0f);

    }

    void ChangeOffsetPos(ImVec2 ClickRel)
    {
        IBR_FullView::EqCenter = (ClickRel - ViewSizeOnDraw() * 0.5) * ViewScale;
    }

    ImVec2 EqCenter = { 0.0f,0.0f };
    float Ratio = 1.0;
    void RenderUI()
    {
        static float TmpScale;
        TmpScale = 100.0f * IBR_FullView::Ratio;
        ImGui::SliderFloat(locc("GUI_ZoomRatio"), &TmpScale, RatioMin, RatioMax, "%.0f%%", ImGuiSliderFlags_Logarithmic);
        TmpScale = floor(TmpScale / 5.0f) * 5.0f;
        IBR_FullView::Ratio = TmpScale / 100.0f;

        auto VS = ViewSizeOnDraw();
        ImGui::Text(locc("GUI_ViewTitle"));
        ImGui::BeginChildFrame(114514 + 2, VS + ImVec2{ 5, 8 });
        auto CRect = ImGui::GetCursorScreenPos();
        ImGui::Dummy(VS);

        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            auto MP = ImGui::GetIO().MousePos;
            ChangeOffsetPos(MP - CRect);
        }
        DrawView(ImGui::GetWindowDrawList(), CRect);
        ImGui::EndChildFrame();
    }
}
