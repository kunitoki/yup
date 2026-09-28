// Packs the velocity field into the 8-bit channels of a surface.
vec4 encodeVel(vec2 v) {
    vec2 t = clamp((v + vec2(1000.0)) * vec2(0.0005), vec2(0.0), vec2(1.0));
    vec2 x = floor(t * vec2(65535.0) + vec2(0.5));
    vec2 hi = floor(x / vec2(256.0));
    vec2 lo = x - hi * vec2(256.0);
    return vec4(lo.x, hi.x, lo.y, hi.y) / 255.0;
}
vec2 decodeVel(vec4 c) {
    vec2 lo = floor(vec2(c.r, c.b) * vec2(255.0) + vec2(0.5));
    vec2 hi = floor(vec2(c.g, c.a) * vec2(255.0) + vec2(0.5));
    vec2 x = hi * vec2(256.0) + lo;
    return x * vec2(2000.0 / 65535.0) - vec2(1000.0);
}
