#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "AegisIdTechUniversal.h"
#include "AegisUniversalOverlay.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace
{
    struct Rect2D
    {
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        bool valid = false;
    };

    bool g_drawEnabled = true;
    bool g_drawBoxes = true;
    bool g_drawCornerBoxes = true;
    bool g_drawFilledBoxes = false;
    bool g_drawLines = true;
    bool g_drawLabels = true;
    bool g_hideInvisible = true;
    float g_boxThickness = 1.5f;
    float g_lineThickness = 1.25f;
    ImVec4 g_boxColor = ImVec4(0.0f, 0.95f, 0.35f, 1.0f);
    ImVec4 g_lineColor = ImVec4(0.0f, 0.72f, 1.0f, 0.9f);
    ImVec4 g_fillColor = ImVec4(0.0f, 0.95f, 0.35f, 0.12f);

    AegisIdTechVec3 Add(const AegisIdTechVec3& a, const AegisIdTechVec3& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    bool IsFinite(float value)
    {
        return std::isfinite(value) != 0;
    }

    bool ProjectBounds(const AegisIdTechEntitySnapshot& entity, Rect2D& rect, std::uint32_t& projected, std::uint32_t& clipped)
    {
        const std::array<AegisIdTechVec3, 8> corners = {
            Add(entity.origin, { entity.boundsMin.x, entity.boundsMin.y, entity.boundsMin.z }),
            Add(entity.origin, { entity.boundsMax.x, entity.boundsMin.y, entity.boundsMin.z }),
            Add(entity.origin, { entity.boundsMin.x, entity.boundsMax.y, entity.boundsMin.z }),
            Add(entity.origin, { entity.boundsMax.x, entity.boundsMax.y, entity.boundsMin.z }),
            Add(entity.origin, { entity.boundsMin.x, entity.boundsMin.y, entity.boundsMax.z }),
            Add(entity.origin, { entity.boundsMax.x, entity.boundsMin.y, entity.boundsMax.z }),
            Add(entity.origin, { entity.boundsMin.x, entity.boundsMax.y, entity.boundsMax.z }),
            Add(entity.origin, { entity.boundsMax.x, entity.boundsMax.y, entity.boundsMax.z })
        };

        rect = {};
        for (const AegisIdTechVec3& corner : corners)
        {
            AegisIdTechProjectedPoint point = {};
            if (!AegisIdTech_ProjectWorldToScreen(&corner, &point))
            {
                ++clipped;
                continue;
            }

            if (point.clipped || !IsFinite(point.x) || !IsFinite(point.y))
            {
                ++clipped;
                continue;
            }

            ++projected;
            if (!rect.valid)
            {
                rect.minX = rect.maxX = point.x;
                rect.minY = rect.maxY = point.y;
                rect.valid = true;
            }
            else
            {
                rect.minX = std::min(rect.minX, point.x);
                rect.minY = std::min(rect.minY, point.y);
                rect.maxX = std::max(rect.maxX, point.x);
                rect.maxY = std::max(rect.maxY, point.y);
            }
        }

        return rect.valid && rect.maxX > rect.minX && rect.maxY > rect.minY;
    }

    void DrawCornerBox(ImDrawList* drawList, const Rect2D& rect, ImU32 color)
    {
        const float width = rect.maxX - rect.minX;
        const float height = rect.maxY - rect.minY;
        const float x = rect.minX;
        const float y = rect.minY;
        const float w = width * 0.25f;
        const float h = height * 0.22f;

        drawList->AddLine(ImVec2(x, y), ImVec2(x + w, y), color, g_boxThickness);
        drawList->AddLine(ImVec2(x, y), ImVec2(x, y + h), color, g_boxThickness);
        drawList->AddLine(ImVec2(x + width, y), ImVec2(x + width - w, y), color, g_boxThickness);
        drawList->AddLine(ImVec2(x + width, y), ImVec2(x + width, y + h), color, g_boxThickness);
        drawList->AddLine(ImVec2(x, y + height), ImVec2(x + w, y + height), color, g_boxThickness);
        drawList->AddLine(ImVec2(x, y + height), ImVec2(x, y + height - h), color, g_boxThickness);
        drawList->AddLine(ImVec2(x + width, y + height), ImVec2(x + width - w, y + height), color, g_boxThickness);
        drawList->AddLine(ImVec2(x + width, y + height), ImVec2(x + width, y + height - h), color, g_boxThickness);
    }

    void DrawEntityOverlay()
    {
        if (!g_drawEnabled)
            return;

        const std::uint32_t count = std::min<std::uint32_t>(AegisIdTech_GetEntityCount(), 512);
        if (count == 0)
            return;

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const ImU32 boxColor = ImGui::ColorConvertFloat4ToU32(g_boxColor);
        const ImU32 lineColor = ImGui::ColorConvertFloat4ToU32(g_lineColor);
        const ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(g_fillColor);

        std::uint32_t projected = 0;
        std::uint32_t clipped = 0;
        for (std::uint32_t i = 0; i < count; ++i)
        {
            AegisIdTechEntitySnapshot entity = {};
            if (!AegisIdTech_GetEntitySnapshot(i, &entity))
                continue;
            if (g_hideInvisible && !entity.visible)
                continue;

            Rect2D rect = {};
            if (!ProjectBounds(entity, rect, projected, clipped))
                continue;

            if (g_drawFilledBoxes)
                drawList->AddRectFilled(ImVec2(rect.minX, rect.minY), ImVec2(rect.maxX, rect.maxY), fillColor, 0.0f);
            if (g_drawBoxes)
                drawList->AddRect(ImVec2(rect.minX, rect.minY), ImVec2(rect.maxX, rect.maxY), boxColor, 0.0f, 0, g_boxThickness);
            if (g_drawCornerBoxes)
                DrawCornerBox(drawList, rect, boxColor);
            if (g_drawLines)
                drawList->AddLine(ImVec2(display.x * 0.5f, display.y), ImVec2((rect.minX + rect.maxX) * 0.5f, rect.maxY), lineColor, g_lineThickness);
            if (g_drawLabels && entity.name[0])
                drawList->AddText(ImVec2(rect.minX, std::max(0.0f, rect.minY - 16.0f)), boxColor, entity.name);
        }
    }

    void DrawEntityTable()
    {
        const std::uint32_t count = std::min<std::uint32_t>(AegisIdTech_GetEntityCount(), 128);
        if (ImGui::BeginTable("idtech-entities", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Class");
            ImGui::TableSetupColumn("Origin");
            ImGui::TableSetupColumn("Team");
            ImGui::TableSetupColumn("Visible");
            ImGui::TableSetupColumn("Flags");
            ImGui::TableHeadersRow();
            for (std::uint32_t i = 0; i < count; ++i)
            {
                AegisIdTechEntitySnapshot entity = {};
                if (!AegisIdTech_GetEntitySnapshot(i, &entity))
                    continue;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%llu", static_cast<unsigned long long>(entity.id));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entity.name);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entity.className);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f, %.2f, %.2f", entity.origin.x, entity.origin.y, entity.origin.z);
                ImGui::TableNextColumn();
                ImGui::Text("%d", entity.team);
                ImGui::TableNextColumn();
                ImGui::Text("%s", entity.visible ? "yes" : "no");
                ImGui::TableNextColumn();
                ImGui::Text("0x%08x", entity.flags);
            }
            ImGui::EndTable();
        }
    }
}

extern "C" const char* AegisUniversalOverlay_GetEngineOverlayName()
{
    return "id Tech";
}

extern "C" void AegisUniversalOverlay_PollEngineProviders()
{
    AegisIdTech_UpdateProviders();
}

extern "C" void AegisUniversalOverlay_DrawEngineOverlay()
{
    DrawEntityOverlay();
}

extern "C" void AegisUniversalOverlay_DrawEngineMenu()
{
    if (ImGui::BeginTabItem("Adapter"))
    {
        AegisIdTechCapabilityInfo capability = {};
        AegisIdTech_GetCapabilityInfo(&capability);
        AegisIdTechAdapterTiming timing = {};
        AegisIdTech_GetAdapterTiming(&timing);

        ImGui::Text("Renderer backend: %ls", capability.rendererBackend);
        ImGui::TextWrapped("Details: %ls", capability.details);
        ImGui::Separator();
        ImGui::Text("id Tech detected: %s", capability.idTechDetected ? "pass" : "warn");
        ImGui::Text("Game API: %s | VM API: %s | Renderer API: %s | Scripting API: %s",
            capability.gameApiFound ? "pass" : "warn",
            capability.vmApiFound ? "pass" : "warn",
            capability.rendererApiFound ? "pass" : "warn",
            capability.scriptingApiFound ? "pass" : "warn");
        ImGui::Text("Entity provider: %s | Matrix provider: %s | Viewport provider: %s",
            capability.entityProviderRegistered ? "pass" : "warn",
            capability.viewProjectionProviderRegistered ? "pass" : "warn",
            capability.viewportProviderRegistered ? "pass" : "warn");
        ImGui::Text("Viewport valid: %s | Matrix valid: %s | W2S: %s",
            capability.viewportValid ? "pass" : "warn",
            capability.matrixValid ? "pass" : "warn",
            capability.w2sProjectionWorking ? "pass" : "warn");
        ImGui::Text("Frame %llu | entities %u | projected %u | clipped %u",
            static_cast<unsigned long long>(timing.frameId),
            timing.entityCount,
            timing.projectedCount,
            timing.clippedCount);
        ImGui::Text("Provider timing: entity %.3f ms | matrix %.3f ms | viewport %.3f ms",
            timing.entityProviderMs,
            timing.matrixProviderMs,
            timing.viewportProviderMs);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Overlay"))
    {
        ImGui::Checkbox("Draw provider overlay", &g_drawEnabled);
        ImGui::Checkbox("Hide invisible entities", &g_hideInvisible);
        ImGui::Checkbox("Boxes", &g_drawBoxes);
        ImGui::Checkbox("Corner boxes", &g_drawCornerBoxes);
        ImGui::Checkbox("Filled boxes", &g_drawFilledBoxes);
        ImGui::Checkbox("Lines", &g_drawLines);
        ImGui::Checkbox("Labels", &g_drawLabels);
        ImGui::SliderFloat("Box thickness", &g_boxThickness, 0.5f, 6.0f, "%.1f");
        ImGui::SliderFloat("Line thickness", &g_lineThickness, 0.5f, 6.0f, "%.1f");
        ImGui::ColorEdit4("Box color", &g_boxColor.x);
        ImGui::ColorEdit4("Line color", &g_lineColor.x);
        ImGui::ColorEdit4("Fill color", &g_fillColor.x);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Entities"))
    {
        ImGui::Text("Entity count: %u", AegisIdTech_GetEntityCount());
        if (ImGui::Button("Print current entities to console"))
            AegisIdTech_PrintCurrentEntities();
        DrawEntityTable();
        ImGui::EndTabItem();
    }
}
