typedef struct {
    float real;
    float imag;
} Complex;

Complex complex_add(Complex a, Complex b) {
    Complex result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

Complex complex_sub(Complex a, Complex b) {
    Complex result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}

Complex complex_mul(Complex a, Complex b) {
    Complex result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

__kernel void fft_stage(__global Complex *data,
                        const int stage,
                        const int n) {
    const int id = get_global_id(0);
    if (id >= n / 2) {
        return;
    }

    const int step = 1 << stage;
    const int group = id / step;
    const int offset = id % step;

    const int idx1 = group * (2 * step) + offset;
    const int idx2 = idx1 + step;

    const Complex even = data[idx1];
    const Complex odd = data[idx2];

    const float angle = -2.0f * 3.14159265358979323846f * (float)offset / (float)(2 * step);
    const Complex twiddle = { cos(angle), sin(angle) };
    const Complex product = complex_mul(odd, twiddle);

    data[idx1] = complex_add(even, product);
    data[idx2] = complex_sub(even, product);
}
