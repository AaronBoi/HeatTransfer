#include "heat_solver.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>

#include "grid_state.h"
#include "thomas_algorithm.h"

using namespace std;
using namespace std::chrono;

// void heatConduction(float simdt)
// {
//     auto start = high_resolution_clock::now();
//     int x_p, x_n, y_p, y_n, z_p, z_n;

//     float faceSurface = pow(cellSize, 2);
//     float mass = pow(cellSize, 3) * material.density;

//     for (int x = 0; x < numX; x++) {
//         for (int y = 0; y < numY; y++) {
//             for (int z = 0; z < numZ; z++) {

//                 if (isMaterial[idx(x, y, z)] == 0) continue;

//                 x_p = x + 1;
//                 x_n = x - 1;
//                 y_p = y + 1;
//                 y_n = y - 1;
//                 z_p = z + 1;
//                 z_n = z - 1;

//                 if (x_p >= numX) x_p = x;
//                 if (x_n < 0)     x_n = x;
//                 if (y_p >= numY) y_p = y;
//                 if (y_n < 0)     y_n = y;
//                 if (z_p >= numZ) z_p = z;
//                 if (z_n < 0)     z_n = z;

//                 // Intersurface thermal radiation effects.
//                 // Penetration depth of steel is only ~120nm, so no intermetallic "fake" conduction effect from thermal radiation.
//                 // So only account for radiation loss when surface is next to air.
                
//                 float var = 0;
//                 if (isMaterial[x_p][y][z] == 0) var += pow(temperature[x_p][y][z], 4) - pow(temperature[idx(x, y, z)], 4);
//                 if (isMaterial[x_n][y][z] == 0) var += pow(temperature[x_n][y][z], 4) - pow(temperature[idx(x, y, z)], 4);
//                 if (isMaterial[x][y_p][z] == 0) var += pow(temperature[x][y_p][z], 4) - pow(temperature[idx(x, y, z)], 4);
//                 if (isMaterial[x][y_n][z] == 0) var += pow(temperature[x][y_n][z], 4) - pow(temperature[idx(x, y, z)], 4);
//                 if (isMaterial[x][y][z_p] == 0) var += pow(temperature[x][y][z_p], 4) - pow(temperature[idx(x, y, z)], 4);
//                 if (isMaterial[x][y][z_n] == 0) var += pow(temperature[x][y][z_n], 4) - pow(temperature[idx(x, y, z)], 4);

//                 float radiationHeat = material.thermalEmissivity * faceSurface * StefanBoltzmannConstant * var;
//                 float radiationTempChange = radiationHeat / (material.specificHeatCapacity * mass);

//                 if (isMaterial[x_p][y][z] == 0) x_p = x;
//                 if (isMaterial[x_n][y][z] == 0) x_n = x;
//                 if (isMaterial[x][y_p][z] == 0) y_p = y;
//                 if (isMaterial[x][y_n][z] == 0) y_n = y;
//                 if (isMaterial[x][y][z_p] == 0) z_p = z;
//                 if (isMaterial[x][y][z_n] == 0) z_n = z;

//                 // Normal heat conduction with central differential quotient.
//                 float dudx_2 = temperature[x_p][y][z] - 2 * temperature[idx(x, y, z)] + temperature[x_n][y][z];
//                 float dudy_2 = temperature[x][y_p][z] - 2 * temperature[idx(x, y, z)] + temperature[x][y_n][z];
//                 float dudz_2 = temperature[x][y][z_p] - 2 * temperature[idx(x, y, z)] + temperature[x][y][z_n];

//                 float laplaceU = 0.5 / pow(cellSize, 2) * (dudx_2 + dudy_2 + dudz_2);

//                 float dusimdt = material.thermalDiffusivity * laplaceU;

//                 temp[idx(x, y, z)] = temperature[idx(x, y, z)] + simdt * (dusimdt + radiationTempChange);
//             }
//         }
//     }
//     memcpy(temperature, temp, sizeof(temperature));

//     auto stop = high_resolution_clock::now();
//     timings[0] = duration_cast<microseconds>(stop - start).count();
// }

void radiationLoss(float simdt)
{
    auto start = high_resolution_clock::now();

    float faceSurface = pow(cellSize, 2);
    float mass = pow(cellSize, 3) * material.density / 10;    // Division Factor because 
    float thermalMass = material.specificHeatCapacity * mass; // J/K

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {

                if (isMaterial[idx(x, y, z)] == 0) continue;

                int x_p = x + 1, x_n = x - 1;
                int y_p = y + 1, y_n = y - 1;
                int z_p = z + 1, z_n = z - 1;

                if (x_p >= numX) x_p = x;
                if (x_n < 0)     x_n = x;
                if (y_p >= numY) y_p = y;
                if (y_n < 0)     y_n = y;
                if (z_p >= numZ) z_p = z;
                if (z_n < 0)     z_n = z;

                float T_self = temperature[idx(x, y, z)];

                // Linearize q = eps*sigma*(T^4 - T_amb^4) as q = h*(T_self - T_amb),
                // with h evaluated at the start-of-step temperature. This turns a very
                // stiff explicit T^4 update into an ordinary implicit (backward-Euler)
                // linear solve per cell — unconditionally stable regardless of simdt.
                float G = 0.0f;         // total radiative conductance, W/K
                float G_Tamb_sum = 0.0f; // sum of (conductance * ambient temp), W

                auto accumulate = [&](int nx, int ny, int nz) {
                    if (isMaterial[idx(nx, ny, nz)] != 0) return;
                    float T_amb = temperature[idx(nx, ny, nz)];
                    float h = material.thermalEmissivity * StefanBoltzmannConstant
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
                temp[idx(x, y, z)] = (thermalMass * T_self + simdt * G_Tamb_sum) / (thermalMass + simdt * G);
            }
        }
    }
    //memcpy(temperature, temp, sizeof(temperature));
    temperature = temp;

    auto stop = high_resolution_clock::now();
    timings[2] = duration_cast<microseconds>(stop - start).count();
}

void heatConductionCrankNicolson(float simdt, MaterialConstants material)
{

    auto start = high_resolution_clock::now();

    vector<float> T_s(numX * numY * numZ);
    vector<float> T_ss(numX * numY * numZ); // temp matrices for the Crank-Nicolson ADI sweeps

    auto idx = [](int x, int y, int z) { return (x * numY + y) * numZ + z; };

    float r1 = material.thermalDiffusivity * simdt / (cellSize * cellSize);
    float r2 = r1;
    float r3 = r1;

    vector<float> a1(numX - 2, -r1 / 2);
    vector<float> b1(numX - 2, 1 + r1);
    vector<float> c1(numX - 2, -r1 / 2);

    vector<float> a2(numY - 2, -r2 / 2);
    vector<float> b2(numY - 2, 1 + r2);
    vector<float> c2(numY - 2, -r2 / 2);

    vector<float> a3(numZ - 2, -r3 / 2);
    vector<float> b3(numZ - 2, 1 + r3);
    vector<float> c3(numZ - 2, -r3 / 2);

    // Neumann (insulated) boundary at the domain edges: fold the ghost-point mirror into the diagonal instead of correcting the RHS.
    b1[0] += a1[0];
    b1[b1.size() - 1] += c1[0];

    b2[0] += a2[0];
    b2[b2.size() - 1] += c2[0];

    b3[0] += a3[0];
    b3[b3.size() - 1] += c3[0];

    //memcpy(temp, temperature, sizeof(temp));
    temp = temperature;

    float T_c;
    float T_ip;
    float T_im;
    float T_jp;
    float T_jm;
    float T_kp;
    float T_km;

    // X direction

    

    vector<float> d(numX - 2);
    vector<float> f;
    vector<float> c_star(numX - 2);
    c_star[0] = c1[0] / b1[0];
    for (int i = 1; i < numX - 2; i++)
    {
        float m = b1[i] - c_star[i - 1] * a1[i];
        c_star[i] = c1[i] / m;
    }

    for (int j = 1; j < numY - 1; j++) {
        for (int k = 1; k < numZ - 1; k++) {

            for (int i = 1; i < numX - 1; i++) {

                T_c  = temp[idx(i, j, k)];
                T_ip = (isMaterial[idx(i + 1, j, k)] == 0) ? T_c : temp[idx(i + 1, j, k)];
                T_im = (isMaterial[idx(i - 1, j, k)] == 0) ? T_c : temp[idx(i - 1, j, k)];
                T_jp = (isMaterial[idx(i, j + 1, k)] == 0) ? T_c : temp[idx(i, j + 1, k)];
                T_jm = (isMaterial[idx(i, j - 1, k)] == 0) ? T_c : temp[idx(i, j - 1, k)];
                T_kp = (isMaterial[idx(i, j, k + 1)] == 0) ? T_c : temp[idx(i, j, k + 1)];
                T_km = (isMaterial[idx(i, j, k - 1)] == 0) ? T_c : temp[idx(i, j, k - 1)];

                d[i - 1] = r1 / 2.0 * (T_im + T_ip)
                         + r2 * (T_jm + T_jp) + r3 * (T_km + T_kp)
                         + (1 - r1 - 2 * r2 - 2 * r3) * T_c;
            }

            f = ThomasAlgorithm(a1, b1, c1, d);
            //f = ThomasAlgorithmOptimized(a1, b1, c1, d, c_star);

            for (int i = 1; i < numX - 1; i++)
            {
                T_s[idx(i, j, k)] = f[i - 1];
            }
        }
    }

    // T_s has no value yet at i = 0 / numX-1 (never solved for). Mirror the
    // adjacent interior value so the Y-sweep's cross-coupling term sees a
    // physically sensible "no flux crossed this boundary" value there.
    for (int j = 1; j < numY - 1; j++) {
        for (int k = 1; k < numZ - 1; k++) {
            T_s[idx(0, j, k)]        = T_s[idx(1, j, k)];
            T_s[idx(numX - 1, j, k)] = T_s[idx(numX - 2, j, k)];
        }
    }

    // Y direction
    //memcpy(temp, temperature, sizeof(temp));
    temp = temperature;
    d.resize(numY - 2);

    c_star.resize(numY - 2);
    c_star[0] = c2[0] / b2[0];
    for (int i = 1; i < numY - 2; i++)
    {
        float m = b2[i] - c_star[i - 1] * a2[i];
        c_star[i] = c2[i] / m;
    }

    for (int i = 1; i < numX - 1; i++) {
        for (int k = 1; k < numZ - 1; k++) {
            for (int j = 1; j < numY - 1; j++) {
                T_c  = temp[idx(i, j, k)];
                T_ip = (isMaterial[idx(i + 1, j, k)] == 0) ? T_c : temp[idx(i + 1, j, k)];
                T_im = (isMaterial[idx(i - 1, j, k)] == 0) ? T_c : temp[idx(i - 1, j, k)];
                T_jp = (isMaterial[idx(i, j + 1, k)] == 0) ? T_c : temp[idx(i, j + 1, k)];
                T_jm = (isMaterial[idx(i, j - 1, k)] == 0) ? T_c : temp[idx(i, j - 1, k)];
                T_kp = (isMaterial[idx(i, j, k + 1)] == 0) ? T_c : temp[idx(i, j, k + 1)];
                T_km = (isMaterial[idx(i, j, k - 1)] == 0) ? T_c : temp[idx(i, j, k - 1)];

                d[j - 1] = r1 / 2.0 * (T_im + T_ip + T_s[idx(i - 1, j, k)] + T_s[idx(i + 1, j, k)])
                         + r2 / 2.0 * (T_jm + T_jp)
                         + r3 * (T_km + T_kp)
                         + (1 - r1 - r2 - 2 * r3) * T_c - r1 * T_s[idx(i, j, k)];
            }

            f = ThomasAlgorithm(a2, b2, c2, d);
            //f = ThomasAlgorithmOptimized(a2, b2, c2, d, c_star);

            for (int j = 1; j < numY - 1; j++)
            {
                T_ss[idx(i, j, k)] = f[j - 1];
            }
        }
    }

    // Same gap as T_s above, this time for T_ss at j = 0 / numY-1.
    for (int i = 1; i < numX - 1; i++) {
        for (int k = 1; k < numZ - 1; k++) {
            T_ss[idx(i, 0, k)]        = T_ss[idx(i, 1, k)];
            T_ss[idx(i, numY - 1, k)] = T_ss[idx(i, numY - 2, k)];
        }
    }

    // Z direction
    //memcpy(temp, temperature, sizeof(temp));
    temp = temperature;
    d.resize(numZ - 2);
    c_star.resize(numZ - 2);
    c_star[0] = c3[0] / b3[0];
    for (int i = 1; i < numZ - 2; i++)
    {
        float m = b3[i] - c_star[i - 1] * a3[i];
        c_star[i] = c3[i] / m;
    }


    for (int i = 1; i < numX - 1; i++) {
        for (int j = 1; j < numY - 1; j++) {
            for (int k = 1; k < numZ - 1; k++) {
                T_c  = temp[idx(i, j, k)];
                T_ip = (isMaterial[idx(i + 1, j, k)] == 0) ? T_c : temp[idx(i + 1, j, k)];
                T_im = (isMaterial[idx(i - 1, j, k)] == 0) ? T_c : temp[idx(i - 1, j, k)];
                T_jp = (isMaterial[idx(i, j + 1, k)] == 0) ? T_c : temp[idx(i, j + 1, k)];
                T_jm = (isMaterial[idx(i, j - 1, k)] == 0) ? T_c : temp[idx(i, j - 1, k)];
                T_kp = (isMaterial[idx(i, j, k + 1)] == 0) ? T_c : temp[idx(i, j, k + 1)];
                T_km = (isMaterial[idx(i, j, k - 1)] == 0) ? T_c : temp[idx(i, j, k - 1)];

                d[k - 1] = r1 / 2.0 * (T_im + T_ip + T_s[idx(i - 1, j, k)] + T_s[idx(i + 1, j, k)])
                         + r2 / 2.0 * (T_jm + T_jp + T_ss[idx(i, j - 1, k)] + T_ss[idx(i, j + 1, k)])
                         + r3 / 2.0 * (T_km + T_kp)
                         + (1 - r1 - r2 - r3) * T_c - r1 * T_s[idx(i, j, k)] - r2 * T_ss[idx(i, j, k)];
            }

            f = ThomasAlgorithm(a3, b3, c3, d);
            //f = ThomasAlgorithmOptimized(a3, b3, c3, d, c_star);

            for (int k = 1; k < numZ - 1; k++)
            {
                temperature[idx(i, j, k)] = f[k - 1];
            }
        }
    }


    auto stop = high_resolution_clock::now();
    timings[0] = duration_cast<microseconds>(stop - start).count();
}
