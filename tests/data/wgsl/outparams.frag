#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 uv;
vec3 gAccum;
float arr[4];
void setOne(out float x) { x = 1.0; }
void addTo(inout vec3 v, float k) { v += vec3(k); v.x = v.y; }
void swap(inout float a, inout float b) { float t = a; a = b; b = t; }
float twice(float v) { v *= 2.0; return v; }
void forward(inout vec3 v) { addTo(v, 1.0); }
void main() {
    vec4 c = vec4(0.0);
    setOne(c.y);
    setOne(arr[2]);
    addTo(gAccum, 2.0);
    forward(gAccum);
    float a = 1.0, b = 2.0;
    swap(a, b);
    int i = 0;
    arr[i++] = 3.0;
    float t = (a = 4.0, a + b);
    c.xy *= 2.0;
    c.zx = vec2(a, b);
    c.rgb += gAccum;
    fragColor = c + vec4(twice(t), arr[1], float(i), fragColor.x);
}
