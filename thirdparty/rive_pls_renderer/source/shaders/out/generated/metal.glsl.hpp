#pragma once

#include "metal.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char metal[] = R"===(#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define l0 half2
#define O half3
#define i half4
#define Z5 short
#define M ushort
#define d float2
#define C1 float3
#define G3 packed_float3
#define g float4
#define Z7 bool3
#define y0 uint2
#define T uint4
#define m0 int2
#define F5 int4
#define M ushort
#define A float2x2
#define d6 half3x4
#endif
#define M0 i
#define j0 O
#define Y1 l0
#define E0 h
#define O0 M
#define p inline
#define x2(g4) thread g4&
#define notEqual(e0,g0) ((e0)!=(g0))
#define lessThanEqual(e0,g0) ((e0)<=(g0))
#define greaterThanEqual(e0,g0) ((e0)>=(g0))
#define h0(e0,g0) ((e0)*(g0))
#define atan atan2
#define inversesqrt rsqrt
#define W3(c,a) struct a{
#define F4(a) };
#define T0(a) struct a{
#define q0(c,G,a) G a
#define U0 };
#define w0(a6,r,a,G) G a=r[a6].a
#define z1 struct Y{
#define I(c,G,a) G a
#define c6 [[flat]]
#define k0 [[center_no_perspective]]
#define GB
#define A1 g p1[[position]][[invariant]];};
#define Q(a,G) thread G&a=K.a
#define S(a)
#define N(a,G) G a=K.a
#define U1 struct ha{
#define V1 };
#define K3 struct Z2{
#define N3 };
#define L3(c,a1,a) constant y0*a[[buffer(c)]]
#define Q2(c,a1,a) constant T*a[[buffer(c)]]
#define M3(c,a1,a) constant g*a[[buffer(c)]]
#define z0(a,A0) L1.a[A0]
#define p2(a,A0) L1.a[A0]
#define N1 struct ia{
#define O1 };
#define E2 struct a3{
#define F2 };
#define P2(c,a) [[texture(c)]]texture2d<uint>a
#define h4(c,a) [[texture(c)]]texture2d<float>a
#define w1(c,a) [[texture(c)]]texture2d<h>a
#define I3(c2,a) constexpr sampler a(filter::linear,mip_filter::none);
#define k3(c2,a) constexpr sampler a(filter::linear,mip_filter::linear);
#define H1(c3,F) q1.c3.read(y0(F))
#define L2(c3,p0,F) q1.c3.sample(p0,F)
#define Q3(c3,p0,F,W2) q1.c3.sample(p0,F,level(W2))
#define P3(c3,p0,F,X2,Y2) q1.c3.sample(p0,F,gradient2d(X2,Y2))
#define T2 ,ia q1,ha L1
#define i3 ,q1,L1
#ifdef WC
#define e1(a,P,r,j,L) __attribute__((visibility("default")))Y vertex a(uint j[[vertex_id]],uint L[[instance_id]],constant uint&ja[[buffer(w7)]],constant SB&v[[buffer(N2)]],constant P*r[[buffer(0)]]T2){L+=ja;Y K;
#else
#define e1(a,P,r,j,L) __attribute__((visibility("default")))Y vertex a(uint j[[vertex_id]],uint L[[instance_id]],constant SB&v[[buffer(N2)]],constant P*r[[buffer(0)]]T2){Y K;
#endif
#define j5(a,P,r,j,L) __attribute__((visibility("default")))Y vertex a(uint j[[vertex_id]],constant SB&v[[buffer(N2)]],constant ZB&J[[buffer(v3)]],constant P*r[[buffer(0)]]T2){Y K;
#define n4(a,W1,X1,k2,l2,j) __attribute__((visibility("default")))Y vertex a(uint j[[vertex_id]],constant SB&v[[buffer(N2)]],constant ZB&J[[buffer(v3)]],constant W1*X1[[buffer(0)]],constant k2*l2[[buffer(1)]]){Y K;
#define f1(h6) K.p1=h6;}return K;
#define q2(i4,a) i4 __attribute__((visibility("default")))fragment a(Y K[[stage_in]]){
#define r2(l) return l;}
#define v4 ,d n0,a3 q1,Z2 L1
#define K2 ,n0,q1,L1
#ifdef RD
#define D1 struct H0{
#ifdef SD
#define C0(c,a) device uint*a[[buffer(c+w3),raster_order_group(0)]]
#define D0(c,a) device uint*a[[buffer(c+w3),raster_order_group(0)]]
#define J3(c,a) device atomic_uint*a[[buffer(c+w3),raster_order_group(0)]]
#else
#define C0(c,a) device uint*a[[buffer(c+w3)]]
#define D0(c,a) device uint*a[[buffer(c+w3)]]
#define J3(c,a) device atomic_uint*a[[buffer(c+w3)]]
#endif
#define E1 };
#define G2 ,H0 Z,uint M1
#define j1 ,Z,M1
#define N0(e) unpackUnorm4x8(Z.e[M1])
#define L0(e) Z.e[M1]
#define n3(e) atomic_load_explicit(&Z.e[M1],memory_order::memory_order_relaxed)
#define F0(e,l) Z.e[M1]=packUnorm4x8(l)
#define Z0(e,l) Z.e[M1]=(l)
#define o3(e,l) atomic_store_explicit(&Z.e[M1],l,memory_order::memory_order_relaxed)
#define d0(e)
#define C4(e)
#define A4(e,R0) atomic_fetch_max_explicit(&Z.e[M1],R0,memory_order::memory_order_relaxed)
#define B4(e,R0) atomic_fetch_add_explicit(&Z.e[M1],R0,memory_order::memory_order_relaxed)
#define m1
#define n1
#define j4(a) __attribute__((visibility("default")))fragment a(H0 Z,constant SB&v[[buffer(N2)]],Y K[[stage_in]],a3 q1,Z2 L1){d n0=K.p1.xy;y0 H=y0(metal::floor(n0));uint M1=H.y*v.f7+H.x;
#define d8(a) __attribute__((visibility("default")))fragment a(H0 Z,constant SB&v[[buffer(N2)]],constant ZB&J[[buffer(v3)]],Y K[[stage_in]],a3 q1,Z2 L1){d n0=K.p1.xy;y0 H=y0(metal::floor(n0));uint M1=H.y*v.f7+H.x;
#define Q1(a) void j4(a)
#define T3(a) void d8(a)
#define a2 }
#define m3(a) i j4(a){i P0;
#define y4(a) i d8(a){i P0;
#define U3 }return P0;a2
#else
#define D1 struct H0{
#define C0(c,a) [[color(c)]]i a
#define D0(c,a) [[color(c)]]uint a
#define J3 D0
#define E1 };
#define G2 ,thread H0&e2,thread H0&Z
#define j1 ,e2,Z
#define N0(e) e2.e
#define L0(e) e2.e
#define n3(e) L0
#define F0(e,l) Z.e=(l)
#define Z0(e,l) Z.e=(l)
#define o3(e) Z0
#define d0(e) Z.e=e2.e
#define C4(e) e2.e=Z.e
p uint e6(thread uint&n,uint x){uint K1=n;n=metal::max(K1,x);return K1;}
#define A4(e,R0) e6(Z.e,R0)
p uint g6(thread uint&n,uint x){uint K1=n;n=K1+x;return K1;}
#define B4(e,R0) g6(Z.e,R0)
#define m1
#define n1
#define j4(a,...) H0 __attribute__((visibility("default")))fragment a(__VA_ARGS__){d n0[[maybe_unused]]=K.p1.xy;H0 Z;
#define Q1(a,...) j4(a,H0 e2,Y K[[stage_in]],a3 q1,Z2 L1)
#define T3(a) j4(a,H0 e2,Y K[[stage_in]],a3 q1,Z2 L1,constant ZB&J[[buffer(v3)]])
#define a2 }return Z;
#define e8(a,...) struct ka{i la[[f(0)]];H0 Z;};ka __attribute__((visibility("default")))fragment a(__VA_ARGS__){d n0[[maybe_unused]]=K.p1.xy;i P0;H0 Z;
#define m3(a) e8(a,H0 e2,Y K[[stage_in]],a3 q1,Z2 L1)
#define y4(a) e8(a,H0 e2,Y K[[stage_in]],a3 q1,Z2 L1,__VA_ARGS__ constant ZB&J[[buffer(v3)]])
#define U3 }return{.la=P0,.Z=Z};
#endif
using namespace metal;template<int y1>p vec<uint,y1>floatBitsToUint(vec<float,y1>x){return as_type<vec<uint,y1>>(x);}template<int y1>p vec<int,y1>floatBitsToInt(vec<float,y1>x){return as_type<vec<int,y1>>(x);}p uint floatBitsToUint(float x){return as_type<uint>(x);}p int floatBitsToInt(float x){return as_type<int>(x);}template<int y1>p vec<float,y1>uintBitsToFloat(vec<uint,y1>x){return as_type<vec<float,y1>>(x);}p float uintBitsToFloat(uint x){return as_type<float>(x);}p l0 unpackHalf2x16(uint x){return as_type<l0>(x);}p uint packHalf2x16(l0 x){return as_type<uint>(x);}p i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}p uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}p A inverse(A c1){A i6=A(c1[1][1],-c1[0][1],-c1[1][0],c1[0][0]);float ma=(i6[0][0]*c1[0][0])+(i6[0][1]*c1[1][0]);return i6*(1/ma);}p O mix(O m,O b,Z7 f0){O f8;for(int k=0;k<3;++k)f8[k]=f0[k]?b[k]:m[k];return f8;}p d6 i5(O m,h b,O f0,h na,O j6,h V){return d6(m.x,m.y,m.z,b,f0.x,f0.y,f0.z,na,j6.x,j6.y,j6.z,V);}
)===";
} // namespace glsl
} // namespace pls
} // namespace rive