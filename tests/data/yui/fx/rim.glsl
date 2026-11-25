// A material lit at its rim, compiled into fx/rim.glsl.raster.ysl and fx/rim.glsl.full.ysl by yup_add_yui_resources
YUP_MATERIAL_PARAMS
{
    vec4 rim;
    float strength;
} params;

vec4 yupMaterial (YupSurface surface)
{
    float fresnel = pow (1.0 - max (dot (surface.normal, surface.view), 0.0), 3.0);
    return vec4 (params.rim.rgb * fresnel, fresnel * params.strength);
}
