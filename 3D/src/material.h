#pragma once

// Physical constants for a conductive material.
struct MaterialConstants {
    float thermalDiffusivity;    // m^2/s
    float density;                // kg/m^3
    float specificHeatCapacity;   // J/(kg*K)
    float thermalEmissivity;
};

extern MaterialConstants Steel;    // AISI 1010 steel
extern MaterialConstants material; // material currently active in the sim
