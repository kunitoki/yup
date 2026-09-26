#version 450

// GLSL 450 fragment shader: hard opaque circle, no blending.
//
// vOffset is the raw quad corner offset in [-0.5, 0.5].
// Multiply by 2 to normalise to [-1, 1] for a correct
// unit-circle distance test that works with any viewport aspect.
layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vOffset;

layout(location = 0) out vec4 outColor;

void main() {
    // Raw offset is always [-0.5, 0.5] on both axes regardless of
    // viewport aspect compensation. Normalise to [-1, 1] for a true circle.
    float dist = length(vOffset * 2.0);
    if (dist > 1.0)
        discard;
    outColor = vColor;
}
