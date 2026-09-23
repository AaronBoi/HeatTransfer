#pragma once

#include "material.h"

// Finds the (y, z) grid cell on the pipe's material surface at the given
// angle, for cross-section x. Returns false if no material cell is found
// (shouldn't happen for a valid cylinder cross-section).
bool findSurfaceCell(float angleDeg, int x, int &yOut, int &zOut);

// Advances the inlet's angular position by rotationSpeed * dt, clears the
// previous inlet cell, and marks/heats the new one.
float rotateAndApplyInlet(float dt);
