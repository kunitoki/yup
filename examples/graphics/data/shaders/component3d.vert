#version 450

// Only applies the MVP matrix, so the CPU picking in the MeshSurfaceMapper matches the GPU
// rasterization exactly. Texture coordinates have v pointing down, like the panel.
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(set = 0, binding = 0) uniform Uniforms { mat4 modelViewProjection; } u;
layout(location = 0) out vec2 v_uv;

void main() {
    gl_Position = u.modelViewProjection * vec4(a_position, 1.0);
    v_uv = a_uv;
}
