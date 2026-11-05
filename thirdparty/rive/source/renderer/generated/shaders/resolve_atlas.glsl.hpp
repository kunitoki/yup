#pragma once

#include "resolve_atlas.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char resolve_atlas[] = R"===(#ifdef CB
n1(GG,d0,A,C,r){e I;I.x=(C!=2)?-1.:3.;I.y=(C!=1)?-1.:3.;I.zw=c(.0,1.);C1(I);}
#endif
#ifdef EB
f ivec2 zf(){return ivec2(floor(gl_FragCoord));}
#ifdef DE
layout(location=0) inout R x0;layout(location=1) out i U4;void main(){U4.x=uintBitsToFloat(x0.x);}
#elif defined(EE)
#ifdef LE
__pixel_local_outEXT V1{layout(r32f) float x0;};
#else
__pixel_local_inEXT V1{layout(r32f) float x0;};layout(location=0) out i U4;
#endif
void main(){
#ifdef LE
x0=.0;
#else
U4.x=x0;
#endif
}
#elif defined(EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui) uniform highp upixelLocalANGLE x0;layout(location=0) out i U4;void main(){U4.x=uintBitsToFloat(pixelLocalLoadANGLE(x0).x);}
#elif defined(FE)
layout(binding=0,r32i) uniform highp coherent iimage2D fa;layout(location=0) out i U4;void main(){U4.x=float(imageLoad(fa,zf()).x)*(1./we);}
#elif defined(HF)
Z2(O2,0,KF);layout(location=0) out i U4;void main(){i T=j1(KF,zf());U4.x=(T.x-T.y)*Nb+(T.z-T.w)*255.;}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive