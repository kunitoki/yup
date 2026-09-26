#version 450

// GLSL 450 vertex shader: expands each vertex into a clip-space quad corner.
layout(location = 0) in vec2 aCenter;
layout(location = 1) in vec2 aOffset;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aSize;

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vOffset;

void main() {
    gl_Position = vec4(aCenter + aOffset * aSize, 0.0, 1.0);
    vColor = aColor;
    vOffset = aOffset;
}
