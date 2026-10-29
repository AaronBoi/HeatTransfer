#pragma once

#include <vector>
using namespace std;

// Solves a tridiagonal linear system A x = d using the Thomas algorithm.
// a, b, c are the sub-, main-, and super-diagonals of the equation matrix.
// Returns the solved vector x.
vector<float> ThomasAlgorithm(const vector<float>& a, const vector<float>& b, const vector<float>& c, const vector<float>& d);
std::vector<float> ThomasAlgorithmOptimized(std::vector<float> a, std::vector<float> b, std::vector<float> c, std::vector<float> d, std::vector<float> c_star);
