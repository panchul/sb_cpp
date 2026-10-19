#if defined(__APPLE__)
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Complex {
    float real = 0.0f;
    float imag = 0.0f;
};

bool isPowerOfTwo(std::size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

std::size_t nextPowerOfTwo(std::size_t value) {
    std::size_t power = 1;
    while (power < value) {
        power <<= 1;
    }
    return power;
}

Complex complexAdd(const Complex& a, const Complex& b) {
    return {a.real + b.real, a.imag + b.imag};
}

Complex complexSub(const Complex& a, const Complex& b) {
    return {a.real - b.real, a.imag - b.imag};
}

Complex complexMul(const Complex& a, const Complex& b) {
    return {
        a.real * b.real - a.imag * b.imag,
        a.real * b.imag + a.imag * b.real,
    };
}

std::vector<Complex> generateSignal(std::size_t n) {
    std::vector<Complex> signal(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double angle = 2.0 * M_PI * static_cast<double>(i) / static_cast<double>(n);
        const float a = static_cast<float>(std::sin(angle * 3.0));
        const float b = static_cast<float>(0.5 * std::cos(angle * 11.0));
        signal[i] = {a + b, 0.0f};
    }
    return signal;
}

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

struct DeviceInfo {
    cl_platform_id platform = nullptr;
    cl_device_id device = nullptr;
    std::string platformName;
    std::string deviceName;
    bool isGPU = false;
};

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

DeviceInfo chooseOpenCLDevice() {
    cl_uint numPlatforms = 0;
    cl_int err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        return {};
    }

    std::vector<cl_platform_id> platforms(numPlatforms);
    clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);

    for (cl_platform_id platform : platforms) {
        cl_uint numDevices = 0;
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        if (numDevices == 0) {
            continue;
        }

        std::vector<cl_device_id> devices(numDevices);
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);

        for (cl_device_id device : devices) {
            char platformBuffer[256] = {};
            char deviceBuffer[256] = {};
            clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platformBuffer), platformBuffer, nullptr);
            clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(deviceBuffer), deviceBuffer, nullptr);

            cl_device_type deviceType = 0;
            clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(deviceType), &deviceType, nullptr);

            DeviceInfo info;
            info.platform = platform;
            info.device = device;
            info.platformName = platformBuffer;
            info.deviceName = deviceBuffer;
            info.isGPU = (deviceType & CL_DEVICE_TYPE_GPU) != 0;

            if (info.isGPU) {
                return info;
            }

            if (!info.platformName.empty() && !info.deviceName.empty()) {
                // Keep the best fallback candidate.
                if (!info.platformName.empty()) {
                    // We need a stable fallback, so remember the first non-GPU candidate.
                    static DeviceInfo fallback;
                    if (!fallback.device) {
                        fallback = info;
                    }
                }
            }
        }
    }

    // Fallback to a CPU OpenCL device when available.
    cl_platform_id fallbackPlatform = nullptr;
    cl_device_id fallbackDevice = nullptr;
    std::vector<cl_platform_id> fallbackPlatforms(numPlatforms);
    clGetPlatformIDs(numPlatforms, fallbackPlatforms.data(), nullptr);
    for (cl_platform_id platform : fallbackPlatforms) {
        cl_uint numDevices = 0;
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        if (numDevices == 0) {
            continue;
        }
        std::vector<cl_device_id> devices(numDevices);
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
        for (cl_device_id device : devices) {
            cl_device_type deviceType = 0;
            clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(deviceType), &deviceType, nullptr);
            if ((deviceType & CL_DEVICE_TYPE_CPU) != 0 || (deviceType & CL_DEVICE_TYPE_DEFAULT) != 0) {
                char platformBuffer[256] = {};
                char deviceBuffer[256] = {};
                clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platformBuffer), platformBuffer, nullptr);
                clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(deviceBuffer), deviceBuffer, nullptr);
                DeviceInfo info;
                info.platform = platform;
                info.device = device;
                info.platformName = platformBuffer;
                info.deviceName = deviceBuffer;
                info.isGPU = false;
                return info;
            }
        }
    }

    return {};
}

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

void printSpectrum(const std::vector<Complex>& data, std::size_t limit = 8) {
    const std::size_t count = std::min(limit, data.size());
    std::cout << "FFT bins (magnitude):\n";
    for (std::size_t i = 0; i < count; ++i) {
        const double mag = std::sqrt(static_cast<double>(data[i].real) * data[i].real + static_cast<double>(data[i].imag) * data[i].imag);
        std::cout << "  bin[" << i << "] = " << std::fixed << std::setprecision(4) << mag << '\n';
    }
}

}  // namespace

int main() {
    const std::size_t n = 256;
    const std::vector<Complex> signal = generateSignal(n);
    const std::vector<Complex> reference = cpuFft(signal);

    std::cout << "OpenCL FFT demo\n";
    std::cout << "Signal length: " << n << "\n";

    std::vector<Complex> fftOutput;
    bool openClUsed = runOpenCLFFT(signal, fftOutput);
    if (!openClUsed) {
        std::cout << "No OpenCL device found. Falling back to CPU reference FFT.\n";
        fftOutput = reference;
    }

    double maxDifference = 0.0;
    for (std::size_t i = 0; i < fftOutput.size(); ++i) {
        const double diff = std::hypot(static_cast<double>(fftOutput[i].real - reference[i].real),
                                      static_cast<double>(fftOutput[i].imag - reference[i].imag));
        maxDifference = std::max(maxDifference, diff);
    }

    if (openClUsed) {
        std::cout << "OpenCL device successfully used.\n";
    }
    std::cout << "Max difference from CPU reference: " << maxDifference << '\n';
    printSpectrum(fftOutput);

    return 0;
}
