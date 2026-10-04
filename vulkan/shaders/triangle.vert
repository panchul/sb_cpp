#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(push_constant) uniform PushConstants {
    vec2 offset;
    vec2 scale;
    float rotation;
    float time;
} push;

layout(location = 0) out vec3 fragColor;

void main() {
    float cosA = cos(push.rotation);
    float sinA = sin(push.rotation);

    vec2 rotated = vec2(
        inPosition.x * cosA - inPosition.y * sinA,
        inPosition.x * sinA + inPosition.y * cosA
    );

    vec2 finalPos = rotated * push.scale + push.offset;
    gl_Position = vec4(finalPos, 0.0, 1.0);
    fragColor = inColor * (0.8 + 0.2 * sin(push.time + finalPos.x + finalPos.y));
}
