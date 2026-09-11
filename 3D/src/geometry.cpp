#include "geometry.h"

#include <cmath>
#include <cstring>

#include "grid_config.h"
#include "grid_state.h"
#include "app_state.h"

using namespace std;

void setCylinderHollow()
{
    float middle_y = numY / 2.0;
    float middle_z = numZ / 2.0;
    float radius = numY / 2.0 - 1; // so there is 1 layer of air

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                float midpoint_sq = pow(y - middle_y + 0.5, 2) + pow(z - middle_z + 0.5, 2);
                if (midpoint_sq <= pow(radius, 2))
                {
                    isMaterial[x][y][z] = 1;
                }
            }
        }
    }

    radius = radius * cylinderRadiusInside / cylinderRadiusOutside - 1;
    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                float midpoint_sq = pow(y - middle_y + 0.5, 2) + pow(z - middle_z + 0.5, 2);
                if (midpoint_sq <= pow(radius, 2))
                {
                    isMaterial[x][y][z] = 0;
                }
            }
        }
    }
}

void setPlane()
{
    for (int y = 1; y < numY - 1; y++) {
        for (int z = 1; z < numZ - 1; z++) {
            isMaterial[numX / 2][y][z] = 1;
            isMaterial[numX / 2 - 1][y][z] = 1;
            isMaterial[numX / 2 + 1][y][z] = 1;
        }
    }
}
