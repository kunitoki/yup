vec2 map(vec3 p)
{
    vec2 res = vec2(p.y + 1.0, 0.0);

    for (int i = 0; i < 3; ++i)
    {
        float a = iTime * 0.7 + float(i) * 2.0944;
        vec3 center = vec3(1.4 * cos(a), 0.3 * sin(iTime * 1.3 + float(i)), 1.4 * sin(a));
        float d = length(p - center) - 0.55;

        if (d < res.x)
            res = vec2(d, float(i + 1));
    }

    return res;
}

vec3 calcNormal(vec3 p)
{
    const vec2 e = vec2(0.001, 0.0);
    return normalize(vec3(map(p + e.xyy).x - map(p - e.xyy).x,
                          map(p + e.yxy).x - map(p - e.yxy).x,
                          map(p + e.yyx).x - map(p - e.yyx).x));
}

float softShadow(vec3 p, vec3 l)
{
    float shadow = 1.0;
    float t = 0.02;

    for (int i = 0; i < 32; ++i)
    {
        float h = map(p + l * t).x;
        shadow = min(shadow, 8.0 * h / t);
        t += clamp(h, 0.02, 0.2);

        if (shadow < 0.001 || t > 6.0)
            break;
    }

    return clamp(shadow, 0.0, 1.0);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = (2.0 * fragCoord - iResolution.xy) / iResolution.y;

    vec3 ro = vec3(0.0, 0.6, 4.0);
    vec3 rd = normalize(vec3(uv.x, uv.y - 0.15, -1.6));

    float t = 0.0;
    vec2 hit = vec2(-1.0);

    for (int i = 0; i < 100; ++i)
    {
        vec2 h = map(ro + rd * t);

        if (h.x < 0.001)
        {
            hit = vec2(t, h.y);
            break;
        }

        t += h.x;

        if (t > 20.0)
            break;
    }

    vec3 sky = mix(vec3(0.6, 0.75, 0.95), vec3(0.15, 0.2, 0.35), uv.y * 0.5 + 0.5);
    vec3 col = sky;

    if (hit.x > 0.0)
    {
        vec3 p = ro + rd * hit.x;
        vec3 n = calcNormal(p);
        vec3 l = normalize(vec3(0.6, 0.8, 0.4));

        float diffuse = max(dot(n, l), 0.0);
        float shadow = softShadow(p, l);
        float specular = pow(max(dot(reflect(-l, n), -rd), 0.0), 32.0);

        vec3 albedo = hit.y < 0.5
                        ? vec3(0.35 + 0.15 * mod(floor(p.x) + floor(p.z), 2.0))
                        : 0.5 + 0.5 * cos(hit.y * 2.0 + vec3(0.0, 2.0, 4.0));

        col = albedo * (0.15 + 0.85 * diffuse * shadow) + specular * shadow * 0.5;
        col = mix(col, sky, 1.0 - exp(-0.02 * hit.x * hit.x));
    }

    fragColor = vec4(pow(col, vec3(0.4545)), 1.0);
}
