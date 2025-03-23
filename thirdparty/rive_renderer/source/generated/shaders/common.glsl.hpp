#pragma once

#include "common.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char common[] = R"===(#define I1 3.141592653589793238
#ifndef CB
#define g3 float(.5)
#else
#define g3 float(.0)
#endif
#ifdef r9
d h T3(float x){return x;}d h n6(uint x){return float(x);}d h Lb(O x){return float(x);}d h J7(int x){return float(x);}d i g5(e xyzw){return xyzw;}d C T5(c xy){return xy;}d i m9(D xyzw){return vec4(xyzw);}d O K7(h x){return uint(x);}d O M1(uint x){return x;}
#else
d h T3(float x){return(h)x;}d h n6(uint x){return(h)x;}d h Lb(O x){return(h)x;}d h J7(int x){return(h)x;}d i g5(e xyzw){return(i)xyzw;}d C T5(c xy){return(C)xy;}d i m9(D xyzw){return(i)xyzw;}d O K7(h x){return(O)x;}d O M1(uint x){return(O)x;}
#endif
d h d1(h x){return x;}d C Q3(C xy){return xy;}d C Q3(h x,h y){C S;S.x=x,S.y=y;return S;}d C Q3(h x){C S;S.x=x,S.y=x;return S;}d c Mb(float x){return c(x,x);}d p K0(h x,h y,h z){p S;S.x=x,S.y=y,S.z=z;return S;}d p K0(h x){p S;S.x=x,S.y=x,S.z=x;return S;}d i f2(h x,h y,h z,h w){i S;S.x=x,S.y=y,S.z=z,S.w=w;return S;}d i f2(p xyz,h w){i S;S.xyz=xyz;S.w=w;return S;}d i f2(h x){i S;S.x=x,S.y=x,S.z=x,S.w=x;return S;}d D4 Nb(bool b){return D4(b,b);}d j5 zb(p k,p b,p D0){j5 S;S[0]=k;S[1]=b;S[2]=D0;return S;}d k5 Ab(p k,p b){k5 S;S[0]=k;S[1]=b;return S;}d Y r1(e x){return Y(x.xy,x.zw);}d uint l9(O x){return x;}d uint v9(uint T){return(T&Ob)-1u;}d c E4(c k,c b,float t){return(b-k)*t+k;}d h o6(uint w9,uint F4){return w9==0u?.0:unpackHalf2x16((w9+Pb)*F4).x;}d float x9(c B1){B1=normalize(B1);float g1=acos(clamp(B1.x,-1.,1.));return B1.y>=.0?g1:-g1;}d i y9(i j){return f2(j.xyz*j.w,j.w);}d i w4(i j){if(.0<j.w&&j.w<1.){j.xyz*=1./j.w;j.xyz=mix(j.xyz,K0(1.),greaterThan(j.xyz,K0(254.5/255.)));}return j;}d h E7(i z9){C A9=min(z9.xy,z9.zw);h Qb=min(A9.x,A9.y);return Qb;}d float j7(c x){return abs(x.x)+abs(x.y);}
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
G4(w3,VB)float n9;float B9;float Rb;float Sb;uint C9;uint Tb;uint Gb;uint Hb;p6 X5;c f5;c D9;uint P2;uint F4;float q1;uint Ub;l5(m)
#endif
#ifdef AB
d e m6(c E9,float Vb,float F9){return e(E9.x*Vb-1.,E9.y*F9-sign(F9),0.,1.);}
#define v2(A) m6(A,m.Rb,m.Sb)
#ifndef CB
d e W5(Y F1,c O1,c L7){c M7=abs(F1[0])+abs(F1[1]);if(M7.x!=.0&&M7.y!=.0){c g0=1./M7;c g4=q0(F1,L7)+O1;const float Wb=.5;return e(g4,-g4)*g0.xyxy+g0.xyxy+Wb;}else{return O1.xyxy;}}
#else
d float N7(uint m5){return 1.-float(m5)*(2./32768.);}
#ifdef BB
d void G9(Y F1,c O1,c L7){if(F1!=Y(0)){c g4=q0(F1,L7)+O1.xy;gl_ClipDistance[0]=g4.x+1.;gl_ClipDistance[1]=g4.y+1.;gl_ClipDistance[2]=1.-g4.x;gl_ClipDistance[3]=1.-g4.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=O1.x-.5;}}
#endif
#endif
#endif
#ifdef SC
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
G4(H4,DC)e V5;c G0;float G2;float Rd;e F1;c O1;uint m2;uint m3;uint m5;l5(l0)
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive