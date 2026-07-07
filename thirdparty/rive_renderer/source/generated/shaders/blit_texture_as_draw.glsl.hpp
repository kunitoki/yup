#pragma once

#include "blit_texture_as_draw.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char blit_texture_as_draw[] = R"===(h2
#ifdef VC
J0 c0(0,c,U0);
#endif
a2
#ifdef CB
R3 S3 A4 B4 A1(a0)B1 C1(WE,a0,G,v,T){c l2;l2.x=(v&1)==0?-1.:1.;l2.y=(v&2)==0?-1.:1.;
#ifdef VC
Y(U0,c);U0.x=l2.x*.5+.5;U0.y=l2.y*-.5+.5;k0(U0);
#endif
g N=g(l2,0,1);D1(N);}
#endif
#ifdef FB
B3
#ifdef GD
Te(Z4,T3,BC);
#else
X2(Z4,T3,BC);
#endif
C3
#ifdef VC
a5 U3(Ue)c5
#endif
Y2(i,DE){i l8;
#ifdef VC
B(U0,c);l8=Q6(BC,Ue,U0,.0);
#elif defined(GD)
l8=(m8(BC,0,U(floor(S.xy)))+m8(BC,1,U(floor(S.xy)))+m8(BC,2,U(floor(S.xy)))+m8(BC,3,U(floor(S.xy))))*0.25;
#else
l8=v1(BC,U(floor(S.xy)));
#endif
G2(l8);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive