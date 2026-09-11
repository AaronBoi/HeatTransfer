#pragma once

#include <raylib.h>

// Maps a cell's temperature to a display color via HSV.
Color getGridColor(int x, int y, int z);

// Counts material (solid) cells in the grid.
int getNumberCubes();

// Builds one combined mesh containing a unit cube at each position in
// cubePositions (flattened xyz triples), each cube scaled to sideLength.
Mesh GenMeshIdenticalCubes(float sideLength);

// Recolors every material cube in an existing GenMeshIdenticalCubes mesh to
// match the current temperature field, and uploads the new colors to the GPU.
void SetAllCubeColors(Mesh &mesh, int numCubes, int vertsPerCube);

void DrawCubes(); // currently a no-op; kept as a quick single-cube debug hook
