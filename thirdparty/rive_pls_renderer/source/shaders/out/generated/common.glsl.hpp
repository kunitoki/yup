#pragma once

#include "common.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char common[] = R"===(#define E4 float(3.141592653589793238)
#ifndef CB
#define k2 float(.5)
#else
#define k2 float(.0)
#endif
p uint Z6(uint D){return(D&j9)-1u;}p d r3(d m,d b,float t){return(b-m)*t+m;}p h F4(uint a7,uint v3){return a7==0u?.0:unpackHalf2x16((a7+k9)*v3).x;}p float c7(d R0){float d7=.0;if(abs(R0.x)>abs(R0.y)){R0=d(R0.y,-R0.x);d7=E4/2.;}return atan(R0.y,R0.x)+d7;}p i p2(i f){return M0(f.xyz*f.w,f.w);}p i S3(i f){if(f.w!=.0)f.xyz*=1.0/f.w;return f;}p A W0(g x){return A(x.xy,x.zw);}p uint W6(M x){return x;}p h B5(i e7){l0 f7=min(e7.xy,e7.zw);h l9=min(f7.x,f7.y);return l9;}p float o5(d x){return abs(x.x)+abs(x.y);}
#ifdef V
X3(O2,RB)float E5;float F5;float m9;float g7;uint h7;uint n9;uint f9;uint g9;G5 p4;uint v3;float j2;G4(v)
#define i2(F) g((F).x*v.m9-1.,(F).y*-v.g7+sign(v.g7),.0,1.)
#ifndef CB
p g n4(A X0,d i1,d H5){d I5=abs(X0[0])+abs(X0[1]);if(I5.x!=.0&&I5.y!=.0){d U=1./I5;d P2=h0(X0,H5)+i1;const float o9=.5;return g(P2,-P2)*U.xyxy+U.xyxy+o9;}else{return i1.xyxy;}}
#else
p float J5(uint Y3){return 1.-float(Y3)*(2./32768.);}
#ifdef Z
p void i7(A X0,d i1,d H5){if(X0!=A(0)){d P2=h0(X0,H5)+i1.xy;gl_ClipDistance[0]=P2.x+1.;gl_ClipDistance[1]=P2.y+1.;gl_ClipDistance[2]=1.-P2.x;gl_ClipDistance[3]=1.-P2.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=i1.x-.5;}}
#endif
#endif
#endif
#ifdef BC
X3(w3,YB)g m5;d K0;float J2;float Ja;g X0;d i1;uint G1;uint a2;uint Y3;G4(J)
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive