#version 450

// One direction of a separable Gaussian blur, run twice for the bloom.
layout(set = 0, binding = 0) uniform texture2D u_tex;
layout(set = 0, binding = 1) uniform sampler u_samp;
layout(set = 0, binding = 2) uniform Params
{
    float sigma;
    float radius;
    float width;
    float height;
    float directionX;
    float directionY;
    float pad0;
    float pad1;
} p;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec2 size = vec2(p.width, p.height);
    vec2 uv = gl_FragCoord.xy / size;
    vec2 stepSize = vec2(p.directionX, p.directionY) / size;
    float inverse = 0.5 / (p.sigma * p.sigma);
    int radius = int(clamp(p.radius, 1.0, 64.0));

    vec4 sum = texture(sampler2D(u_tex, u_samp), uv);
    float weights = 1.0;

    for (int i = 1; i <= radius; ++i)
    {
        float weight = exp(-float(i * i) * inverse);
        vec2 offset = stepSize * float(i);
        sum += (texture(sampler2D(u_tex, u_samp), uv + offset) + texture(sampler2D(u_tex, u_samp), uv - offset)) * weight;
        weights += 2.0 * weight;
    }

    fragColor = sum / weights;
}
