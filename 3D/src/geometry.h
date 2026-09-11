#pragma once

// Carves the isMaterial grid into a hollow cylinder (pipe wall) whose
// cross-section is centered in the Y-Z plane and constant along X.
void setCylinderHollow();

// Alternative test geometry: a flat 3-cell-thick solid slab through the
// middle of the domain (handy for simpler validation setups).
void setPlane();

// Resets temperature/isMaterial/heatInlet to initial conditions and builds
// the pipe geometry. Clears the `reset` flag when done.
void init();
