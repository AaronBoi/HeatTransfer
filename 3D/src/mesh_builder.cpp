#include "mesh_builder.h"

#include <rlgl.h>
#include <chrono>
#include <vector>
#include <raymath.h>

#include "grid_state.h"
#include "app_state.h"

using namespace std;
using namespace std::chrono;

Color getGridColor(int x, int y, int z)
{
    float displayTemp = showAsKelvin ? temperature[idx(x, y, z)] : temperature[idx(x, y, z)] - 273.15f;
    displayTemp -= roomTemperature;
    displayTemp = (displayTemp <= 0) ? 0.1 : displayTemp; 
    return ColorFromHSV(log10(displayTemp) * 360.0f / 6.0f, 1, 1);
}

int getNumberCubes()
{
    int num = 0;
    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                if (isMaterial[idx(x, y, z)] != 0 && isMaterial[idx(x, y, z)] != 1) continue;
                num += isMaterial[idx(x, y, z)];
            }
        }
    }
    return num;
}

Mesh GenMeshIdenticalCubes(float sideLength)
{

    int numCubes = getNumberCubes();
    //int cubePositions[3 * numCubes] = {};
    vector<int> cubePositions(3 * numCubes);


    int index = 0;
    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                if (isMaterial[idx(x, y, z)] == 1)
                {
                    cubePositions[3 * index] = x;
                    cubePositions[3 * index + 1] = y;
                    cubePositions[3 * index + 2] = z;
                    index++;
                }
            }
        }
    }

    Mesh singleMesh = GenMeshCube(sideLength, sideLength, sideLength);

    int vertsPerCube = singleMesh.triangleCount * 3; // 36 for a cube — one vertex per index entry

    Mesh mesh = { 0 };
    mesh.vertexCount   = vertsPerCube * numCubes;
    mesh.triangleCount = singleMesh.triangleCount * numCubes;

    mesh.vertices  = (float *)RL_MALLOC(3 * mesh.vertexCount * sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(2 * mesh.vertexCount * sizeof(float));
    mesh.normals   = (float *)RL_MALLOC(3 * mesh.vertexCount * sizeof(float));
    mesh.colors    = (unsigned char *)RL_MALLOC(4 * 3 * mesh.vertexCount * sizeof(unsigned char));
    mesh.indices   = nullptr;

    for (int i = 0; i < numCubes; i++)
    {
        int vOffset = i * vertsPerCube;

        for (int j = 0; j < vertsPerCube; j++)
        {
            unsigned short srcVert = singleMesh.indices[j]; // which original vertex this slot needs

            mesh.vertices[3 * (vOffset + j) + 0] =
                singleMesh.vertices[3 * srcVert + 0] + cubePositions[3 * i + 0] * sideLength;
            mesh.vertices[3 * (vOffset + j) + 1] =
                singleMesh.vertices[3 * srcVert + 1] + cubePositions[3 * i + 1] * sideLength;
            mesh.vertices[3 * (vOffset + j) + 2] =
                singleMesh.vertices[3 * srcVert + 2] + cubePositions[3 * i + 2] * sideLength;

            mesh.normals[3 * (vOffset + j) + 0] = singleMesh.normals[3 * srcVert + 0];
            mesh.normals[3 * (vOffset + j) + 1] = singleMesh.normals[3 * srcVert + 1];
            mesh.normals[3 * (vOffset + j) + 2] = singleMesh.normals[3 * srcVert + 2];

            mesh.texcoords[2 * (vOffset + j) + 0] = singleMesh.texcoords[2 * srcVert + 0];
            mesh.texcoords[2 * (vOffset + j) + 1] = singleMesh.texcoords[2 * srcVert + 1];
        }
    }

    UnloadMesh(singleMesh);
    UploadMesh(&mesh, true);
    return mesh;
}

void SetAllCubeColors(Mesh &mesh, int numCubes, int vertsPerCube)
{
    auto start = high_resolution_clock::now();

    int i = 0;

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                if (isMaterial[idx(x, y, z)] == 0) continue;

                int vOffset = i * vertsPerCube;
                Color c = getGridColor(x, y, z);

                if (idx(x, y, z) == guiState.selectedSensor)
                {

                    c = GRAY;
                }
                

                if (heatInlet[idx(x, y, z)] != 0)
                {
                    c = ColorFromHSV(0, 0, 0);
                }

                for (int k = 0; k < vertsPerCube; k++)
                {
                    mesh.colors[4 * (vOffset + k) + 0] = c.r;
                    mesh.colors[4 * (vOffset + k) + 1] = c.g;
                    mesh.colors[4 * (vOffset + k) + 2] = c.b;
                    mesh.colors[4 * (vOffset + k) + 3] = c.a;
                }

                i++;
            }
        }
    }

    UpdateMeshBuffer(mesh, 3, mesh.colors, 4 * mesh.vertexCount * sizeof(unsigned char), 0);


    auto stop = high_resolution_clock::now();
    timings[1] = duration_cast<microseconds>(stop - start).count();
}
