#version 450

// Bands of light drifting across each other: the aurora behind the scene of shapes.yui, and the glow of the screen
// of computer.yui. A drawing paints it, so it reads no picture: it writes one, with its colors multiplied by its
// alpha, over what the drawing painted before.

layout(set = 0, binding = 0) uniform Parameters
{
    vec4 ink;           // property ink: color, as it is written: sRGB, not multiplied by its alpha
    vec4 resolution;    // filled in: the width and the height of the picture in pixels, then 1 / width and 1 / height
    float flow;         // property flow: number
    float time;         // filled in: seconds, which also has the component painted again at every frame
} params;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main()
{
    // As wide as high, whatever the shape of the component
    vec2 p = (uv - 0.5) * vec2 (params.resolution.x * params.resolution.w, 1.0);
    float t = params.time * params.flow;

    // Three bands of light drifting across each other
    float bands = sin (p.x * 2.6 + sin (p.y * 1.9 + t * 0.31) * 1.4 + t * 0.23)
                + sin (p.y * 3.4 - p.x * 1.2 - t * 0.37)
                + sin (length (p - vec2 (0.3, -0.2)) * 4.1 - t * 0.41);

    float glow = smoothstep (0.2, 2.6, bands + 1.2);

    // Two tones of the ink: the second one has its channels turned
    vec3 tone = mix (params.ink.rgb, params.ink.brg, 0.5 + 0.5 * sin (p.x * 1.7 - t * 0.19));
    float alpha = params.ink.a * glow * 0.75;

    color = vec4 (tone * alpha, alpha);
}
