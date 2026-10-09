#version 450

// Draws the window canvas onto a swapchain image with one fullscreen triangle,
// rotated by the surface pre-transform so the compositor does not have to.

layout (push_constant) uniform PresentParameters
{
    vec4 rotation;  // Row-major 2x2 matrix applied to clip space positions
    int decodeSrgb; // Non-zero when the swapchain encodes sRGB itself
} parameters;

layout (location = 0) out vec2 v_texCoord;

void main()
{
    const vec2 position = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0 - 1.0;

    v_texCoord = position * 0.5 + 0.5;

    gl_Position = vec4 (dot (parameters.rotation.xy, position),
                        dot (parameters.rotation.zw, position),
                        0.0,
                        1.0);
}
