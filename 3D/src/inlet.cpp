#include "inlet.h"

#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>

#include "grid_config.h"
#include "grid_state.h"
#include "app_state.h"

float InletPositionAngle = 0.0f;   // On which angle the inlet is currently on the pipe
float InletPositionX = numX / 2;   // Where on the X axis the inlet is

float heatPerDistance = heatPerDistancekJcm * 1000 * 100;   // J/m
float heatPerSecond = 100;

bool findSurfaceCell(float angleDeg, int x, int &yOut, int &zOut)
{
    float angleRad = angleDeg * DEG2RAD;
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

        if (isMaterial[x][y][z] == 1)
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
    heatInlet[x][y][z] = 0;

    InletPositionAngle += rotationSpeed * dt;

    findSurfaceCell(InletPositionAngle, x, y, z);

    if (rotationSpeed > 0.01)
    {
        heatPerSecond = heatPerDistance / (rotationSpeed * cylinderRadiusOutside); // J/s
    }
    
    heatInlet[x][y][z] = heatPerSecond / (material.specificHeatCapacity * material.density * cellVolume);
    isMaterial[x][y][z] = 1.0;

    temperature[x][y][z] += dt * heatInlet[x][y][z];

    return InletPositionAngle;
}
