#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define D half2
#define r half3
#define i half4
#define X ushort
#define c float2
#define V float3
#define L3 packed_float3
#define g float4
#define E4 bool2
#define n6 bool3
#define w7 bool4
#define W0 uint2
#define Q uint4
#define U int2
#define Z5 int4
#define X ushort
#define Z float2x2
#define V6 half3x3
#define W6 half2x3
#define h5 half4x4
#endif
#define e inline
#define e1(g2) thread g2&
#define T4(g2) thread g2&
#define equal(A,F) ((A)==(F))
#define notEqual(A,F) ((A)!=(F))
#define lessThan(A,F) ((A)<(F))
#define greaterThan(A,F) ((A)>(F))
#define Z0(A,F) ((A)*(F))
#define inversesqrt rsqrt
#define m6(f,a) struct a{
#define v7(a) };
#define A1(a) struct a{
#define p0(f,W,a) W a
#define B1 };
#define q0(O8,G,a,W) W a=G[O8].a
#define h2 struct f0{
#define c0(f,W,a) W a
#define R4 [[flat]]
#define J0 [[center_no_perspective]]
#ifndef OPTIONALLY_FLAT
#define OPTIONALLY_FLAT
#endif
#define a2 g L0[[position]][[invariant]];};
#define Y(a,W) thread W&a=R.a
#define k0(a)
#define B(a,W) W a=R.a
#define A4 struct T8{
#define B4 };
#define N3 struct B5{
#define O3 };
#define J5(f,y1,a) constant W0*a[[buffer(Q0(f))]]
#define F4(f,y1,a) constant Q*a[[buffer(Q0(f))]]
#define K5(f,y1,a) constant g*a[[buffer(Q0(f))]]
#define P0(a,y0) x2.a[y0]
#define L5(a,y0) x2.a[y0]
#define R3 struct U8{
#define S3 };
#define B3 struct H3{
#define C3 };
#define a5 struct E7{
#define c5 };
#define D4(M,f,a) [[texture(f)]]texture2d<uint>a
#define e5(M,f,a) [[texture(f)]]texture2d<float>a
#define X2(M,f,a) [[texture(f)]]texture2d<d>a
#define k5(M,f,a) [[texture(f)]]texture2d<d>a
#define f6(M,f,a) [[texture(f)]]texture1d_array<d>a
#define X3(x7,a) constexpr sampler a(filter::linear,mip_filter::none);
#define o6(M,f,a) [[sampler(f)]]sampler a;
#define U3(a) [[sampler(T3)]]sampler a;
#define v1(h0,l) M0.h0.read(W0(l))
#define r5(h0,p,l) M0.h0.sample(p,l)
#define m2(h0,p,l,X0) M0.h0.sample(p,l,level(X0))
#define v5(h0,p,l,P1) M0.h0.sample(p,l,bias(P1))
#define g8(h0,p,l) M0.h0.sample(K4.p,l)
#define Q6(h0,p,l,X0) M0.h0.sample(K4.p,l,level(X0))
#define y7(h0,p,l,P1) M0.h0.sample(K4.p,l,bias(P1))
#define U6(h0,p,q,p6,Q8,X0) M0.h0.sample(p,q,p6)
#define i6 ,constant NB&k,U8 M0,T8 x2
#define v3 ,k,M0,x2
#ifdef ENABLE_INSTANCE_INDEX
#define C1(a,a0,G,v,T) __attribute__((visibility("default")))f0 vertex a(uint v[[vertex_id]],uint T[[instance_id]],constant uint&Bg[[buffer(Q0(zc))]],constant NB&k[[buffer(Q0(n3))]],constant a0*G[[buffer(0)]],U8 M0,T8 x2){T+=Bg;f0 R;
#else
#define C1(a,a0,G,v,T) __attribute__((visibility("default")))f0 vertex a(uint v[[vertex_id]],uint T[[instance_id]],constant NB&k[[buffer(Q0(n3))]],constant a0*G[[buffer(0)]],U8 M0,T8 x2){f0 R;
#endif
#define S7(a,a0,G,v,T) __attribute__((visibility("default")))f0 vertex a(uint v[[vertex_id]],constant NB&k[[buffer(Q0(n3))]],constant LC&A0[[buffer(Q0(a6))]],constant a0*G[[buffer(0)]],U8 M0,T8 x2){f0 R;
#define E6(a,h3,i3,w3,x3,v) __attribute__((visibility("default")))f0 vertex a(uint v[[vertex_id]],constant NB&k[[buffer(Q0(n3))]],constant LC&A0[[buffer(Q0(a6))]],constant h3*i3[[buffer(0)]],constant w3*x3[[buffer(1)]]){f0 R;
#define D1(A5) R.L0=A5;}return R;
#define Y2(Q1,a) Q1 __attribute__((visibility("default")))fragment a(f0 R[[stage_in]],H3 M0){
#define q6(Q1,a) Q1 __attribute__((visibility("default")))fragment a(f0 R[[stage_in]],H3 M0,bool r6[[front_facing]]){
#define G2(C) return C;}
#define G6 ,c S,H3 M0,B5 x2,E7 K4
#define S2 ,S,M0,x2,K4
#define F3 ,H3 M0
#define g1 ,M0
#define a7
#define w5
#ifdef PLS_IMPL_DEVICE_BUFFER
#define J1 struct n1{
#ifdef PLS_IMPL_DEVICE_BUFFER_RASTER_ORDERED
#define r0(f,a) device uint*a[[buffer(Q0(f+c6)),raster_order_group(0)]]
#define k1(f,a) device uint*a[[buffer(Q0(f+c6)),raster_order_group(0)]]
#define E2(f,a) device atomic_uint*a[[buffer(Q0(f+c6)),raster_order_group(0)]]
#else
#define r0(f,a) device uint*a[[buffer(Q0(f+c6))]]
#define k1(f,a) device uint*a[[buffer(Q0(f+c6))]]
#define E2(f,a) device atomic_uint*a[[buffer(Q0(f+c6))]]
#endif
#define K1 };
#define P3 ,n1 R0,uint C0
#define M1 ,R0,C0
#define H0(h) unpackUnorm4x8(R0.h[C0])
#define d1(h) R0.h[C0]
#define T2(h) atomic_load_explicit(&R0.h[C0],memory_order::memory_order_relaxed)
#define v0(h,C) R0.h[C0]=packUnorm4x8(C)
#define f1(h,C) R0.h[C0]=(C)
#define U2(h,C) atomic_store_explicit(&R0.h[C0],C,memory_order::memory_order_relaxed)
#define r2(h)
#define Y1(h)
#define W4(h,q) atomic_fetch_max_explicit(&R0.h[C0],q,memory_order::memory_order_relaxed)
#define X4(h,q) atomic_fetch_add_explicit(&R0.h[C0],q,memory_order::memory_order_relaxed)
#define v2
#define w2
#define F7(a) __attribute__((visibility("default")))fragment a(n1 R0,constant NB&k[[buffer(Q0(n3))]],f0 R[[stage_in]],H3 M0,E7 K4,B5 x2){c S=R.L0.xy;W0 E=W0(metal::floor(S));uint C0=E.y*k.q5+E.x;
#define yd(a) __attribute__((visibility("default")))fragment a(n1 R0,constant NB&k[[buffer(Q0(n3))]],constant LC&A0[[buffer(Q0(a6))]],f0 R[[stage_in]],E7 K4,H3 M0,B5 x2){c S=R.L0.xy;W0 E=W0(metal::floor(S));uint C0=E.y*k.q5+E.x;
#define m1(a) void F7(a)
#define O5(a) void yd(a)
#define U1 }
#define o2(a) i F7(a){i l1;
#define r4(a) i yd(a){i l1;
#define l3 }return l1;U1
#else
#define J1 struct n1{
#define r0(f,a) [[color(f)]]i a
#define k1(f,a) [[color(f)]]uint a
#define E2 k1
#define K1 };
#define P3 ,thread n1&f4,thread n1&R0
#define M1 ,f4,R0
#define H0(h) f4.h
#define d1(h) f4.h
#define T2(h) d1
#define v0(h,C) R0.h=(C)
#define f1(h,C) R0.h=(C)
#define U2(h) f1
#define r2(h) R0.h=f4.h
#define Y1(h) R0.h=f4.h
e uint y5(thread uint&n0,uint x){uint a1=n0;n0=metal::max(a1,x);return a1;}
#define W4(h,q) y5(R0.h,q)
e uint z5(thread uint&n0,uint x){uint a1=n0;n0=a1+x;return a1;}
#define X4(h,q) z5(R0.h,q)
#define v2
#define w2
#define F7(a,...) n1 __attribute__((visibility("default")))fragment a(__VA_ARGS__){c S[[maybe_unused]]=R.L0.xy;n1 R0;
#define m1(a,...) F7(a,n1 f4,constant NB&k[[buffer(Q0(n3))]],f0 R[[stage_in]],E7 K4,H3 M0,B5 x2)
#define O5(a) F7(a,n1 f4,constant NB&k[[buffer(Q0(n3))]],f0 R[[stage_in]],H3 M0,B5 x2,E7 K4,constant LC&A0[[buffer(Q0(a6))]])
#define U1 }return R0;
#define zd(a,...) struct Cg{i Dg[[j(0)]];n1 R0;};Cg __attribute__((visibility("default")))fragment a(__VA_ARGS__){c S[[maybe_unused]]=R.L0.xy;i l1;n1 R0;
#define o2(a) zd(a,n1 f4,constant NB&k[[buffer(Q0(n3))]],f0 R[[stage_in]],H3 M0,B5 x2)
#define r4(a) zd(a,n1 f4,constant NB&k[[buffer(Q0(n3))]],f0 R[[stage_in]],H3 M0,B5 x2,__VA_ARGS__ constant LC&A0[[buffer(Q0(a6))]])
#define l3 }return{.Dg=l1,.R0=R0};
#endif
#define n4 r0
#define discard discard_fragment()
using namespace metal;template<int O1>e vec<uint,O1>floatBitsToUint(vec<float,O1>x){return as_type<vec<uint,O1>>(x);}template<int O1>e vec<int,O1>floatBitsToInt(vec<float,O1>x){return as_type<vec<int,O1>>(x);}e uint floatBitsToUint(float x){return as_type<uint>(x);}e int floatBitsToInt(float x){return as_type<int>(x);}template<int O1>e vec<float,O1>uintBitsToFloat(vec<uint,O1>x){return as_type<vec<float,O1>>(x);}e float uintBitsToFloat(uint x){return as_type<float>(x);}e D unpackHalf2x16(uint x){return as_type<D>(x);}e uint packHalf2x16(D x){return as_type<uint>(x);}e i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}e uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}e Z inverse(Z o1){Z Ma=Z(o1[1][1],-o1[0][1],-o1[1][0],o1[0][0]);float Eg=(Ma[0][0]*o1[0][0])+(Ma[0][1]*o1[1][0]);return Ma*(1/Eg);}e r mix(r o,r b,n6 I1){r G7;for(int D0=0;D0<3;++D0)G7[D0]=I1[D0]?b[D0]:o[D0];return G7;}e c mix(c o,c b,E4 I1){c G7;for(int D0=0;D0<2;++D0)G7[D0]=I1[D0]?b[D0]:o[D0];return G7;}e c mix(c o,c b,float t){return mix(o,b,c(t));}e float mod(float x,float y){return fmod(x,y);}