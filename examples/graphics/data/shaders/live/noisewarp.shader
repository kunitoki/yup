float fbm(vec2 p)
{
    float v = 0.0;
    float a = 0.5;

    for (int i = 0; i < 5; ++i)
    {
        v += a * texture(iChannel0, p / iChannelResolution[0].xy).x;
        p = p * 2.03 + vec2(17.0, 31.0);
        a *= 0.5;
    }

    return v;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 p = fragCoord / iResolution.y * 64.0;
    float t = iTime * 4.0;

    vec2 q = vec2(fbm(p + vec2(0.0, t)), fbm(p + vec2(52.0, -t)));
    vec2 r = vec2(fbm(p + 64.0 * q + vec2(17.0, 9.2) + 0.5 * t),
                  fbm(p + 64.0 * q + vec2(83.0, 28.0) - 0.3 * t));
    float f = fbm(p + 64.0 * r);

    vec3 col = mix(vec3(0.1, 0.15, 0.3), vec3(0.95, 0.6, 0.25), clamp(f * f * 2.0, 0.0, 1.0));
    col = mix(col, vec3(0.2, 0.7, 0.8), clamp(length(q) - 0.5, 0.0, 1.0));

    fragColor = vec4(col, 1.0);
}
