#pragma once

#include "render_atlas.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char render_atlas[] = R"===(#ifdef BB
A1(c0)r0(0,g,TB);r0(1,g,UB);B1
#endif
h2 J0 d0(0,g,I);Z1
#ifdef BB
C1(IF,c0,G,v,S){v0(v,G,TB,g);v0(v,G,UB,g);Y(I,g);g O;uint m0;c i0;if(p9(TB,UB,S,m0,i0,I p3)){Q K4=P0(LB,m0*4u+2u);V p7=uintBitsToFloat(K4.yzw);i0=i0*p7.x+p7.yz;O=o8(i0,k.Zc.x,k.Zc.y);
#ifdef JC
O.y=-O.y;
#endif
}else{O=g(k.O2,k.O2,k.O2,k.O2);}l0(I);D1(O);}
#endif
#ifdef EB
#ifdef EC
e d v6(g J,bool Cg C3){d n=d8(J i1);if(!Cg)n=-n;return n;}
#endif
#ifdef KD
layout(location=0)inout Q n0;
#ifdef EC
void main(){float n=uintBitsToFloat(n0.x);n+=v6(I,gl_FrontFacing i1);n0.x=floatBitsToUint(n);}
#endif
#ifdef MC
void main(){float n=uintBitsToFloat(n0.x);n=max(n,w4(I));n0.x=floatBitsToUint(n);}
#endif
#elif defined(LD)
__pixel_localEXT n1{layout(r32f)float n0;};
#ifdef EC
void main(){n0+=v6(I,gl_FrontFacing i1);}
#endif
#ifdef MC
void main(){n0=max(n0,w4(I));}
#endif
#elif defined(EXPORTED_ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui)uniform highp upixelLocalANGLE n0;
#ifdef EC
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(n0).x);n+=v6(I,gl_FrontFacing i1);pixelLocalStoreANGLE(n0,Q(floatBitsToUint(n)));}
#endif
#ifdef MC
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(n0).x);n=max(n,w4(I));pixelLocalStoreANGLE(n0,Q(floatBitsToUint(n)));}
#endif
#elif defined(MD)
layout(binding=0,r32i)uniform highp coherent iimage2D V8;ivec2 vd(){return ivec2(floor(T));}int wd(float n){return int(n*zc);}
#ifdef EC
void main(){int n=wd(v6(I,gl_FrontFacing i1));imageAtomicAdd(V8,vd(),n);}
#endif
#ifdef MC
void main(){int n=wd(w4(I));imageAtomicMax(V8,vd(),n);}
#endif
#elif defined(GE)
#ifdef EC
q6(i,HE){B(I,g);d n=v6(I,r6 i1);if(abs(n)>lf-1e-3){F2(n>.0?B0(.0,.0,1./255.,.0):B0(.0,.0,.0,1./255.));}else{n*=1./ka;F2(B0(max(n,.0),max(-n,.0),.0,.0));}}
#endif
#ifdef MC
V2(i,IE){B(I,g);d n=w4(I i1);n*=1./ka;F2(B0(n,.0,.0,.0));}
#endif
#else
#ifdef EC
q6(float,HE){B(I,g);F2(v6(I,r6 i1));}
#endif
#ifdef MC
V2(float,IE){B(I,g);F2(w4(I i1));}
#endif
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive