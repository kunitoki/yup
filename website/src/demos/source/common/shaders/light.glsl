// Lighting shared by the 3D shaders of the website demos: a procedural studio in the site
// colors and a glossy shading model. Directions are in world space, y up, toward the viewer +z.

const vec3 fieldColor = vec3(0.024, 0.090, 0.227);
const vec3 blueColor = vec3(0.0, 0.353, 1.0);
const vec3 paleColor = vec3(0.612, 0.761, 1.0);
const vec3 keyLight = vec3(-0.4924, 0.7385, 0.4616); // normalized (-0.8, 1.2, 0.75)

// A dark floor, a soft blue horizon, a pale soft box up and to the left and a thin blue
// strip light on the right, which is what glossy faces reflect.
vec3 environment(vec3 direction)
{
    vec3 color = mix(fieldColor * 0.5, fieldColor * 1.5 + blueColor * 0.06, smoothstep(-0.3, 0.45, direction.y));
    color += paleColor * 1.4 * smoothstep(0.86, 0.97, dot(direction, keyLight));
    color += blueColor * 0.9 * (1.0 - smoothstep(0.0, 0.04, abs(direction.x - 0.82))) * smoothstep(-0.2, 0.3, direction.y);
    return color;
}

// Diffuse light from the soft box, a highlight that tightens with gloss, and a fresnel
// reflection of the studio that grows toward grazing angles.
vec3 shade(vec3 albedo, vec3 normal, vec3 toEye, float gloss)
{
    float diffuse = max(dot(normal, keyLight), 0.0);
    float highlight = pow(max(dot(normal, normalize(keyLight + toEye)), 0.0), mix(8.0, 96.0, gloss)) * gloss;
    float fresnel = 0.04 + 0.96 * pow(1.0 - max(dot(normal, toEye), 0.0), 5.0);

    vec3 lit = albedo * (0.4 + 0.7 * diffuse) + paleColor * highlight;
    return mix(lit, environment(reflect(-toEye, normal)), fresnel * gloss);
}
