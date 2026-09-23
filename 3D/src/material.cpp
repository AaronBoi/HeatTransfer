#include "material.h"

#include <cmath>

MaterialConstants Steel = { // Constants for Steel AISI 1010
    .thermalDiffusivity = 18.8f * (float) pow(10, -6),
    .density = 7.87f * (float) pow(10, 3),
    .specificHeatCapacity = 448.0f,
    .thermalEmissivity = 0.3f,
};

MaterialConstants material = Steel;
