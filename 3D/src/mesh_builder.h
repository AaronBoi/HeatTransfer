#pragma once

#include <raylib.h>

#include "scene.h"

// Maps a cell's temperature to a display color via HSV.
Color getGridColor(const Scene& scene, int x, int y, int z);

// Counts material (solid) cells in the grid.
int getNumberCubes(const Scene& scene);

// Builds one combined mesh containing a unit cube at each position in
// cubePositions (flattened xyz triples), each cube scaled to sideLength.
Mesh GenMeshIdenticalCubes(const Scene& scene, float sideLength);

// Recolors every material cube in an existing GenMeshIdenticalCubes mesh to
// match the current temperature field, and uploads the new colors to the GPU.
void SetAllCubeColors(const Scene& scene, Mesh &mesh, int numCubes, int vertsPerCube);
