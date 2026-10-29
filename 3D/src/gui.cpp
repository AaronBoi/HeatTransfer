#include "gui.h"





string configurationFilepath = "resources/configuration.txt";


void LoadConfiguration(Scene& scene)
{   
    Config& config = scene.config;
    string line;
    string value;
    string name;
    ifstream inFile(configurationFilepath);

    string delimiter = ": ";
    while (getline(inFile, line))
    {
        value = line.substr(line.find(delimiter) + delimiter.size());
        name = line.substr(0, line.find(delimiter));

        if (name == "numY") config.cylinder_numY = stoi(value);
        if (name == "Pipe length") config.cylinderLength_mm = stof(value);
        if (name == "Pipe outer diameter") config.cylinderDiameterOutside_mm = stof(value);
        if (name == "Pipe wall thickness") config.wallThickness_mm = stof(value);
        if (name == "Heat Per Distance") config.heatPerDistancekJcm = stof(value);
        if (name == "SensorPositionIndex") config.selectedSensor = stoi(value);
    }
    scene.gui.reset = true;
    

    
}

void SaveConfiguration(Scene& scene)
{
    Config& config = scene.config;
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
        to_string(config.cylinder_numY),
        to_string(config.cylinderLength_mm),
        to_string(config.cylinderDiameterOutside_mm),
        to_string(config.wallThickness_mm),
        to_string(config.heatPerDistancekJcm),
        to_string(config.selectedSensor),
    };

    ofstream outFile(configurationFilepath);
    for (int i; i < int(names.size()); i++)
    {
        outFile << names[i] << ": " << values[i] << "\n";
    }
    
    scene.gui.reset = true;
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


void simulationWindow(Scene& scene)
{
    ImGui::Begin("Simulation GUI");
    ImGui::BeginDisabled(scene.gui.cameraActive);

    if (ImGui::Button("Start"))
        scene.gui.running = true;
    if (ImGui::Button("Stop"))
        scene.gui.running = false;
    if (ImGui::Button("Reset"))
        scene.gui.reset = true;

    if (ImGui::Button("Kelvin/Celsius"))
        scene.gui.showAsKelvin = !scene.gui.showAsKelvin;

    ImGui::SameLine();
    if (scene.gui.showAsKelvin) {
        ImGui::Text("Kelvin");
    }
    else {
        ImGui::Text("Celsius");
    }

    ImGui::InputInt("Stop Time", &scene.gui.stopTime);

    ImGui::Text("Time passed: ");
    ImGui::SameLine();
    ImGui::Text("%f", scene.gui.timePassed);
    ImGui::SameLine();
    ImGui::Text(" seconds");

    ImGui::Text("SpeedUp Factor");
    ImGui::SliderFloat("SpeedUp", &scene.gui.speedUp, 0.1f, 3.0f);

    ImGui::EndDisabled(); 
    ImGui::End();
    return;
    
}

void configurationWindow(Scene& scene)
{
    ImGui::Begin("Configuration Window");
    ImGui::BeginDisabled(scene.gui.cameraActive);

    ImGui::InputInt("numY", &scene.config.cylinder_numY);
    ImGui::InputFloat("Pipe length in mm", &scene.config.cylinderLength_mm);
    ImGui::InputFloat("Pipe outer diameter in mm", &scene.config.cylinderDiameterOutside_mm);
    ImGui::InputFloat("Pipe wall thickness in mm", &scene.config.wallThickness_mm);
    ImGui::InputFloat("Heat Per Distance in KJ/cm", &scene.config.heatPerDistancekJcm);
    ImGui::InputFloat("Rotation Speed in cm/s", &scene.config.rotationSpeedcms);

    if(ImGui::Button("Place Sensor")) scene.gui.selectMode = !scene.gui.selectMode;

    if(ImGui::Button("Load Preview")) scene.gui.reset = true;
    if(ImGui::Button("Save Configuration")) SaveConfiguration(scene);
    //ImGui::SameLine();
    if(ImGui::Button("Load Configuration")) LoadConfiguration(scene);

    ImGui::EndDisabled(); 
    ImGui::End();

    return;
}

void MakeImGui(Scene& scene)
{
    rlImGuiBegin();
    

    simulationWindow(scene);
    configurationWindow(scene);

    //ImGui::ShowDemoWindow();

    
    rlImGuiEnd();
}



