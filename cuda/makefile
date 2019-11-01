#
# https://www.nvidia.com/docs/IO/116711/sc11-cuda-c-basics.pdf
#

all: cuda plain

cuda: add_cuda.cu
	nvcc -o cuda add_cuda.cu
    
plain: add_plain.cpp
	g++ -o plain add_plain.cpp          

test: plain cuda
	time ./plain
	time ./cuda