#version 450

// Liquid glass over a live Component: a lens magnifies under the pointer and rings spread from
// where it moved. The sample offset is mirrored on the CPU by RippleEffect::sampleOffset, so the
// pointer reaches the widget drawn under it; keep the two in sync.
layout(set = 0, binding = 0) uniform texture2D u_tex;
layout(set = 0, binding = 1) uniform sampler u_samp;
layout(set = 0, binding = 2) uniform Params
{
    vec4 frame;    // width, height in pixels, unused, number of rings
    vec4 lens;     // center u, v, radius (fraction of the height), strength
    vec4 rings[8]; // center u, v, radius reached (fraction of the height), amplitude
} p;

layout(location = 0) out vec4 fragColor;

const float ringWavenumber = 70.0;
const float ringWidth = 0.05;
const vec3 paleColor = vec3(0.612, 0.761, 1.0);
const vec3 lightDirection = vec3(-0.4924, -0.7385, 0.4616); // from the top left, y down

// The height of the glass: a bump under the lens plus the travelling rings.
float glassHeight(vec2 uv, out vec2 offset)
{
    vec2 aspect = vec2(p.frame.x / p.frame.y, 1.0);
    offset = vec2(0.0);

    vec2 fromLens = (uv - p.lens.xy) * aspect;
    float bump = p.lens.w * exp(-dot(fromLens, fromLens) / (p.lens.z * p.lens.z));
    offset -= (uv - p.lens.xy) * bump;

    float height = bump;
    for (int i = 0; i < 8; ++i)
    {
        if (i >= int(p.frame.w))
            break;

        vec4 ring = p.rings[i];
        vec2 fromRing = (uv - ring.xy) * aspect;
        float reach = length(fromRing);
        float across = reach - ring.z;
        float wave = ring.w * sin(across * ringWavenumber) * exp(-across * across / (ringWidth * ringWidth));

        offset += normalize(fromRing + 1e-5) / aspect * wave;
        height += wave * 8.0;
    }

    return height;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / p.frame.xy;
    vec2 texel = 1.0 / p.frame.xy;

    vec2 offset;
    vec2 unused;
    float height = glassHeight(uv, offset);
    float heightX = glassHeight(uv + vec2(texel.x, 0.0), unused);
    float heightY = glassHeight(uv + vec2(0.0, texel.y), unused);
    vec3 normal = normalize(vec3((height - heightX) * p.frame.y * 0.02, (height - heightY) * p.frame.y * 0.02, 1.0));

    // A touch of dispersion: the blue bends a little more than the rest.
    vec4 color = texture(sampler2D(u_tex, u_samp), uv + offset);
    float blue = texture(sampler2D(u_tex, u_samp), uv + offset * 1.25).b;
    color.b = mix(color.b, blue, 0.8);

    float highlight = pow(max(dot(reflect(-lightDirection, normal), vec3(0.0, 0.0, 1.0)), 0.0), 48.0);
    float shade = 0.88 + 0.24 * dot(normal, lightDirection);

    fragColor = vec4(color.rgb * shade + paleColor * highlight * 0.7 * color.a, color.a);
}
