#version 450

// Shadertoy's y-up pixel space, since RHI targets read top-left-origin everywhere.
layout(set = 0, binding = 0) uniform Params
{
    float time;
    float width;
    float height;
    float pad;
} u;

layout(set = 0, binding = 1) uniform Samples
{
    vec4 samples[64];
} waveform;

layout(location = 0) out vec4 fragColor;

const vec3 accent = vec3(0.447, 0.918, 0.824);
const float focal = 2.8;
const vec3 background = vec3(0.055, 0.067, 0.078);

float fetchSample(int index)
{
    return waveform.samples[index >> 2][index & 3];
}

float wave(float phase)
{
    float position = phase * 255.0;
    int i0 = int(position);
    int i1 = min(i0 + 1, 255);
    return mix(fetchSample(i0), fetchSample(i1), position - float(i0));
}

void main()
{
    vec2 resolution = vec2(u.width, u.height);
    vec2 I = vec2(gl_FragCoord.x, u.height - gl_FragCoord.y);

    // The ridge on the far wall, four units away, spans one period across the width.
    vec3 direction = normalize(vec3(I + I - resolution, -u.height * focal));
    float frequency = u.height * focal / (8.0 * u.width);

    float scroll = 0.5 + u.time * 0.01;
    float hue = u.time * 0.15;

    vec3 color = vec3(0.0);
    float z = 0.0;

    for (int i = 0; i < 90; ++i)
    {
        vec3 p = z * direction + vec3(0.0, 1.0, 1.0);

        float r = max(-p.y, 0.0);
        p.y += r + r;

        p.y -= wave(fract(p.x * frequency + scroll));

        for (float octave = 2.0; octave < 30.0; octave += octave)
            p.y += 0.12 * cos(p.x * octave + 0.6 * u.time * cos(octave) + z) / octave;

        float plane = p.z + 3.0;
        float d = (0.1 * r + abs(p.y - 1.0) / (1.0 + r + r + r * r) + max(plane, -plane * 0.1)) / 8.0;
        z += d;

        float phase = z * 0.5 + hue;
        vec3 tone = accent * (cos(phase) + 1.3) + vec3(0.0, 0.15, 0.08) * cos(phase + 2.0);
        color += tone / max(d * z, 1.0e-4);
    }

    fragColor = vec4(max(tanh(color / 900.0), background), 1.0);
}
