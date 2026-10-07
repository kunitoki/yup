#version 450

// Samples the window canvas. Rive renders sRGB-encoded, premultiplied colors: when the
// swapchain is an sRGB format, which encodes on write, they are decoded first.

layout (push_constant) uniform PresentParameters
{
    vec4 rotation;
    int decodeSrgb;
} parameters;

layout (set = 0, binding = 0) uniform sampler2D u_canvas;

layout (location = 0) in vec2 v_texCoord;
layout (location = 0) out vec4 o_color;

vec3 srgbToLinear (vec3 color)
{
    return mix (color / 12.92, pow ((color + 0.055) / 1.055, vec3 (2.4)), step (vec3 (0.04045), color));
}

void main()
{
    vec4 color = texture (u_canvas, v_texCoord);

    if (parameters.decodeSrgb != 0 && color.a > 0.0)
        color.rgb = srgbToLinear (color.rgb / color.a) * color.a;

    o_color = color;
}
