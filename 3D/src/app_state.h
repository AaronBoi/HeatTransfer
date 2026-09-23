#pragma once

struct ConfigState {
    float cylinderLength = 50.0f;           
    float cylinderDiameterOutside = 14.0f;
    float wallThickness = 2.0f;
    int numY = 30; 
    float roomTemperature = 293.0f;
    float InletPositionAngle = 0.0f; 
    float heatPerDistancekJcm = 0.15f;
    float rotationSpeedcms = 0.3;
    int selectedSensor = -1;
};

extern ConfigState guiState;

// Mutable runtime/UI state shared between the simulation loop, the renderer,
// and the ImGui panel.
extern bool running;
extern bool reset;
extern bool showAsKelvin;
extern bool activateMouse;
extern float timePassed;
extern float rotationSpeed; // degrees/second
extern bool selectMode;
extern int stopTime;


