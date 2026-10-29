#include "heat_solver.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>

#include "thomas_algorithm.h"

using namespace std;
using namespace std::chrono;

void radiationLoss(Scene& scene, float simdt)
{
    auto start = high_resolution_clock::now();

    auto& temp = scene.state.temp;
    auto& temperature = scene.state.temperature;
    auto& isMaterial = scene.state.isMaterial;


    int numX = scene.state.numX;
    int numY = scene.state.numY;
    int numZ = scene.state.numZ;

    float faceSurface = pow(scene.state.cellSize, 2);
    float mass = pow(scene.state.cellSize, 3) * material.density;
    float thermalMass = material.specificHeatCapacity * mass; // J/K

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {

                if (isMaterial[scene.idx(x, y, z)] == 0) continue;

                int x_p = x + 1, x_n = x - 1;
                int y_p = y + 1, y_n = y - 1;
                int z_p = z + 1, z_n = z - 1;

                if (x_p >= numX) x_p = x;
                if (x_n < 0)     x_n = x;
                if (y_p >= numY) y_p = y;
                if (y_n < 0)     y_n = y;
                if (z_p >= numZ) z_p = z;
                if (z_n < 0)     z_n = z;

                float T_self = temperature[scene.idx(x, y, z)];

                // Linearize q = eps*sigma*(T^4 - T_amb^4) as q = h*(T_self - T_amb),
                // with h evaluated at the start-of-step temperature. This turns a very
                // stiff explicit T^4 update into an ordinary implicit (backward-Euler)
                // linear solve per cell — unconditionally stable regardless of simdt.
                float G = 0.0f;         // total radiative conductance, W/K
                float G_Tamb_sum = 0.0f; // sum of (conductance * ambient temp), W

                auto accumulate = [&](int nx, int ny, int nz) {
                    if (isMaterial[scene.idx(nx, ny, nz)] != 0) return;
                    float T_amb = temperature[scene.idx(nx, ny, nz)];
                    float h = material.thermalEmissivity * scene.StefanBoltzmannConstant
                              * (T_self * T_self + T_amb * T_amb) * (T_self + T_amb);
                    float g = h * faceSurface;
                    G += g;
                    G_Tamb_sum += g * T_amb;
                };

                accumulate(x_p, y, z);
                accumulate(x_n, y, z);
                accumulate(x, y_p, z);
                accumulate(x, y_n, z);
                accumulate(x, y, z_p);
                accumulate(x, y, z_n);

                // Implicit solve of: thermalMass*(T_new - T_self)/simdt = G*(T_amb_avg - T_new)
                temp[scene.idx(x, y, z)] = (thermalMass * T_self + simdt * G_Tamb_sum) / (thermalMass + simdt * G);
            }
        }
    }
    //memcpy(temperature, temp, sizeof(temperature));
    temperature = temp;

    auto stop = high_resolution_clock::now();
    timings[2] = duration_cast<microseconds>(stop - start).count();
}

// ADI Crank-Nicolson (Douglas-Gunn form) where heat only flows between two solid cells.
// Air faces and domain edges are insulated: the coupling is simply missing, both in the
// explicit part (heatFlow) and in the matrix (lower/diagonal/upper) of every sweep.
// Every sweep solves ALL cells of a line, so there are no special edge layers or ghost cells.
 
// Solves (I - implicitWeight * L_axis) x = values along one line of cells with ThomasAlgorithm.
// 'values' holds the right-hand side going in and the solution coming out.
static void solveLine(const vector<int>& isMaterial, vector<float>& values,
                      int firstCell, int stride, int lineLength, float implicitWeight)
{
    vector<float> lower(lineLength), diagonal(lineLength), upper(lineLength), rightHandSide(lineLength);
 
    for (int i = 0; i < lineLength; i++) {
        int cell = firstCell + i * stride;
        bool isSolid        = isMaterial[cell] != 0;
        bool coupledToPrev  = isSolid && i > 0              && isMaterial[cell - stride] != 0;
        bool coupledToNext  = isSolid && i < lineLength - 1 && isMaterial[cell + stride] != 0;
 
        // Every coupled neighbor adds -weight off the diagonal and +weight on it.
        // Air cells and uncoupled sides get 0, so an air cell becomes an identity row.
        lower[i]    = coupledToPrev ? -implicitWeight : 0.0f;
        upper[i]    = coupledToNext ? -implicitWeight : 0.0f;
        diagonal[i] = 1.0f + implicitWeight * ((coupledToPrev ? 1 : 0) + (coupledToNext ? 1 : 0));
        rightHandSide[i] = values[cell];
    }
 
    vector<float> solution = ThomasAlgorithm(lower, diagonal, upper, rightHandSide);
 
    for (int i = 0; i < lineLength; i++)
        values[firstCell + i * stride] = solution[i];
}
 
// theta = 0.5 -> Crank-Nicolson, theta = 1 -> implicit Euler (damps rough data, first-order).
void heatConductionCrankNicolson(Scene& scene, float simdt, MaterialConstants material, float theta = 0.5f)
{
    auto start = high_resolution_clock::now();
 
    const int numX = scene.state.numX;
    const int numY = scene.state.numY;
    const int numZ = scene.state.numZ;
    auto& temperature = scene.state.temperature;
    auto& increment   = scene.state.temp;     // work buffer: temperature change of this step
    const auto& isMaterial = scene.state.isMaterial;
 
    const int strideX = numY * numZ;
    const int strideY = numZ;
    const int strideZ = 1;
 
    // r = alpha*dt/h^2: how much of a cell's temperature difference to a neighbor flows per step
    const float diffusionNumber = material.thermalDiffusivity * simdt / (scene.state.cellSize * scene.state.cellSize);
    const float implicitWeight  = theta * diffusionNumber;
 
    // increment = r * (sum over solid neighbors of (T_neighbor - T_cell)).
    // Each solid-solid face is counted once with + and once with - , so the total is exactly 0.
    for (int i = 0; i < numX; i++) {
        for (int j = 0; j < numY; j++) {
            for (int k = 0; k < numZ; k++) {
                int cell = scene.idx(i, j, k);
                if (!isMaterial[cell]) { increment[cell] = 0.0f; continue; }
 
                float heatFlow = 0.0f;
                float T = temperature[cell];
                if (i > 0        && isMaterial[cell - strideX]) heatFlow += temperature[cell - strideX] - T;
                if (i < numX - 1 && isMaterial[cell + strideX]) heatFlow += temperature[cell + strideX] - T;
                if (j > 0        && isMaterial[cell - strideY]) heatFlow += temperature[cell - strideY] - T;
                if (j < numY - 1 && isMaterial[cell + strideY]) heatFlow += temperature[cell + strideY] - T;
                if (k > 0        && isMaterial[cell - strideZ]) heatFlow += temperature[cell - strideZ] - T;
                if (k < numZ - 1 && isMaterial[cell + strideZ]) heatFlow += temperature[cell + strideZ] - T;
 
                increment[cell] = diffusionNumber * heatFlow;
            }
        }
    }
    // X sweep, then Y sweep, then Z sweep: each one is implicit along a single axis.
    for (int j = 0; j < numY; j++)
        for (int k = 0; k < numZ; k++)
            solveLine(isMaterial, increment, scene.idx(0, j, k), strideX, numX, implicitWeight);
 
    for (int i = 0; i < numX; i++)
        for (int k = 0; k < numZ; k++)
            solveLine(isMaterial, increment, scene.idx(i, 0, k), strideY, numY, implicitWeight);
 
    for (int i = 0; i < numX; i++)
        for (int j = 0; j < numY; j++)
            solveLine(isMaterial, increment, scene.idx(i, j, 0), strideZ, numZ, implicitWeight);
 
    for (size_t cell = 0; cell < temperature.size(); cell++)
        temperature[cell] += increment[cell];
 
    auto stop = high_resolution_clock::now();
    timings[0] = duration_cast<microseconds>(stop - start).count();
}





