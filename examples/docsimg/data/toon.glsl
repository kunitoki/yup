YUP_MATERIAL_PARAMS
{
    vec4 tint;          // multiplies the base color of the material, linear
    vec4 light;         // where the light of the material comes from: a direction in the world
    float steps;        // how many flat tones the surface is shaded in
} params;

vec4 yupMaterial (YupSurface surface)
{
    // Nothing lights the surface: the function shades it, here by how much it faces a direction of its own
    float facing = 0.5 + 0.5 * dot (surface.normal, normalize (params.light.xyz));

    // Flat tones: the first where the surface faces the light, each of the others half as bright as the one before
    float level = clamp (ceil (facing * params.steps), 1.0, params.steps);
    float tone = exp2 (level - params.steps);

    return vec4 (surface.baseColor.rgb * params.tint.rgb * tone, surface.baseColor.a);
}
