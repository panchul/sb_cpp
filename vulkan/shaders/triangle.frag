#version 450

// This fragment shader is executed for each rasterized pixel covered by the mesh.
// Its purpose is to determine the final color of that pixel after interpolation
// from the vertex shader outputs.

// Incoming interpolated color produced by the vertex shader.
layout(location = 0) in vec3 fragColor;

// Final framebuffer output: RGBA color for the current pixel.
layout(location = 0) out vec4 outColor;

void main() {
    // The main() function in a Vulkan fragment shader is the per-fragment entry
    // point. It computes the final color that will be written to the render target.
    // In this sample, the fragment shader simply uses the interpolated vertex color
    // and writes it to the color attachment, which is then presented to the user.
    outColor = vec4(fragColor, 1.0);
}
