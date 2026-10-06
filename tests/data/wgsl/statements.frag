#version 450
// Switch fallthrough over nested statements, swizzle compound stores and statement-level effects
layout(location = 0) out vec4 fragColor;
layout(location = 0) flat in int mode;
layout(location = 1) in vec4 tint;
int counter;
bool bump() { counter++; return counter > 2; }
void main() {
    float r = 0.0;
    switch (mode) {
        case 0:
            if (counter > 1) { r = 1.0; } else { r = 2.0; }
            while (r < 4.0) { r += 1.0; }
            do { r -= 0.5; } while (r > 3.0);
            for (int i = 0; i < 2; i++) { r += float(i); }
            switch (counter) { case 1: r += 1.0; break; default: break; }
        case 1: {
            float weights[2] = float[2](0.25, 0.75);
            r += weights[1];
            break;
        }
        default:
            r = -1.0;
    }
    vec4 v = tint;
    ivec4 bits = ivec4(mode);
    v.st = v.pq;
    v.xy -= vec2(1.0);
    v.zw /= vec2(2.0);
    bits.xy %= ivec2(3);
    bits.zw <<= ivec2(1);
    bits.xy >>= ivec2(1);
    bits.yz &= ivec2(7);
    bits.xw ^= ivec2(5);
    bits.zy |= ivec2(8);
    mode > 1 ? counter++ : counter--;
    (mode > 2) && bump();
    (mode < 0) || bump();
    for (int i = 0; (counter += 1) < 6; mode > 1 ? i++ : i--) { r += 1.0; }
    fragColor = vec4(r) + v + vec4(bits);
}
