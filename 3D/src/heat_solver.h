#pragma once

#include "material.h"
#include "scene.h"

#include "vector"

using namespace std;

// Explicit (forward-Euler) finite-difference solver, including a simple
// surface-radiation term at material/air interfaces. Only stable for small
// dt relative to cellSize^2 / diffusivity.
//void heatConduction(float dt);

// Alternating-Direction-Implicit Crank-Nicolson solver. Unconditionally
// stable; solves one implicit tridiagonal system per axis per step via
// ThomasAlgorithm. Domain edges and material/air interfaces are both
// treated as insulated (zero-flux / Neumann) boundaries.
void heatConductionCrankNicolson(Scene& scene, float dt, MaterialConstants material);

void radiationLoss(Scene& scene, float simdt);
