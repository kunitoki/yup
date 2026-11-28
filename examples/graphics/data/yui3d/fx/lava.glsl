// Lava: a dark crust opening over a glowing melt. The pattern lives in the world and not on the mesh, so an
// object that moves goes through it.

YUP_MATERIAL_PARAMS
{
    vec4 tint;      // property tint: color, the color of the melt, given linear
    float flow;     // property flow: number, how fast the melt moves
    float amount;   // property amount: number, how much of the crust is open
    float energy;   // property energy: number, the brightness
    float time;     // no property has that name: the seconds the material has been shown for
} params;

// A cheap turbulence: three octaves of sines, each bent by the one before
float lavaHeat (vec3 p, float drift)
{
    float heat = 0.0;
    float octave = 1.0;

    for (int i = 0; i < 3; ++i)
    {
        p += 0.6 * sin (p.yzx * 1.7 * octave + drift);
        heat += sin (p.x * 2.3 * octave) * sin (p.y * 2.9 * octave + drift * 0.5) * sin (p.z * 2.1 * octave) / octave;
        octave *= 2.0;
    }

    return 0.5 + 0.5 * heat;
}

vec4 yupMaterial (YupSurface surface)
{
    float heat = lavaHeat (surface.position * 2.2, params.time * params.flow * 0.4);

    // The melt shows where the heat passes what the crust holds back
    float melt = smoothstep (1.0 - params.amount, 1.35 - params.amount, heat);

    // No light reaches a material: one from above, written here, gives the crust its shape
    float shade = 0.35 + 0.65 * max (dot (surface.normal, normalize (vec3 (0.3, 0.9, 0.4))), 0.0);
    vec3 crust = vec3 (0.045, 0.032, 0.03) * shade * (0.6 + 0.4 * heat);

    // The melt keeps its tint, and only its hottest veins turn towards yellow
    vec3 glow = params.tint.rgb * (0.3 + 1.5 * melt * melt) + vec3 (1.0, 0.55, 0.12) * pow (melt, 6.0) * 0.7;

    return vec4 (mix (crust, glow * params.energy, melt), 1.0);
}
