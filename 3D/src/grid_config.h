#pragma once

#include <cmath>

// Constants for the Simulation. Physical constants and also geometric variables for world building / discretization.

const float dt = 1.0f / 60;

const float cylinderLength = 0.05f;              // meters
const float cylinderRadiusInside = 0.011f / 2;  // meters
const float cylinderRadiusOutside = 0.014f / 2; // meters

const int numY = 30; // number of cells along Y
const int numX = float(numY) * cylinderLength / cylinderRadiusOutside / 2;
const int numZ = numY;

const float cellSize = cylinderLength / numX; // meters
const float cellVolume = pow(cellSize, 3);    // m^3
const float drawSize = cylinderLength / numX * 50;          // render-space cube size

const float roomTemperature = 0.0f; // Kelvin, initial temperature of all grid cells

const float StefanBoltzmannConstant = 5.67f * pow(10, -8); // W/(m^2*K^4)
