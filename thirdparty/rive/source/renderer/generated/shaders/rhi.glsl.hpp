#pragma once

#include "rhi.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char rhi[] = R"===(#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define F half2
#define v half3
#define i half4
#define O ushort
#define c float2
#define M float3
#define e float4
#define m5 bool2
#define Y6 bool3
#define l8 bool4
#define R0 uint2
#define R uint4
#define f0 int2
#define D6 int4
#define O ushort
#define X float2x2
#define G7 half3x3
#define H7 half2x3
#define n5 half4x4
#endif
typedef M l4;
#ifdef AF
#ifdef LF
typedef min16uint O;
#endif
#else
#ifdef LF
typedef uint O;
#endif
#endif
#define Af(E,J) E##J
#define f inline
#define p1(H2) out H2
#define p7(H2) inout H2
#define h1(a) struct a{
#define K(g,k0,a) k0 a:Af(jl,g)
#define i1 };
#define L(W9,A,a,k0) k0 a=A.a
#define k8(g,a) cbuffer a{struct{
#define T9(a) }a;}
#define o2 struct l0{
#define I0 noperspective
#define NB nointerpolation
#define l3 nointerpolation
#define W(g,k0,a) k0 a:Af(TEXCOORD,g)
#ifdef OD
#define P1 e T0:SV_Position;e zc:SV_ClipDistance;};
#else
#define P1 e T0:SV_Position;};
#endif
#define V(a,k0) k0 a
#define e0(a) a0.a=a
#define q(a,k0) k0 a=a0.a
#ifdef CB
#define A4
#define B4
#endif
#ifdef EB
#define X3
#define Y3
#endif
#define K5
#define L5
#define v5(h0,g,a) Texture2D<R>a
#define Z6(h0,g,a) Texture2D<e>a
#ifdef YD
#define Zg(h0,g,a) Texture2DMS<i>a
#endif
#define Z2(h0,g,a) Texture2D<i>a
#define U5(h0,g,a) Texture2D<d>a
#define P6(h0,g,a) Texture2DArray<d>a
#define S4(g,a) SamplerState a;
#define f4 S4
#define Q6 S4
#define a7(h0,g,a) S4(g,a)
#define D4(a) S4(C4,a)
#ifdef ml
#define n9(a,zj,o) a.nl(o,zj)
#endif
#define j1(a,o) a[o]
#define Z5(a,p,o) a.Sample(p,o)
#define r2(a,p,o,g1) a.SampleLevel(p,o,g1)
#define a6(a,p,o,j2) a.SampleBias(p,o,j2)
#define F7(a,p,G,c7,Y9,g1) a.SampleLevel(p,M(G,0.5,c7),g1)
#define g9(v0,p,o) Z5(v0,p,o)
#define k5(v0,p,o,g1) r2(v0,p,o,g1)
#define m8(v0,p,o,j2) a6(v0,p,o,j2)
#define R2
#define S2
#ifdef BF
#define f3 RasterizerOrderedTexture2D
#else
#define f3 RWTexture2D
#endif
#if defined(EB)&&defined(BB)
#ifdef HG
#define Aj (B2+1)
#define K4(a) [[vk::input_attachment_index(Aj)]]SubpassInputMS<i>a
#define J4(a) wb(n5(a.SubpassLoad(0),a.SubpassLoad(1),a.SubpassLoad(2),a.SubpassLoad(3)),ga)
#elif defined(IG)
#define K4(a) Texture2DMS<i>a
#define J4(a) wb(n5(a.Load(D,0),a.Load(D,1),a.Load(D,2),a.Load(D,3)),ga)
#else
#define K4(a) Texture2D a
#define J4(a) a[D]
#endif
#endif
#define I1
#define J1
#ifdef UC
#define z0(g,a) f3<pl i>a
#else
#define z0(g,a) f3<uint>a
#endif
#define c5 z0
#define x1(g,a) f3<uint>a
#define r3 o1
#define v3 q1
#if COMPILER_METAL||FORCE_ATOMIC_BUFFER
#define Y2(g,a) RWBuffer<uint>a
#define r3(h) h[E0]
#define v3(h,B) h[E0]=B
#else
#define Y2 x1
#define r3 o1
#define v3 q1
#endif
#ifdef UC
#define L0(h) h[D]
#else
#define L0(h) unpackUnorm4x8(h[D])
#endif
#define o1(h) h[D]
#ifdef UC
#define y0(h,B) h[D]=(B)
#else
#define y0(h,B) h[D]=packUnorm4x8(B)
#endif
#define q1(h,B) h[D]=(B)
#define L4 z0
#define e4 L0
#define r5 y0
#if COMPILER_METAL||FORCE_ATOMIC_BUFFER
f uint c6(RWBuffer<uint>P3,uint E0,uint x){uint l1;InterlockedMax(P3[E0],x,l1);return l1;}
#define H5(h,G) c6(h,E0,G)
f uint d6(RWBuffer<uint>P3,uint E0,uint x){uint l1;InterlockedAdd(P3[E0],x,l1);return l1;}
#define I5(h,G) d6(h,E0,G)
#else
f uint c6(f3<uint>P3,f0 D,uint x){uint l1;InterlockedMax(P3[D],x,l1);return l1;}
#define H5(h,G) c6(h,D,G)
f uint d6(f3<uint>P3,f0 D,uint x){uint l1;InterlockedAdd(P3[D],x,l1);return l1;}
#define I5(h,G) d6(h,D,G)
#endif
#define Q2(h)
#define l2(h)
#define T6
#define S3
#define g4
#define r1
#ifdef JG
#define Gc
#define Hc(Q3) (Q3)
#else
#define Gc uint baseInstance;
#define Hc(Q3) ((Q3)+baseInstance)
#endif
#if defined(ND)&&!defined(KG)
#define Ic uint baseVertex;
#define Jc(g3) ((g3)+baseVertex)
#else
#define Ic
#define Jc(g3) (g3)
#endif
#ifdef MF
#define n1(a,d0,A,C,r) Gc Ic e a(d0 A,uint g3:SV_VertexID,uint Q3:SV_InstanceID):SV_Position{uint C=Jc(g3);uint r=Hc(Q3);
#define C1(e6) return e6;}
#else
#define n1(a,d0,A,C,r) Gc Ic l0 a(d0 A,uint g3:SV_VertexID,uint Q3:SV_InstanceID){uint C=Jc(g3);uint r=Hc(Q3);l0 a0;
#define O8(a,d0,A,H1,j0,C,r) l0 a(d0 A,H1 j0,uint C:SV_VertexID){l0 a0;e T0;
#define n7(a,G3,H3,V3,n3,H1,j0,C) l0 a(G3 H3,V3 n3,H1 j0,uint C:SV_VertexID){l0 a0;e T0;
#define C1(e6) a0.T0=e6;}return a0;
#endif
#if COMPILER_DXC&&(COMPILER_VULKAN||COMPILER_GLSL_ES3_1)
#define Lc(Kc) (Kc)
#else
#define Lc(Kc) (!(Kc))
#endif
#ifdef MF
#define v2(U0,a) EARLYDEPTHSTENCIL U0 a(e T0:SV_Position):SV_Target{c Y=T0.xy;
#define d7(U0,a) ql U0 a(e T0:SV_Position,uint ga:SV_Coverage,bool Mc:SV_IsFrontFace):SV_Target{c Y=T0.xy;bool e7=Lc(Mc);
#else
#define v2(U0,a) EARLYDEPTHSTENCIL U0 a(l0 a0,uint ga:SV_Coverage):SV_Target{c Y=a0.T0.xy;f0 D=f0(floor(Y));uint E0=D.y*j.X5+D.x;
#define d7(U0,a) U0 a(l0 a0,uint ga:SV_Coverage,bool Mc:SV_IsFrontFace):SV_Target{c Y=a0.T0.xy;f0 D=f0(floor(Y));uint E0=D.y*j.X5+D.x;bool e7=Lc(Mc);
#endif
#define w2(B) return B;}
#ifdef OD
#define C6 ,out e gl_ClipDistance
#define I4 ,a0.zc
#else
#define C6
#define I4
#endif
#define q7 ,c Y
#define q3 ,Y
#define r4 ,f0 D
#define d2 ,D
#define E1(a) EARLYDEPTHSTENCIL void a(l0 a0){c Y=a0.T0.xy;f0 D=f0(floor(Y));uint E0=D.y*j.X5+D.x;
#if defined(Q)&&defined(OB)
#define L1 I3
#else
#define L1 }
#endif
#define K2(a) EARLYDEPTHSTENCIL i a(l0 a0):SV_Target{c Y=a0.T0.xy;f0 D=f0(floor(Y));uint E0=D.y*j.X5+D.x;i S1;
#define I3 }return S1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(E,J) ((E)==(J))
#define notEqual(E,J) ((E)!=(J))
#define lessThan(E,J) ((E)<(J))
#define greaterThan(E,J) ((E)>(J))
#define G0(E,J) mul(J,E)
#define i5
#define j5
#define n4
#define p4
#define o4(g,a1,a) StructuredBuffer<R>a
#define c8(g,a1,a) StructuredBuffer<uint>a
#define p6(g,a1,a) StructuredBuffer<e>a
#define p0(a,m0) a[m0]
#define Q5(a,m0) a[m0]
f F unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return F(f16tof32(x),f16tof32(y));}f uint packHalf2x16(c F2){uint x=f32tof16(F2.x);uint y=f32tof16(F2.y);return(y<<16)|x;}f i unpackUnorm4x8(uint u){R z1=R(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(z1)*(1./255.);}f c unpackUnorm2x16(uint u){R0 z1=R0(u&0xffffu,u>>16);return c(z1)*(1./65535.);}f uint packUnorm4x8(i k){R z1=(R(saturate(k)*255.)&0xff)<<R(0,8,16,24);z1.xy|=z1.zw;z1.x|=z1.y;return z1.x;}f X inverse(X F1){X Bc=X(F1[1][1],-F1[0][1],-F1[1][0],F1[0][0]);return Bc*(1./determinant(F1));}f float mix(float x,float y,float s){return lerp(x,y,s);}f c mix(c x,c y,c s){return lerp(x,y,s);}f M mix(M x,M y,M s){return lerp(x,y,s);}f e mix(e x,e y,e s){return lerp(x,y,s);}f float fract(float x){return frac(x);}f c fract(c x){return frac(x);}f M fract(M x){return frac(x);}f e fract(e x){return frac(x);}f float mod(float x,float y){return fmod(x,y);}f float h3(float x){return sign(x);}f c h3(c x){return sign(x);}f M h3(M x){return sign(x);}f e h3(e x){return sign(x);}
#define sign h3
f float i3(float x){return abs(x);}f c i3(c x){return abs(x);}f M i3(M x){return abs(x);}f e i3(e x){return abs(x);}
#define abs i3
f float j3(float x){return sqrt(x);}f c j3(c x){return sqrt(x);}f M j3(M x){return sqrt(x);}f e j3(e x){return sqrt(x);}
#define sqrt j3
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive