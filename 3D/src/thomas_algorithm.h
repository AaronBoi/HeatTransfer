#pragma once

#include <vector>

// Solves a tridiagonal linear system A x = d using the Thomas algorithm.
// a, b, c are the sub-, main-, and super-diagonals of the equation matrix.
// Returns the solved vector x.
std::vector<float> ThomasAlgorithm(std::vector<float> a, std::vector<float> b, std::vector<float> c, std::vector<float> d);
