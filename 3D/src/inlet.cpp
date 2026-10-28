#include "inlet.h"

#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>

bool findSurfaceCell(Scene& scene, float angleRad, int x, int &yOut, int &zOut)
{
    int numY = scene.state.numY;
    int numZ = scene.state.numZ;
    float middle_y =  numY / 2.0f;
    float middle_z = numZ / 2.0f;
    float outerRadius = numY / 2.0f;

    // Step inward from just past the outer edge until we hit real material
    for (float r = outerRadius; r > 0; r -= 0.25f)
    {
        int y = (int)floorf(middle_y + r * sinf(angleRad));
        int z = (int)floorf(middle_z + r * cosf(angleRad));

        y = std::max(0, std::min(numY - 1, y));
        z = std::max(0, std::min(numZ - 1, z));

        if (scene.state.isMaterial[scene.idx(x, y, z)] == 1)
        {
            yOut = y;
            zOut = z;
            return true;
        }
    }
    return false; // shouldn't happen if x is a valid cylinder cross-section
}

float rotateAndApplyInlet(Scene& scene, float simdt)
{   
    InletState& inlet = scene.inlet;

    if (inlet.hasActiveCell)
    {
        scene.state.heatInlet[scene.idx(inlet.activeX, inlet.activeY, inlet.activeZ)] = 0;
    }

    inlet.positionAngle += inlet.rotationSpeed / (scene.state.cylinderRadiusOutside) * simdt;

    int x = inlet.positionX;
    int y, z;

    if (!findSurfaceCell(scene, inlet.positionAngle, x, y, z))
    {
        inlet.hasActiveCell = false;
        return inlet.positionAngle;
    }
    


    scene.state.heatInlet[scene.idx(x, y, z)] = inlet.heatPerSecond / (material.specificHeatCapacity * material.density * scene.state.cellVolume);
    //isMaterial[idx(x, y, z)] = 1.0;

    scene.state.temperature[scene.idx(x, y, z)] += simdt * scene.state.heatInlet[scene.idx(x, y, z)];

    inlet.activeX = x;
    inlet.activeY = y;
    inlet.activeZ = z;
    inlet.hasActiveCell = true;

    return inlet.positionAngle;
}
