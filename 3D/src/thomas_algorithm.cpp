#include "thomas_algorithm.h"

using namespace std;

vector<float> ThomasAlgorithm(vector<float> a, vector<float> b, vector<float> c, vector<float> d)
{
    int size = d.size();

    vector<float> c_star(size);
    vector<float> d_star(size);
    vector<float> f(size);

    c_star[0] = c[0] / b[0];
    d_star[0] = d[0] / b[0];

    for (int i = 1; i < size; i++)
    {
        float m = b[i] - c_star[i - 1] * a[i];
        c_star[i] = c[i] / m;
        d_star[i] = (d[i] - d_star[i - 1] * a[i]) / m;
    }

    int k = size - 1;
    f[k] = d_star[k];

    for (int i = k - 1; i >= 0; i--)
    {
        f[i] = d_star[i] - c_star[i] * f[i + 1];
    }

    return f;
}
