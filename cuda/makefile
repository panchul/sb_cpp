#
# https://www.nvidia.com/docs/IO/116711/sc11-cuda-c-basics.pdf
#

all: cuda plain vectorAdd_cuda_sleep vectorAdd_cuda vectorAdd_plain list_threads

cuda: add_cuda.cu
	nvcc -o cuda add_cuda.cu

plain: add_plain.cpp
	g++ -o plain add_plain.cpp          

vectorAdd_cuda: vectorAdd.cu
	nvcc -o vectorAdd_cuda -DDO_CUDA vectorAdd.cu 

vectorAdd_cuda_sleep: vectorAdd_sleep.cu
	nvcc -o vectorAdd_cuda_sleep -DDO_CUDA vectorAdd_sleep.cu 

vectorAdd_plain: vectorAdd.cu
	nvcc -o vectorAdd_plain vectorAdd.cu 

list_threads: list_threads.cu
	nvcc -o list_threads list_threads.cu 

test: plain cuda vectorAdd_cuda vectorAdd_plain
	bash -c "time ./plain"
	bash -c "time ./cuda"
	bash -c "time ./vectorAdd_plain"
	bash -c "time ./vectorAdd_cuda"
	#bash -c "time ./vectorAdd_cuda_sleep"
	
clean:	
	rm -rf ./cuda ./plain ./vectorAdd_plain ./vectorAdd_cuda ./vectorAdd_cuda_sleep ./list_threads
