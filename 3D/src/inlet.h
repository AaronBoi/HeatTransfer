#pragma once

#include "material.h"

// Where on the pipe's circumference / length the heat inlet currently sits.
extern float InletPositionAngle; // degrees
extern float InletPositionX;     // grid index along X

// Heating power delivered by the inlet.
extern float heatPerDistance;     // J/m, derived from the above
extern float heatPerSecond;             // J/s, recomputed from rotation speed

// Finds the (y, z) grid cell on the pipe's material surface at the given
// angle, for cross-section x. Returns false if no material cell is found
// (shouldn't happen for a valid cylinder cross-section).
bool findSurfaceCell(float angleDeg, int x, int &yOut, int &zOut);

// Advances the inlet's angular position by rotationSpeed * dt, clears the
// previous inlet cell, and marks/heats the new one.
float rotateAndApplyInlet(float dt);
