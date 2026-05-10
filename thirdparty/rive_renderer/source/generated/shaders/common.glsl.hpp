#pragma once

#include "common.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char common[] = R"===(#define x3 3.14159265359
#define p8 6.28318530718
#define T6 1.57079632679
#ifndef AB
#define k4 float(.5)
#else
#define k4 float(.0)
#endif
#define H3(l) o8(l,k.Re,k.Se)
#ifdef WE
#define Tb(N,f,a) d5(N,f,a)
#define D4 g
#define R9(q) q
#define U5(q) q
#define S9(q) uintBitsToFloat(q)
#define e5(q) floatBitsToUint(q)
#else
#define Tb(N,f,a) E4(N,f,a)
#define D4 Q
#define R9(q) floatBitsToUint(q)
#define U5(q) uintBitsToFloat(q)
#define S9(q) q
#define e5(q) q
#endif
#define Te(a,l,q8) F1(a,U(l)+U(-1,0))q8,F1(a,U(l)+U(0,0))q8,F1(a,U(l)+U(0,-1))q8,F1(a,U(l)+U(-1,-1))q8
#define f5(q) U6(QC,T9,q,Ub,float(Ub),.0).x
#define Wb(q) U6(QC,T9,q,Vb,float(Vb),.0).x
#ifdef Xb
e d l4(float x){return x;}e d V5(uint x){return float(x);}e d Ue(X x){return float(x);}e d U9(int x){return float(x);}e i X4(g xyzw){return xyzw;}e D R7(c xy){return xy;}e i Ob(Q xyzw){return vec4(xyzw);}e X W5(d x){return uint(x);}e X i2(uint x){return x;}
#else
e d l4(float x){return(d)x;}e d V5(uint x){return(d)x;}e d Ue(X x){return(d)x;}e d U9(int x){return(d)x;}e i X4(g xyzw){return(i)xyzw;}e D R7(c xy){return(D)xy;}e i Ob(Q xyzw){return(i)xyzw;}e X W5(d x){return(X)x;}e X i2(uint x){return(X)x;}
#endif
e d G0(d x){return x;}e D A2(D xy){return xy;}e D A2(d x,d y){D L;L.x=x,L.y=y;return L;}e D A2(d x){D L;L.x=x,L.y=x;return L;}e c J6(float x){return c(x,x);}e r T0(d x,d y,d z){r L;L.x=x,L.y=y,L.z=z;return L;}e r T0(d x){r L;L.x=x,L.y=x,L.z=x;return L;}e i B0(d x,d y,d z,d w){i L;L.x=x,L.y=y,L.z=z,L.w=w;return L;}e i B0(r xyz,d w){i L;L.xyz=xyz;L.w=w;return L;}e i B0(d x){i L;L.x=x,L.y=x,L.z=x,L.w=x;return L;}e i B0(i x){return x;}e F4 Ve(bool b){return F4(b,b);}e V6 wh(r o,r b,r J1){V6 L;L[0]=o;L[1]=b;L[2]=J1;return L;}e W6 xh(r o,r b){W6 L;L[0]=o;L[1]=b;return L;}e g5 yh(i o,i b,i J1,i We){g5 L;L[0]=o;L[1]=b;L[2]=J1;L[3]=We;return L;}e a0 j2(g x){return a0(x.xy,x.zw);}e uint Bb(X x){return x;}e c X5(c o,c b,float t){return(b-o)*t+o;}e d r8(uint Yb,uint Y5){return Yb==0u?.0:unpackHalf2x16((Yb+Xe)*Y5).x;}e float Zb(c e2){e2=normalize(e2);float j1=acos(clamp(e2.x,-1.,1.));return e2.y>=.0?j1:-j1;}e i zh(i j){return B0(j.xyz*j.w,j.w);}e r B6(i V9){return V9.xyz*(V9.w!=.0?1./V9.w:.0);}e d d3(D X6){return min(X6.x,X6.y);}e d d3(r ac){return min(d3(ac.xy),ac.z);}e d d3(i bc){D X6=min(bc.xy,bc.zw);d Ye=min(X6.x,X6.y);return Ye;}e d G5(D Y6){return max(Y6.x,Y6.y);}e d G5(r cc){return max(G5(cc.xy),cc.z);}e d G5(i dc){D Y6=max(dc.xy,dc.zw);d Ze=max(Y6.x,Y6.y);return Ze;}e float w9(c x){return abs(x.x)+abs(x.y);}e d W9(d x,d X9,d Y9){
#if defined(XE)||defined(RC)
#ifdef RC
if(RC==af)
#endif
{if(x<Y9)if(x>X9)return x;else return X9;else return Y9;}
#endif
return clamp(x,X9,Y9);}e d ec(c K0,d B2,d j3){d bf=fract(0.06711056*K0.x+0.00583715*K0.y);d cf=fract(52.9829189*bf);return(cf*B2)+j3;}
#if 0
e d Ah(c K0,float B2,float j3){int x=int(K0.x);int y=int(K0.y);int fc=(x^y);int b=(y>>1)&1;b|=(fc&2);b|=(y&1)<<2;b|=(fc&1)<<3;float df=float(b);d ef=l4(df)/16.0;return(ef*B2)+j3;}e d Bh(c K0,float B2,float j3){K0.y*=0.5;K0.x=fract(K0.x*0.5+K0.y);K0.y=fract(K0.y);float J3=(K0.y*0.5+K0.x);return(J3*B2)+j3;}
#endif
#ifdef IB
e d Z9(c K0,d B2,d j3){return IB?ec(K0,B2,j3):.0;}e r M3(r j,c K0,d B2,d j3){return IB?(ec(K0,B2,j3)+j):j;}
#else
e d Z9(c K0,float B2,float j3){return 0.;}e r M3(r j,c K0,d B2,d j3){return j;}
#endif
#ifdef BB
e g o8(c gc,float ff,float hc){return g(gc.x*ff-1.,gc.y*hc-sign(hc),0.,1.);}
#ifndef AB
e g T7(a0 k2,c D2,c aa){c ba=abs(k2[0])+abs(k2[1]);if(ba.x!=.0&&ba.y!=.0){c H=1./ba;c h5=Z0(k2,aa)+D2;const float gf=.5;return g(h5,-h5)*H.xyxy+H.xyxy+gf;}else{return D2.xyxy;}}
#else
e float ca(uint Z6){return 1.-float(Z6)*(2./32768.);}
#ifdef Z
e void ic(a0 k2,c D2,c aa a7){
#ifndef ZD
if(any(notEqual(g(k2),g(.0,.0,.0,.0)))){c h5=Z0(k2,aa)+D2.xy;gl_ClipDistance[0]=h5.x+1.;gl_ClipDistance[1]=h5.y+1.;gl_ClipDistance[2]=1.-h5.x;gl_ClipDistance[3]=1.-h5.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=D2.x-.5;}
#endif
}
#endif
#endif
#endif
#ifdef EB
#ifdef VB
e d h3(d j){return(j<=0.04045)?j/12.92:pow(abs((j+0.055)/1.055),2.4);}e r h3(r j){return T0(h3(j.x),h3(j.y),h3(j.z));}e i h3(i j){return B0(h3(j.xyz),j.w);}
#endif
#endif
#if defined(EB)&&defined(AB)&&!defined(K)
e i jc(g5 c7,int v8){if(v8==0xf){return(c7[0]+c7[1]+c7[2]+c7[3])*.25;}else{i hf=g(notEqual(v8&Z5(1,2,4,8),Z5(0)));i L=Z0(c7,hf);int w8=(v8&5)+((v8>>1)&5);w8=(w8&3)+(w8>>2);L*=1./float(w8);return L;}}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive