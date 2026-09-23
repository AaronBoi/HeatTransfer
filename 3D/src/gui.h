#pragma once

#include <imgui.h>
#include <rlImGui.h>
#include <cmath>
#include <vector>
#include <fstream>
#include <string>
#include <iostream>

using namespace std;

// Renders the ImGui control panel (start/stop/reset, temperature color
// scale, Kelvin/Celsius toggle, elapsed time, rotation speed).
void MakeImGui();

void SaveSensorData(vector<float>timeArr, vector<float> temperature_arr);
