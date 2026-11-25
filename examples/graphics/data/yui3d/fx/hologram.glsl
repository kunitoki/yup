// A hologram: light where the shape turns away from the eye, thin lines climbing through it and, now and then,
// a wider band. The alpha it returns is blended by the Material3D, so what is behind shows through.
//
// A material is a function, not a whole shader: the renderer calls it for each pixel of the mesh with what it
// knows of the surface, in world space, and shows the linear color it returns without lighting it.

YUP_MATERIAL_PARAMS
{
    vec4 tint;      // property tint: color, given linear
    float flow;     // property flow: number, how fast the lines climb
    float amount;   // property amount: number, how close the lines are
    float energy;   // property energy: number, the brightness
    float time;     // no property has that name: the seconds the material has been shown for
} params;

vec4 yupMaterial (YupSurface surface)
{
    float rim = pow (1.0 - max (dot (surface.normal, surface.view), 0.0), 2.5);

    // The lines follow the height in the world, so they stay level while the object turns
    float climb = params.time * params.flow;
    float lines = smoothstep (0.35, 1.0, 0.5 + 0.5 * sin (surface.position.y * (20.0 + params.amount * 80.0) - climb * 6.0));
    float band = smoothstep (0.92, 1.0, sin (surface.position.y * 2.0 - climb * 1.5));

    float glow = 0.06 + rim * 1.6 + lines * 0.35 + band * 0.8;
    float alpha = clamp (0.05 + rim + lines * 0.3 + band * 0.5, 0.0, 1.0);

    return vec4 (params.tint.rgb * glow * params.energy, alpha);
}
