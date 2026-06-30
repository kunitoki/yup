#pragma once

#include "resolve_atlas.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char resolve_atlas[] = R"===(#ifdef BB
C1(JF,c0,G,v,S){g O;O.x=(v!=2)?-1.:3.;O.y=(v!=1)?-1.:3.;O.zw=c(.0,1.);D1(O);}
#endif
#ifdef EB
e ivec2 xd(){return ivec2(floor(gl_FragCoord));}
#ifdef KD
layout(location=0)inout Q n0;layout(location=1)out i g4;void main(){g4.x=uintBitsToFloat(n0.x);}
#elif defined(LD)
#ifdef QD
__pixel_local_outEXT n1{layout(r32f)float n0;};
#else
__pixel_local_inEXT n1{layout(r32f)float n0;};layout(location=0)out i g4;
#endif
void main(){
#ifdef QD
n0=.0;
#else
g4.x=n0;
#endif
}
#elif defined(EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui)uniform highp upixelLocalANGLE n0;layout(location=0)out i g4;void main(){g4.x=uintBitsToFloat(pixelLocalLoadANGLE(n0).x);}
#elif defined(MD)
layout(binding=0,r32i)uniform highp coherent iimage2D V8;layout(location=0)out i g4;void main(){g4.x=float(imageLoad(V8,xd()).x)*(1./zc);}
#elif defined(GE)
U2(X2,0,JE);layout(location=0)out i g4;void main(){i J=F1(JE,xd());g4.x=(J.x-J.y)*ka+(J.z-J.w)*255.;}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive