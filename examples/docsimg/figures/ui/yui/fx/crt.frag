#version 450

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;          // property tint: color, as it is written: sRGB, not multiplied by its alpha
    vec4 resolution;    // filled in: the width and the height of the picture in pixels, then 1 / width and 1 / height
    float intensity;    // property intensity: number
    float scale;        // filled in: the pixels a unit of the component takes, 2 on a dense display
    float time;         // filled in: seconds, which has the component painted again at every frame
} params;

layout(set = 0, binding = 1) uniform texture2D source;         // what the component painted
layout(set = 0, binding = 2) uniform sampler sourceSampler;

layout(location = 0) in vec2 uv;                               // (0, 0) at the top left, (1, 1) at the bottom right
layout(location = 0) out vec4 color;

void main()
{
    vec4 pixel = texture (sampler2D (source, sourceSampler), uv);                     // multiplied by its alpha

    // A line a fifth darker every four units of the component, whatever the density of the display, crawling down
    float row = uv.y * params.resolution.y / params.scale;
    float lines = 0.9 + 0.1 * sin (row * 1.5708 - params.time * 4.0);

    // Only the corners get darker: the middle of an edge is 0.5 away from the middle of the picture, a corner 0.7
    float corners = 1.0 - 0.5 * smoothstep (0.5, 0.7, length (uv - vec2 (0.5)));

    // The tint is a glow added where the picture is, and no brighter than its alpha lets it be
    vec3 glow = min (pixel.rgb + params.tint.rgb * pixel.a * 0.25, vec3 (pixel.a));

    color = mix (pixel, vec4 (glow * lines * corners, pixel.a), params.intensity);    // still multiplied by its alpha
}
