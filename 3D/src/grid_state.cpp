#include "grid_state.h"


ConfigState def;

float dt = 1.0f / 60;

float cylinderLength = 0.05f;           
float cylinderRadiusInside = 0.011f / 2;  
float cylinderRadiusOutside = 0.014f / 2;

int numY = 30; 
int numX = float(numY) * cylinderLength / cylinderRadiusOutside / 2;
int numZ = numY;

float cellSize = cylinderLength / numX;
float cellVolume = pow(cellSize, 3);   
float drawSize = cylinderLength / numX * 50;         

float roomTemperature = 293.0f;

float rotationSpeedcms = 1;
float InletPositionAngle = 0.0f; 
float InletPositionX = numX / 2;   
float heatPerSecond = 100;
float heatPerDistance;

vector<float> temperature(numX * numY * numZ);
vector<float> temp(numX * numY * numZ);
vector<float> heatInlet(numX * numY * numZ);
vector<int>   isMaterial(numX * numY * numZ);

int idx(int x, int y, int z) {
    return (x * numY + y) * numZ + z;
}

void updateFromConfigState(ConfigState state)
//Calculates variables from GUI input, like HeatPerDistanceKJcm -> HeatPerDistance
{
    cylinderLength = state.cylinderLength / 1000.0f;
    
    cylinderRadiusOutside = (state.cylinderDiameterOutside / 2) / 1000.0f;
    cylinderRadiusInside = (cylinderRadiusOutside - state.wallThickness / 1000.0f);

    numX = float(state.numY) * cylinderLength / cylinderRadiusOutside / 2 + 4;
    numY = state.numY + 4;
    numZ = state.numY + 4;
    cellSize = cylinderLength / numX;
    cellVolume = pow(cellSize, 3);   
    drawSize = cylinderLength / numX * 50;    
    heatPerDistance = state.heatPerDistancekJcm * 1000 * 100;

    InletPositionX = numX / 2;
    InletPositionAngle = state.InletPositionAngle;

    rotationSpeedcms = state.rotationSpeedcms;

    temperature.resize(numX * numY * numZ);
    temp.resize(numX * numY * numZ);
    heatInlet.resize(numX * numY * numZ);
    isMaterial.resize(numX * numY * numZ);

}

int timings[10];
