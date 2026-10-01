#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) flat in int mode;
int counter;
bool bump() { counter++; return counter > 2; }
void main() {
    float r = 0.0;
    switch (mode) {
        case 0:
        case 1: r = 1.0;
        case 2: r += 2.0; break;
        case 3: { r = 3.0; break; }
        default: r = -1.0;
    }
    switch (mode) { case 5u - 5u: r = 7.0; }
    int k = 0;
    do { k++; if (k == 2) continue; r += 1.0; } while (k < 4);
    for (int x = 0, y = 10; x < y; x++, y--) { r += float(x); }
    for (;;) { if (bump()) break; }
    while (bump() && counter < 10) { r -= 1.0; }
    bool z = mode > 2 || bump();
    float sel = mode > 1 ? r : -r;
    float side = mode > 1 ? (r += 1.0) : 0.0;
    r = r > 0.0 ? r : (counter++ > 3 ? 1.0 : 2.0);
    fragColor = vec4(r, sel, side, z ? 1.0 : 0.0);
}
