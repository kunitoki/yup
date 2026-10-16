void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 m = iMouse.xy == vec2(0.0) ? 0.5 * iResolution.xy : iMouse.xy;
    vec2 toMouse = fragCoord - m;
    float d = length(toMouse) / iResolution.y;

    float wave = sin(d * 60.0 - iTime * 6.0) * exp(-d * 4.0);
    vec2 uv = fragCoord / iResolution.xy + normalize(toMouse + 1e-4) * wave * 0.01;

    vec3 col = 0.5 + 0.5 * cos(6.2831 * (uv.xyx * 1.5 + vec3(0.0, 0.33, 0.67)) + iTime * 0.5);
    col *= 0.75 + 0.25 * wave;

    vec2 g = abs(fract(uv * vec2(16.0, 9.0)) - 0.5);
    col = mix(col, vec3(1.0), smoothstep(0.45, 0.5, max(g.x, g.y)) * 0.35);

    float pressed = iMouse.z > 0.0 ? 1.0 : 0.0;
    col += pressed * 0.4 * smoothstep(0.02, 0.0, abs(d - fract(iTime * 0.5) * 0.6));

    fragColor = vec4(col, 1.0);
}
