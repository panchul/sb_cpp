//
// $ nvcc add.cu -o add_cuda
// $ ./add_cuda
// Max error: 0.000000
//
#include <cuda_runtime.h>

#include <iostream>
#include <math.h>

using namespace std;

// Kernel function to add the elements of two arrays
__global__
void add(int n, float *x, float *y, float *z, int batch_size)
{
  int i = blockDim.x * blockIdx.x + threadIdx.x;
  i *= batch_size;
  if(i<n) {
    int e = min(n, i + batch_size);
	  while ( i < e) {
      z[i] = x[i] + y[i];
      ++i;
    }
  }
}

int main(void) {

  int N = 1<<30;
  float *dx;
  float *dy;
  float *dz;

  // Allocate Unified Memory – accessible from CPU or GPU --- probably SLOW
  cudaMallocManaged(&dx, N*sizeof(float));
  cudaMallocManaged(&dy, N*sizeof(float));
  cudaMallocManaged(&dz, N*sizeof(float));

  // initialize x and y arrays on the host
  for (int i = 0; i < N; i++) {
    dx[i] = i;
    dy[i] = -i;
  }

    int block_size = 4096;
    int threadsPerBlock = 4096;
    int blocksPerGrid =(N/block_size + threadsPerBlock - 1) / threadsPerBlock;
    cout << "CUDA kernel launch with " << blocksPerGrid << " blocks of " << threadsPerBlock << " threads\n";

    add<<<blocksPerGrid,threadsPerBlock>>>(N, dx, dy, dz, block_size);

  // Wait for GPU to finish before accessing on host
  cudaDeviceSynchronize();

  // Check for errors (all values should be 0.0f)
  float maxError = 0.0f;
  for (int i = 0; i < N; i++)
    maxError = fmax(maxError, fabs(dz[i]-0.0f));
  std::cout << "Max error: " << maxError << std::endl;

  cudaFree(dx);
  cudaFree(dy);
  cudaFree(dz);
  
  return 0;
}
