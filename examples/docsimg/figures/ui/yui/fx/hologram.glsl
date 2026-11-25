YUP_MATERIAL_PARAMS
{
    vec4 rim;           // property rim: color, given linear, its alpha as it is
    float strength;     // property strength: number
    float time;         // no property has that name: the seconds the material has been shown for
} params;

vec4 yupMaterial (YupSurface surface)
{
    // Light where the surface turns away from the eye
    float fresnel = pow (1.0 - max (dot (surface.normal, surface.view), 0.0), 3.0);

    // Level lines climbing through the object: they follow the height in the world
    float lines = 0.5 + 0.5 * sin (surface.position.y * 40.0 - params.time * 4.0);

    vec3 glow = params.rim.rgb * (fresnel + 0.35 * lines * lines) * params.strength;

    // A linear color, and the alpha the alpha mode of the Material3D uses
    return vec4 (surface.baseColor.rgb + glow, surface.baseColor.a);
}
