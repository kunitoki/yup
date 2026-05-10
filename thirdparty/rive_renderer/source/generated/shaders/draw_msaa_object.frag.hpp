#pragma once

#include "draw_msaa_object.frag.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_msaa_object_frag[] = R"===(#ifdef EB
#ifdef KB
y3 U2(Y4,R3,DC);
#ifdef FB
g7(JD);
#endif
z3 Z4 S3(R5)a5
#endif
V2(i,HB){
#ifdef KB
B(U0,c);
#else
B(k1,g);
#ifdef DB
B(C2,c);
#endif
#ifdef FB
B(Y1,d);
#endif
#endif
#ifdef KB
i j=y7(DC,R5,U0,k.ad)*A0.y4;
#else
d n=
#ifdef DB
clamp(m2(VC,I9,C2,.0).x,G0(.0),G0(1.));
#else
1.;
#endif
i j=M7(k1,n R2);
#endif
#if defined(FB)&&!defined(K)
#ifdef KB
j.xyz=B6(j);X n2=i2(A0.n2);
#else
X n2=W5(Y1);
#endif
i M1=S8(JD);j.xyz=R4(j.xyz,M1,n2);j.xyz*=j.w;
#elif defined(ME)&&defined(K)&&!defined(KB)
j.xyz*=j.w;
#endif
#ifdef VB
if(VB){j=h3(j);}
#endif
j.xyz=M3(j.xyz,T.xy,k.v3,k.w3);F2(j);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive