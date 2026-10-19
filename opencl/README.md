# OpenCL FFT demo

This project is a beginner-friendly OpenCL example for signal processing.

What this program does:

- creates a synthetic signal
- runs either an FFT or a magnitude calculation
- performs the work on an OpenCL device when one is available
- falls back to a CPU version if OpenCL is not available

Why OpenCL?

OpenCL lets the CPU hand heavy parallel work to a GPU or another accelerator.
For example, the FFT and magnitude operations both involve many independent math operations.
Instead of doing all of them on the CPU, OpenCL lets the device process them in parallel.

How the program works:

1. The C++ host code creates a signal.
2. The host chooses an OpenCL device (GPU preferred, CPU fallback).
3. The host loads the OpenCL kernel source from `fft_kernel.cl`.
4. The host creates buffers in device memory and copies input data there.
5. The kernel runs in parallel across many work-items.
6. The host reads the result back and prints it.

Important OpenCL basics:

- Platform: a vendor implementation, such as Apple, NVIDIA, AMD, Intel.
- Device: a GPU, CPU, or accelerator inside the platform.
- Context: the environment used to manage devices and memory.
- Command queue: the place where the host sends commands to the device.
- Kernel: the OpenCL program that runs on the device.
- Work-item: one parallel execution instance of a kernel.
- `get_global_id(0)`: tells each work-item which element it is responsible for.

Files in this project:

- `fft_opencl.cpp`: C++ host code for OpenCL setup and CLI parsing
- `fft_kernel.cl`: OpenCL kernel source code
- `CMakeLists.txt`: build configuration

Build:

```bash
cmake -S opencl -B opencl/build
cmake --build opencl/build
```

Run examples:

```bash
./opencl/build/fft_opencl
./opencl/build/fft_opencl --length 512
./opencl/build/fft_opencl --operation magnitude
./opencl/build/fft_opencl --length 1024 --operation magnitude
./opencl/build/fft_opencl --help
```

Command-line arguments:

- `--length, -n`: signal length (default: 256)
- `--operation, -o`: `fft` or `magnitude` (default: `fft`)
- `--help, -h`: shows help

Important note about FFT length:

This FFT implementation is a radix-2 FFT, so the signal length must be a power of two.
Examples: 256, 512, 1024, 2048, ...

Understanding the sample signal:

The sample signal is created by combining a sine wave and a cosine wave at different frequencies.
This makes the signal interesting and gives the FFT something meaningful to analyze.

The program prints the first few results so the output stays readable.
