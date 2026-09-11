#include "material.h"

#include <cmath>

MaterialConstants Steel = { // Constants for Steel AISI 1010
    .thermalDiffusivity = 18.8 * pow(10, -6),
    .density = 7.87 * pow(10, 3),
    .specificHeatCapacity = 448,
    .thermalEmissivity = 0.3,
};

MaterialConstants material = Steel;
