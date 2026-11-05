#pragma once

#include "metal.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char metal[] = R"===(#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define F half2
#define v half3
#define i half4
#define O ushort
#define c float2
#define M float3
#define l4 packed_float3
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
#define f inline
#define p1(H2) thread H2&
#define p7(H2) thread H2&
#define equal(E,J) ((E)==(J))
#define notEqual(E,J) ((E)!=(J))
#define lessThan(E,J) ((E)<(J))
#define greaterThan(E,J) ((E)>(J))
#define G0(E,J) ((E)*(J))
#define inversesqrt rsqrt
#define k8(g,a) struct a{
#define T9(a) };
#define h1(a) struct a{
#define K(g,k0,a) k0 a
#define i1 };
#define L(W9,A,a,k0) k0 a=A[W9].a
#define o2 struct l0{
#define W(g,k0,a) k0 a
#define l3 [[flat]]
#define I0 [[center_no_perspective]]
#ifndef NB
#define NB
#endif
#define P1 e T0[[position]][[invariant]];};
#define V(a,k0) thread k0&a=a0.a
#define e0(a)
#define q(a,k0) k0 a=a0.a
#define i5 struct x8{
#define j5 };
#define n4 struct y8{
#define p4 };
#define o4(g,a1,a) constant R*a[[buffer(f1(g))]]
#define c8(g,a1,a) constant uint*a[[buffer(f1(g))]]
#define p6(g,a1,a) constant e*a[[buffer(f1(g))]]
#define p0(a,m0) T2.a[m0]
#define Q5(a,m0) T2.a[m0]
#define A4 struct z8{
#define B4 };
#define X3 struct y5{
#define Y3 };
#define K5 struct ca{
#define L5 };
#define v5(h0,g,a) [[texture(g)]]texture2d<uint>a
#define Z6(h0,g,a) [[texture(g)]]texture2d<float>a
#define Z2(h0,g,a) [[texture(g)]]texture2d<d>a
#define U5(h0,g,a) [[texture(g)]]texture2d<d>a
#define P6(h0,g,a) [[texture(g)]]texture1d_array<d>a
#define f4(Y5,a) constexpr sampler a(filter::linear,mip_filter::none);
#define Q6(Y5,a) constexpr sampler a(filter::linear,mip_filter::none,address::repeat);
#define a7(h0,g,a) [[sampler(g)]]sampler a;
#define D4(a) [[sampler(C4)]]sampler a;
#define j1(v0,o) c1.v0.read(R0(o))
#define Z5(v0,p,o) c1.v0.sample(p,o)
#define r2(v0,p,o,g1) c1.v0.sample(p,o,level(g1))
#define a6(v0,p,o,j2) c1.v0.sample(p,o,bias(j2))
#define g9(v0,p,o) c1.v0.sample(f6.p,o)
#define k5(v0,p,o,g1) c1.v0.sample(f6.p,o,level(g1))
#define m8(v0,p,o,j2) c1.v0.sample(f6.p,o,bias(j2))
#define F7(v0,p,G,c7,Y9,g1) c1.v0.sample(p,G,c7)
#define T6 ,constant RB&j,z8 c1,x8 T2
#define S3 ,j,c1,T2
#ifdef ND
#define n1(a,d0,A,C,r) __attribute__((visibility("default"))) l0 vertex a(uint g3[[vertex_id]],uint r[[instance_id]],constant uint&mj[[buffer(f1(P7))]],constant RB&j[[buffer(f1(a4))]],z8 c1,x8 T2){uint C=g3+mj;l0 a0;
#elif defined(HE)
#define n1(a,d0,A,C,r) __attribute__((visibility("default"))) l0 vertex a(uint C[[vertex_id]],uint r[[instance_id]],constant uint&nj[[buffer(f1(P7))]],constant RB&j[[buffer(f1(a4))]],constant d0*A[[buffer(0)]],z8 c1,x8 T2){r+=nj;l0 a0;
#else
#define n1(a,d0,A,C,r) __attribute__((visibility("default"))) l0 vertex a(uint C[[vertex_id]],uint r[[instance_id]],constant RB&j[[buffer(f1(a4))]],constant d0*A[[buffer(0)]],z8 c1,x8 T2){l0 a0;
#endif
#define O8(a,d0,A,H1,j0,C,r) __attribute__((visibility("default"))) l0 vertex a(uint C[[vertex_id]],uint r[[instance_id]],constant RB&j[[buffer(f1(a4))]],constant d0*A[[buffer(0)]],const device H1*j0[[buffer(2)]],z8 c1,x8 T2){l0 a0;
#define n7(a,G3,H3,V3,n3,H1,j0,C) __attribute__((visibility("default"))) l0 vertex a(uint C[[vertex_id]],uint r[[instance_id]],constant RB&j[[buffer(f1(a4))]],constant G3*H3[[buffer(0)]],constant V3*n3[[buffer(1)]],const device H1*j0[[buffer(2)]]){l0 a0;
#define C1(e6) a0.T0=e6;}return a0;
#if defined(BB)&&!defined(CF)
#if defined(DB)||defined(FB)
#define uf ,y8 T2
#else
#define uf
#endif
#ifdef LB
#ifdef DF
#define L4(g,a) device uint*a[[buffer(f1(g+o5)),raster_order_group(0)]]
#else
#define L4(g,a) device uint*a[[buffer(f1(g+o5))]]
#endif
f i vf(i k,uint oj){return oj!=0u?k.zyxw:k;}
#define e4(h) vf(unpackUnorm4x8(W0.h[E0]),wf)
#define r5(h,B) W0.h[E0]=packUnorm4x8(vf(B,wf))
#define Cc(U0) void
#define da ,V1 W0,constant uint&wf[[buffer(f1(P7))]]
#define Dc R0 D=R0(metal::floor(Y));uint E0=D.y*j.X5+D.x;
#define w2(B) }
#elif defined(CG)
#define K4(a) Z2(O2,ve,a)
#define J4(a) j1(a,Y)
#define da
#define Cc(U0) U0
#define Dc
#else
#define K4(a)
#define J4(a) pj
#if defined(H)&&!defined(Q)
#define da ,i pj[[color(0)]]
#else
#define da
#endif
#define Cc(U0) U0
#define Dc
#endif
#define v2(U0,a) [[early_fragment_tests]]Cc(U0) __attribute__((visibility("default"))) fragment a(l0 a0[[stage_in]],constant RB&j[[buffer(f1(a4))]],y5 c1,ca f6 uf da){c Y[[maybe_unused]]=a0.T0.xy;Dc
#else
#define v2(U0,a) U0 __attribute__((visibility("default"))) fragment a(l0 a0[[stage_in]],y5 c1){
#endif
#define d7(U0,a) U0 __attribute__((visibility("default"))) fragment a(l0 a0[[stage_in]],y5 c1,bool e7[[front_facing]]){
#ifndef w2
#define w2(B) return B;}
#endif
#define q7 ,constant RB&j,c Y,y5 c1,y8 T2,ca f6
#define q3 ,j,Y,c1,T2,f6
#define g4 ,y5 c1
#define r1 ,c1
#define C6
#define I4
#ifdef DG
#define I1 struct V1{
#ifdef DF
#define z0(g,a) device uint*a[[buffer(f1(g+o5)),raster_order_group(0)]]
#define x1(g,a) device uint*a[[buffer(f1(g+o5)),raster_order_group(0)]]
#define Y2(g,a) device atomic_uint*a[[buffer(f1(g+o5)),raster_order_group(0)]]
#else
#define z0(g,a) device uint*a[[buffer(f1(g+o5))]]
#define x1(g,a) device uint*a[[buffer(f1(g+o5))]]
#define Y2(g,a) device atomic_uint*a[[buffer(f1(g+o5))]]
#endif
#define J1 };
#define r4 ,V1 W0,uint E0
#define d2 ,W0,E0
#define L0(h) unpackUnorm4x8(W0.h[E0])
#define o1(h) W0.h[E0]
#define r3(h) atomic_load_explicit(&W0.h[E0],memory_order::memory_order_relaxed)
#define y0(h,B) W0.h[E0]=packUnorm4x8(B)
#define q1(h,B) W0.h[E0]=(B)
#define v3(h,B) atomic_store_explicit(&W0.h[E0],B,memory_order::memory_order_relaxed)
#define Q2(h)
#define l2(h)
#define H5(h,G) atomic_fetch_max_explicit(&W0.h[E0],G,memory_order::memory_order_relaxed)
#define I5(h,G) atomic_fetch_add_explicit(&W0.h[E0],G,memory_order::memory_order_relaxed)
#define R2
#define S2
#define ea(a) __attribute__((visibility("default"))) fragment a(V1 W0,constant RB&j[[buffer(f1(a4))]],l0 a0[[stage_in]],y5 c1,ca f6,y8 T2){c Y=a0.T0.xy;R0 D=R0(metal::floor(Y));uint E0=D.y*j.X5+D.x;
#define E1(a) void ea(a)
#define L1 }
#define K2(a) i ea(a){i S1;
#define I3 }return S1;L1
#else
#define I1 struct V1{
#define z0(g,a) [[color(g)]]i a
#define x1(g,a) [[color(g)]]uint a
#define Y2 x1
#define J1 };
#define r4 ,thread V1&g6,thread V1&W0
#define d2 ,g6,W0
#define L0(h) g6.h
#define o1(h) g6.h
#define r3(h) o1
#define y0(h,B) W0.h=(B)
#define q1(h,B) W0.h=(B)
#define v3(h) q1
#define Q2(h) W0.h=g6.h
#define l2(h) W0.h=g6.h
f uint c6(thread uint&C0,uint x){uint l1=C0;C0=metal::max(l1,x);return l1;}
#define H5(h,G) c6(W0.h,G)
f uint d6(thread uint&C0,uint x){uint l1=C0;C0=l1+x;return l1;}
#define I5(h,G) d6(W0.h,G)
#define R2
#define S2
#define ea(a,...) V1 __attribute__((visibility("default"))) fragment a(__VA_ARGS__){c Y[[maybe_unused]]=a0.T0.xy;V1 W0;
#if defined(BB)&&defined(LB)&&!defined(CF)
#define E1(a) v2(void,a)
#define L1 }
#else
#define E1(a,...) ea(a,V1 g6,constant RB&j[[buffer(f1(a4))]],l0 a0[[stage_in]],ca f6,y5 c1,y8 T2)
#define L1 }return W0;
#endif
#define sj(a,...) struct qj{i rj[[k(0)]];V1 W0;};qj __attribute__((visibility("default"))) fragment a(__VA_ARGS__){c Y[[maybe_unused]]=a0.T0.xy;i S1;V1 W0;
#define K2(a) sj(a,V1 g6,constant RB&j[[buffer(f1(a4))]],l0 a0[[stage_in]],y5 c1,y8 T2)
#define I3 }return{.rj=S1,.W0=W0};
#endif
#define c5 z0
#define discard discard_fragment()
using namespace metal;template<int i2>f vec<uint,i2>floatBitsToUint(vec<float,i2>x){return as_type<vec<uint,i2>>(x);}template<int i2>f vec<int,i2>floatBitsToInt(vec<float,i2>x){return as_type<vec<int,i2>>(x);}f uint floatBitsToUint(float x){return as_type<uint>(x);}f int floatBitsToInt(float x){return as_type<int>(x);}template<int i2>f vec<float,i2>uintBitsToFloat(vec<uint,i2>x){return as_type<vec<float,i2>>(x);}f float uintBitsToFloat(uint x){return as_type<float>(x);}f F unpackHalf2x16(uint x){return as_type<F>(x);}f uint packHalf2x16(F x){return as_type<uint>(x);}f i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}f uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}f c unpackUnorm2x16(uint x){return unpack_unorm2x16_to_float(x);}f X inverse(X F1){X Ec=X(F1[1][1],-F1[0][1],-F1[1][0],F1[0][0]);float tj=(Ec[0][0]*F1[0][0])+(Ec[0][1]*F1[1][0]);return Ec*(1/tj);}f v mix(v l,v b,Y6 Y1){v A8;for(int P0=0;P0<3;++P0) A8[P0]=Y1[P0]?b[P0]:l[P0];return A8;}f c mix(c l,c b,m5 Y1){c A8;for(int P0=0;P0<2;++P0) A8[P0]=Y1[P0]?b[P0]:l[P0];return A8;}f c mix(c l,c b,float t){return mix(l,b,c(t));}f float mod(float x,float y){return fmod(x,y);}
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive