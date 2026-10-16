void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 p = (2.0 * fragCoord - iResolution.xy) / iResolution.y;
    float t = iTime * 0.6;

    float v = sin(p.x * 3.0 + t);
    v += sin(3.0 * (p.x * sin(t * 0.5) + p.y * cos(t * 0.3)) + t);
    v += sin(length(p * 4.0 + vec2(sin(t), cos(t * 1.3))) * 2.0 - t);
    v *= 0.5;

    vec3 col = 0.5 + 0.5 * cos(3.14159 * v + vec3(0.0, 2.1, 4.2));
    fragColor = vec4(col, 1.0);
}
