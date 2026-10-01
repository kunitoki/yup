#version 450
layout(location = 0) in vec3 match;
layout(location = 1) in vec2 target;
layout(location = 0) out vec2 ref;
layout(location = 1) flat out ivec2 loop;
struct type { float module; vec2 self; };
float mix2(float a, float b) { return a + b; }
float over(float a) { return a; }
float over(vec2 a) { return a.x + a.y; }
float over(int a) { return float(a); }
float unnamed(float, float b) { return b; }
void main() {
    type t = type(1.0, target);
    float let_ = mix2(t.module, over(t.self)) + over(2) + over(1.5) + unnamed(1.0, 2.0);
    ref = t.self * let_;
    loop = ivec2(-2147483648, 0x7fffffff);
    gl_Position = vec4(match, 1.0);
}
