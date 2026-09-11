#include "gui.h"

#include <imgui.h>
#include <rlImGui.h>
#include <cmath>

#include "app_state.h"

void MakeImGui()
{
    rlImGuiBegin();
    ImGui::Begin("Simulation GUI");

    if (ImGui::Button("Start"))
        running = true;
    if (ImGui::Button("Stop"))
        running = false;
    if (ImGui::Button("Reset"))
        reset = true;
    if (ImGui::Button("Walls with mouseclick"))
    {
        walls_with_mouse = !walls_with_mouse;
    }
    ImGui::SameLine();
    ImGui::Text("%d", walls_with_mouse);

    ImGui::Spacing();
    ImGui::Text("Max Temperature Color: ");
    ImGui::SliderInt("Max Temp Color", &maxDrawTemp, 300, 10000);
    ImGui::Spacing();

    if (ImGui::Button("Kelvin/Celsius"))
        showAsKelvin = !showAsKelvin;

    ImGui::SameLine();
    if (showAsKelvin) {
        ImGui::Text("Kelvin");
    }
    else {
        ImGui::Text("Celsius");
    }

    ImGui::Text("Time passed: ");
    ImGui::SameLine();
    ImGui::Text("%f", round(timePassed));
    ImGui::SameLine();
    ImGui::Text(" seconds");

    ImGui::Spacing();
    ImGui::Text("Rotation Speed (°/s):");
    ImGui::InputFloat("Rot Speed", &rotationSpeed);
    ImGui::Spacing();

    //ImGui::ShowDemoWindow();

    ImGui::End();
    rlImGuiEnd();
}
