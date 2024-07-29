#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define F half2
#define q half3
#define j half4
#define Z4 short
#define N ushort
#define d float2
#define i0 float3
#define M3 packed_float3
#define g float4
#define r8 bool3
#define B0 uint2
#define W uint4
#define o0 int2
#define P5 int4
#define N ushort
#define B float2x2
#define m6 half3x4
#endif
#define j0 j
#define c0 q
#define l1 F
#define V h
#define J0 N
#define i inline
#define A2(n4) thread n4&
#define notEqual(f0,k0) ((f0)!=(k0))
#define lessThanEqual(f0,k0) ((f0)<=(k0))
#define greaterThanEqual(f0,k0) ((f0)>=(k0))
#define l0(f0,k0) ((f0)*(k0))
#define atan atan2
#define inversesqrt rsqrt
#define d4(e,a) struct a{
#define N4(a) };
#define U0(a) struct a{
#define v0(e,I,a) I a
#define V0 };
#define z0(k6,v,a,I) I a=v[k6].a
#define D1 struct a0{
#define K(e,I,a) I a
#define l6 [[flat]]
#define n0 [[center_no_perspective]]
#define OPTIONALLY_FLAT
#define E1 g v1[[position]][[invariant]];};
#define S(a,I) thread I&a=M.a
#define U(a)
#define P(a,I) I a=M.a
#define Y1 struct sa{
#define Z1 };
#define Q3 struct g3{
#define T3 };
#define R3(e,d1,a) constant B0*a[[buffer(e)]]
#define W2(e,d1,a) constant W*a[[buffer(e)]]
#define S3(e,d1,a) constant g*a[[buffer(e)]]
#define C0(a,D0) O1.a[D0]
#define v2(a,D0) O1.a[D0]
#define Q1 struct ta{
#define R1 };
#define K2 struct h3{
#define L2 };
#define V2(x0,e,a) [[texture(e)]]texture2d<uint>a
#define o4(x0,e,a) [[texture(e)]]texture2d<float>a
#define z1(x0,e,a) [[texture(e)]]texture2d<h>a
#define O3(g2,a) constexpr sampler a(filter::linear,mip_filter::none);
#define q3(g2,a) constexpr sampler a(filter::linear,mip_filter::linear);
#define K1(i3,H) w1.i3.read(B0(H))
#define R2(i3,r0,H) w1.i3.sample(r0,H)
#define W3(i3,r0,H,d3) w1.i3.sample(r0,H,level(d3))
#define V3(i3,r0,H,e3,f3) w1.i3.sample(r0,H,gradient2d(e3,f3))
#define Z2 ,ta w1,sa O1
#define o3 ,w1,O1
#ifdef ENABLE_INSTANCE_INDEX
#define g1(a,R,v,k,O) __attribute__((visibility("default")))a0 vertex a(uint k[[vertex_id]],uint O[[instance_id]],constant uint&ua[[buffer(F7)]],constant SB&A[[buffer(T2)]],constant R*v[[buffer(0)]]Z2){O+=ua;a0 M;
#else
#define g1(a,R,v,k,O) __attribute__((visibility("default")))a0 vertex a(uint k[[vertex_id]],uint O[[instance_id]],constant SB&A[[buffer(T2)]],constant R*v[[buffer(0)]]Z2){a0 M;
#endif
#define v5(a,R,v,k,O) __attribute__((visibility("default")))a0 vertex a(uint k[[vertex_id]],constant SB&A[[buffer(T2)]],constant ZB&L[[buffer(B3)]],constant R*v[[buffer(0)]]Z2){a0 M;
#define y4(a,a2,c2,n2,o2,k) __attribute__((visibility("default")))a0 vertex a(uint k[[vertex_id]],constant SB&A[[buffer(T2)]],constant ZB&L[[buffer(B3)]],constant a2*c2[[buffer(0)]],constant n2*o2[[buffer(1)]]){a0 M;
#define h1(q6) M.v1=q6;}return M;
#define w2(q4,a) q4 __attribute__((visibility("default")))fragment a(a0 M[[stage_in]]){
#define x2(m) return m;}
#define D4 ,d p0,h3 w1,g3 O1
#define Q2 ,p0,w1,O1
#ifdef PLS_IMPL_DEVICE_BUFFER
#define G1 struct L0{
#ifdef PLS_IMPL_DEVICE_BUFFER_RASTER_ORDERED
#define F0(e,a) device uint*a[[buffer(e+C3),raster_order_group(0)]]
#define G0(e,a) device uint*a[[buffer(e+C3),raster_order_group(0)]]
#define P3(e,a) device atomic_uint*a[[buffer(e+C3),raster_order_group(0)]]
#else
#define F0(e,a) device uint*a[[buffer(e+C3)]]
#define G0(e,a) device uint*a[[buffer(e+C3)]]
#define P3(e,a) device atomic_uint*a[[buffer(e+C3)]]
#endif
#define H1 };
#define M2 ,L0 X,uint P1
#define m1 ,X,P1
#define I0(c) unpackUnorm4x8(X.c[P1])
#define H0(c) X.c[P1]
#define w3(c) atomic_load_explicit(&X.c[P1],memory_order::memory_order_relaxed)
#define w0(c,m) X.c[P1]=packUnorm4x8(m)
#define P0(c,m) X.c[P1]=(m)
#define x3(c,m) atomic_store_explicit(&X.c[P1],m,memory_order::memory_order_relaxed)
#define n1(c)
#define A1(c)
#define K4(c)
#define I4(c,S0) atomic_fetch_max_explicit(&X.c[P1],S0,memory_order::memory_order_relaxed)
#define J4(c,S0) atomic_fetch_add_explicit(&X.c[P1],S0,memory_order::memory_order_relaxed)
#define p1
#define q1
#define r4(a) __attribute__((visibility("default")))fragment a(L0 X,constant SB&A[[buffer(T2)]],a0 M[[stage_in]],h3 w1,g3 O1){d p0=M.v1.xy;B0 J=B0(metal::floor(p0));uint P1=J.y*A.p7+J.x;
#define x8(a) __attribute__((visibility("default")))fragment a(L0 X,constant SB&A[[buffer(T2)]],constant ZB&L[[buffer(B3)]],a0 M[[stage_in]],h3 w1,g3 O1){d p0=M.v1.xy;B0 J=B0(metal::floor(p0));uint P1=J.y*A.p7+J.x;
#define T1(a) void r4(a)
#define Z3(a) void x8(a)
#define f2 }
#define v3(a) j r4(a){j Q0;
#define G4(a) j x8(a){j Q0;
#define a4 }return Q0;f2
#else
#define G1 struct L0{
#define F0(e,a) [[color(e)]]j a
#define G0(e,a) [[color(e)]]uint a
#define P3 G0
#define H1 };
#define M2 ,thread L0&W1,thread L0&X
#define m1 ,W1,X
#define I0(c) W1.c
#define H0(c) W1.c
#define w3(c) H0
#define w0(c,m) X.c=(m)
#define P0(c,m) X.c=(m)
#define x3(c) P0
#define n1(c) X.c=W1.c
#define A1(c) X.c=W1.c
#define K4(c) W1.c=X.c
i uint n6(thread uint&o,uint x){uint N1=o;o=metal::max(N1,x);return N1;}
#define I4(c,S0) n6(X.c,S0)
i uint p6(thread uint&o,uint x){uint N1=o;o=N1+x;return N1;}
#define J4(c,S0) p6(X.c,S0)
#define p1
#define q1
#define r4(a,...) L0 __attribute__((visibility("default")))fragment a(__VA_ARGS__){d p0[[maybe_unused]]=M.v1.xy;L0 X;
#define T1(a,...) r4(a,L0 W1,a0 M[[stage_in]],h3 w1,g3 O1)
#define Z3(a) r4(a,L0 W1,a0 M[[stage_in]],h3 w1,g3 O1,constant ZB&L[[buffer(B3)]])
#define f2 }return X;
#define y8(a,...) struct va{j wa[[f(0)]];L0 X;};va __attribute__((visibility("default")))fragment a(__VA_ARGS__){d p0[[maybe_unused]]=M.v1.xy;j Q0;L0 X;
#define v3(a) y8(a,L0 W1,a0 M[[stage_in]],h3 w1,g3 O1)
#define G4(a) y8(a,L0 W1,a0 M[[stage_in]],h3 w1,g3 O1,__VA_ARGS__ constant ZB&L[[buffer(B3)]])
#define a4 }return{.wa=Q0,.X=X};
#endif
#define discard discard_fragment()
using namespace metal;template<int C1>i vec<uint,C1>floatBitsToUint(vec<float,C1>x){return as_type<vec<uint,C1>>(x);}template<int C1>i vec<int,C1>floatBitsToInt(vec<float,C1>x){return as_type<vec<int,C1>>(x);}i uint floatBitsToUint(float x){return as_type<uint>(x);}i int floatBitsToInt(float x){return as_type<int>(x);}template<int C1>i vec<float,C1>uintBitsToFloat(vec<uint,C1>x){return as_type<vec<float,C1>>(x);}i float uintBitsToFloat(uint x){return as_type<float>(x);}i F unpackHalf2x16(uint x){return as_type<F>(x);}i uint packHalf2x16(F x){return as_type<uint>(x);}i j unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}i uint packUnorm4x8(j x){return pack_half_to_unorm4x8(x);}i B inverse(B e1){B r6=B(e1[1][1],-e1[0][1],-e1[1][0],e1[0][0]);float xa=(r6[0][0]*e1[0][0])+(r6[0][1]*e1[1][0]);return r6*(1/xa);}i q mix(q n,q b,r8 g0){q z8;for(int l=0;l<3;++l)z8[l]=g0[l]?b[l]:n[l];return z8;}i m6 r5(q n,h b,q g0,h ya,q v6,h h0){return m6(n.x,n.y,n.z,b,g0.x,g0.y,g0.z,ya,v6.x,v6.y,v6.z,h0);}