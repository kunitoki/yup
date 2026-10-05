#pragma once

#include "hlsl.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char hlsl[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define C half2
#define v half3
#define i half4
#define c float2
#define O float3
#define e float4
#define R4 bool2
#define B6 bool3
#define I7 bool4
#define O0 uint2
#define M uint4
#define e0 int2
#define m6 int4
#define Y float2x2
#define k7 half3x3
#define l7 half2x3
#define S4 half4x4
#endif
typedef O c4;
#ifdef VE
#define Q min16uint
#else
#define Q uint
#endif
#define f inline
#define i1(w2) out w2
#define V6(w2) inout w2
#define c1(a) struct a{
#define K(g,j0,a) j0 a:a
#define d1 };
#define L(h9,D,a,j0) j0 a=D.a
#define pe(g) register(b##g)
#define H7(g,a) cbuffer a:pe(g){struct{
#define e9(a) }a;}
#define l2 struct v0{
#define E0 noperspective
#define KB nointerpolation
#define Z2 nointerpolation
#define V(g,j0,a) j0 a:TEXCOORD##g
#define e2 e U0:SV_Position;};
#define T(a,j0) j0 a
#define Z(a) g0.a=a
#define q(a,j0) j0 a=g0.a
#ifdef BB
#define j4
#define k4
#endif
#ifdef FB
#define N3
#define O3
#endif
#define x5
#define y5
#define V4(c0,g,a) uniform Texture2D<M>a:register(t##g)
#define C6(c0,g,a) uniform Texture2D<e>a:register(t##g)
#define i3(c0,g,a) uniform Texture2D<unorm e>a:register(t##g)
#define D5(c0,g,a) uniform Texture2D<d>a:register(t##g)
#define q6(c0,g,a) uniform Texture1DArray<d>a:register(t##g)
#define M5(g,a) SamplerState a:register(s##g);
#define o4 M5
#define D6(c0,g,a) M5(g,a)
#define m4(a) M5(l4,a)
#define p1(a,l) a[l]
#define K5(a,o,l) a.Sample(o,l)
#define o2(a,o,l,Y0) a.SampleLevel(o,l,Y0)
#define L5(a,o,l,Y1) a.SampleBias(o,l,Y1)
#define j7(a,o,F,E6,j9,Y0) a.SampleLevel(o,c(F,E6),Y0)
#define w8(q0,o,l) K5(q0,o,l)
#define i6(q0,o,l,Y0) o2(q0,o,l,Y0)
#define K7(q0,o,l,Y1) L5(q0,o,l,Y1)
#define E2
#define F2
#ifdef WE
#define U2 RasterizerOrderedTexture2D
#else
#define U2 RWTexture2D
#endif
#define R1
#ifdef RC
#define B0(g,a) uniform U2<unorm i>a:register(u##g)
#else
#define B0(g,a) uniform U2<uint>a:register(u##g)
#endif
#define I4 B0
#define o1(g,a) uniform U2<uint>a:register(u##g)
#define L2 o1
#define f3 h1
#define g3 j1
#define S1
#ifdef RC
#define N0(h) h[H]
#else
#define N0(h) unpackUnorm4x8(h[H])
#endif
#define h1(h) h[H]
#ifdef RC
#define y0(h,E) h[H]=(E)
#else
#define y0(h,E) h[H]=packUnorm4x8(E)
#endif
#define j1(h,E) h[H]=(E)
f uint N5(U2<uint>D3,e0 H,uint x){uint e1;InterlockedMax(D3[H],x,e1);return e1;}
#define p5(h,F) N5(h,H,F)
f uint O5(U2<uint>D3,e0 H,uint x){uint e1;InterlockedAdd(D3[H],x,e1);return e1;}
#define q5(h,F) O5(h,H,F)
#define D2(h)
#define Z1(h)
#define w6
#define G3
#define R3
#define k1
#define p7
#define Y4
#define v1(a,d0,D,G,r) cbuffer Qj:pe(wd){uint Xh;uint a##Rj;uint a##Sj;uint a##Tj;}v0 main(d0 D,uint G:SV_VertexID,uint A4:SV_InstanceID){uint r=A4+Xh;v0 g0;
#define h8(a,d0,D,A1,h0,G,r) v0 main(d0 D,A1 h0,uint G:SV_VertexID){v0 g0;e U0;
#define T6(a,w3,x3,J3,K3,A1,h0,G) v0 main(w3 x3,J3 K3,A1 h0,uint G:SV_VertexID){v0 g0;e U0;
#define w1(P5) g0.U0=P5;}return g0;
#define j3(F1,a) F1 main(v0 g0):SV_Target{
#define G6(F1,a) F1 main(v0 g0,bool H6:SV_IsFrontFace):SV_Target{
#define P2(E) return E;}
#define W6 ,c f0
#define e3 ,f0
#define h4 ,e0 H
#define U1 ,H
#define T1(a) [earlydepthstencil]void main(v0 g0){c f0=g0.U0.xy;e0 H=e0(floor(f0));
#define h2 }
#define z2(a) [earlydepthstencil]i main(v0 g0):SV_Target{c f0=g0.U0.xy;e0 H=e0(floor(f0));i K1;
#define z3 }return K1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(B,J) ((B)==(J))
#define notEqual(B,J) ((B)!=(J))
#define lessThan(B,J) ((B)<(J))
#define greaterThan(B,J) ((B)>(J))
#define M0(B,J) mul(J,B)
#define P4
#define Q4
#define f4
#define g4
#define Z5(g,E1,a) StructuredBuffer<O0>a:register(t##g)
#define W4(g,E1,a) StructuredBuffer<M>a:register(t##g)
#define a6(g,E1,a) StructuredBuffer<e>a:register(t##g)
#define p0(a,D0) a[D0]
#define l5(a,D0) a[D0]
f C unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return C(f16tof32(x),f16tof32(y));}f uint packHalf2x16(c r2){uint x=f32tof16(r2.x);uint y=f32tof16(r2.y);return(y<<16)|x;}f i unpackUnorm4x8(uint u){M q1=M(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(q1)*(1./255.);}f c unpackUnorm2x16(uint u){O0 q1=O0(u&0xffffu,u>>16);return c(q1)*(1./65535.);}f uint packUnorm4x8(i p){M q1=(M(saturate(p)*255.)&0xff)<<M(0,8,16,24);q1.xy|=q1.zw;q1.x|=q1.y;return q1.x;}f Y inverse(Y y1){Y ub=Y(y1[1][1],-y1[0][1],-y1[1][0],y1[0][0]);return ub*(1./determinant(y1));}f float mix(float x,float y,bool s){return s?y:x;}f c mix(c x,c y,R4 s){return s?y:x;}f O mix(O x,O y,B6 s){return s?y:x;}f e mix(e x,e y,I7 s){return s?y:x;}f d mix(d x,d y,bool s){return s?y:x;}f C mix(C x,C y,R4 s){return s?y:x;}f v mix(v x,v y,B6 s){return s?y:x;}f i mix(i x,i y,I7 s){return s?y:x;}f float mix(float x,float y,float s){return lerp(x,y,s);}f c mix(c x,c y,c s){return lerp(x,y,s);}f O mix(O x,O y,O s){return lerp(x,y,s);}f e mix(e x,e y,e s){return lerp(x,y,s);}f d mix(d x,d y,d s){return lerp(x,y,s);}f C mix(C x,C y,C s){return lerp(x,y,s);}f v mix(v x,v y,v s){return lerp(x,y,s);}f i mix(i x,i y,i s){return lerp(x,y,s);}f float fract(float x){return frac(x);}f c fract(c x){return frac(x);}f O fract(O x){return frac(x);}f e fract(e x){return frac(x);}f d fract(d x){return frac(x);}f C fract(C x){return C(frac(x));}f v fract(v x){return v(frac(x));}f i fract(i x){return i(frac(x));}f float mod(float x,float y){return fmod(x,y);}f d V2(d x){return sign(x);}f C V2(C x){return C(sign(x));}f v V2(v x){return v(sign(x));}f i V2(i x){return i(sign(x));}f float V2(float x){return sign(x);}f c V2(c x){return sign(x);}f O V2(O x){return sign(x);}f e V2(e x){return sign(x);}
#define sign V2
f d W2(d x){return abs(x);}f C W2(C x){return C(abs(x));}f v W2(v x){return v(abs(x));}f i W2(i x){return i(abs(x));}f float W2(float x){return abs(x);}f c W2(c x){return abs(x);}f O W2(O x){return abs(x);}f e W2(e x){return abs(x);}
#define abs W2
f d X2(d x){return sqrt(x);}f C X2(C x){return C(sqrt(x));}f v X2(v x){return v(sqrt(x));}f i X2(i x){return i(sqrt(x));}f float X2(float x){return sqrt(x);}f c X2(c x){return sqrt(x);}f O X2(O x){return sqrt(x);}f e X2(e x){return sqrt(x);}
#define sqrt X2
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive