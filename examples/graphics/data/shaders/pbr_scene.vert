#version 450

// Shared vertex stage for the sky backdrop and the sphere grid. A non-zero
// backdropRadius in the Scene block wraps the same unit-sphere mesh around
// the camera; zero positions the instanced sphere grid.
//
// The projection maps z/w into [0, 1], which every backend treats as a
// monotonically increasing depth, so a single shader drives the depth test
// correctly on all of them.
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

// Per-instance material, fed from a per-instance vertex buffer (locations 3-5).
layout(location = 3) in vec4 a_material; // metallic, roughness, uvScale, normalStrength
layout(location = 4) in vec4 a_tint;     // linear albedo / metal f0 rgb, roughness-map amount
layout(location = 5) in vec4 a_extra;    // clearcoat, clearcoat roughness, ao scale, unused

layout(set = 0, binding = 0) uniform Scene {
    vec4 camera;   // yaw, pitch, distance, aspect
    vec4 material; // exposure, sunIntensity, prefilterMaxLod, backdropRadius (0 = sphere grid)
    vec4 light;    // sun direction xyz
} u;

layout(location = 0) out vec3 v_worldPosition;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec3 v_cameraPosition;
layout(location = 4) out vec4 v_material;
layout(location = 5) out vec4 v_tint;
layout(location = 6) out vec4 v_extra;

const int kColumns = 5;
const int kRows = 3;
const float kSpacing = 2.6;

void main() {
    float yaw = u.camera.x;
    float pitch = u.camera.y;
    float orbitRadius = u.camera.z;
    float aspect = u.camera.w;

    vec3 cameraPosition = vec3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch)) * orbitRadius;

    // A non-zero backdropRadius wraps the same unit-sphere mesh around the
    // camera as the sky backdrop; zero positions the instanced sphere grid.
    // gl_InstanceIndex is always 0 for the backdrop, which is drawn with a
    // single instance.
    float backdropRadius = u.material.w;
    float isBackdrop = step(0.001, backdropRadius);

    int instance = gl_InstanceIndex;
    int column = instance % kColumns;
    int row = instance / kColumns;

    vec2 offset = (vec2(float(column), float(row)) - vec2(float(kColumns - 1), float(kRows - 1)) * 0.5) * kSpacing;
    vec3 gridPosition = a_position + vec3(offset, 0.0);
    vec3 backdropPosition = cameraPosition + a_position * backdropRadius;
    vec3 worldPosition = mix(gridPosition, backdropPosition, isBackdrop);

    vec3 forward = normalize(-cameraPosition);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);

    vec3 toVertex = worldPosition - cameraPosition;
    vec3 viewSpace = vec3(dot(toVertex, right), dot(toVertex, up), dot(toVertex, forward));

    // z/w lands in [0, 1], which every backend treats as monotonically increasing
    // depth. Keep the near plane as far out as the scene allows: a near plane of
    // 0.1 against a far plane of 120 would squeeze the whole grid into a fraction
    // of a percent of the depth range and z-fight.
    float fov = 1.7320508;  // cot(30 degrees) => 60 degree vertical field of view
    float zNear = 1.0;
    float zFar = 120.0;

    gl_Position = vec4(viewSpace.x * fov / aspect,
                       viewSpace.y * fov,
                       zFar * (viewSpace.z - zNear) / (zFar - zNear),
                       viewSpace.z);

    v_worldPosition = worldPosition;
    v_normal = a_normal;
    v_uv = a_uv;
    v_cameraPosition = cameraPosition;

    v_material = a_material;
    v_tint = a_tint;
    v_extra = a_extra;
}
