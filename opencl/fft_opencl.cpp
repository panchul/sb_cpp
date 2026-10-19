#if defined(__APPLE__)
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// This file implements a small OpenCL-based signal-processing demo.
// The program can calculate either:
//  1) a Fast Fourier Transform (FFT), which turns a time-domain signal into frequency-domain bins,
//  2) a simple magnitude calculation, which tells us how large each signal sample is.
//
// The overall workflow is:
//  - parse command-line parameters,
//  - generate a signal,
//  - run the requested operation,
//  - print the result in a readable way.
namespace {

// Complex stores one complex number as real and imaginary parts.
// In an FFT, each sample is a complex value, even if the input signal is real.
struct Complex {
    float real = 0.0f;
    float imag = 0.0f;
};

// This enum describes which calculation the user wants to run.
enum class OperationMode {
    FFT,
    Magnitude
};

// All command-line settings are grouped here so the program can pass them around cleanly.
struct CliOptions {
    std::size_t signalLength = 256;
    OperationMode operation = OperationMode::FFT;
};

// Checks whether a positive integer is a power of two.
// FFT algorithms usually require this property because the radix-2 FFT divides the data repeatedly by 2.
bool isPowerOfTwo(std::size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

// Shows the expected command-line syntax so a new user knows what can be passed in.
void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [--length N] [--operation fft|magnitude]\n";
    std::cout << "  --length, -n     Number of signal samples (default: 256).\n";
    std::cout << "  --operation, -o Operation to run: 'fft' or 'magnitude' (default: fft).\n";
    std::cout << "  --help, -h      Show this help message.\n";
}

// Converts a text value like "256" into a std::size_t.
// This is used when parsing the length parameter from the command line.
bool parseUnsignedSize(const std::string& text, std::size_t& value) {
    if (text.empty()) {
        return false;
    }

    try {
        const std::size_t parsed = static_cast<std::size_t>(std::stoull(text));
        value = parsed;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// Accepts the operation string and converts it into our enum.
// This makes the program accept user-friendly names while keeping the code type-safe.
OperationMode parseOperation(const std::string& text) {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (lower == "fft") {
        return OperationMode::FFT;
    }
    if (lower == "magnitude" || lower == "mag") {
        return OperationMode::Magnitude;
    }

    throw std::invalid_argument("Unsupported operation: " + text);
}

// Parses the arguments supplied by the user.
// If any argument is invalid, it prints an error and returns false.
bool parseCommandLineArgs(int argc, char** argv, CliOptions& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return false;
        }

        if (arg == "--length" || arg == "-n") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after " << arg << "\n";
                printUsage(argv[0]);
                return false;
            }

            if (!parseUnsignedSize(argv[++i], options.signalLength)) {
                std::cerr << "Signal length must be a valid positive integer.\n";
                printUsage(argv[0]);
                return false;
            }
            continue;
        }

        if (arg == "--operation" || arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after " << arg << "\n";
                printUsage(argv[0]);
                return false;
            }

            try {
                options.operation = parseOperation(argv[++i]);
            } catch (const std::exception& ex) {
                std::cerr << ex.what() << "\n";
                printUsage(argv[0]);
                return false;
            }
            continue;
        }

        std::cerr << "Unknown argument: " << arg << "\n";
        printUsage(argv[0]);
        return false;
    }

    return true;
}

// This function creates a synthetic time-domain signal.
// We combine a sine wave and a cosine wave at different frequencies so the FFT will show interesting frequency peaks.
// The resulting signal is still a real-valued signal, but we store it as complex values with an imaginary part of zero.
std::vector<Complex> generateSignal(std::size_t n) {
    std::vector<Complex> signal(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double angle = 2.0 * M_PI * static_cast<double>(i) / static_cast<double>(n);
        const float sineComponent = static_cast<float>(std::sin(angle * 3.0));
        const float cosineComponent = static_cast<float>(0.5 * std::cos(angle * 11.0));
        signal[i] = {sineComponent + cosineComponent, 0.0f};
    }
    return signal;
}

// Adds two complex numbers.
// This is used when combining the even and odd branches during the FFT butterfly operation.
Complex complexAdd(const Complex& a, const Complex& b) {
    return {a.real + b.real, a.imag + b.imag};
}

// Subtracts two complex numbers.
// In FFT math, each butterfly computes even - odd * twiddle, which uses subtraction.
Complex complexSub(const Complex& a, const Complex& b) {
    return {a.real - b.real, a.imag - b.imag};
}

// Multiplies two complex numbers using standard complex algebra:
// (a + bi) * (c + di) = (ac - bd) + (ad + bc)i
Complex complexMul(const Complex& a, const Complex& b) {
    return {
        a.real * b.real - a.imag * b.imag,
        a.real * b.imag + a.imag * b.real,
    };
}

// Reorders input data into bit-reversed order.
// This is the first step in many FFT implementations because the butterfly algorithm expects data in this order.
void bitReversePermute(std::vector<Complex>& data) {
    const std::size_t n = data.size();
    if (n <= 1) {
        return;
    }

    std::size_t j = 0;
    for (std::size_t i = 1; i < n; ++i) {
        std::size_t bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            std::swap(data[i], data[j]);
        }
    }
}

// A simple CPU implementation of the FFT.
// It is useful as a correctness reference when we compare the OpenCL result against a known-good host implementation.
std::vector<Complex> cpuFft(const std::vector<Complex>& input) {
    std::vector<Complex> data = input;
    bitReversePermute(data);

    const std::size_t n = data.size();
    const std::size_t stages = static_cast<std::size_t>(std::log2(n));

    for (std::size_t stage = 0; stage < stages; ++stage) {
        const std::size_t step = 1ULL << stage;
        for (std::size_t group = 0; group < n / (2 * step); ++group) {
            for (std::size_t offset = 0; offset < step; ++offset) {
                const std::size_t idx1 = group * (2 * step) + offset;
                const std::size_t idx2 = idx1 + step;

                const Complex even = data[idx1];
                const Complex odd = data[idx2];
                const double angle = -2.0 * M_PI * static_cast<double>(offset) / static_cast<double>(2 * step);
                const Complex twiddle = {static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle))};
                const Complex product = complexMul(odd, twiddle);

                data[idx1] = complexAdd(even, product);
                data[idx2] = complexSub(even, product);
            }
        }
    }

    return data;
}

// A CPU reference implementation for calculating signal magnitude.
// This mirrors the OpenCL magnitude operation so the two versions can be compared exactly.
std::vector<float> cpuMagnitude(const std::vector<Complex>& signal) {
    std::vector<float> magnitudes(signal.size());
    for (std::size_t i = 0; i < signal.size(); ++i) {
        magnitudes[i] = std::sqrt(signal[i].real * signal[i].real + signal[i].imag * signal[i].imag);
    }
    return magnitudes;
}

// Measures how long a function takes and returns the duration in milliseconds.
template <typename Func>
double measureDurationMs(Func&& func) {
    const auto start = std::chrono::high_resolution_clock::now();
    func();
    const auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// Converts OpenCL device types into readable names.
std::string deviceTypeToString(cl_device_type type) {
    if ((type & CL_DEVICE_TYPE_GPU) != 0) {
        return "GPU";
    }
    if ((type & CL_DEVICE_TYPE_CPU) != 0) {
        return "CPU";
    }
    if ((type & CL_DEVICE_TYPE_ACCELERATOR) != 0) {
        return "ACCELERATOR";
    }
    if ((type & CL_DEVICE_TYPE_DEFAULT) != 0) {
        return "DEFAULT";
    }
    return "OTHER";
}

// Stores a discovered OpenCL platform/device pair.
//
// In OpenCL, a platform is a vendor implementation such as Apple, NVIDIA, AMD, Intel, etc.
// Inside a platform, there may be one or more devices, such as a GPU, CPU, or accelerator.
//
// The program prefers a GPU, but if no GPU exists, it will accept a CPU OpenCL device as a fallback.
struct DeviceInfo {
    cl_platform_id platform = nullptr;
    cl_device_id device = nullptr;
    std::string platformName;
    std::string deviceName;
    cl_device_type deviceType = 0;
    std::size_t computeUnits = 0;
    std::size_t maxWorkGroupSize = 0;
    std::size_t globalMemoryBytes = 0;
    bool isGPU = false;
};

// Loads the OpenCL kernel source file from the filesystem.
//
// A kernel file is the OpenCL source code that the device will compile and run.
// The host program writes the source code into a string, then passes it to clCreateProgramWithSource().
// This is the bridge between the C++ host and the GPU device code.
std::string readKernelSource(const std::string& preferredPath) {
    std::vector<std::string> candidates = {
        preferredPath,
        "fft_kernel.cl",
        "./fft_kernel.cl",
        "../opencl/fft_kernel.cl",
        "../fft_kernel.cl",
    };

    for (const std::string& candidate : candidates) {
        std::ifstream file(candidate);
        if (file.is_open()) {
            std::ostringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }

    throw std::runtime_error("Unable to open kernel source: " + preferredPath);
}

// Enumerates all OpenCL devices visible to the current machine.
// This is useful for debugging, printing a device summary, and choosing the best device for the workload.
std::vector<DeviceInfo> enumerateOpenCLDevices() {
    std::vector<DeviceInfo> devices;

    cl_uint numPlatforms = 0;
    cl_int err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        return devices;
    }

    std::vector<cl_platform_id> platforms(numPlatforms);
    clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);

    for (cl_platform_id platform : platforms) {
        cl_uint numDevices = 0;
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        if (numDevices == 0) {
            continue;
        }

        std::vector<cl_device_id> platformDevices(numDevices);
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, platformDevices.data(), nullptr);

        for (cl_device_id device : platformDevices) {
            char platformBuffer[256] = {};
            char deviceBuffer[256] = {};
            clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platformBuffer), platformBuffer, nullptr);
            clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(deviceBuffer), deviceBuffer, nullptr);

            cl_device_type deviceType = 0;
            clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(deviceType), &deviceType, nullptr);

            cl_uint computeUnits = 0;
            clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(computeUnits), &computeUnits, nullptr);

            size_t maxWorkGroupSize = 0;
            clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkGroupSize), &maxWorkGroupSize, nullptr);

            cl_ulong globalMemBytes = 0;
            clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(globalMemBytes), &globalMemBytes, nullptr);

            DeviceInfo info;
            info.platform = platform;
            info.device = device;
            info.platformName = platformBuffer;
            info.deviceName = deviceBuffer;
            info.deviceType = deviceType;
            info.computeUnits = static_cast<std::size_t>(computeUnits);
            info.maxWorkGroupSize = static_cast<std::size_t>(maxWorkGroupSize);
            info.globalMemoryBytes = static_cast<std::size_t>(globalMemBytes);
            info.isGPU = (deviceType & CL_DEVICE_TYPE_GPU) != 0;
            devices.push_back(info);
        }
    }

    return devices;
}

// Scans all available OpenCL platforms and devices.
//
// This function is like a device explorer.
// It asks OpenCL: "What platforms and devices are available to me?"
// Then it tries to pick the best device for the job.
//
// Why do we prefer GPU?
//   - GPUs are designed for parallel work.
//   - FFT code usually runs much faster on GPU than on CPU for large input sizes.
//
// If no GPU is present, we fall back to a CPU OpenCL device so the program still works.
DeviceInfo chooseOpenCLDevice() {
    const std::vector<DeviceInfo> devices = enumerateOpenCLDevices();
    for (const DeviceInfo& deviceInfo : devices) {
        if (deviceInfo.isGPU) {
            return deviceInfo;
        }
    }

    for (const DeviceInfo& deviceInfo : devices) {
        if (deviceInfo.deviceType == CL_DEVICE_TYPE_CPU || deviceInfo.deviceType == CL_DEVICE_TYPE_DEFAULT) {
            return deviceInfo;
        }
    }

    if (!devices.empty()) {
        return devices.front();
    }

    return {};
}

// Prints a summary of all discovered OpenCL devices.
// This is helpful when debugging performance or when a user wants to know which device is being used.
void printDeviceSummary(const std::vector<DeviceInfo>& devices) {
    if (devices.empty()) {
        std::cout << "No OpenCL devices found.\n";
        return;
    }

    std::cout << "OpenCL devices discovered:\n";
    for (std::size_t i = 0; i < devices.size(); ++i) {
        const DeviceInfo& device = devices[i];
        const double memMiB = static_cast<double>(device.globalMemoryBytes) / (1024.0 * 1024.0);
        std::cout << "  [" << i << "] Platform: " << device.platformName
                  << " | Device: " << device.deviceName
                  << " | Type: " << deviceTypeToString(device.deviceType)
                  << " | Compute units: " << device.computeUnits
                  << " | Max work-group: " << device.maxWorkGroupSize
                  << " | Global memory: " << std::fixed << std::setprecision(1) << memMiB << " MiB\n";
    }
}

// Runs the FFT on the OpenCL device.
//
// This is the main GPU execution pipeline for the FFT:
//   1) choose a device,
//   2) create a context,
//   3) create a command queue,
//   4) load the kernel source,
//   5) build the OpenCL program,
//   6) create memory buffers,
//   7) send the input data to the device,
//   8) launch the kernel repeatedly for each FFT stage,
//   9) read the results back to the host.
//
// In beginner terms: the CPU prepares the work, the GPU does the math, and then the CPU fetches the result.
bool runOpenCLFFT(const std::vector<Complex>& input, std::vector<Complex>& output) {
    const DeviceInfo selected = chooseOpenCLDevice();
    if (!selected.device) {
        return false;
    }

    const std::size_t n = input.size();
    if (n == 0 || !isPowerOfTwo(n)) {
        throw std::runtime_error("The FFT length must be a positive power-of-two value.");
    }

    cl_int err = CL_SUCCESS;
    cl_context context = clCreateContext(nullptr, 1, &selected.device, nullptr, nullptr, &err);
    if (err != CL_SUCCESS || !context) {
        std::cerr << "clCreateContext failed with code " << err << '\n';
        return false;
    }

    cl_command_queue queue = clCreateCommandQueue(context, selected.device, 0, &err);
    if (err != CL_SUCCESS || !queue) {
        std::cerr << "clCreateCommandQueue failed with code " << err << '\n';
        clReleaseContext(context);
        return false;
    }

    const std::string kernelPath = "fft_kernel.cl";
    std::string source;
    try {
        source = readKernelSource(kernelPath);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    const char* sourceText = source.c_str();
    cl_program program = clCreateProgramWithSource(context, 1, &sourceText, nullptr, &err);
    if (err != CL_SUCCESS || !program) {
        std::cerr << "clCreateProgramWithSource failed with code " << err << '\n';
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    err = clBuildProgram(program, 1, &selected.device, "-cl-fast-relaxed-math", nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::size_t logSize = 0;
        clGetProgramBuildInfo(program, selected.device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::string buildLog(logSize, '\0');
        clGetProgramBuildInfo(program, selected.device, CL_PROGRAM_BUILD_LOG, buildLog.size(), &buildLog[0], nullptr);
        std::cerr << "OpenCL build failed:\n" << buildLog << '\n';
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    cl_kernel kernel = clCreateKernel(program, "fft_stage", &err);
    if (err != CL_SUCCESS || !kernel) {
        std::cerr << "clCreateKernel failed with code " << err << '\n';
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(Complex) * n, nullptr, &err);
    if (err != CL_SUCCESS || !buffer) {
        std::cerr << "clCreateBuffer failed with code " << err << '\n';
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    std::vector<Complex> staged = input;
    bitReversePermute(staged);
    err = clEnqueueWriteBuffer(queue, buffer, CL_TRUE, 0, sizeof(Complex) * n, staged.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "clEnqueueWriteBuffer failed with code " << err << '\n';
        clReleaseMemObject(buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    const std::size_t stages = static_cast<std::size_t>(std::log2(n));
    for (std::size_t stage = 0; stage < stages; ++stage) {
        err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
        err |= clSetKernelArg(kernel, 1, sizeof(cl_int), &stage);
        err |= clSetKernelArg(kernel, 2, sizeof(cl_int), &n);
        if (err != CL_SUCCESS) {
            std::cerr << "clSetKernelArg failed with code " << err << '\n';
            break;
        }

        const std::size_t globalSize = n / 2;
        err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &globalSize, nullptr, 0, nullptr, nullptr);
        if (err != CL_SUCCESS) {
            std::cerr << "clEnqueueNDRangeKernel failed with code " << err << '\n';
            break;
        }
        clFinish(queue);
    }

    output.resize(n);
    err = clEnqueueReadBuffer(queue, buffer, CL_TRUE, 0, sizeof(Complex) * n, output.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "clEnqueueReadBuffer failed with code " << err << '\n';
        clReleaseMemObject(buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    clReleaseMemObject(buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    return true;
}

// Runs the magnitude calculation on the OpenCL device.
//
// This is a simpler parallel task than the FFT:
//   - each input sample is processed independently,
//   - each work-item computes one magnitude,
//   - there is no repeated FFT stage loop.
//
// This is a great example of how OpenCL shines for simple, element-wise math on many values at once.
bool runOpenCLMagnitude(const std::vector<Complex>& input, std::vector<float>& output) {
    const DeviceInfo selected = chooseOpenCLDevice();
    if (!selected.device) {
        return false;
    }

    const std::size_t n = input.size();
    if (n == 0) {
        return false;
    }

    cl_int err = CL_SUCCESS;
    cl_context context = clCreateContext(nullptr, 1, &selected.device, nullptr, nullptr, &err);
    if (err != CL_SUCCESS || !context) {
        std::cerr << "clCreateContext failed with code " << err << '\n';
        return false;
    }

    cl_command_queue queue = clCreateCommandQueue(context, selected.device, 0, &err);
    if (err != CL_SUCCESS || !queue) {
        std::cerr << "clCreateCommandQueue failed with code " << err << '\n';
        clReleaseContext(context);
        return false;
    }

    const std::string kernelPath = "fft_kernel.cl";
    std::string source;
    try {
        source = readKernelSource(kernelPath);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    const char* sourceText = source.c_str();
    cl_program program = clCreateProgramWithSource(context, 1, &sourceText, nullptr, &err);
    if (err != CL_SUCCESS || !program) {
        std::cerr << "clCreateProgramWithSource failed with code " << err << '\n';
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    err = clBuildProgram(program, 1, &selected.device, "-cl-fast-relaxed-math", nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::size_t logSize = 0;
        clGetProgramBuildInfo(program, selected.device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::string buildLog(logSize, '\0');
        clGetProgramBuildInfo(program, selected.device, CL_PROGRAM_BUILD_LOG, buildLog.size(), &buildLog[0], nullptr);
        std::cerr << "OpenCL build failed:\n" << buildLog << '\n';
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    cl_kernel kernel = clCreateKernel(program, "signal_magnitude", &err);
    if (err != CL_SUCCESS || !kernel) {
        std::cerr << "clCreateKernel failed with code " << err << '\n';
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    cl_mem inputBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(Complex) * n, nullptr, &err);
    if (err != CL_SUCCESS || !inputBuffer) {
        std::cerr << "clCreateBuffer (input) failed with code " << err << '\n';
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * n, nullptr, &err);
    if (err != CL_SUCCESS || !outputBuffer) {
        std::cerr << "clCreateBuffer (output) failed with code " << err << '\n';
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    err = clEnqueueWriteBuffer(queue, inputBuffer, CL_TRUE, 0, sizeof(Complex) * n, input.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "clEnqueueWriteBuffer failed with code " << err << '\n';
        clReleaseMemObject(outputBuffer);
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    if (err != CL_SUCCESS) {
        std::cerr << "clSetKernelArg failed with code " << err << '\n';
        clReleaseMemObject(outputBuffer);
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    const std::size_t globalSize = n;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &globalSize, nullptr, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "clEnqueueNDRangeKernel failed with code " << err << '\n';
        clReleaseMemObject(outputBuffer);
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    clFinish(queue);

    output.resize(n);
    err = clEnqueueReadBuffer(queue, outputBuffer, CL_TRUE, 0, sizeof(float) * n, output.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "clEnqueueReadBuffer failed with code " << err << '\n';
        clReleaseMemObject(outputBuffer);
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    clReleaseMemObject(outputBuffer);
    clReleaseMemObject(inputBuffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    return true;
}

// Computes the magnitude of a complex value using the Pythagorean formula:
// magnitude = sqrt(real^2 + imag^2)
std::vector<float> computeSignalMagnitude(const std::vector<Complex>& signal) {
    std::vector<float> magnitudes(signal.size());
    for (std::size_t i = 0; i < signal.size(); ++i) {
        magnitudes[i] = std::sqrt(signal[i].real * signal[i].real + signal[i].imag * signal[i].imag);
    }
    return magnitudes;
}

// Computes the maximum absolute difference between two floating-point arrays.
// This is used to compare CPU and OpenCL results in a numeric and easy-to-read way.
double maxAbsDifference(const std::vector<float>& a, const std::vector<float>& b) {
    double maxDifference = 0.0;
    const std::size_t count = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < count; ++i) {
        maxDifference = std::max(maxDifference, std::abs(static_cast<double>(a[i]) - static_cast<double>(b[i])));
    }
    return maxDifference;
}

// Prints the first several magnitude values to keep the output short and readable.
void printSignalMagnitude(const std::vector<float>& values, std::size_t limit = 8) {
    const std::size_t count = std::min(limit, values.size());
    std::cout << "Signal magnitude values (first " << count << "):\n";
    for (std::size_t i = 0; i < count; ++i) {
        std::cout << "  sample[" << i << "] = " << std::fixed << std::setprecision(4) << values[i] << '\n';
    }
}

// Prints the frequency-domain magnitude of an FFT result.
void printSpectrum(const std::vector<Complex>& data, std::size_t limit = 8) {
    const std::size_t count = std::min(limit, data.size());
    std::cout << "FFT bins (magnitude):\n";
    for (std::size_t i = 0; i < count; ++i) {
        const double mag = std::sqrt(static_cast<double>(data[i].real) * data[i].real +
                                    static_cast<double>(data[i].imag) * data[i].imag);
        std::cout << "  bin[" << i << "] = " << std::fixed << std::setprecision(4) << mag << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    CliOptions options;

    if (!parseCommandLineArgs(argc, argv, options)) {
        return 1;
    }

    if (options.signalLength == 0) {
        std::cerr << "Signal length must be greater than zero.\n";
        return 1;
    }

    const std::vector<Complex> signal = generateSignal(options.signalLength);
    const std::vector<DeviceInfo> devices = enumerateOpenCLDevices();
    printDeviceSummary(devices);

    std::cout << "OpenCL signal processing demo\n";
    std::cout << "Requested operation: " << (options.operation == OperationMode::FFT ? "fft" : "magnitude") << "\n";
    std::cout << "Signal length: " << options.signalLength << "\n";

    if (options.operation == OperationMode::Magnitude) {
        const std::vector<float> cpuReference = cpuMagnitude(signal);
        const double cpuTimeMs = measureDurationMs([&]() {
            auto ignored = cpuMagnitude(signal);
            (void)ignored;
        });

        std::vector<float> magnitudeOutput;
        const double openClStart = std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        const bool openClUsed = runOpenCLMagnitude(signal, magnitudeOutput);
        const double openClTimeMs = measureDurationMs([&]() {
            std::vector<float> temp;
            const bool ok = runOpenCLMagnitude(signal, temp);
            if (ok) {
                magnitudeOutput = temp;
            }
        });

        if (!openClUsed) {
            std::cout << "No OpenCL device found. Falling back to CPU magnitude calculation.\n";
            magnitudeOutput = cpuReference;
        }

        const double maxDifference = maxAbsDifference(magnitudeOutput, cpuReference);
        std::cout << "CPU reference time: " << cpuTimeMs << " ms\n";
        std::cout << "OpenCL time: " << (openClUsed ? openClTimeMs : 0.0) << " ms\n";
        std::cout << "Max difference from CPU reference: " << maxDifference << '\n';
        printSignalMagnitude(magnitudeOutput, 8);
        return 0;
    }

    if (!isPowerOfTwo(options.signalLength)) {
        std::size_t lower = 1;
        while (lower < options.signalLength) {
            lower <<= 1;
        }
        std::size_t upper = lower;
        if (lower > options.signalLength) {
            upper = lower;
            lower = lower >> 1;
        }

        std::cerr << "FFT mode requires a positive power-of-two signal length.\n";
        std::cerr << "Closest valid choices near " << options.signalLength << ": "
                  << lower << " and " << upper << "\n";
        return 1;
    }

    const std::vector<Complex> cpuReference = cpuFft(signal);
    const double cpuTimeMs = measureDurationMs([&]() {
        auto ignored = cpuFft(signal);
        (void)ignored;
    });

    std::vector<Complex> fftOutput;
    const double openClTimeMs = measureDurationMs([&]() {
        std::vector<Complex> temp;
        const bool ok = runOpenCLFFT(signal, temp);
        if (ok) {
            fftOutput = temp;
        }
    });
    const bool openClUsed = !fftOutput.empty();
    if (!openClUsed) {
        std::cout << "No OpenCL device found. Falling back to CPU reference FFT.\n";
        fftOutput = cpuReference;
    }

    double maxDifference = 0.0;
    for (std::size_t i = 0; i < fftOutput.size(); ++i) {
        const double diff = std::hypot(static_cast<double>(fftOutput[i].real - cpuReference[i].real),
                                      static_cast<double>(fftOutput[i].imag - cpuReference[i].imag));
        maxDifference = std::max(maxDifference, diff);
    }

    std::cout << "CPU reference time: " << cpuTimeMs << " ms\n";
    std::cout << "OpenCL time: " << (openClUsed ? openClTimeMs : 0.0) << " ms\n";
    if (openClUsed) {
        std::cout << "OpenCL device successfully used.\n";
    }
    std::cout << "Max difference from CPU reference: " << maxDifference << '\n';
    printSpectrum(fftOutput, 8);

    return 0;
}
