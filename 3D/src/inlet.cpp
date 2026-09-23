#include "inlet.h"

#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>

#include "grid_state.h"
#include "app_state.h"

bool findSurfaceCell(float angleRad, int x, int &yOut, int &zOut)
{
    float middle_y = numY / 2.0f;
    float middle_z = numZ / 2.0f;
    float outerRadius = numY / 2.0f;

    // Step inward from just past the outer edge until we hit real material
    for (float r = outerRadius; r > 0; r -= 0.25f)
    {
        int y = (int)floorf(middle_y + r * sinf(angleRad));
        int z = (int)floorf(middle_z + r * cosf(angleRad));

        y = std::max(0, std::min(numY - 1, y));
        z = std::max(0, std::min(numZ - 1, z));

        if (isMaterial[idx(x, y, z)] == 1)
        {
            yOut = y;
            zOut = z;
            return true;
        }
    }
    return false; // shouldn't happen if x is a valid cylinder cross-section
}

float rotateAndApplyInlet(float dt)
{
    int x = InletPositionX;
    int y, z;

    findSurfaceCell(InletPositionAngle, x, y, z);
    heatInlet[idx(x, y, z)] = 0;

    InletPositionAngle += rotationSpeedcms / (100.0f * cylinderRadiusOutside) * dt;

    findSurfaceCell(InletPositionAngle, x, y, z);

    if (rotationSpeedcms > 0.01)
    {
        heatPerSecond = heatPerDistance / (rotationSpeedcms); // J/s
    }
    
    heatInlet[idx(x, y, z)] = heatPerSecond / (material.specificHeatCapacity * material.density * cellVolume);
    isMaterial[idx(x, y, z)] = 1.0;

    temperature[idx(x, y, z)] += dt * heatInlet[idx(x, y, z)];

    return InletPositionAngle;
}
