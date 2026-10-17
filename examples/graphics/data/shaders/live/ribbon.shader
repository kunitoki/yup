// Ribbon Clouds by Abhishek Shivakumar (@godofecht).
// Copyright (c) 2026 Abhishek Shivakumar. Distributed under the ISC license above.

#define STEPS 20
#define OCTAVES 3

#define TAU 6.28318530718

float hash3(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float noise3(vec3 x)
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f * f * (3.0 - 2.0 * f);

    float a = hash3(p);
    float b = hash3(p + vec3(1, 0, 0));
    float c = hash3(p + vec3(0, 1, 0));
    float d = hash3(p + vec3(1, 1, 0));

    float e = hash3(p + vec3(0, 0, 1));
    float g = hash3(p + vec3(1, 0, 1));
    float h = hash3(p + vec3(0, 1, 1));
    float j = hash3(p + vec3(1, 1, 1));

    return mix(
        mix(mix(a, b, f.x), mix(c, d, f.x), f.y),
        mix(mix(e, g, f.x), mix(h, j, f.x), f.y),
        f.z
    );
}

float fbm(vec3 p, float footprint)
{
    float s = 0.0;
    float total = 0.0;
    float a = 0.5;

    for (int i = 0; i < OCTAVES; ++i)
    {
        float visibility =
            1.0 - smoothstep(0.25, 0.85, footprint);

        if (visibility <= 0.001)
            break;

        float w = a * visibility;

        s += w * noise3(p);
        total += w;

        p *= 2.02;
        footprint *= 2.02;
        a *= 0.5;
    }

    float n = 0.97 * s / max(total, 0.0001);

    return mix(0.48, n, clamp(total * 2.0, 0.0, 1.0));
}

mat2 rot(float a)
{
    float c = cos(a);
    float s = sin(a);

    return mat2(c, -s, s, c);
}

vec3 palette(float t)
{
    return 0.55 + 0.45 *
        cos(TAU * (vec3(0.00, 0.12, 0.24) + t));
}

// One damped elastic impulse.
float spring(float dt)
{
    float onset = smoothstep(0.0, 0.4, dt);

    return onset *
        exp(-0.65 * dt) *
        sin(10.0 * dt);
}

// 0 -> 40 -> 0, repeating every 80 seconds.
// Elastic motion occurs after each turning point.
float oscillatingTime(float realTime)
{
    const float halfPeriod = 40.0;
    const float period = 80.0;
    const float elasticity = 0.055;

    float cycle = mod(realTime, period);

    // Sinusoidal position
    float phase = TAU * cycle / period;
    float wave = 0.5 - 0.5 * cos(phase);

    float bottomBounce = spring(cycle);

    float peakAge = max(cycle - halfPeriod, 0.0);
    float topBounce = spring(peakAge);

    wave += elasticity * (bottomBounce - topBounce);

    wave = clamp(wave, 0.0, 1.0);

    return halfPeriod * wave;
}

void mainImage(out vec4 O, vec2 U)
{
    vec2 R = iResolution.xy;
    vec2 uv = (2.0 * U - R) / R.y;

    float t = oscillatingTime(iTime);

    vec2 p = rot(0.15 * sin(0.35 * t)) * uv;

    float zoom = exp(0.20 * t);
    float pulse = 1.0 + 0.12 * sin(1.3 * t);

    vec3 col = vec3(0.0);
    float trans = 1.0;
    float glow = 0.0;

    vec3 bg = vec3(0.01, 0.005, 0.02);

    bg += 0.04 *
        palette(0.1 * length(uv) - 0.05 * t);

    for (int i = 0; i < STEPS; ++i)
    {
        float fi = float(i) / float(STEPS);
        float depth = fi * fi;

        float z = 2.5 * fi + 1.25 * t;

        float scale =
            mix(0.6, 5.0, depth) * zoom * pulse;

        float angle =
            0.35 * fi +
            0.2 * sin(t + 9.0 * fi);

        vec2 q = rot(angle) * (p * scale);

        q += 0.18 * vec2(
            sin(z + 2.0 * fi),
            cos(1.3 * z - fi)
        );


        float footprint = 2.2 * scale / R.y;
        float detailReduction =
            max(1.0, footprint / 0.20);

        vec3 pp = vec3(
            q * (1.1 / detailReduction),
            z
        );

        float filteredFootprint =
            footprint / detailReduction;


        float f = fbm(pp, filteredFootprint);

        if (filteredFootprint < 0.3)
        {
            float secondary = noise3(
                pp * 1.9 + vec3(2.0, -1.0, 0.7)
            );

            f = mix(f, secondary * 0.97, 0.22);
        }

        float bands = sin(
            TAU * (8.0 * f - 1.6 * fi + 0.18 * t)
        );

        float ribbon = pow(
            max(0.0, 1.0 - abs(bands)),
            14.0
        );

        float cloud = smoothstep(0.42, 0.92, f);

        float shell = exp(
            -2.5 * abs(
                length(q) -
                (0.55 + 0.45 *
                    sin(2.0 * f + 1.7 * fi))
            )
        );

        float density =
            0.95 * ribbon +
            0.28 * cloud +
            0.22 * shell;

        density *=
            mix(0.035, 0.11, 1.0 - fi) *
            (56.0 / float(STEPS));

        vec3 c = palette(
            f + 0.13 * fi - 0.07 * t
        );

        c += 0.35 *
            palette(0.7 * f - 0.11 * t + 0.4) *
            shell;

        c += 0.25 *
            vec3(1.0, 0.3, 0.8) *
            ribbon;

        col += c * density * trans;

        glow += density * (0.6 + 0.4 * cloud);

        trans *= 1.0 -
            clamp(density * 0.85, 0.0, 0.92);

        if (trans < 0.01)
            break;
    }

    col += bg * trans;

    col += 0.12 * glow *
        palette(
            0.12 * length(uv) - 0.15 * t
        );

    float spark = hash3(
        vec3(floor(U * 0.5), 0.0)
    );

    float twinkle =
        0.5 + 0.5 * sin(6.0 * t + TAU * spark);

    col +=
        0.18 *
        smoothstep(0.995, 1.0, spark) *
        twinkle *
        palette(length(uv) + 0.2 * t);

    float vignette =
        max(0.0, 1.0 - 0.16 * dot(uv, uv));

    col *= vignette;

    col = 1.0 - exp(-1.4 * max(col, vec3(0.0)));
    col = pow(col, vec3(0.85));

    O = vec4(col, 1.0);
}
