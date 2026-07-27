#pragma once

#include "resolve_atlas.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char resolve_atlas[] = R"===(#ifdef CB
C1(LF,a0,G,v,T){g N;N.x=(v!=2)?-1.:3.;N.y=(v!=1)?-1.:3.;N.zw=c(.0,1.);D1(N);}
#endif
#ifdef FB
e ivec2 Cd(){return ivec2(floor(gl_FragCoord));}
#ifdef MD
layout(location=0)inout Q m0;layout(location=1)out i h4;void main(){h4.x=uintBitsToFloat(m0.x);}
#elif defined(ND)
#ifdef TD
__pixel_local_outEXT n1{layout(r32f)float m0;};
#else
__pixel_local_inEXT n1{layout(r32f)float m0;};layout(location=0)out i h4;
#endif
void main(){
#ifdef TD
m0=.0;
#else
h4.x=m0;
#endif
}
#elif defined(EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui)uniform highp upixelLocalANGLE m0;layout(location=0)out i h4;void main(){h4.x=uintBitsToFloat(pixelLocalLoadANGLE(m0).x);}
#elif defined(OD)
layout(binding=0,r32i)uniform highp coherent iimage2D V8;layout(location=0)out i h4;void main(){h4.x=float(imageLoad(V8,Cd()).x)*(1./Ec);}
#elif defined(ME)
X2(a3,0,PE);layout(location=0)out i h4;void main(){i J=v1(PE,Cd());h4.x=(J.x-J.y)*ka+(J.z-J.w)*255.;}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive