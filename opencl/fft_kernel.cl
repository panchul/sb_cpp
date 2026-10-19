// This OpenCL source file defines the math used by the signal-processing demo.
//
// A complex number is represented here as a pair of floating-point values:
//   real = the real component
//   imag = the imaginary component
//
// We define a few small helper functions for adding, subtracting, and multiplying
// complex numbers. These operations are the basic building blocks of the FFT algorithm.
typedef struct {
    float real;
    float imag;
} Complex;

// This function adds two complex numbers.
// It is used when combining the even and odd parts in an FFT butterfly.
Complex complex_add(Complex a, Complex b) {
    Complex result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

// This function subtracts two complex numbers.
// In FFT math, the second half of each butterfly is often computed as:
//   even - odd * twiddle
Complex complex_sub(Complex a, Complex b) {
    Complex result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}

// This function multiplies two complex numbers using the standard formula:
// (a + bi) * (c + di) = (ac - bd) + (ad + bc)i
Complex complex_mul(Complex a, Complex b) {
    Complex result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

// Returns the magnitude of a complex number using the Pythagorean relation:
// magnitude = sqrt(real^2 + imag^2)
//
// This is useful because a signal sample may be represented as a complex value,
// and the magnitude tells us how large that sample is without caring about its phase.
float complex_magnitude(Complex value) {
    return sqrt(value.real * value.real + value.imag * value.imag);
}

// This kernel performs one butterfly stage of the FFT.
//
// Think of the FFT as a repeated pattern of small math operations called a butterfly.
// A butterfly combines two complex numbers and produces two new numbers.
// The overall FFT is just many butterfly stages chained together.
//
// OpenCL model:
//   - A kernel is executed by many work-items at the same time.
//   - Each work-item is like a tiny thread.
//   - get_global_id(0) tells this work-item which position it should handle.
//
// In this kernel, every work-item handles one butterfly operation.
// The kernel does not process the entire FFT by itself; it just handles one stage.
// The host code calls this kernel repeatedly, once for each FFT stage.
//
// Input:
//   data - a complex array in bit-reversed order
//   stage - which FFT stage we are currently computing
//   n - total number of samples in the FFT
//
// Output:
//   data is updated in place with the transformed values for this stage.
__kernel void fft_stage(__global Complex *data,
                        const int stage,
                        const int n) {
    // get_global_id(0) is one of the most important OpenCL built-ins.
    // It returns the unique global index of this work-item.
    //
    // Example:
    //   If the global work size is 128, the work-items are numbered 0, 1, 2, ..., 127.
    //   Each work-item is responsible for one butterfly in that stage.
    //
    // This is the OpenCL equivalent of saying:
    //   "Which element / task am I responsible for in the parallel calculation?"
    const int id = get_global_id(0);

    // Each FFT stage has n / 2 butterfly operations.
    // If the kernel is launched with more work-items than needed, these extra items will exit immediately.
    // This is a common OpenCL pattern: guard against out-of-range work-items.
    if (id >= n / 2) {
        return;
    }

    // For a radix-2 FFT, step = 2^stage.
    // This tells us the distance between the two numbers being combined in the butterfly.
    //
    // Example:
    //   stage 0 -> step = 1
    //   stage 1 -> step = 2
    //   stage 2 -> step = 4
    const int step = 1 << stage;

    // We are grouping the data into blocks.
    // 'group' identifies which block this butterfly belongs to.
    // 'offset' tells us where in that block we are.
    // Together they tell us exactly which pair of numbers to combine.
    const int group = id / step;
    const int offset = id % step;

    // Pair of complex values in this butterfly.
    // The left value is at idx1 and the right value is at idx2.
    // The two values are separated by 'step'.
    const int idx1 = group * (2 * step) + offset;
    const int idx2 = idx1 + step;

    const Complex even = data[idx1];
    const Complex odd = data[idx2];

    // Twiddle factor:
    // This is the rotation term used by the FFT.
    // The formula is:
    //   exp(-i * 2*pi * offset / (2 * step))
    //
    // We calculate it with cos() and sin() because it is a complex number on the unit circle.
    // Multiplying by this factor rotates the odd value before adding or subtracting it.
    const float angle = -2.0f * 3.14159265358979323846f * (float)offset / (float)(2 * step);
    const Complex twiddle = { cos(angle), sin(angle) };
    const Complex product = complex_mul(odd, twiddle);

    // Butterfly operation:
    //   even + odd * twiddle
    //   even - odd * twiddle
    //
    // This is the core FFT math.
    // It combines the two inputs into the output pair.
    // The data array is updated in place, so the next stage reads these new values.
    data[idx1] = complex_add(even, product);
    data[idx2] = complex_sub(even, product);
}

// This kernel computes the magnitude of each complex value in a signal.
//
// A signal may be represented as complex numbers, even when the imaginary part is zero.
// The magnitude tells us "how large" each sample is, regardless of direction/phase.
//
// In OpenCL, a kernel is launched across many work-items. Each work-item handles one element.
// The work-item index is provided by get_global_id(0), which is the global index in the NDRange.
//
// The phrase "NDRange" sounds complicated, but it simply means:
//   "the total parallel job we are launching across the device"
//
// Example:
//   if the input has 256 values, the kernel is launched with 256 work-items.
//   work-item 0 computes magnitude for input[0],
//   work-item 1 computes magnitude for input[1],
//   ...
//   work-item 255 computes magnitude for input[255].
//
// Input:
//   input - complex data array
//
// Output:
//   output - floating-point magnitudes for each corresponding input sample
__kernel void signal_magnitude(__global const Complex *input,
                              __global float *output) {
    // get_global_id(0) is the OpenCL equivalent of:
    //   "Which item in the parallel job is this one?"
    //
    // It gives each work-item a unique global index.
    // This means each work-item can safely read one input element and write one output element
    // without overwriting another work-item's result.
    const int id = get_global_id(0);

    // For this sample, calculate the magnitude:
    //   sqrt(real^2 + imag^2)
    // and store it in the same position in the output array.
    output[id] = complex_magnitude(input[id]);
}
