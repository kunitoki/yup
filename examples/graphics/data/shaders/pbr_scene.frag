#version 450

layout(location = 0) in vec3 v_worldPosition;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec3 v_cameraPosition;
layout(location = 4) in vec4 v_material;
layout(location = 5) in vec4 v_tint;
layout(location = 6) in vec4 v_extra;

layout(set = 0, binding = 0) uniform Scene {
    vec4 camera;   // yaw, pitch, distance, aspect
    vec4 material; // exposure, sunIntensity, prefilterMaxLod, backdropRadius (0 = sphere grid)
    vec4 light;    // sun direction xyz
} u;

layout(set = 0, binding = 1) uniform textureCube u_irradiance;
layout(set = 0, binding = 2) uniform textureCube u_prefilter;
layout(set = 0, binding = 3) uniform texture2D   u_brdf;
layout(set = 0, binding = 4) uniform texture2D   u_albedo;
layout(set = 0, binding = 5) uniform texture2D   u_normalMap;
layout(set = 0, binding = 6) uniform sampler     u_samp;
layout(set = 0, binding = 7) uniform texture2D   u_rma;
layout(set = 0, binding = 8) uniform sampler     u_sampRepeat;

layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

float distributionGGX(float nDotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float d = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.0001);
}

float geometrySmithDirect(float nDotV, float nDotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float ggxV = nDotV / (nDotV * (1.0 - k) + k);
    float ggxL = nDotL / (nDotL * (1.0 - k) + k);
    return ggxV * ggxL;
}

vec3 acesTonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    float metallic = clamp(v_material.x, 0.0, 1.0);
    float baseRoughness = clamp(v_material.y, 0.06, 1.0);
    float mapAmount = v_tint.a;
    float aoScale = v_extra.z;

    // Analytic tangent frame for a unit sphere, perturbed by the uploaded
    // normal map.
    vec3 geometricNormal = normalize(v_normal);

    // cross(Y, N) collapses to zero at the poles, and normalizing that yields NaN
    // shading normals in a band around them - pick a different reference axis there.
    vec3 reference = abs(geometricNormal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(reference, geometricNormal));
    vec3 bitangent = cross(geometricNormal, tangent);

    vec2 uv = v_uv * max(v_material.z, 0.25);
    vec3 tangentNormal = texture(sampler2D(u_normalMap, u_sampRepeat), uv).xyz * 2.0 - 1.0;
    tangentNormal.xy *= v_material.w;

    vec3 n = normalize(mat3(tangent, bitangent, geometricNormal) * tangentNormal);
    vec3 v = normalize(v_cameraPosition - v_worldPosition);
    vec3 r = reflect(-v, n);

    vec3 albedoTex = pow(texture(sampler2D(u_albedo, u_sampRepeat), uv).rgb, vec3(2.2));
    vec3 albedo = albedoTex * v_tint.rgb;

    vec3 rma = texture(sampler2D(u_rma, u_sampRepeat), uv).rgb;
    float roughness = clamp(mix(baseRoughness, baseRoughness * (rma.g / 0.5), mapAmount), 0.06, 1.0);
    float ao = mix(1.0, rma.b, aoScale);

    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    float nDotV = max(dot(n, v), 0.0001);

    // Direct sun contribution.
    vec3 l = normalize(u.light.xyz);
    vec3 h = normalize(v + l);
    float nDotL = max(dot(n, l), 0.0);
    float nDotH = max(dot(n, h), 0.0);
    float vDotH = max(dot(v, h), 0.0);

    vec3 fresnel = f0 + (vec3(1.0) - f0) * pow(1.0 - vDotH, 5.0);
    float ndf = distributionGGX(nDotH, roughness);
    float geometry = geometrySmithDirect(nDotV, nDotL, roughness);

    vec3 specularDirect = (ndf * geometry * fresnel) / max(4.0 * nDotV * nDotL, 0.0001);
    vec3 diffuseDirect = (vec3(1.0) - fresnel) * (1.0 - metallic) * albedo / PI;
    vec3 direct = (diffuseDirect + specularDirect) * nDotL * u.material.y;

    // Image-based ambient: diffuse irradiance plus the split-sum specular term.
    vec3 fresnelIbl = f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - nDotV, 5.0);
    vec3 kD = (vec3(1.0) - fresnelIbl) * (1.0 - metallic);

    vec3 irradiance = textureLod(samplerCube(u_irradiance, u_samp), n, 0.0).rgb;
    vec3 diffuseIbl = irradiance * albedo;

    // The prefilter chain is read by explicit LOD: a mip-narrowed texture view
    // would silently degrade to mip 0 on OpenGL.
    vec3 prefiltered = textureLod(samplerCube(u_prefilter, u_samp), r, roughness * u.material.z).rgb;
    vec2 brdf = texture(sampler2D(u_brdf, u_samp), vec2(nDotV, roughness)).rg;
    vec3 specularIbl = prefiltered * (fresnelIbl * brdf.x + brdf.y);

    // Clearcoat: a glossy dielectric layer whose own, much smoother roughness
    // lobe replaces the base specular reflection as the coat builds up.
    float clearcoat = v_extra.x;
    if (clearcoat > 0.001)
    {
        float ccRoughness = clamp(v_extra.y, 0.03, 0.5);
        float fcc = 0.04 + 0.96 * pow(1.0 - nDotV, 5.0);
        vec2 ccBrdf = texture(sampler2D(u_brdf, u_samp), vec2(nDotV, ccRoughness)).rg;
        vec3 ccPrefiltered = textureLod(samplerCube(u_prefilter, u_samp), r, ccRoughness * u.material.z).rgb;
        vec3 ccSpecular = ccPrefiltered * (fcc * ccBrdf.x + ccBrdf.y);
        specularIbl = mix(specularIbl, ccSpecular, clearcoat);
    }

    vec3 color = ao * (kD * diffuseIbl + specularIbl) + direct;

    color = acesTonemap(color * u.material.x);
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
