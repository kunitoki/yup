// An iridescent shell: its color depends on the angle it is seen at, as oil on water does, over a body of the
// tint. On a smooth shape the colors run in bands, which move as it turns.

YUP_MATERIAL_PARAMS
{
    vec4 tint;      // property tint: color, the color of the body, given linear
    float flow;     // property flow: number, how fast the colors drift
    float amount;   // property amount: number, how many colors the angle goes through
    float energy;   // property energy: number, the brightness
    float time;     // no property has that name: the seconds the material has been shown for
} params;

vec4 yupMaterial (YupSurface surface)
{
    float facing = max (dot (surface.normal, surface.view), 0.0);
    float rim = pow (1.0 - facing, 3.0);

    // The film is thicker along a grazing ray: each channel comes back brightest at another thickness
    float thickness = facing * (1.0 + params.amount * 4.0) + surface.position.y * 0.6 + params.time * params.flow * 0.15;
    vec3 film = 0.5 + 0.5 * cos (2.0 * yupPi * (thickness + vec3 (0.0, 0.33, 0.67)));

    // No light reaches a material: one from above, written here, gives the faces their shape
    float shade = 0.35 + 0.65 * max (dot (surface.normal, normalize (vec3 (0.3, 0.9, 0.4))), 0.0);

    vec3 shell = mix (params.tint.rgb * 0.35, film, 0.25 + 0.75 * rim) * shade + vec3 (rim * 0.4);

    return vec4 (shell * params.energy, 1.0);
}
