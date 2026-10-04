#version 450

// This vertex shader is executed once per vertex in the mesh.
// Its job is to transform each incoming vertex from object space into clip space,
// so the rasterizer can project the 3D cube onto the 2D framebuffer.

// Input vertex data from the CPU-side vertex buffer.
// - inPosition: local object-space coordinates (x, y, z)
// - inColor: per-vertex color used to tint the final fragment shading
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

// A small block of values pushed from the C++ side each frame.
// These represent the current animation state and camera/projection settings.
layout(push_constant) uniform PushConstants {
    float rotationX;
    float rotationY;
    float rotationZ;
    float aspect;
    float time;
} push;

// Output passed to the fragment shader.
layout(location = 0) out vec3 fragColor;

void main() {
    // The main() function in a Vulkan vertex shader is the entry point for the
    // vertex-processing stage. The GPU calls it once for every vertex in the
    // draw call, and it must produce the final position in clip space via
    // gl_Position. Without this, Vulkan would have no screen-space vertex data
    // to rasterize.

    // Rotate the object around each axis based on the current animation state.
    // These sine/cosine values are computed from push constants that the C++ code
    // updates each frame.
    float cx = cos(push.rotationX);
    float sx = sin(push.rotationX);
    float cy = cos(push.rotationY);
    float sy = sin(push.rotationY);
    float cz = cos(push.rotationZ + push.time * 0.35);
    float sz = sin(push.rotationZ + push.time * 0.35);

    // Apply X rotation.
    vec3 posX = vec3(
        inPosition.x,
        inPosition.y * cx - inPosition.z * sx,
        inPosition.y * sx + inPosition.z * cx
    );

    // Apply Y rotation.
    vec3 posY = vec3(
        posX.x * cy + posX.z * sy,
        posX.y,
        -posX.x * sy + posX.z * cy
    );

    // Apply Z rotation.
    vec3 posZ = vec3(
        posY.x * cz - posY.y * sz,
        posY.x * sz + posY.y * cz,
        posY.z
    );

    // Move the model away from the camera so it is visible in front of the view.
    vec3 viewPos = posZ + vec3(0.0, 0.0, -2.8);

    // Perspective projection parameters.
    float nearPlane = 0.1;
    float farPlane = 10.0;
    float f = 1.0 / tan(radians(55.0) * 0.5);

    // Build a projection matrix matching the Vulkan clip-space convention.
    // This converts 3D view-space coordinates into normalized device coordinates.
    mat4 projection = mat4(
        f / push.aspect, 0.0, 0.0, 0.0,
        0.0, -f, 0.0, 0.0,
        0.0, 0.0, farPlane / (nearPlane - farPlane), -1.0,
        0.0, 0.0, (nearPlane * farPlane) / (nearPlane - farPlane), 0.0
    );

    // The primary output of the vertex shader: final clip-space position.
    // Vulkan rasterization uses this to determine which fragments to draw.
    gl_Position = projection * vec4(viewPos, 1.0);

    // Pass the interpolated color to the fragment shader.
    // The sine term adds a subtle animation effect to keep the object lively.
    fragColor = inColor * (0.78 + 0.22 * sin(push.time + posZ.x + posZ.y + posZ.z));
}
