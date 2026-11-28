#version 450

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;          // property tint: color, as it is written: sRGB, not multiplied by its alpha
    float gain;         // property gain: number
} params;

// property levels: number[] - one number for each pixel along x, in the red channel
layout(set = 0, binding = 1) uniform texture2D levels;
layout(set = 0, binding = 2) uniform sampler levelsSampler;

layout(location = 0) in vec2 uv;                               // (0, 0) at the top left, (1, 1) at the bottom right
layout(location = 0) out vec4 color;

void main()
{
    // The sampler interpolates between two numbers, so the curve is smooth whatever its width
    float level = clamp (texture (sampler2D (levels, levelsSampler), vec2 (uv.x, 0.5)).r * params.gain, 0.0, 1.0);
    float height = 1.0 - uv.y;                                 // 0 at the bottom, 1 at the top

    // What is under the curve, brighter toward it, and the curve itself
    float under = 1.0 - smoothstep (level - 0.01, level + 0.01, height);
    float fill = under * (0.12 + 0.4 * height / max (level, 0.001));
    float line = 1.0 - smoothstep (0.0, 0.025, abs (height - level));
    float alpha = params.tint.a * clamp (fill + line, 0.0, 1.0);

    color = vec4 (params.tint.rgb * alpha, alpha);             // multiplied by its alpha, as every picture is
}
