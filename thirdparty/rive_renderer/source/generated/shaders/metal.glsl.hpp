#pragma once

#include "metal.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char metal[] = R"===(#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define g half
#define G half2
#define A half3
#define i half4
#define a0 ushort
#define c float2
#define Z float3
#define a4 packed_float3
#define f float4
#define a5 bool2
#define G7 bool3
#define N0 uint2
#define M uint4
#define c0 int2
#define h7 int4
#define a0 ushort
#define S float2x2
#define U5 half3x3
#define V5 half2x3
#endif
#define d inline
#define k1(P1) thread P1&
#define i4(P1) thread P1&
#define equal(o,r) ((o)==(r))
#define notEqual(o,r) ((o)!=(r))
#define lessThan(o,r) ((o)<(r))
#define C0(o,r) ((o)*(r))
#define inversesqrt rsqrt
#define e5(e,a) struct a{
#define W5(a) };
#define U0(a) struct a{
#define i0(e,W,a) W a
#define V0 };
#define l0(H7,B,a,W) W a=B[H7].a
#define o1 struct k0{
#define H(e,W,a) W a
#define L2 [[flat]]
#define n0 [[center_no_perspective]]
#ifndef OB
#define OB
#endif
#define p1 f g1[[position]][[invariant]];};
#define L(a,W) thread W&a=X.a
#define P(a)
#define N(a,W) W a=X.a
#define E3 struct M7{
#define F3 };
#define w3 struct H4{
#define x3 };
#define g4(e,f1,a) constant N0*a[[buffer(P0(e))]]
#define O3(e,f1,a) constant M*a[[buffer(P0(e))]]
#define h4(e,f1,a) constant f*a[[buffer(P0(e))]]
#define w0(a,v0) W1.a[v0]
#define k4(a,v0) W1.a[v0]
#define P2 struct N7{
#define Q2 };
#define R2 struct l3{
#define S2 };
#define p4 struct x6{
#define q4 };
#define I3(g0,e,a) [[texture(e)]]texture2d<uint>a
#define r4(g0,e,a) [[texture(e)]]texture2d<float>a
#define C2(g0,e,a) [[texture(e)]]texture2d<g>a
#define y4(g0,e,a) [[texture(e)]]texture2d<g>a
#define l5(g0,e,a) [[texture(e)]]texture1d_array<g>a
#define P3(m2,a) constexpr sampler a(filter::linear,mip_filter::none);
#define G3(Ae,a) [[sampler(Ae)]]sampler a;
#define d1(A0,l) J0.A0.read(N0(l))
#define D4(A0,p,l) J0.A0.sample(p,l)
#define T1(A0,p,l,G0) J0.A0.sample(p,l,level(G0))
#define p5(A0,p,l,X2) J0.A0.gather(p,(l)*(X2))
#define o4(A0,p,l) J0.A0.sample(I4.p,l)
#define C7(A0,p,l,G0) J0.A0.sample(I4.p,l,level(G0))
#define T5(A0,p,m,w5,K7,G0) J0.A0.sample(p,m,w5)
#define q5 ,constant VB&q,N7 J0,M7 W1
#define Y1 ,q,J0,W1
#ifdef OD
#define q1(a,f0,B,n,K) __attribute__((visibility("default")))k0 vertex a(uint n[[vertex_id]],uint K[[instance_id]],constant uint&Be[[buffer(P0(Ua))]],constant VB&q[[buffer(P0(K3))]],constant f0*B[[buffer(0)]],N7 J0,M7 W1){K+=Be;k0 X;
#else
#define q1(a,f0,B,n,K) __attribute__((visibility("default")))k0 vertex a(uint n[[vertex_id]],uint K[[instance_id]],constant VB&q[[buffer(P0(K3))]],constant f0*B[[buffer(0)]],N7 J0,M7 W1){k0 X;
#endif
#define G6(a,f0,B,n,K) __attribute__((visibility("default")))k0 vertex a(uint n[[vertex_id]],constant VB&q[[buffer(P0(K3))]],constant EC&m0[[buffer(P0(f5))]],constant f0*B[[buffer(0)]],N7 J0,M7 W1){k0 X;
#define N4(a,a2,c2,v2,w2,n) __attribute__((visibility("default")))k0 vertex a(uint n[[vertex_id]],constant VB&q[[buffer(P0(K3))]],constant EC&m0[[buffer(P0(f5))]],constant a2*c2[[buffer(0)]],constant v2*w2[[buffer(1)]]){k0 X;
#define h1(w6) X.g1=w6;}return X;
#define e2(E4,a) E4 __attribute__((visibility("default")))fragment a(k0 X[[stage_in]],l3 J0){
#define f2(F) return F;}
#define C5 ,c y0,l3 J0,H4 W1,x6 I4
#define Y2 ,y0,J0,W1,I4
#define n5 ,l3 J0
#define x1 ,J0
#ifdef LE
#define x2 struct z1{
#ifdef ME
#define M0(e,a) device uint*a[[buffer(P0(e+g5)),raster_order_group(0)]]
#define Y0(e,a) device uint*a[[buffer(P0(e+g5)),raster_order_group(0)]]
#define f4(e,a) device atomic_uint*a[[buffer(P0(e+g5)),raster_order_group(0)]]
#else
#define M0(e,a) device uint*a[[buffer(P0(e+g5))]]
#define Y0(e,a) device uint*a[[buffer(P0(e+g5))]]
#define f4(e,a) device atomic_uint*a[[buffer(P0(e+g5))]]
#endif
#define y2 };
#define y3 ,z1 E0,uint K2
#define G1 ,E0,K2
#define I0(h) unpackUnorm4x8(E0.h[K2])
#define j1(h) E0.h[K2]
#define m4(h) atomic_load_explicit(&E0.h[K2],memory_order::memory_order_relaxed)
#define T0(h,F) E0.h[K2]=packUnorm4x8(F)
#define l1(h,F) E0.h[K2]=(F)
#define n4(h,F) atomic_store_explicit(&E0.h[K2],F,memory_order::memory_order_relaxed)
#define E2(h)
#define i2(h)
#define G5(h,m) atomic_fetch_max_explicit(&E0.h[K2],m,memory_order::memory_order_relaxed)
#define H5(h,m) atomic_fetch_add_explicit(&E0.h[K2],m,memory_order::memory_order_relaxed)
#define h2
#define j2
#define y6(a) __attribute__((visibility("default")))fragment a(z1 E0,constant VB&q[[buffer(P0(K3))]],k0 X[[stage_in]],l3 J0,x6 I4,H4 W1){c y0=X.g1.xy;N0 I=N0(metal::floor(y0));uint K2=I.y*q.Ga+I.x;
#define Rb(a) __attribute__((visibility("default")))fragment a(z1 E0,constant VB&q[[buffer(P0(K3))]],constant EC&m0[[buffer(P0(f5))]],k0 X[[stage_in]],x6 I4,l3 J0,H4 W1){c y0=X.g1.xy;N0 I=N0(metal::floor(y0));uint K2=I.y*q.Ga+I.x;
#define z2(a) void y6(a)
#define R4(a) void Rb(a)
#define M2 }
#define A3(a) i y6(a){i K1;
#define F5(a) i Rb(a){i K1;
#define P4 }return K1;M2
#else
#define x2 struct z1{
#define M0(e,a) [[color(e)]]i a
#define Y0(e,a) [[color(e)]]uint a
#define f4 Y0
#define y2 };
#define y3 ,thread z1&m3,thread z1&E0
#define G1 ,m3,E0
#define I0(h) m3.h
#define j1(h) m3.h
#define m4(h) j1
#define T0(h,F) E0.h=(F)
#define l1(h,F) E0.h=(F)
#define n4(h) l1
#define E2(h) E0.h=m3.h
#define i2(h) E0.h=m3.h
d uint r6(thread uint&h0,uint x){uint B1=h0;h0=metal::max(B1,x);return B1;}
#define G5(h,m) r6(E0.h,m)
d uint v6(thread uint&h0,uint x){uint B1=h0;h0=B1+x;return B1;}
#define H5(h,m) v6(E0.h,m)
#define h2
#define j2
#define y6(a,...) z1 __attribute__((visibility("default")))fragment a(__VA_ARGS__){c y0[[maybe_unused]]=X.g1.xy;z1 E0;
#define z2(a,...) y6(a,z1 m3,constant VB&q[[buffer(P0(K3))]],k0 X[[stage_in]],x6 I4,l3 J0,H4 W1)
#define R4(a) y6(a,z1 m3,k0 X[[stage_in]],l3 J0,H4 W1,x6 I4,constant EC&m0[[buffer(P0(f5))]])
#define M2 }return E0;
#define Sb(a,...) struct Ce{i De[[j(0)]];z1 E0;};Ce __attribute__((visibility("default")))fragment a(__VA_ARGS__){c y0[[maybe_unused]]=X.g1.xy;i K1;z1 E0;
#define A3(a) Sb(a,z1 m3,k0 X[[stage_in]],l3 J0,H4 W1)
#define F5(a) Sb(a,z1 m3,k0 X[[stage_in]],l3 J0,H4 W1,__VA_ARGS__ constant EC&m0[[buffer(P0(f5))]])
#define P4 }return{.De=K1,.E0=E0};
#endif
#define O4 M0
#define discard discard_fragment()
using namespace metal;template<int y1>d vec<uint,y1>floatBitsToUint(vec<float,y1>x){return as_type<vec<uint,y1>>(x);}template<int y1>d vec<int,y1>floatBitsToInt(vec<float,y1>x){return as_type<vec<int,y1>>(x);}d uint floatBitsToUint(float x){return as_type<uint>(x);}d int floatBitsToInt(float x){return as_type<int>(x);}template<int y1>d vec<float,y1>uintBitsToFloat(vec<uint,y1>x){return as_type<vec<float,y1>>(x);}d float uintBitsToFloat(uint x){return as_type<float>(x);}d G unpackHalf2x16(uint x){return as_type<G>(x);}d uint packHalf2x16(G x){return as_type<uint>(x);}d i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}d uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}d S inverse(S W0){S C9=S(W0[1][1],-W0[0][1],-W0[1][0],W0[0][0]);float Ee=(C9[0][0]*W0[0][0])+(C9[0][1]*W0[1][0]);return C9*(1/Ee);}d A mix(A k,A b,G7 B0){A z6;for(int C=0;C<3;++C)z6[C]=B0[C]?b[C]:k[C];return z6;}d c mix(c k,c b,a5 B0){c z6;for(int C=0;C<2;++C)z6[C]=B0[C]?b[C]:k[C];return z6;}d c mix(c k,c b,float t){return mix(k,b,c(t));}d float mod(float x,float y){return fmod(x,y);}
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive