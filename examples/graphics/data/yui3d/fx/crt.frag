#version 450

// A cathode ray tube over the picture of a component: a bulging glass, colors landing slightly apart, dark lines
// crawling down and corners fading out. A stylesheet runs it with `effect: Crt`.
//
// An effect reads the picture of its component and writes the one that is shown. Both have their colors
// multiplied by their alpha.

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 resolution;    // filled in: the width and the height of the picture in pixels, then 1 / width and 1 / height
    float intensity;    // property intensity: number
    float scale;        // filled in: the pixels of a logical unit, 2 on a dense display
    float time;         // filled in: seconds, which also has the component painted again at every frame
} params;

layout(set = 0, binding = 1) uniform texture2D source;
layout(set = 0, binding = 2) uniform sampler sourceSampler;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main()
{
    // The glass bulges: the picture is read further out towards its corners, and nothing is there past its edges
    vec2 centered = uv - 0.5;
    vec2 bent = uv + centered * dot (centered, centered) * 0.35 * params.intensity;
    vec2 inside = step (vec2 (0.0), bent) * step (bent, vec2 (1.0));

    // Red and blue land on either side of green, further apart away from the center
    vec2 apart = centered * 0.012 * params.intensity;
    vec4 red = texture (sampler2D (source, sourceSampler), bent + apart);
    vec4 green = texture (sampler2D (source, sourceSampler), bent);
    vec4 blue = texture (sampler2D (source, sourceSampler), bent - apart);

    // A dark line every three logical units, whatever the density of the display
    float rows = bent.y * params.resolution.y / params.scale;
    float lines = 1.0 - 0.45 * params.intensity * (0.5 + 0.5 * sin (rows * 2.0944 - params.time * 3.0));
    float corners = 1.0 - 0.7 * params.intensity * smoothstep (0.4, 0.95, length (centered) * 1.3);

    // Darker, and as opaque as it was
    color = vec4 (vec3 (red.r, green.g, blue.b) * lines * corners, green.a) * inside.x * inside.y;
}
