#version 450
layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 blend;
layout(location = 0) flat in uint id;
layout(location = 1) noperspective centroid in vec2 uv;
void main() { color = vec4(uv, 0.0, 1.0); blend = vec4(float(id)); }
