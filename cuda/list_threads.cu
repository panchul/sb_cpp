//
// nvcc list_threads.cu
// 
// basic into to cuda kernel
//
#include <cuda_runtime.h>
#include <cstdlib>
#include <iostream>

using namespace std;

__global__
void saveTid(int *tids, int numElements) {
    int tid = blockDim.x * blockIdx.x + threadIdx.x;
	if (tid < numElements) {
        tids[tid*2] = blockIdx.x;
        tids[tid*2+1] = threadIdx.x;
    }
}

int main(int argc, char *argv[]) {

    if(argc < 3) {
        cout << "missing argument.\nUsage: list_threads <numElements> <numThreads>\n"
                "try: list_threads 20 5\n";
        return -1;
    }

    int numElements = atoi(argv[1]);
    int numThreads = atoi(argv[2]);
    int *dTids;
    
    int threadsPerBlock = numThreads;
    int blocksPerGrid =(numElements + threadsPerBlock - 1) / threadsPerBlock;
    cout << "CUDA kernel launch with " << blocksPerGrid << " blocks of " << threadsPerBlock << " threads\n";

    cudaMalloc(&dTids, sizeof(int)*numElements*2);
    saveTid<<<blocksPerGrid,threadsPerBlock>>>(dTids, numElements);
    
    int *hTids = new int[numElements * 2];
    
    cudaMemcpy(hTids, dTids, sizeof(int) * numElements * 2, cudaMemcpyDeviceToHost);

    for(int i = 0; i < numElements; ++i) {
        std::cout << i << ": blockId " << hTids[i*2] << ", threadId " << hTids[i*2+1] << "\n";
    }

    delete[] hTids;
    cudaFree(dTids);

    return 0;
}
