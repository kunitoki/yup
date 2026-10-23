#version 450

// The synthwave look over the whole interface, each part scaled by its own amount:
// - curvature: the picture bulges like the glass of a tube, inside a bezel with
//   rounded corners; SynthNeonEffect maps the mouse through the same curve;
// - phosphor mask: red, green and blue stripes a point wide, like an aperture grille;
// Sizes are in points, device pixels divided by pixelScale, so every display looks alike.
// - chromatic aberration: red and blue drift apart toward the corners, like a lens;
// - neon glow: the blurred bright colors are screened back over the interface;
// - color grade: shadows lifted into violet and colors pushed a little more saturated;
// - scanlines: fine horizontal lines, one every three points;
// - vignette: corners pulled down;
// - grain: static noise that breaks banding and gives the surfaces some texture.
layout(set = 0, binding = 0) uniform Params
{
    float width;
    float height;
    float aberration;
    float glow;
    float grade;
    float scanlines;
    float vignette;
    float grain;
    float curvature;
    float phosphor;
    float cornerRadius;
    float pixelScale;
} p;

layout(set = 0, binding = 1) uniform texture2D u_interface;
layout(set = 0, binding = 2) uniform sampler u_interfaceSampler;
layout(set = 0, binding = 3) uniform texture2D u_glow;
layout(set = 0, binding = 4) uniform sampler u_glowSampler;

layout(location = 0) out vec4 fragColor;

vec3 fetch(vec2 uv)
{
    return texture(sampler2D(u_interface, u_interfaceSampler), uv).rgb;
}

// The point of the flat picture shown at a point of the curved screen, both in -1 to 1.
vec2 curve(vec2 screen)
{
    return screen * (1.0 + p.curvature * vec2(screen.y * screen.y, screen.x * screen.x));
}

void main()
{
    vec2 size = vec2(p.width, p.height);
    vec2 screen = gl_FragCoord.xy / size * 2.0 - 1.0;
    vec2 picture = curve(screen);

    // Outside the picture, and past the rounded corners of the tube, is the bezel.
    vec2 corner = max(abs(picture) - (1.0 - p.cornerRadius), 0.0) / max(p.cornerRadius, 1.0e-4);
    float inside = (1.0 - smoothstep(0.9, 1.0, length(corner))) * step(max(abs(picture.x), abs(picture.y)), 1.0);

    if (inside <= 0.0)
    {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 uv = picture * 0.5 + 0.5;
    vec2 centered = uv - 0.5;

    vec2 shift = centered * dot(centered, centered) * 4.0 * p.aberration / size;
    vec3 color = vec3(fetch(uv + shift).r, fetch(uv).g, fetch(uv - shift).b);

    vec3 glow = texture(sampler2D(u_glow, u_glowSampler), uv).rgb * p.glow;
    color = 1.0 - (1.0 - color) * (1.0 - clamp(glow, 0.0, 1.0));

    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luminance), color, 1.0 + 0.3 * p.grade);
    color += p.grade * (1.0 - luminance) * (1.0 - luminance) * vec3(0.07, 0.0, 0.12);

    vec2 point = gl_FragCoord.xy / max(p.pixelScale, 1.0);

    color *= 1.0 - p.scanlines * (0.5 - 0.5 * cos(point.y * 6.2831853 / 3.0));

    int column = int(mod(floor(point.x), 3.0));
    vec3 stripe = vec3(column == 0 ? 1.0 : 0.0, column == 1 ? 1.0 : 0.0, column == 2 ? 1.0 : 0.0);
    color *= mix(vec3(1.0), 0.6 + 0.8 * stripe, p.phosphor);

    vec2 aspect = centered * vec2(p.width / p.height, 1.0);
    color *= 1.0 - p.vignette * smoothstep(0.35, 1.05, length(aspect));

    float noise = fract(sin(dot(floor(point), vec2(12.9898, 78.233))) * 43758.5453) - 0.5;
    color += noise * p.grain;

    fragColor = vec4(clamp(color, 0.0, 1.0) * inside, 1.0);
}
