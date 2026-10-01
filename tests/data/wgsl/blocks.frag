#version 450
// The fragment side of blocks.vert, plus the sample builtins
layout(location = 0) in VertexData {
    vec2 uv;
    flat int id;
    layout(location = 3) centroid vec3 normal;
    vec4 colors[2];
} vin;
layout(location = 0) out vec4 color;
void main() {
    color = vec4(vin.uv, float(vin.id), 1.0) + vin.colors[0] + vin.colors[1] + vec4(vin.normal, 0.0);
    color.x += float(gl_SampleID);
    gl_SampleMask[0] = gl_SampleMaskIn[0];
}
