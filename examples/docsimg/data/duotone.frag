#version 450

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 shadows;       // the color the dark parts of the picture take
    vec4 highlights;    // the color its bright parts take
    float amount;       // 0 leaves the picture as it is, 1 replaces its colors
} params;

layout(set = 0, binding = 1) uniform texture2D source;
layout(set = 0, binding = 2) uniform sampler sourceSampler;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main()
{
    vec4 pixel = texture (sampler2D (source, sourceSampler), uv);

    // The picture is multiplied by its alpha, and so is the color written
    float luma = dot (pixel.rgb, vec3 (0.2126, 0.7152, 0.0722)) / max (pixel.a, 0.001);
    vec4 toned = vec4 (mix (params.shadows.rgb, params.highlights.rgb, luma) * pixel.a, pixel.a);

    color = mix (pixel, toned, params.amount);
}
