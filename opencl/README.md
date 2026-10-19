# OpenCL FFT demo

This project demonstrates a radix-2 FFT implemented in OpenCL. It prefers a GPU device when one is available, but it will use a CPU OpenCL device automatically if no GPU is present. If OpenCL itself is unavailable, it falls back to a CPU reference implementation.

Build:

```bash
cmake -S opencl -B opencl/build
cmake --build opencl/build
```

Run:

```bash
./opencl/build/fft_opencl
```

The program generates a test signal, runs the FFT on the chosen OpenCL device (or CPU fallback), and compares the result against a host-side reference implementation.
