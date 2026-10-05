#pragma once

#include "metal.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char metal[] = R"===(#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define C half2
#define v half3
#define i half4
#define Q ushort
#define c float2
#define O float3
#define c4 packed_float3
#define e float4
#define R4 bool2
#define B6 bool3
#define I7 bool4
#define O0 uint2
#define M uint4
#define e0 int2
#define m6 int4
#define Q ushort
#define Y float2x2
#define k7 half3x3
#define l7 half2x3
#define S4 half4x4
#endif
#define f inline
#define i1(w2) thread w2&
#define V6(w2) thread w2&
#define equal(B,J) ((B)==(J))
#define notEqual(B,J) ((B)!=(J))
#define lessThan(B,J) ((B)<(J))
#define greaterThan(B,J) ((B)>(J))
#define M0(B,J) ((B)*(J))
#define inversesqrt rsqrt
#define H7(g,a) struct a{
#define e9(a) };
#define c1(a) struct a{
#define K(g,j0,a) j0 a
#define d1 };
#define L(h9,D,a,j0) j0 a=D[h9].a
#define l2 struct v0{
#define V(g,j0,a) j0 a
#define Z2 [[flat]]
#define E0 [[center_no_perspective]]
#ifndef KB
#define KB
#endif
#define e2 e U0[[position]][[invariant]];};
#define T(a,j0) thread j0&a=g0.a
#define Z(a)
#define q(a,j0) j0 a=g0.a
#define P4 struct l9{
#define Q4 };
#define f4 struct m9{
#define g4 };
#define Z5(g,E1,a) constant O0*a[[buffer(V1(g))]]
#define W4(g,E1,a) constant M*a[[buffer(V1(g))]]
#define a6(g,E1,a) constant e*a[[buffer(V1(g))]]
#define p0(a,D0) E3.a[D0]
#define l5(a,D0) E3.a[D0]
#define j4 struct n9{
#define k4 };
#define N3 struct Q5{
#define O3 };
#define x5 struct vb{
#define y5 };
#define V4(c0,g,a) [[texture(g)]]texture2d<uint>a
#define C6(c0,g,a) [[texture(g)]]texture2d<float>a
#define i3(c0,g,a) [[texture(g)]]texture2d<d>a
#define D5(c0,g,a) [[texture(g)]]texture2d<d>a
#define q6(c0,g,a) [[texture(g)]]texture1d_array<d>a
#define o4(J7,a) constexpr sampler a(filter::linear,mip_filter::none);
#define D6(c0,g,a) [[sampler(g)]]sampler a;
#define m4(a) [[sampler(l4)]]sampler a;
#define p1(q0,l) f1.q0.read(O0(l))
#define K5(q0,o,l) f1.q0.sample(o,l)
#define o2(q0,o,l,Y0) f1.q0.sample(o,l,level(Y0))
#define L5(q0,o,l,Y1) f1.q0.sample(o,l,bias(Y1))
#define w8(q0,o,l) f1.q0.sample(J6.o,l)
#define i6(q0,o,l,Y0) f1.q0.sample(J6.o,l,level(Y0))
#define K7(q0,o,l,Y1) f1.q0.sample(J6.o,l,bias(Y1))
#define j7(q0,o,F,E6,j9,Y0) f1.q0.sample(o,F,E6)
#define w6 ,constant UB&j,n9 f1,l9 E3
#define G3 ,j,f1,E3
#ifdef CE
#define v1(a,d0,D,G,r) __attribute__((visibility("default"))) v0 vertex a(uint G[[vertex_id]],uint r[[instance_id]],constant uint&Yh[[buffer(V1(wd))]],constant UB&j[[buffer(V1(U4))]],constant d0*D[[buffer(0)]],n9 f1,l9 E3){r+=Yh;v0 g0;
#else
#define v1(a,d0,D,G,r) __attribute__((visibility("default"))) v0 vertex a(uint G[[vertex_id]],uint r[[instance_id]],constant UB&j[[buffer(V1(U4))]],constant d0*D[[buffer(0)]],n9 f1,l9 E3){v0 g0;
#endif
#define h8(a,d0,D,A1,h0,G,r) __attribute__((visibility("default"))) v0 vertex a(uint G[[vertex_id]],uint r[[instance_id]],constant UB&j[[buffer(V1(U4))]],constant d0*D[[buffer(0)]],const device A1*h0[[buffer(2)]],n9 f1,l9 E3){v0 g0;
#define T6(a,w3,x3,J3,K3,A1,h0,G) __attribute__((visibility("default"))) v0 vertex a(uint G[[vertex_id]],uint r[[instance_id]],constant UB&j[[buffer(V1(U4))]],constant w3*x3[[buffer(0)]],constant J3*K3[[buffer(1)]],const device A1*h0[[buffer(2)]]){v0 g0;
#define w1(P5) g0.U0=P5;}return g0;
#define j3(F1,a) F1 __attribute__((visibility("default"))) fragment a(v0 g0[[stage_in]],Q5 f1){
#define G6(F1,a) F1 __attribute__((visibility("default"))) fragment a(v0 g0[[stage_in]],Q5 f1,bool H6[[front_facing]]){
#define P2(E) return E;}
#define W6 ,constant UB&j,c f0,Q5 f1,m9 E3,vb J6
#define e3 ,j,f0,f1,E3,J6
#define R3 ,Q5 f1
#define k1 ,f1
#define p7
#define Y4
#ifdef VF
#define R1 struct a2{
#ifdef WF
#define B0(g,a) device uint*a[[buffer(V1(g+n6)),raster_order_group(0)]]
#define o1(g,a) device uint*a[[buffer(V1(g+n6)),raster_order_group(0)]]
#define L2(g,a) device atomic_uint*a[[buffer(V1(g+n6)),raster_order_group(0)]]
#else
#define B0(g,a) device uint*a[[buffer(V1(g+n6))]]
#define o1(g,a) device uint*a[[buffer(V1(g+n6))]]
#define L2(g,a) device atomic_uint*a[[buffer(V1(g+n6))]]
#endif
#define S1 };
#define h4 ,a2 Z0,uint K0
#define U1 ,Z0,K0
#define N0(h) unpackUnorm4x8(Z0.h[K0])
#define h1(h) Z0.h[K0]
#define f3(h) atomic_load_explicit(&Z0.h[K0],memory_order::memory_order_relaxed)
#define y0(h,E) Z0.h[K0]=packUnorm4x8(E)
#define j1(h,E) Z0.h[K0]=(E)
#define g3(h,E) atomic_store_explicit(&Z0.h[K0],E,memory_order::memory_order_relaxed)
#define D2(h)
#define Z1(h)
#define p5(h,F) atomic_fetch_max_explicit(&Z0.h[K0],F,memory_order::memory_order_relaxed)
#define q5(h,F) atomic_fetch_add_explicit(&Z0.h[K0],F,memory_order::memory_order_relaxed)
#define E2
#define F2
#define o9(a) __attribute__((visibility("default"))) fragment a(a2 Z0,constant UB&j[[buffer(V1(U4))]],v0 g0[[stage_in]],Q5 f1,vb J6,m9 E3){c f0=g0.U0.xy;O0 H=O0(metal::floor(f0));uint K0=H.y*j.A6+H.x;
#define T1(a) void o9(a)
#define h2 }
#define z2(a) i o9(a){i K1;
#define z3 }return K1;h2
#else
#define R1 struct a2{
#define B0(g,a) [[color(g)]]i a
#define o1(g,a) [[color(g)]]uint a
#define L2 o1
#define S1 };
#define h4 ,thread a2&R5,thread a2&Z0
#define U1 ,R5,Z0
#define N0(h) R5.h
#define h1(h) R5.h
#define f3(h) h1
#define y0(h,E) Z0.h=(E)
#define j1(h,E) Z0.h=(E)
#define g3(h) j1
#define D2(h) Z0.h=R5.h
#define Z1(h) Z0.h=R5.h
f uint N5(thread uint&x0,uint x){uint e1=x0;x0=metal::max(e1,x);return e1;}
#define p5(h,F) N5(Z0.h,F)
f uint O5(thread uint&x0,uint x){uint e1=x0;x0=e1+x;return e1;}
#define q5(h,F) O5(Z0.h,F)
#define E2
#define F2
#define o9(a,...) a2 __attribute__((visibility("default"))) fragment a(__VA_ARGS__){c f0[[maybe_unused]]=g0.U0.xy;a2 Z0;
#define T1(a,...) o9(a,a2 R5,constant UB&j[[buffer(V1(U4))]],v0 g0[[stage_in]],vb J6,Q5 f1,m9 E3)
#define h2 }return Z0;
#define bi(a,...) struct Zh{i ai[[p(0)]];a2 Z0;};Zh __attribute__((visibility("default"))) fragment a(__VA_ARGS__){c f0[[maybe_unused]]=g0.U0.xy;i K1;a2 Z0;
#define z2(a) bi(a,a2 R5,constant UB&j[[buffer(V1(U4))]],v0 g0[[stage_in]],Q5 f1,m9 E3)
#define z3 }return{.ai=K1,.Z0=Z0};
#endif
#define I4 B0
#define discard discard_fragment()
using namespace metal;template<int X1>f vec<uint,X1>floatBitsToUint(vec<float,X1>x){return as_type<vec<uint,X1>>(x);}template<int X1>f vec<int,X1>floatBitsToInt(vec<float,X1>x){return as_type<vec<int,X1>>(x);}f uint floatBitsToUint(float x){return as_type<uint>(x);}f int floatBitsToInt(float x){return as_type<int>(x);}template<int X1>f vec<float,X1>uintBitsToFloat(vec<uint,X1>x){return as_type<vec<float,X1>>(x);}f float uintBitsToFloat(uint x){return as_type<float>(x);}f C unpackHalf2x16(uint x){return as_type<C>(x);}f uint packHalf2x16(C x){return as_type<uint>(x);}f i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}f uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}f c unpackUnorm2x16(uint x){return unpack_unorm2x16_to_float(x);}f Y inverse(Y y1){Y wb=Y(y1[1][1],-y1[0][1],-y1[1][0],y1[0][0]);float ci=(wb[0][0]*y1[0][0])+(wb[0][1]*y1[1][0]);return wb*(1/ci);}f v mix(v k,v b,B6 O1){v Q7;for(int L0=0;L0<3;++L0) Q7[L0]=O1[L0]?b[L0]:k[L0];return Q7;}f c mix(c k,c b,R4 O1){c Q7;for(int L0=0;L0<2;++L0) Q7[L0]=O1[L0]?b[L0]:k[L0];return Q7;}f c mix(c k,c b,float t){return mix(k,b,c(t));}f float mod(float x,float y){return fmod(x,y);}
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive