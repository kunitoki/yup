// Ribbon Clouds by Abhishek Shivakumar (@godofecht).
// Copyright (c) 2026 Abhishek Shivakumar. Distributed under the ISC license above.

// Value noise from the 256x256 noise texture on iChannel0: each z slice is the texture shifted by (37, 17),
// and the smoothstepped fraction makes the hardware bilinear fetch do the xy interpolation. The noise repeats
// every 256 units in z, so z can wrap there without a seam.
float noise3(vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f * f * (3.0 - 2.0 * f);

    vec2 uv = mod(p.xy + vec2(37.0, 17.0) * p.z, 256.0) + f.xy + 0.5;
    float a = textureLod(iChannel0, uv / 256.0, 0.0).x;
    float b = textureLod(iChannel0, (uv + vec2(37.0, 17.0)) / 256.0, 0.0).x;

    return mix(a, b, f.z);
}

// One noise scaled to the contrast of the average of two independent ones
float noise(vec3 x)
{
    return 0.5 + 0.7071 * (noise3(x) - 0.5);
}

// Octaves fade to their mean once a noise cell shrinks below a few pixels, instead of aliasing into bright speckles
float fbm(vec3 p, int octaves, float cellsPerPixel)
{
    float s = 0.5 * (1.0 - exp2(-float(octaves)));
    float a = 0.5;

    for (int i = 0; i < octaves; ++i)
    {
        float w = clamp(2.0 - 4.0 * cellsPerPixel, 0.0, 1.0);
        if (w <= 0.0)
        {
            break;
        }

        s += a * w * (noise(p) - 0.5);
        p *= 2.0; // exact octaves keep the 256 z period, see noise3
        a *= 0.5;
        cellsPerPixel *= 2.0;
    }

    return s;
}

mat2 rot(float a)
{
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c);
}

vec3 palette(float t)
{
    return 0.55 + 0.45 * cos(6.28318 * (vec3(0.00, 0.12, 0.24) + t));
}

// The layers stream between these scales and are recycled at either end, faded in and out, so the zoom can go
// on in either direction without the coordinates growing
const int layers = 32;
const float nearScale = 0.6;
const float farScale = 5.0;
const float zoomDepth = 1.4;  // ln(scale) swung each way, about 4x in and out
const float zoomSpeed = 0.18; // rad/s, a full in and out cycle every ~35 s

void mainImage(out vec4 O, vec2 U)
{
    vec2 R = iResolution.xy;
    vec2 uv = (2.0 * U - R) / R.y;
    float t = iTime;

    vec2 p = uv;
    p *= rot(0.15 * sin(0.35 * t));

    float travel = zoomDepth * sin(zoomSpeed * t) / log(farScale / nearScale) * float(layers);
    float phase = fract(travel);
    float pulse = 1.0 + 0.12 * sin(1.3 * t);

    vec3 col = vec3(0.0);
    float trans = 1.0;
    float glow = 0.0;

    vec3 bg = vec3(0.01, 0.005, 0.02);
    bg += 0.04 * palette(0.1 * length(uv) - 0.05 * t);

    for (int i = 0; i < layers; ++i)
    {
        float fi = (float(i) + 1.0 - phase) / float(layers);
        float z = 2.5 * fi + 1.25 * t;
        float zn = mod(z, 256.0);

        float scale = nearScale * pow(farScale / nearScale, fi) * pulse;
        vec2 q = p * scale;

        q *= rot(0.35 * fi + 0.2 * sin(t + 9.0 * fi));
        q += 0.18 * vec2(sin(z + 2.0 * fi), cos(1.3 * z - fi));

        vec3 pp = vec3(q * 1.1, zn);

        // Rotations keep lengths, so one pixel spans 2 / R.y * scale * 1.1 noise cells
        float cellsPerPixel = 2.2 * scale / R.y;
        float f1 = fbm(pp, 5, cellsPerPixel);
        float f2 = fbm(pp * 2.0 + vec3(2.0, -1.0, 0.7), 3, cellsPerPixel * 2.0);
        float f = mix(f1, f2, 0.35);

        float bands = sin(6.28318 * (8.0 * f - 1.6 * fi + 0.18 * t));
        float ribbon = pow(max(0.0, 1.0 - abs(bands)), 14.0);

        float cloud = smoothstep(0.42, 0.92, f);
        float shell = exp(-2.5 * abs(length(q) - (0.55 + 0.45 * sin(2.0 * f + 1.7 * fi))));
        float density = 0.95 * ribbon + 0.28 * cloud + 0.22 * shell;

        // 32 layers stand in for the original 56, so each one carries 56 / 32 of the density
        float fade = smoothstep(0.0, 0.08, fi) * (1.0 - smoothstep(0.8, 1.0, fi));
        density *= mix(0.035, 0.11, 1.0 - fi) * 1.75 * fade;

        vec3 c = palette(f + 0.13 * fi - 0.07 * t);
        c += 0.35 * palette(0.7 * f - 0.11 * t + 0.4) * shell;
        c += 0.25 * vec3(1.0, 0.3, 0.8) * ribbon;

        col += c * density * trans;
        glow += density * (0.6 + 0.4 * cloud);

        trans *= 1.0 - clamp(density * 0.85, 0.0, 0.92);

        if (trans < 0.01)
        {
            break;
        }
    }

    col += bg * trans;
    col += 0.12 * glow * palette(0.12 * length(uv) - 0.15 * t);

    float vignette = max(0.0, 1.0 - 0.16 * dot(uv, uv));
    col *= vignette;

    col = 1.0 - exp(-1.4 * col);
    col = pow(col, vec3(0.85));

    O = vec4(col, 1.0);
}
