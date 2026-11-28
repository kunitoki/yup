#version 450

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;          // property tint: color, as it is written: sRGB, not multiplied by its alpha
    vec4 resolution;    // filled in: the width and the height of the picture in pixels, then 1 / width and 1 / height
    float count;        // property count: number
    float time;         // filled in: seconds, which has the component painted again at every frame
} params;

layout(location = 0) in vec2 uv;                               // (0, 0) at the top left, (1, 1) at the bottom right
layout(location = 0) out vec4 color;

void main()
{
    // As wide as high whatever the bounds are: width / height is resolution.x * resolution.w
    vec2 p = (uv - vec2 (0.5)) * vec2 (params.resolution.x * params.resolution.w, 1.0);
    float radius = length (p) * 2.0;                                            // 1 at the top and at the bottom

    // Rings leaving the center, fading out as they go
    float rings = 0.5 + 0.5 * cos (radius * params.count * 6.2832 - params.time * 3.0);
    float alpha = params.tint.a * rings * clamp (1.0 - radius, 0.0, 1.0);

    color = vec4 (params.tint.rgb * alpha, alpha);                              // multiplied by its alpha, as every picture is
}
