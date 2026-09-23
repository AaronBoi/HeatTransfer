#pragma once

#include <vector>
#include <cmath>
#include "app_state.h"
using namespace std;

const float StefanBoltzmannConstant = 5.67f * pow(10, -8); // W/(m^2*K^4)

extern float dt;

extern float cylinderLength;              // meters
extern float cylinderRadiusInside;  // meters
extern float cylinderRadiusOutside; // meters

extern int numY; // number of cells along Y
extern int numX;
extern int numZ;

extern float cellSize; // meters
extern float cellVolume;    // m^3
extern float drawSize;          // render-space cube size

extern float roomTemperature; // Kelvin, initial temperature of all grid cells

extern float rotationSpeedcms;     // Rotationspeed of Inlet in cm/s
extern float InletPositionAngle;   // On which angle the inlet is currently on the pipe (Radians)
extern float InletPositionX;   // Where on the X axis the inlet is
extern float heatPerDistance;   // J/m
extern float heatPerSecond;


// Per-cell simulation state, shared by the physics solvers and the renderer.
extern vector<float> temperature; // current temperature field (K)
extern vector<float> temp;        // scratch buffer used mid-solve
extern vector<float> heatInlet;   // per-cell heating rate (K/s)
extern vector<int>   isMaterial;  // 1 = conductive solid, 0 = air/gas

int idx(int x, int y, int z);


void updateFromConfigState(ConfigState state);


extern int timings[10]; // rough profiling: microseconds per named stage
