#version 450
#extension GL_GOOGLE_include_directive : require

#include "pbr_bake.glsl"

void main() {
    fragColor = vec4(proceduralSky(faceDirection(u.face, faceUV())), 1.0);
}
