//
// nvcc list_threads.cu
// 
// basic into to cuda kernel
//

#include <cstdlib>
#include <iostream>

__global__
void saveTid(int *tids) {
    int tid = threadIdx.x;
    tids[tid] tid;
}

int main(int argc, char *argv[]) {

    int numThreads = atoi(argv[1]);
    int *dTids;
    
    cudaMalloc(&tids, sizeof(int)*numThreads);
    saveTid<<< 1, numThreads >>>(dTids);
    
    int *hTids = new int[numThreads];
    
    cudaMemcpy(hTids, dTids, sizeof(int) *numThreads, cudaMemcpyDeviceToHost);

    for(int i = 0; i < numThreads; ++i) {
        std::cout << i << " - " << hTids[i] << "\n";
    }

    delete[] hTids;
    cudaFree(dTids);

    return 0;
}
