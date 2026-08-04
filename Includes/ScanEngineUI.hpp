#pragma once

#include "ScanEngineCore.hpp"

namespace ScanEngine
{

inline void DrawRegionCombo(State &state)
{
    static constexpr RegionType regions[] = {
        RegionType::ALL,
        RegionType::JAVA_HEAP,
        RegionType::C_HEAP,
        RegionType::C_ALLOC,
        RegionType::C_DATA,
        RegionType::C_BSS,
        RegionType::PPSSPP,
        RegionType::ANONYMOUS,
        RegionType::JAVA,
        RegionType::STACK,
        RegionType::ASHMEM,
        RegionType::VIDEO,
        RegionType::OTHER,
        RegionType::BAD,
        RegionType::CODE_APP,
        RegionType::CODE_SYS
    };
    if (ImGui::BeginCombo("Region", RegionTypeName(state.region)))
    {
        for (RegionType region : regions)
        {
            bool selected = state.region == region;
            if (ImGui::Selectable(RegionTypeName(region), selected)) state.region = region;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

inline void DrawTypeCombo(State &state)
{
    static constexpr ValueType types[] = {
        ValueType::DWORD,
        ValueType::FLOAT,
        ValueType::DOUBLE,
        ValueType::WORD,
        ValueType::BYTE,
        ValueType::QWORD
    };
    if (ImGui::BeginCombo("Type", ValueTypeName(state.type)))
    {
        for (ValueType type : types)
        {
            bool selected = state.type == type;
            if (ImGui::Selectable(ValueTypeName(type), selected)) state.type = type;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

inline void DrawResults(State &state)
{
    bool requestPopup = false;
    if (ImGui::BeginTable("ScanResults", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 420.0f)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 230.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableHeadersRow();
        size_t count = 0;
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            count = std::min(state.results.size(), VisibleResultLimit);
        }
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(count));
        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
            {
                Result result{};
                {
                    std::lock_guard<std::mutex> lock(state.mutex);
                    if (static_cast<size_t>(row) >= state.results.size()) continue;
                    result = state.results[static_cast<size_t>(row)];
                }
                std::array<guint8, 8> current{};
                if (SafeRead(result.address, current.data(), ValueTypeSize(result.type)))
                {
                    result.bytes = current;
                    std::lock_guard<std::mutex> lock(state.mutex);
                    if (static_cast<size_t>(row) < state.results.size() && state.results[static_cast<size_t>(row)].address == result.address) state.results[static_cast<size_t>(row)].bytes = current;
                }
                std::string valueText = FormatValue(result.type, result.bytes.data());
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%llX", static_cast<unsigned long long>(result.address));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(valueText.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::PushID(row);
                if (ImGui::Button("Edit"))
                {
                    state.selectedResult = row;
                    std::snprintf(state.editValue, sizeof(state.editValue), "%s", valueText.c_str());
                    requestPopup = true;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    if (requestPopup) ImGui::OpenPopup("Edit Value");
    if (ImGui::BeginPopupModal("Edit Value", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        Result selected{};
        bool valid = false;
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            if (state.selectedResult >= 0 && static_cast<size_t>(state.selectedResult) < state.results.size())
            {
                selected = state.results[static_cast<size_t>(state.selectedResult)];
                valid = true;
            }
        }
        if (valid)
        {
            ImGui::Text("Address: 0x%llX", static_cast<unsigned long long>(selected.address));
            ImGui::Text("Type: %s", ValueTypeName(selected.type));
            ImGui::InputText("New Value", state.editValue, sizeof(state.editValue));
            if (ImGui::Button("Write"))
            {
                if (WriteResult(static_cast<size_t>(state.selectedResult), state.editValue))
                {
                    SetStatus(state, "Value written");
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    SetStatus(state, "Write failed");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        }
        else
        {
            ImGui::TextUnformatted("Result is no longer available");
            if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

inline void DrawUI()
{
    State &state = GetState();
    DrawRegionCombo(state);
    DrawTypeCombo(state);
    ImGui::InputText("Value", state.value, sizeof(state.value));
    bool busy = state.busy.load(std::memory_order_acquire);
    ImGui::BeginDisabled(busy);
    if (ImGui::Button("Search")) StartSearch(false);
    ImGui::SameLine();
    if (ImGui::Button("Refine")) StartSearch(true);
    ImGui::EndDisabled();
    size_t resultCount = 0;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        resultCount = state.results.size();
    }
    std::string statusText = GetStatus(state);
    ImGui::Text("Status: %s", statusText.c_str());
    ImGui::Text("Results: %zu", resultCount);
    if (busy) ImGui::Text("Processed: %zu", state.scanned.load(std::memory_order_relaxed));
    if (resultCount > VisibleResultLimit) ImGui::Text("Showing first %zu results", VisibleResultLimit);
    DrawResults(state);
}

}
