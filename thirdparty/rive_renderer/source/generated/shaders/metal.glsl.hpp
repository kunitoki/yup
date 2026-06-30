#pragma once

#include "metal.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char metal[] = R"===(#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define D half2
#define r half3
#define i half4
#define X ushort
#define c float2
#define V float3
#define I3 packed_float3
#define g float4
#define F4 bool2
#define n6 bool3
#define w7 bool4
#define W0 uint2
#define Q uint4
#define U int2
#define Z5 int4
#define X ushort
#define a0 float2x2
#define V6 half3x3
#define W6 half2x3
#define g5 half4x4
#endif
#define e inline
#define g1(g2) thread g2&
#define U4(g2) thread g2&
#define equal(A,F) ((A)==(F))
#define notEqual(A,F) ((A)!=(F))
#define lessThan(A,F) ((A)<(F))
#define greaterThan(A,F) ((A)>(F))
#define Z0(A,F) ((A)*(F))
#define inversesqrt rsqrt
#define l6(f,a) struct a{
#define v7(a) };
#define A1(a) struct a{
#define r0(f,W,a) W a
#define B1 };
#define v0(N8,G,a,W) W a=G[N8].a
#define h2 struct g0{
#define d0(f,W,a) W a
#define S4 [[flat]]
#define J0 [[center_no_perspective]]
#ifndef NB
#define NB
#endif
#define Z1 g L0[[position]][[invariant]];};
#define Y(a,W) thread W&a=R.a
#define l0(a)
#define B(a,W) W a=R.a
#define B4 struct T8{
#define C4 };
#define K3 struct z5{
#define L3 };
#define H5(f,y1,a) constant W0*a[[buffer(Q0(f))]]
#define G4(f,y1,a) constant Q*a[[buffer(Q0(f))]]
#define I5(f,y1,a) constant g*a[[buffer(Q0(f))]]
#define P0(a,y0) x2.a[y0]
#define J5(a,y0) x2.a[y0]
#define P3 struct U8{
#define Q3 };
#define y3 struct E3{
#define z3 };
#define Z4 struct E7{
#define a5 };
#define E4(N,f,a) [[texture(f)]]texture2d<uint>a
#define d5(N,f,a) [[texture(f)]]texture2d<float>a
#define U2(N,f,a) [[texture(f)]]texture2d<d>a
#define j5(N,f,a) [[texture(f)]]texture2d<d>a
#define e6(N,f,a) [[texture(f)]]texture1d_array<d>a
#define V3(x7,a) constexpr sampler a(filter::linear,mip_filter::none);
#define o6(N,f,a) [[sampler(f)]]sampler a;
#define S3(a) [[sampler(R3)]]sampler a;
#define F1(h0,l) M0.h0.read(W0(l))
#define p5(h0,p,l) M0.h0.sample(p,l)
#define m2(h0,p,l,X0) M0.h0.sample(p,l,level(X0))
#define q5(h0,p,l,P1) M0.h0.sample(p,l,bias(P1))
#define g8(h0,p,l) M0.h0.sample(L4.p,l)
#define Q6(h0,p,l,X0) M0.h0.sample(L4.p,l,level(X0))
#define y7(h0,p,l,P1) M0.h0.sample(L4.p,l,bias(P1))
#define U6(h0,p,q,p6,P8,X0) M0.h0.sample(p,q,p6)
#define h6 ,constant MB&k,U8 M0,T8 x2
#define p3 ,k,M0,x2
#ifdef AE
#define C1(a,c0,G,v,S) __attribute__((visibility("default")))g0 vertex a(uint v[[vertex_id]],uint S[[instance_id]],constant uint&vg[[buffer(Q0(uc))]],constant MB&k[[buffer(Q0(k3))]],constant c0*G[[buffer(0)]],U8 M0,T8 x2){S+=vg;g0 R;
#else
#define C1(a,c0,G,v,S) __attribute__((visibility("default")))g0 vertex a(uint v[[vertex_id]],uint S[[instance_id]],constant MB&k[[buffer(Q0(k3))]],constant c0*G[[buffer(0)]],U8 M0,T8 x2){g0 R;
#endif
#define S7(a,c0,G,v,S) __attribute__((visibility("default")))g0 vertex a(uint v[[vertex_id]],constant MB&k[[buffer(Q0(k3))]],constant LC&A0[[buffer(Q0(a6))]],constant c0*G[[buffer(0)]],U8 M0,T8 x2){g0 R;
#define E6(a,e3,f3,q3,r3,v) __attribute__((visibility("default")))g0 vertex a(uint v[[vertex_id]],constant MB&k[[buffer(Q0(k3))]],constant LC&A0[[buffer(Q0(a6))]],constant e3*f3[[buffer(0)]],constant q3*r3[[buffer(1)]]){g0 R;
#define D1(y5) R.L0=y5;}return R;
#define V2(Q1,a) Q1 __attribute__((visibility("default")))fragment a(g0 R[[stage_in]],E3 M0){
#define q6(Q1,a) Q1 __attribute__((visibility("default")))fragment a(g0 R[[stage_in]],E3 M0,bool r6[[front_facing]]){
#define F2(C) return C;}
#define G6 ,c T,E3 M0,z5 x2,E7 L4
#define R2 ,T,M0,x2,L4
#define C3 ,E3 M0
#define i1 ,M0
#define a7
#define r5
#ifdef FF
#define K1 struct n1{
#ifdef GF
#define p0(f,a) device uint*a[[buffer(Q0(f+c6)),raster_order_group(0)]]
#define f1(f,a) device uint*a[[buffer(Q0(f+c6)),raster_order_group(0)]]
#define o4(f,a) device atomic_uint*a[[buffer(Q0(f+c6)),raster_order_group(0)]]
#else
#define p0(f,a) device uint*a[[buffer(Q0(f+c6))]]
#define f1(f,a) device uint*a[[buffer(Q0(f+c6))]]
#define o4(f,a) device atomic_uint*a[[buffer(Q0(f+c6))]]
#endif
#define L1 };
#define r4 ,n1 R0,uint c1
#define U1 ,R0,c1
#define H0(h) unpackUnorm4x8(R0.h[c1])
#define e1(h) R0.h[c1]
#define N3(h) atomic_load_explicit(&R0.h[c1],memory_order::memory_order_relaxed)
#define C0(h,C) R0.h[c1]=packUnorm4x8(C)
#define h1(h,C) R0.h[c1]=(C)
#define O3(h,C) atomic_store_explicit(&R0.h[c1],C,memory_order::memory_order_relaxed)
#define r2(h)
#define X1(h)
#define O5(h,q) atomic_fetch_max_explicit(&R0.h[c1],q,memory_order::memory_order_relaxed)
#define Q5(h,q) atomic_fetch_add_explicit(&R0.h[c1],q,memory_order::memory_order_relaxed)
#define v2
#define w2
#define F7(a) __attribute__((visibility("default")))fragment a(n1 R0,constant MB&k[[buffer(Q0(k3))]],g0 R[[stage_in]],E3 M0,E7 L4,z5 x2){c T=R.L0.xy;W0 E=W0(metal::floor(T));uint c1=E.y*k.m6+E.x;
#define td(a) __attribute__((visibility("default")))fragment a(n1 R0,constant MB&k[[buffer(Q0(k3))]],constant LC&A0[[buffer(Q0(a6))]],g0 R[[stage_in]],E7 L4,E3 M0,z5 x2){c T=R.L0.xy;W0 E=W0(metal::floor(T));uint c1=E.y*k.m6+E.x;
#define v1(a) void F7(a)
#define M5(a) void td(a)
#define c2 }
#define o2(a) i F7(a){i m1;
#define v4(a) i td(a){i m1;
#define i3 }return m1;c2
#else
#define K1 struct n1{
#define p0(f,a) [[color(f)]]i a
#define f1(f,a) [[color(f)]]uint a
#define o4 f1
#define L1 };
#define r4 ,thread n1&e4,thread n1&R0
#define U1 ,e4,R0
#define H0(h) e4.h
#define e1(h) e4.h
#define N3(h) e1
#define C0(h,C) R0.h=(C)
#define h1(h,C) R0.h=(C)
#define O3(h) h1
#define r2(h) R0.h=e4.h
#define X1(h) R0.h=e4.h
e uint w5(thread uint&o0,uint x){uint a1=o0;o0=metal::max(a1,x);return a1;}
#define O5(h,q) w5(R0.h,q)
e uint x5(thread uint&o0,uint x){uint a1=o0;o0=a1+x;return a1;}
#define Q5(h,q) x5(R0.h,q)
#define v2
#define w2
#define F7(a,...) n1 __attribute__((visibility("default")))fragment a(__VA_ARGS__){c T[[maybe_unused]]=R.L0.xy;n1 R0;
#define v1(a,...) F7(a,n1 e4,constant MB&k[[buffer(Q0(k3))]],g0 R[[stage_in]],E7 L4,E3 M0,z5 x2)
#define M5(a) F7(a,n1 e4,constant MB&k[[buffer(Q0(k3))]],g0 R[[stage_in]],E3 M0,z5 x2,E7 L4,constant LC&A0[[buffer(Q0(a6))]])
#define c2 }return R0;
#define ud(a,...) struct wg{i xg[[j(0)]];n1 R0;};wg __attribute__((visibility("default")))fragment a(__VA_ARGS__){c T[[maybe_unused]]=R.L0.xy;i m1;n1 R0;
#define o2(a) ud(a,n1 e4,constant MB&k[[buffer(Q0(k3))]],g0 R[[stage_in]],E3 M0,z5 x2)
#define v4(a) ud(a,n1 e4,constant MB&k[[buffer(Q0(k3))]],g0 R[[stage_in]],E3 M0,z5 x2,__VA_ARGS__ constant LC&A0[[buffer(Q0(a6))]])
#define i3 }return{.xg=m1,.R0=R0};
#endif
#define m4 p0
#define discard discard_fragment()
using namespace metal;template<int O1>e vec<uint,O1>floatBitsToUint(vec<float,O1>x){return as_type<vec<uint,O1>>(x);}template<int O1>e vec<int,O1>floatBitsToInt(vec<float,O1>x){return as_type<vec<int,O1>>(x);}e uint floatBitsToUint(float x){return as_type<uint>(x);}e int floatBitsToInt(float x){return as_type<int>(x);}template<int O1>e vec<float,O1>uintBitsToFloat(vec<uint,O1>x){return as_type<vec<float,O1>>(x);}e float uintBitsToFloat(uint x){return as_type<float>(x);}e D unpackHalf2x16(uint x){return as_type<D>(x);}e uint packHalf2x16(D x){return as_type<uint>(x);}e i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}e uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}e a0 inverse(a0 o1){a0 Ha=a0(o1[1][1],-o1[0][1],-o1[1][0],o1[0][0]);float yg=(Ha[0][0]*o1[0][0])+(Ha[0][1]*o1[1][0]);return Ha*(1/yg);}e r mix(r o,r b,n6 J1){r G7;for(int D0=0;D0<3;++D0)G7[D0]=J1[D0]?b[D0]:o[D0];return G7;}e c mix(c o,c b,F4 J1){c G7;for(int D0=0;D0<2;++D0)G7[D0]=J1[D0]?b[D0]:o[D0];return G7;}e c mix(c o,c b,float t){return mix(o,b,c(t));}e float mod(float x,float y){return fmod(x,y);}
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive