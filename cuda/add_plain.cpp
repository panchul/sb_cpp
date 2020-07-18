//
// ??? $ clang++ add_plain.cpp -o add_plain
// $ g++ add_plain.cpp -o add_plain
// $ ./add_plain
// Max error: 0.000000
//
#include <iostream>
#include <math.h>

void add(int n, float *x, float *y, float *z) {
  for (int i = 0; i < n; i++) {
      z[i] = x[i] + y[i];
  }
}

int main(void) {
  int N = 1 << 30;
  float *x = new float[N];
  float *y = new float[N];
  float *z = new float[N];

  for (int i = 0; i < N; i++) {
    x[i] = i;
    y[i] = -i;
  }

  add(N, x, y, z);

  float maxError = 0.0f;
  for (int i = 0; i < N; i++)
    maxError = fmax(maxError, fabs(z[i]-0.0f));
  std::cout << "Max error: " << maxError << std::endl;

  delete [] x;
  delete [] y;
  delete [] z;
  return 0;
}
