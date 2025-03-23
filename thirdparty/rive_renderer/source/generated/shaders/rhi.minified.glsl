#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define C half2
#define p half3
#define i half4
#define G5 short
#define O ushort
#define c float2
#define a0 float3
#define e float4
#define D4 bool2
#define L6 bool3
#define E0 uint2
#define D uint4
#define f0 int2
#define p6 int4
#define O ushort
#define Y float2x2
#define j5 half3x3
#define k5 half2x3
#endif
typedef a0 R3;
#ifdef ENABLE_MIN_16_PRECISION
typedef min16uint O;
#else
typedef uint O;
#endif
#define Ja(G,L) G##L
#define d inline
#define G1(D1) out D1
#define Y3(D1) inout D1
#define L0(a) struct a{
#define h0(f,U,a) U a:Ja(me,f)
#define M0 };
#define k0(N6,o,a,U) U a=o.a
#define G4(f,a) cbuffer a{struct{
#define l5(a) }a;}
#define o1 struct j0{
#define o0 noperspective
#define OPTIONALLY_FLAT nointerpolation
#define D2 nointerpolation
#define H(f,U,a) U a:Ja(TEXCOORD,f)
#define p1 e S0:SV_Position;};
#define P(a,U) U a
#define X(a) V.a=a
#define Z(a,U) U a=V.a
#ifdef VERTEX
#define H2
#define I2
#endif
#ifdef FRAGMENT
#define J2
#define L2
#endif
#define F3(p0,f,a) uniform Texture2D<D>a
#define S4(p0,f,a) uniform Texture2D<e>a
#define o2(p0,f,a) uniform Texture2D<unorm e>a
#define G3(p0,f,a) uniform Texture2D<h>a:register(t##f)
#define I5(O2,a) SamplerState a;
#define H3 I5
#define h4 I5
#define f1(a,A) a[A]
#define p3(a,E,A) a.Sample(E,A)
#define S1(a,E,A,z2) a.SampleLevel(E,A,z2)
#define B8 S1
#define W1
#define Y1
#ifdef ENABLE_RASTERIZER_ORDERED_VIEWS
#define Z1 RasterizerOrderedTexture2D
#else
#define Z1 RWTexture2D
#endif
#define k2
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define N0(f,a) uniform Z1<unorm i>a
#else
#define N0(f,a) uniform Z1<uint>a
#endif
#define P0(f,a) uniform Z1<uint>a
#define V3 P0
#define d4 V0
#define e4 W0
#define l2
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define C0(g) g[F]
#else
#define C0(g) unpackUnorm4x8(g[F])
#endif
#define V0(g) g[F]
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define H0(g,r) g[F]=(r)
#else
#define H0(g,r) g[F]=packUnorm4x8(r)
#endif
#define W0(g,r) g[F]=(r)
d uint J5(Z1<uint>m4,f0 F,uint x){uint l1;InterlockedMax(m4[F],x,l1);return l1;}
#define d5(g,N) J5(g,F,N)
d uint K5(Z1<uint>m4,f0 F,uint x){uint l1;InterlockedAdd(m4[F],x,l1);return l1;}
#define e5(g,N) K5(g,F,N)
#define p2(g)
#define X1(g)
#define P4
#define g2
#define Z0(a,e0,o,l,I) uint baseInstance;j0 a(e0 o,uint l:SV_VertexID,uint C8:SV_InstanceID){uint I=C8+baseInstance;j0 V;
#define U5(a,e0,o,l,I) j0 a(e0 o,uint l:SV_VertexID){j0 V;e S0;
#define v4(a,P1,Q1,h2,i2,l) j0 a(P1 Q1,h2 i2,uint l:SV_VertexID){j0 V;e S0;
#define U0(L5) V.S0=L5;}return V;
#define T1(l4,a) l4 a(j0 V):SV_Target{
#define U1(r) return r;}
#define Z4 ,c v0
#define o3 ,v0
#define l3 ,f0 F
#define v1 ,F
#define n2(a) [earlydepthstencil]void a(j0 V){c v0=V.S0.xy;f0 F=f0(floor(v0));
#define z4(a) n2(a)
#define E2 }
#define n3(a) [earlydepthstencil]i a(j0 V):SV_Target{c v0=V.S0.xy;f0 F=f0(floor(v0));i A1;
#define c5(a) n3(a)
#define x4 }return A1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define notEqual(G,L) ((G)!=(L))
#define lessThanEqual(G,L) ((G)<=(L))
#define lessThan(G,L) ((G)<(L))
#define greaterThan(G,L) ((G)>(L))
#define greaterThanEqual(G,L) ((G)>=(L))
#define q0(G,L) mul(L,G)
#define r3
#define v3
#define j3
#define k3
#define W3(f,R0,a) StructuredBuffer<E0>a
#define I3(f,R0,a) StructuredBuffer<D>a
#define X3(f,R0,a) StructuredBuffer<e>a
#define r0(a,n0) a[n0]
#define a4(a,n0) a[n0]
d C unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return C(f16tof32(x),f16tof32(y));}d uint packHalf2x16(c B1){uint x=f32tof16(B1.x);uint y=f32tof16(B1.y);return(y<<16)|x;}d i unpackUnorm4x8(uint u){D k1=D(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(k1)*(1./255.);}d uint packUnorm4x8(i j){D k1=(D(j*255.)&0xff)<<D(0,8,16,24);k1.xy|=k1.zw;k1.x|=k1.y;return k1.x;}d Y inverse(Y O0){Y D8=Y(O0[1][1],-O0[0][1],-O0[1][0],O0[0][0]);return D8*(1./determinant(O0));}d float mix(float x,float y,float s){return lerp(x,y,s);}d c mix(c x,c y,c s){return lerp(x,y,s);}d a0 mix(a0 x,a0 y,a0 s){return lerp(x,y,s);}d e mix(e x,e y,e s){return lerp(x,y,s);}d float fract(float x){return frac(x);}d c fract(c x){return frac(x);}d a0 fract(a0 x){return frac(x);}d e fract(e x){return frac(x);}d float a2(float x){return sign(x);}d c a2(c x){return sign(x);}d a0 a2(a0 x){return sign(x);}d e a2(e x){return sign(x);}
#define sign a2
d float c2(float x){return abs(x);}d c c2(c x){return abs(x);}d a0 c2(a0 x){return abs(x);}d e c2(e x){return abs(x);}
#define abs c2
d float d2(float x){return sqrt(x);}d c d2(c x){return sqrt(x);}d a0 d2(a0 x){return sqrt(x);}d e d2(e x){return sqrt(x);}
#define sqrt d2
