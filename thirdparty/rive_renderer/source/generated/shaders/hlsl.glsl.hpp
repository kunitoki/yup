#pragma once

#include "hlsl.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char hlsl[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define D half2
#define r half3
#define i half4
#define c float2
#define V float3
#define g float4
#define E4 bool2
#define n6 bool3
#define w7 bool4
#define W0 uint2
#define Q uint4
#define U int2
#define Z5 int4
#define Z float2x2
#define V6 half3x3
#define W6 half2x3
#define h5 half4x4
#endif
typedef V L3;
#ifdef HE
#define X min16uint
#else
#define X uint
#endif
#define e inline
#define e1(g2) out g2
#define T4(g2) inout g2
#define A1(a) struct a{
#define p0(f,W,a) W a:a
#define B1 };
#define q0(O8,G,a,W) W a=G.a
#define wd(f) register(b##f)
#define m6(f,a) cbuffer a:wd(f){struct{
#define v7(a) }a;}
#define h2 struct f0{
#define J0 noperspective
#define OB nointerpolation
#define R4 nointerpolation
#define c0(f,W,a) W a:TEXCOORD##f
#define a2 g L0:SV_Position;};
#define Y(a,W) W a
#define k0(a) R.a=a
#define B(a,W) W a=R.a
#ifdef CB
#define R3
#define S3
#endif
#ifdef FB
#define B3
#define C3
#endif
#define a5
#define c5
#define D4(M,f,a) uniform Texture2D<Q>a:register(t##f)
#define e5(M,f,a) uniform Texture2D<g>a:register(t##f)
#define X2(M,f,a) uniform Texture2D<unorm g>a:register(t##f)
#define k5(M,f,a) uniform Texture2D<d>a:register(t##f)
#define f6(M,f,a) uniform Texture1DArray<d>a:register(t##f)
#define x5(f,a) SamplerState a:register(s##f);
#define X3 x5
#define o6(M,f,a) x5(f,a)
#define U3(a) x5(T3,a)
#define v1(a,l) a[l]
#define r5(a,p,l) a.Sample(p,l)
#define m2(a,p,l,X0) a.SampleLevel(p,l,X0)
#define v5(a,p,l,P1) a.SampleBias(p,l,P1)
#define U6(a,p,q,p6,Q8,X0) a.SampleLevel(p,c(q,p6),X0)
#define g8(h0,p,l) r5(h0,p,l)
#define Q6(h0,p,l,X0) m2(h0,p,l,X0)
#define y7(h0,p,l,P1) v5(h0,p,l,P1)
#define v2
#define w2
#ifdef IE
#define K2 RasterizerOrderedTexture2D
#else
#define K2 RWTexture2D
#endif
#define J1
#ifdef KC
#define r0(f,a) uniform K2<unorm i>a:register(u##f)
#else
#define r0(f,a) uniform K2<uint>a:register(u##f)
#endif
#define n4 r0
#define k1(f,a) uniform K2<uint>a:register(u##f)
#define E2 k1
#define T2 d1
#define U2 f1
#define K1
#ifdef KC
#define H0(h) h[E]
#else
#define H0(h) unpackUnorm4x8(h[E])
#endif
#define d1(h) h[E]
#ifdef KC
#define v0(h,C) h[E]=(C)
#else
#define v0(h,C) h[E]=packUnorm4x8(C)
#endif
#define f1(h,C) h[E]=(C)
e uint y5(K2<uint>p3,U E,uint x){uint a1;InterlockedMax(p3[E],x,a1);return a1;}
#define W4(h,q) y5(h,E,q)
e uint z5(K2<uint>p3,U E,uint x){uint a1;InterlockedAdd(p3[E],x,a1);return a1;}
#define X4(h,q) z5(h,E,q)
#define r2(h)
#define Y1(h)
#define i6
#define v3
#define F3
#define g1
#define a7
#define w5
#define C1(a,a0,G,v,T) cbuffer gi:wd(zc){uint Ag;uint a##hi;uint a##ii;uint a##ji;}f0 main(a0 G,uint v:SV_VertexID,uint D7:SV_InstanceID){uint T=D7+Ag;f0 R;
#define S7(a,a0,G,v,T) f0 main(a0 G,uint v:SV_VertexID){f0 R;g L0;
#define E6(a,h3,i3,w3,x3,v) f0 main(h3 i3,w3 x3,uint v:SV_VertexID){f0 R;g L0;
#define D1(A5) R.L0=A5;}return R;
#define Y2(Q1,a) Q1 main(f0 R):SV_Target{
#define q6(Q1,a) Q1 main(f0 R,bool r6:SV_IsFrontFace):SV_Target{
#define G2(C) return C;}
#define G6 ,c S
#define S2 ,S
#define P3 ,U E
#define M1 ,E
#define m1(a) [earlydepthstencil]void main(f0 R){c S=R.L0.xy;U E=U(floor(S));
#define O5(a) m1(a)
#define U1 }
#define o2(a) [earlydepthstencil]i main(f0 R):SV_Target{c S=R.L0.xy;U E=U(floor(S));i l1;
#define r4(a) o2(a)
#define l3 }return l1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(A,F) ((A)==(F))
#define notEqual(A,F) ((A)!=(F))
#define lessThan(A,F) ((A)<(F))
#define greaterThan(A,F) ((A)>(F))
#define Z0(A,F) mul(F,A)
#define A4
#define B4
#define N3
#define O3
#define J5(f,y1,a) StructuredBuffer<W0>a:register(t##f)
#define F4(f,y1,a) StructuredBuffer<Q>a:register(t##f)
#define K5(f,y1,a) StructuredBuffer<g>a:register(t##f)
#define P0(a,y0) a[y0]
#define L5(a,y0) a[y0]
e D unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return D(f16tof32(x),f16tof32(y));}e uint packHalf2x16(c e2){uint x=f32tof16(e2.x);uint y=f32tof16(e2.y);return(y<<16)|x;}e i unpackUnorm4x8(uint u){Q R1=Q(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(R1)*(1./255.);}e uint packUnorm4x8(i j){Q R1=(Q(j*255.)&0xff)<<Q(0,8,16,24);R1.xy|=R1.zw;R1.x|=R1.y;return R1.x;}e Z inverse(Z o1){Z La=Z(o1[1][1],-o1[0][1],-o1[1][0],o1[0][0]);return La*(1./determinant(o1));}e float mix(float x,float y,bool s){return s?y:x;}e c mix(c x,c y,E4 s){return s?y:x;}e V mix(V x,V y,n6 s){return s?y:x;}e g mix(g x,g y,w7 s){return s?y:x;}e d mix(d x,d y,bool s){return s?y:x;}e D mix(D x,D y,E4 s){return s?y:x;}e r mix(r x,r y,n6 s){return s?y:x;}e i mix(i x,i y,w7 s){return s?y:x;}e float mix(float x,float y,float s){return lerp(x,y,s);}e c mix(c x,c y,c s){return lerp(x,y,s);}e V mix(V x,V y,V s){return lerp(x,y,s);}e g mix(g x,g y,g s){return lerp(x,y,s);}e d mix(d x,d y,d s){return lerp(x,y,s);}e D mix(D x,D y,D s){return lerp(x,y,s);}e r mix(r x,r y,r s){return lerp(x,y,s);}e i mix(i x,i y,i s){return lerp(x,y,s);}e float fract(float x){return frac(x);}e c fract(c x){return frac(x);}e V fract(V x){return frac(x);}e g fract(g x){return frac(x);}e d fract(d x){return frac(x);}e D fract(D x){return D(frac(x));}e r fract(r x){return r(frac(x));}e i fract(i x){return i(frac(x));}e float mod(float x,float y){return fmod(x,y);}e d L2(d x){return sign(x);}e D L2(D x){return D(sign(x));}e r L2(r x){return r(sign(x));}e i L2(i x){return i(sign(x));}e float L2(float x){return sign(x);}e c L2(c x){return sign(x);}e V L2(V x){return sign(x);}e g L2(g x){return sign(x);}
#define sign L2
e d M2(d x){return abs(x);}e D M2(D x){return D(abs(x));}e r M2(r x){return r(abs(x));}e i M2(i x){return i(abs(x));}e float M2(float x){return abs(x);}e c M2(c x){return abs(x);}e V M2(V x){return abs(x);}e g M2(g x){return abs(x);}
#define abs M2
e d N2(d x){return sqrt(x);}e D N2(D x){return D(sqrt(x));}e r N2(r x){return r(sqrt(x));}e i N2(i x){return i(sqrt(x));}e float N2(float x){return sqrt(x);}e c N2(c x){return sqrt(x);}e V N2(V x){return sqrt(x);}e g N2(g x){return sqrt(x);}
#define sqrt N2
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive