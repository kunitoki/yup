#version 450

// Large pixels and few colors over the picture of a component. A stylesheet runs it with `effect: Pixelate`.
// It has no `time`, so the component is painted again only when it changes.

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 resolution;    // filled in: the width and the height of the picture in pixels, then 1 / width and 1 / height
    float intensity;    // property intensity: number
    float scale;        // filled in: the pixels of a logical unit, 2 on a dense display
} params;

layout(set = 0, binding = 1) uniform texture2D source;
layout(set = 0, binding = 2) uniform sampler sourceSampler;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main()
{
    // Squares of 1 to 14 logical units, each showing what the picture has at its center
    float side = max (1.0, floor (params.intensity * 14.0 + 0.5)) * params.scale;
    vec2 squares = params.resolution.xy / side;
    vec4 pixel = texture (sampler2D (source, sourceSampler), (floor (uv * squares) + 0.5) / squares);

    // Fewer levels of each color as the squares grow. The levels are those of the color itself: the picture has
    // it multiplied by its alpha
    float levels = mix (24.0, 4.0, params.intensity);
    vec3 straight = pixel.rgb / max (pixel.a, 0.0001);

    color = vec4 (floor (straight * levels + 0.5) / levels * pixel.a, pixel.a);
}
