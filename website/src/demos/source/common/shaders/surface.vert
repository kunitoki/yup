#version 450

// Places a live Component texture on a mesh. The uniform block is only read here: the fragment
// stage receives the lift and the eye through varyings.
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;

layout(set = 0, binding = 0) uniform Uniforms
{
    mat4 viewProjection;
    mat4 model;
    vec4 params; // lift from 0 (flat 2D) to 1 (lit 3D), time in seconds, audio glow and warmth from 0 to 1
    vec4 eye;    // camera position in world space
} u;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_world;
layout(location = 2) out vec3 v_normal;
layout(location = 3) out vec3 v_eye;
layout(location = 4) out float v_lift;
layout(location = 5) out vec2 v_audio;

void main()
{
    vec4 world = u.model * vec4(a_position, 1.0);

    v_uv = a_uv;
    v_world = world.xyz;
    v_normal = mat3(u.model) * a_normal;
    v_eye = u.eye.xyz;
    v_lift = u.params.x;
    v_audio = u.params.zw;

    gl_Position = u.viewProjection * world;
}
