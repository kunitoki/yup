#version 450

// Bloom prefilter: averages four texels of the full size interface into one of the
// smaller target and keeps only what is brighter than the threshold, with a soft knee.
// Colored light glows fully, white and grey surfaces only by whiteGlow, so the accents
// shine without large pale areas such as the keys flooding their surroundings.
layout(set = 0, binding = 0) uniform texture2D u_tex;
layout(set = 0, binding = 1) uniform sampler u_samp;
layout(set = 0, binding = 2) uniform Params
{
    float threshold;
    float knee;
    float width;
    float height;
    float whiteGlow;
    float pad0;
    float pad1;
    float pad2;
} p;

layout(location = 0) out vec4 fragColor;

vec3 fetch(vec2 uv)
{
    return texture(sampler2D(u_tex, u_samp), uv).rgb;
}

void main()
{
    vec2 size = vec2(p.width, p.height);
    vec2 uv = gl_FragCoord.xy / size;
    vec2 offset = 0.25 / size;

    vec3 color = 0.25 * (fetch(uv + vec2(-offset.x, -offset.y)) + fetch(uv + vec2(offset.x, -offset.y))
                         + fetch(uv + vec2(-offset.x, offset.y)) + fetch(uv + vec2(offset.x, offset.y)));

    float brightness = max(color.r, max(color.g, color.b));
    float soft = clamp(brightness - p.threshold + p.knee, 0.0, 2.0 * p.knee);
    soft = soft * soft / (4.0 * p.knee + 1.0e-4);

    float contribution = max(soft, brightness - p.threshold) / max(brightness, 1.0e-4);

    float saturation = (brightness - min(color.r, min(color.g, color.b))) / max(brightness, 1.0e-4);
    contribution *= mix(p.whiteGlow, 1.0, smoothstep(0.1, 0.5, saturation));

    fragColor = vec4(color * contribution, 1.0);
}
