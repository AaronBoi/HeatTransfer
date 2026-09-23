#include "gui.h"

using namespace std;

#include "app_state.h"

string configurationFilepath = "resources/configuration.txt";

void LoadConfiguration()
{   
    string line;
    string value;
    string name;
    ifstream inFile(configurationFilepath);

    string delimiter = ": ";
    while (getline(inFile, line))
    {
        value = line.substr(line.find(delimiter) + delimiter.size());
        name = line.substr(0, line.find(delimiter));

        if (name == "numY") guiState.numY = stoi(value);
        if (name == "Pipe length") guiState.cylinderLength = stof(value);
        if (name == "Pipe outer diameter") guiState.cylinderDiameterOutside = stof(value);
        if (name == "Pipe wall thickness") guiState.wallThickness = stof(value);
        if (name == "Heat Per Distance") guiState.heatPerDistancekJcm = stof(value);
        if (name == "SensorPositionIndex") guiState.selectedSensor = stoi(value);
    }
    reset = true;
}

void SaveConfiguration()
{
    vector<string> names = {
        "numY",
        "Pipe length",
        "Pipe outer diameter",
        "Pipe wall thickness",
        "Heat Per Distance",
        "SensorPositionIndex",
    };
    vector<string> values = 
    {
        to_string(guiState.numY),
        to_string(guiState.cylinderLength),
        to_string(guiState.cylinderDiameterOutside),
        to_string(guiState.wallThickness),
        to_string(guiState.heatPerDistancekJcm),
        to_string(guiState.selectedSensor),
    };

    ofstream outFile(configurationFilepath);
    for (int i; i < int(names.size()); i++)
    {
        outFile << names[i] << ": " << values[i] << "\n";
    }
    
    reset = true;
}

void SaveSensorData(vector<float>timeArr, vector<float> temperatureArr)
{
    ofstream outFile("Sensordata/sensor.csv");
    outFile << "time(s)" << ";"<< "temperature(K)" << "\n";
    for (int i; i < int(timeArr.size()); i++)
    {
        outFile << showpoint << timeArr[i] << ";" << temperatureArr[i] << "\n";
    }
}


void simulationWindow()
{
    ImGui::Begin("Simulation GUI");

    if (ImGui::Button("Start"))
        running = true;
    if (ImGui::Button("Stop"))
        running = false;
    if (ImGui::Button("Reset"))
        reset = true;

    if (ImGui::Button("Kelvin/Celsius"))
        showAsKelvin = !showAsKelvin;

    ImGui::SameLine();
    if (showAsKelvin) {
        ImGui::Text("Kelvin");
    }
    else {
        ImGui::Text("Celsius");
    }

    ImGui::InputInt("Stop Time", &stopTime);

    ImGui::Text("Time passed: ");
    ImGui::SameLine();
    ImGui::Text("%f", timePassed);
    ImGui::SameLine();
    ImGui::Text(" seconds");

    ImGui::End();
    return;
}

void configurationWindow()
{
    ImGui::Begin("Configuration Window");

    ImGui::InputInt("numY", &guiState.numY);
    ImGui::InputFloat("Pipe length in mm", &guiState.cylinderLength);
    ImGui::InputFloat("Pipe outer diameter in mm", &guiState.cylinderDiameterOutside);
    ImGui::InputFloat("Pipe wall thickness in mm", &guiState.wallThickness);
    ImGui::InputFloat("Heat Per Distance in KJ/cm", &guiState.heatPerDistancekJcm);
    ImGui::InputFloat("Rotation Speed in cm/s", &guiState.rotationSpeedcms);

    if(ImGui::Button("Place Sensor")) selectMode = !selectMode;

    if(ImGui::Button("Load Preview")) reset = true;
    if(ImGui::Button("Save Configuration")) SaveConfiguration();
    //ImGui::SameLine();
    if(ImGui::Button("Load Configuration")) LoadConfiguration();

    ImGui::End();

    return;
}

void MakeImGui()
{
    rlImGuiBegin();
    

    simulationWindow();
    configurationWindow();

    //ImGui::ShowDemoWindow();

    
    rlImGuiEnd();
}



