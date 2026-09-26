// Packs a scalar field into the 8-bit channels of a surface.
vec3 encodeScalar(float v) {
    float t = clamp((v + 2048.0) * (1.0 / 4096.0), 0.0, 1.0);
    float x = floor(t * 16777215.0 + 0.5);
    float b0 = mod(x, 256.0);
    float b1 = mod(floor(x / 256.0), 256.0);
    float b2 = floor(x / 65536.0);
    return vec3(b0, b1, b2) / 255.0;
}
float decodeScalar(vec3 c) {
    vec3 b = floor(c * vec3(255.0) + vec3(0.5));
    float x = b.x + b.y * 256.0 + b.z * 65536.0;
    return x * (4096.0 / 16777215.0) - 2048.0;
}
