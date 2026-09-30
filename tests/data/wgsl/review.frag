#version 450
// Corner cases from the transpiler review: scoping, typing and evaluation order
layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2DShadow shadowMap;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

struct Hit { float t; };

int counter;

uint hash(uint x) { return x ^ 0x9E3779B9; }
Hit trace(out float d) { d = 1.0; return Hit(2.0); }
void fill(out vec4 c) { c.rgb = vec3(0.5); c.a = 1.0; }
void accumulate(inout float a, float b) { a += b; }
int next() { counter++; return counter & 1; }

float march()
{
    float step = 0.1;
    float length = 2.0;
    return step * length;
}

void main()
{
    const int N = 4;
    float weights[N];
    for (int i = 0; i < N; ++i)
        weights[i] = float(i);

    int i = 0;
    do { int i = 10; } while (++i < 3);

    float d;
    Hit h = trace(d);

    vec4 slots[2];
    slots[next()].xy = vec2(1.0);
    slots[next()].zw++;

    vec4 c;
    fill(c);

    float x = 1.0;
    accumulate(x, x++);

    uvec4 bits = uvec4(hash(42));
    bits = (bits & 0xFFu) >> 2u;
    bits <<= 1u;

    int lod = 1;
    float a = 2.0;
    vec2 v = a.xx;

    float g = textureGatherOffset(shadowMap, uv, 0.5, ivec2(1, 0)).x;
    vec4 t = textureLod(tex, uv, lod) + texture(tex, uv, lod);

    uint m = 0xFFFFFFFF;
    int big = 3000000000;

    switch (i)
    {
        case N: x = 2.0; break;
        default: break;
    }

    fragColor = t + c + slots[0] + vec4(h.t + d + x + weights[1] + march() + g + float(bits.x + m) + float(big), v, step(0.5, a));
}
