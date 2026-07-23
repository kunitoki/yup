#pragma once

#include "render_atlas.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char render_atlas[] = R"===(#ifdef CB
A1(a0)p0(0,g,SB);p0(1,g,TB);B1
#endif
h2 J0 c0(0,g,I);a2
#ifdef CB
C1(KF,a0,G,v,T){q0(v,G,SB,g);q0(v,G,TB,g);Y(I,g);g N;uint l0;c i0;if(p9(SB,TB,T,l0,i0,I v3)){Q J4=P0(MB,l0*4u+2u);V p7=uintBitsToFloat(J4.yzw);i0=i0*p7.x+p7.yz;N=o8(i0,k.ed.x,k.ed.y);
#ifdef JC
N.y=-N.y;
#endif
}else{N=g(k.P2,k.P2,k.P2,k.P2);}k0(I);D1(N);}
#endif
#ifdef FB
#ifdef FC
e d v6(g J,bool Ig F3){d n=d8(J g1);if(!Ig)n=-n;return n;}
#endif
#ifdef MD
layout(location=0)inout Q m0;
#ifdef FC
void main(){float n=uintBitsToFloat(m0.x);n+=v6(I,gl_FrontFacing g1);m0.x=floatBitsToUint(n);}
#endif
#ifdef MC
void main(){float n=uintBitsToFloat(m0.x);n=max(n,v4(I));m0.x=floatBitsToUint(n);}
#endif
#elif defined(ND)
__pixel_localEXT n1{layout(r32f)float m0;};
#ifdef FC
void main(){m0+=v6(I,gl_FrontFacing g1);}
#endif
#ifdef MC
void main(){m0=max(m0,v4(I));}
#endif
#elif defined(EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui)uniform highp upixelLocalANGLE m0;
#ifdef FC
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(m0).x);n+=v6(I,gl_FrontFacing g1);pixelLocalStoreANGLE(m0,Q(floatBitsToUint(n)));}
#endif
#ifdef MC
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(m0).x);n=max(n,v4(I));pixelLocalStoreANGLE(m0,Q(floatBitsToUint(n)));}
#endif
#elif defined(OD)
layout(binding=0,r32i)uniform highp coherent iimage2D V8;ivec2 Ad(){return ivec2(floor(S));}int Bd(float n){return int(n*Ec);}
#ifdef FC
void main(){int n=Bd(v6(I,gl_FrontFacing g1));imageAtomicAdd(V8,Ad(),n);}
#endif
#ifdef MC
void main(){int n=Bd(v4(I));imageAtomicMax(V8,Ad(),n);}
#endif
#elif defined(ME)
#ifdef FC
q6(i,NE){B(I,g);d n=v6(I,r6 g1);if(abs(n)>qf-1e-3){G2(n>.0?B0(.0,.0,1./255.,.0):B0(.0,.0,.0,1./255.));}else{n*=1./ka;G2(B0(max(n,.0),max(-n,.0),.0,.0));}}
#endif
#ifdef MC
Y2(i,OE){B(I,g);d n=v4(I g1);n*=1./ka;G2(B0(n,.0,.0,.0));}
#endif
#else
#ifdef FC
q6(float,NE){B(I,g);G2(v6(I,r6 g1));}
#endif
#ifdef MC
Y2(float,OE){B(I,g);G2(v4(I g1));}
#endif
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive