#pragma once

#include "metal.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char metal[] = R"===(#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define C half2
#define p half3
#define i half4
#define G5 short
#define O ushort
#define c float2
#define a0 float3
#define R3 packed_float3
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
#define d inline
#define G1(D1) thread D1&
#define Y3(D1) thread D1&
#define notEqual(G,L) ((G)!=(L))
#define lessThanEqual(G,L) ((G)<=(L))
#define lessThan(G,L) ((G)<(L))
#define greaterThan(G,L) ((G)>(L))
#define greaterThanEqual(G,L) ((G)>=(L))
#define q0(G,L) ((G)*(L))
#define inversesqrt rsqrt
#define G4(f,a) struct a{
#define l5(a) };
#define L0(a) struct a{
#define h0(f,U,a) U a
#define M0 };
#define k0(N6,o,a,U) U a=o[N6].a
#define o1 struct j0{
#define H(f,U,a) U a
#define D2 [[flat]]
#define o0 [[center_no_perspective]]
#ifndef OB
#define OB
#endif
#define p1 e S0[[position]][[invariant]];};
#define P(a,U) thread U&a=V.a
#define X(a)
#define Z(a,U) U a=V.a
#define r3 struct R6{
#define v3 };
#define j3 struct n4{
#define k3 };
#define W3(f,R0,a) constant E0*a[[buffer(f)]]
#define I3(f,R0,a) constant D*a[[buffer(f)]]
#define X3(f,R0,a) constant e*a[[buffer(f)]]
#define r0(a,n0) K1.a[n0]
#define a4(a,n0) K1.a[n0]
#define H2 struct S6{
#define I2 };
#define J2 struct Z2{
#define L2 };
#define F3(p0,f,a) [[texture(f)]]texture2d<uint>a
#define S4(p0,f,a) [[texture(f)]]texture2d<float>a
#define o2(p0,f,a) [[texture(f)]]texture2d<h>a
#define G3(p0,f,a) [[texture(f)]]texture2d<h>a
#define H3(O2,a) constexpr sampler a(filter::linear,mip_filter::none);
#define h4(O2,a) constexpr sampler a(filter::linear,mip_filter::linear);
#define f1(a3,A) I0.a3.read(E0(A))
#define p3(a3,E,A) I0.a3.sample(E,A)
#define B8(hd,E,A,z2) hd.sample(E,A,level(z2))
#define S1(a3,E,A,z2) B8(I0.a3,E,A,z2)
#define F6(a3,E,A,T4) I0.a3.gather(E,(A)*(T4))
#define P4 ,constant VB&m,S6 I0,R6 K1
#define g2 ,m,I0,K1
#ifdef LD
#define Z0(a,e0,o,l,I) __attribute__((visibility("default")))j0 vertex a(uint l[[vertex_id]],uint I[[instance_id]],constant uint&id[[buffer(U9)]],constant VB&m[[buffer(w3)]],constant e0*o[[buffer(0)]],S6 I0,R6 K1){I+=id;j0 V;
#else
#define Z0(a,e0,o,l,I) __attribute__((visibility("default")))j0 vertex a(uint l[[vertex_id]],uint I[[instance_id]],constant VB&m[[buffer(w3)]],constant e0*o[[buffer(0)]],S6 I0,R6 K1){j0 V;
#endif
#define U5(a,e0,o,l,I) __attribute__((visibility("default")))j0 vertex a(uint l[[vertex_id]],constant VB&m[[buffer(w3)]],constant DC&l0[[buffer(H4)]],constant e0*o[[buffer(0)]],S6 I0,R6 K1){j0 V;
#define v4(a,P1,Q1,h2,i2,l) __attribute__((visibility("default")))j0 vertex a(uint l[[vertex_id]],constant VB&m[[buffer(w3)]],constant DC&l0[[buffer(H4)]],constant P1*Q1[[buffer(0)]],constant h2*i2[[buffer(1)]]){j0 V;
#define U0(L5) V.S0=L5;}return V;
#define T1(l4,a) l4 __attribute__((visibility("default")))fragment a(j0 V[[stage_in]],Z2 I0){
#define U1(r) return r;}
#define Z4 ,c v0,Z2 I0,n4 K1
#define o3 ,v0,I0,K1
#define x5 ,Z2 I0
#define z1 ,I0
#ifdef NE
#define k2 struct j1{
#ifdef OE
#define N0(f,a) device uint*a[[buffer(f+J4),raster_order_group(0)]]
#define P0(f,a) device uint*a[[buffer(f+J4),raster_order_group(0)]]
#define V3(f,a) device atomic_uint*a[[buffer(f+J4),raster_order_group(0)]]
#else
#define N0(f,a) device uint*a[[buffer(f+J4)]]
#define P0(f,a) device uint*a[[buffer(f+J4)]]
#define V3(f,a) device atomic_uint*a[[buffer(f+J4)]]
#endif
#define l2 };
#define l3 ,j1 x0,uint B2
#define v1 ,x0,B2
#define C0(g) unpackUnorm4x8(x0.g[B2])
#define V0(g) x0.g[B2]
#define d4(g) atomic_load_explicit(&x0.g[B2],memory_order::memory_order_relaxed)
#define H0(g,r) x0.g[B2]=packUnorm4x8(r)
#define W0(g,r) x0.g[B2]=(r)
#define e4(g,r) atomic_store_explicit(&x0.g[B2],r,memory_order::memory_order_relaxed)
#define p2(g)
#define X1(g)
#define d5(g,N) atomic_fetch_max_explicit(&x0.g[B2],N,memory_order::memory_order_relaxed)
#define e5(g,N) atomic_fetch_add_explicit(&x0.g[B2],N,memory_order::memory_order_relaxed)
#define W1
#define Y1
#define M5(a) __attribute__((visibility("default")))fragment a(j1 x0,constant VB&m[[buffer(w3)]],j0 V[[stage_in]],Z2 I0,n4 K1){c v0=V.S0.xy;E0 F=E0(metal::floor(v0));uint B2=F.y*m.C9+F.x;
#define Ha(a) __attribute__((visibility("default")))fragment a(j1 x0,constant VB&m[[buffer(w3)]],constant DC&l0[[buffer(H4)]],j0 V[[stage_in]],Z2 I0,n4 K1){c v0=V.S0.xy;E0 F=E0(metal::floor(v0));uint B2=F.y*m.C9+F.x;
#define n2(a) void M5(a)
#define z4(a) void Ha(a)
#define E2 }
#define n3(a) i M5(a){i A1;
#define c5(a) i Ha(a){i A1;
#define x4 }return A1;E2
#else
#define k2 struct j1{
#define N0(f,a) [[color(f)]]i a
#define P0(f,a) [[color(f)]]uint a
#define V3 P0
#define l2 };
#define l3 ,thread j1&c3,thread j1&x0
#define v1 ,c3,x0
#define C0(g) c3.g
#define V0(g) c3.g
#define d4(g) V0
#define H0(g,r) x0.g=(r)
#define W0(g,r) x0.g=(r)
#define e4(g) W0
#define p2(g) x0.g=c3.g
#define X1(g) x0.g=c3.g
d uint J5(thread uint&B,uint x){uint l1=B;B=metal::max(l1,x);return l1;}
#define d5(g,N) J5(x0.g,N)
d uint K5(thread uint&B,uint x){uint l1=B;B=l1+x;return l1;}
#define e5(g,N) K5(x0.g,N)
#define W1
#define Y1
#define M5(a,...) j1 __attribute__((visibility("default")))fragment a(__VA_ARGS__){c v0[[maybe_unused]]=V.S0.xy;j1 x0;
#define n2(a,...) M5(a,j1 c3,constant VB&m[[buffer(w3)]],j0 V[[stage_in]],Z2 I0,n4 K1)
#define z4(a) M5(a,j1 c3,j0 V[[stage_in]],Z2 I0,n4 K1,constant DC&l0[[buffer(H4)]])
#define E2 }return x0;
#define Ia(a,...) struct jd{i kd[[j(0)]];j1 x0;};jd __attribute__((visibility("default")))fragment a(__VA_ARGS__){c v0[[maybe_unused]]=V.S0.xy;i A1;j1 x0;
#define n3(a) Ia(a,j1 c3,j0 V[[stage_in]],Z2 I0,n4 K1)
#define c5(a) Ia(a,j1 c3,j0 V[[stage_in]],Z2 I0,n4 K1,__VA_ARGS__ constant DC&l0[[buffer(H4)]])
#define x4 }return{.kd=A1,.x0=x0};
#endif
#define discard discard_fragment()
using namespace metal;template<int i1>d vec<uint,i1>floatBitsToUint(vec<float,i1>x){return as_type<vec<uint,i1>>(x);}template<int i1>d vec<int,i1>floatBitsToInt(vec<float,i1>x){return as_type<vec<int,i1>>(x);}d uint floatBitsToUint(float x){return as_type<uint>(x);}d int floatBitsToInt(float x){return as_type<int>(x);}template<int i1>d vec<float,i1>uintBitsToFloat(vec<uint,i1>x){return as_type<vec<float,i1>>(x);}d float uintBitsToFloat(uint x){return as_type<float>(x);}d C unpackHalf2x16(uint x){return as_type<C>(x);}d uint packHalf2x16(C x){return as_type<uint>(x);}d i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}d uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}d Y inverse(Y O0){Y E8=Y(O0[1][1],-O0[0][1],-O0[1][0],O0[0][0]);float ld=(E8[0][0]*O0[0][0])+(E8[0][1]*O0[1][0]);return E8*(1/ld);}d p mix(p k,p b,L6 D0){p N5;for(int v=0;v<3;++v)N5[v]=D0[v]?b[v]:k[v];return N5;}d c mix(c k,c b,D4 D0){c N5;for(int v=0;v<2;++v)N5[v]=D0[v]?b[v]:k[v];return N5;}d float mod(float x,float y){return fmod(x,y);}
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive