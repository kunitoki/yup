#version 450
// Stage IO blocks with member locations and arrays, an unnamed uniform block and a shadowed member
layout(location = 0) in vec3 position;
layout(set = 0, binding = 0) uniform Params { mat4 mvp; float scale; int count; };
layout(location = 0) out VertexData {
    vec2 uv;
    flat int id;
    layout(location = 3) centroid vec3 normal;
    vec4 colors[2];
} vout;
void main() {
    vec3 p = position * scale;
    for (int i = 0; i < count; i++) { p.x += float(i) * scale; }
    {
        float scale = 2.0;
        p.y *= scale;
    }
    vout.uv = p.xy;
    vout.id = gl_VertexIndex;
    vout.normal = normalize(p);
    vout.colors[0] = vec4(1.0);
    vout.colors[1] = vec4(p, 1.0);
    gl_Position = mvp * vec4(p, 1.0);
}
