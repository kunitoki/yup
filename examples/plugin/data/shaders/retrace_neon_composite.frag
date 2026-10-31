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
// - grain: static noise that breaks banding and gives the surfaces some texture;
// - reflection: the glass in front of the picture catching the room, a soft window
//   highlight up left, a sheen where it curves away at the top and toward its edges;
// - opening: the tube powering on, a dot widening into a line that opens into the
//   picture, from 0 when off to 1 when fully on.
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
    float opening;
    float reflection;
    float pad1;
    float pad2;
} p;

layout(set = 0, binding = 1) uniform texture2D u_interface;
layout(set = 0, binding = 2) uniform sampler u_interfaceSampler;
layout(set = 0, binding = 3) uniform texture2D u_glow;
layout(set = 0, binding = 4) uniform sampler u_glowSampler;

layout(location = 0) out vec4 fragColor;

const vec3 offColor = vec3(0.0859, 0.0937, 0.1328);
const float squashSpeed = 1.5;

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

    // Powering on runs a tube switching off backwards: the picture, squashed into a bright
    // line one pixel tall, first fades in while the line widens from the center, then
    // opens vertically as it dims back to its own colors.
    float onePixel = (p.height - 1.0) / p.height;
    float time = (1.0 - clamp(p.opening, 0.0, 1.0)) * (1.0 / squashSpeed + 1.0);
    float squash = clamp(time * squashSpeed, 0.0, onePixel);
    float fade = clamp(time - onePixel / squashSpeed, 0.0, 1.0);
    vec2 source = vec2(centered.x * (1.0 - squash), centered.y / (1.0 - squash)) + 0.5;

    vec2 shift = centered * dot(centered, centered) * 4.0 * p.aberration / size;
    vec3 color = vec3(fetch(source + shift).r, fetch(source).g, fetch(source - shift).b);

    vec3 glow = texture(sampler2D(u_glow, u_glowSampler), source).rgb * p.glow;
    color = 1.0 - (1.0 - color) * (1.0 - clamp(glow, 0.0, 1.0));

    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luminance), color, 1.0 + 0.3 * p.grade);
    color += p.grade * (1.0 - luminance) * (1.0 - luminance) * vec3(0.07, 0.0, 0.12);

    if (p.opening < 1.0)
    {
        // A quarter pixel of margin keeps the line lit on its middle rows, which the curve pushes out.
        float lit = step(abs(centered.y), (1.0 - squash) * 0.5 + 0.25 / p.height) * step(abs(centered.x), 1.0 - fade);
        color = mix(offColor, mix(offColor, color + squash, 1.0 - fade), lit);
    }

    vec2 point = gl_FragCoord.xy / max(p.pixelScale, 1.0);

    color *= 1.0 - p.scanlines * (0.5 - 0.5 * cos(point.y * 6.2831853 / 3.0));

    int column = int(mod(floor(point.x), 3.0));
    vec3 stripe = vec3(column == 0 ? 1.0 : 0.0, column == 1 ? 1.0 : 0.0, column == 2 ? 1.0 : 0.0);
    color *= mix(vec3(1.0), 0.6 + 0.8 * stripe, p.phosphor);

    vec2 aspect = centered * vec2(p.width / p.height, 1.0);
    color *= 1.0 - p.vignette * smoothstep(0.35, 1.05, length(aspect));

    // Screened over the picture, so the glass shows on dark areas and never clips the bright ones.
    if (p.reflection > 0.0)
    {
        vec2 window = screen - vec2(-0.5, -0.7);
        vec2 along = vec2(window.x + window.y, window.y - window.x) * 0.7071 * vec2(1.3, 3.5);
        float highlight = exp(-dot(along, along));

        float top = smoothstep(0.82, 0.98, -screen.y) * (1.0 - smoothstep(0.98, 1.0, -screen.y)) * (1.0 - screen.x * screen.x);
        float sheen = pow(clamp(length(screen) / 1.4142, 0.0, 1.0), 3.0);

        float glass = clamp((highlight + 0.7 * top + 0.5 * sheen) * p.reflection, 0.0, 1.0);
        color = 1.0 - (1.0 - color) * (1.0 - glass * vec3(0.85, 0.9, 1.0));
    }

    float noise = fract(sin(dot(floor(point), vec2(12.9898, 78.233))) * 43758.5453) - 0.5;
    color += noise * p.grain;

    fragColor = vec4(clamp(color, 0.0, 1.0) * inside, 1.0);
}
