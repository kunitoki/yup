#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec4 v;
const float kTiny = 2.3283064365386963e-10;
const float kPi = 3.14159265359;
const int kArr[3] = int[](1, 2, 3);
struct S { vec2 a; float b[2]; };
void main() {
    mat4 m4 = mat4(1.0);
    mat3 m3 = mat3(m4);
    mat4 m4b = mat4(m3);
    mat2 m2 = mat2(2.0);
    mat2x3 m23 = mat2x3(v.xyz, v.yzw);
    vec3 t3 = vec3(v);
    vec2 t2 = vec2(v.xyz);
    float f = float(v);
    int i = int(v.x);
    ivec2 iv = ivec2(v.xy);
    bool b = bool(i);
    S s = S(vec2(1.0), float[2](1.0, 2.0));
    float arr[] = float[](1.0, 2.0, 3.0);
    float agg[2] = { 1.0, 2.0 };
    vec3 m = mod(t3, 2.0) + vec3(mod(f, 3.0));
    fragColor = vec4(m3[0] + m4b[1].xyz + m23[1] + m, m2[0].x + kTiny + kPi + float(kArr[1]) + s.b[1] + float(arr.length()) + agg[1] + (b ? 1.0 : 0.0) + float(iv.x));
}
