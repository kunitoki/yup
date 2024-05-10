#pragma warning(disable:3550)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define l0 half2
#define O half3
#define i half4
#define a6 short
#define M ushort
#define d float2
#define D1 float3
#define g float4
#define c8 bool3
#define x0 uint2
#define T uint4
#define m0 int2
#define G5 int4
#define M ushort
#define A float2x2
#define e6 half3x4
#endif
typedef D1 H3;
#define E0 float
#define Z1 d
#define j0 D1
#define M0 g
typedef min16int a6;typedef min16uint M;
#define O0 min16uint
#define j5 half3x4
#define p inline
#define y2(h4) out h4
#define U0(a) struct a{
#define q0(c,G,a) G a:a
#define V0 };
#define v0(c6,r,a,G) G a=r.a
#define d8(c) register(b##c)
#define X3(c,a) cbuffer a:d8(c){struct{
#define G4(a) }a;}
#define A1 struct X{
#define k0 noperspective
#define OPTIONALLY_FLAT nointerpolation
#define d6 nointerpolation
#define I(c,G,a) G a:TEXCOORD##c
#define B1 g q1:SV_Position;};
#define Q(a,G) G a
#define S(a) K.a=a
#define N(a,G) G a=K.a
#ifdef VERTEX
#define O1
#define P1
#endif
#ifdef FRAGMENT
#define F2
#define G2
#endif
#define Q2(c,a) uniform Texture2D<T>a:register(t##c)
#define i4(c,a) uniform Texture2D<g>a:register(t##c)
#define x1(c,a) uniform Texture2D<unorm g>a:register(t##c)
#define e8(d2,a) SamplerState a:register(s##d2);
#define J3 e8
#define l3 e8
#define I1(a,F) a[F]
#define M2(a,p0,F) a.Sample(p0,F)
#define R3(a,p0,F,X2) a.SampleLevel(p0,F,X2)
#define Q3(a,p0,F,Y2,Z2) a.SampleGrad(p0,F,Y2,Z2)
#define n1
#define o1
#ifdef ENABLE_RASTERIZER_ORDERED_VIEWS
#define B3 RasterizerOrderedTexture2D
#else
#define B3 RWTexture2D
#endif
#define E1
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define C0(c,a) uniform B3<unorm i>a:register(u##c)
#else
#define C0(c,a) uniform B3<uint>a:register(u##c)
#endif
#define D0(c,a) uniform B3<uint>a:register(u##c)
#define K3 D0
#define o3 L0
#define p3 a1
#define F1
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define N0(e) e[H]
#else
#define N0(e) unpackUnorm4x8(e[H])
#endif
#define L0(e) e[H]
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define F0(e,l) e[H]=(l)
#else
#define F0(e,l) e[H]=packUnorm4x8(l)
#endif
#define a1(e,l) e[H]=(l)
p uint f6(B3<uint>g6,m0 H,uint x){uint L1;InterlockedMax(g6[H],x,L1);return L1;}
#define B4(e,S0) f6(e,H,S0)
p uint h6(B3<uint>g6,m0 H,uint x){uint L1;InterlockedAdd(g6[H],x,L1);return L1;}
#define C4(e,S0) h6(e,H,S0)
#define y0(e)
#define D4(w0)
#define U2
#define j3
#define g1(a,P,r,j,L) cbuffer Xa:d8(y7){uint ga;uint a##Ya;uint a##Za;uint a##ab;}X a(P r,uint j:SV_VertexID,uint ha:SV_InstanceID){uint L=ha+ga;X K;
#define k5(a,P,r,j,L) X a(P r,uint j:SV_VertexID){X K;g q1;
#define o4(a,X1,Y1,l2,m2,j) X a(X1 Y1,l2 m2,uint j:SV_VertexID){X K;g q1;
#define h1(i6) K.q1=i6;}return K;
#define r2(j4,a) j4 a(X K):SV_Target{
#define v2(l) return l;}
#define w4 ,d n0
#define L2 ,n0
#define H2 ,m0 H
#define l1 ,H
#define R1(a) [earlydepthstencil]void a(X K){d n0=K.q1.xy;m0 H=m0(floor(n0));
#define U3(a) R1(a)
#define c2 }
#define n3(a) [earlydepthstencil]i a(X K):SV_Target{d n0=K.q1.xy;m0 H=m0(floor(n0));i P0;
#define z4(a) n3(a)
#define V3 }return P0;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define fract frac
#define mix lerp
#define inversesqrt rsqrt
#define notEqual(d0,g0) ((d0)!=(g0))
#define lessThanEqual(d0,g0) ((d0)<=(g0))
#define greaterThanEqual(d0,g0) ((d0)>=(g0))
#define h0(d0,g0) mul(g0,d0)
#define V1
#define W1
#define L3
#define O3
#define M3(c,d1,a) StructuredBuffer<x0>a:register(t##c)
#define R2(c,d1,a) StructuredBuffer<T>a:register(t##c)
#define N3(c,d1,a) StructuredBuffer<g>a:register(t##c)
#define z0(a,A0) a[A0]
#define q2(a,A0) a[A0]
p l0 unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return Z1(f16tof32(x),f16tof32(y));}p uint packHalf2x16(d R0){uint x=f32tof16(R0.x);uint y=f32tof16(R0.y);return(y<<16)|x;}p i unpackUnorm4x8(uint u){T e2=T(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(e2)*(1./255.);}p uint packUnorm4x8(i f){T e2=(T(f*255.)&0xff)<<T(0,8,16,24);e2.xy|=e2.zw;e2.x|=e2.y;return e2.x;}p float atan(float y,float x){return atan2(y,x);}p A inverse(A e1){A ia=A(e1[1][1],-e1[0][1],-e1[1][0],e1[0][0]);return ia*(1./determinant(e1));}