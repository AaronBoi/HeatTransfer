#pragma once

#include "grid_config.h"

// Per-cell simulation state, shared by the physics solvers and the renderer.
extern float temperature[numX][numY][numZ]; // current temperature field (K)
extern float temp[numX][numY][numZ];        // scratch buffer used mid-solve
extern float heatInlet[numX][numY][numZ];   // per-cell heating rate (K/s)
extern int   isMaterial[numX][numY][numZ];  // 1 = conductive solid, 0 = air/gas

extern int timings[10]; // rough profiling: microseconds per named stage
