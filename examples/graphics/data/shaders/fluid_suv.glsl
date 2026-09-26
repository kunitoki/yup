// Maps a logical sample coordinate to the texture space of the backend.
vec2 suv(vec2 uv) {
    return vec2(uv.x, mix(uv.y, 1.0 - uv.y, u.flipY));
}
