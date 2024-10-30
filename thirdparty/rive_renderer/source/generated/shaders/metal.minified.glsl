#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define r half2
#define k half3
#define i half4
#define J4 short
#define M ushort
#define f float2
#define Q float3
#define I3 packed_float3
#define g float4
#define O6 bool3
#define n1 uint2
#define G uint4
#define e0 int2
#define g5 int4
#define M ushort
#define I float2x2
#define w4 half3x3
#define x4 half2x3
#endif
#define d inline
#define j2(f1) thread f1&
#define l4(f1) thread f1&
#define notEqual(L,O) ((L)!=(O))
#define lessThanEqual(L,O) ((L)<=(O))
#define lessThan(L,O) ((L)<(O))
#define greaterThanEqual(L,O) ((L)>=(O))
#define m0(L,O) ((L)*(O))
#define atan atan2
#define inversesqrt rsqrt
#define Y3(c,a) struct a{
#define y4(a) };
#define Y0(a) struct a{
#define a0(c,D,a) D a
#define Z0 };
#define f0(w5,q,a,D) D a=q[w5].a
#define H1 struct S{
#define W(c,D,a) D a
#define H3 [[flat]]
#define w0 [[center_no_perspective]]
#ifndef OPTIONALLY_FLAT
#define OPTIONALLY_FLAT
#endif
#define I1 g F0[[position]][[invariant]];};
#define c0(a,D) thread D&a=E.a
#define d0(a)
#define Y(a,D) D a=E.a
#define f2 struct Ja{
#define g2 };
#define M3 struct z3{
#define P3 };
#define N3(c,Q0,a) constant n1*a[[buffer(c)]]
#define S2(c,Q0,a) constant G*a[[buffer(c)]]
#define O3(c,Q0,a) constant g*a[[buffer(c)]]
#define E0(a,o0) d2.a[o0]
#define Q3(a,o0) d2.a[o0]
#define R1 struct Ka{
#define S1 };
#define H2 struct A3{
#define I2 };
#define R2(v0,c,a) [[texture(c)]]texture2d<uint>a
#define e4(v0,c,a) [[texture(c)]]texture2d<float>a
#define v1(v0,c,a) [[texture(c)]]texture2d<h>a
#define K3(a2,a) constexpr sampler a(filter::linear,mip_filter::none);
#define c3(a2,a) constexpr sampler a(filter::linear,mip_filter::linear);
#define N1(B3,K) E1.B3.read(n1(K))
#define l3(B3,i0,K) E1.B3.sample(i0,K)
#define h3(B3,i0,K,v2) E1.B3.sample(i0,K,level(v2))
#define T2 ,Ka E1,Ja d2
#define Y2 ,E1,d2
#ifdef ENABLE_INSTANCE_INDEX
#define h1(a,V,q,n,H) __attribute__((visibility("default")))S vertex a(uint n[[vertex_id]],uint H[[instance_id]],constant uint&La[[buffer(e8)]],constant SB&P[[buffer(n3)]],constant V*q[[buffer(0)]]T2){H+=La;S E;
#else
#define h1(a,V,q,n,H) __attribute__((visibility("default")))S vertex a(uint n[[vertex_id]],uint H[[instance_id]],constant SB&P[[buffer(n3)]],constant V*q[[buffer(0)]]T2){S E;
#endif
#define T4(a,V,q,n,H) __attribute__((visibility("default")))S vertex a(uint n[[vertex_id]],constant SB&P[[buffer(n3)]],constant ZB&X[[buffer(Z3)]],constant V*q[[buffer(0)]]T2){S E;
#define j4(a,T1,U1,h2,i2,n) __attribute__((visibility("default")))S vertex a(uint n[[vertex_id]],constant SB&P[[buffer(n3)]],constant ZB&X[[buffer(Z3)]],constant T1*U1[[buffer(0)]],constant h2*i2[[buffer(1)]]){S E;
#define i1(O4) E.F0=O4;}return E;
#define q2(x3,a) x3 __attribute__((visibility("default")))fragment a(S E[[stage_in]]){
#define r2(o) return o;}
#define m4 ,f h0,A3 E1,z3 d2
#define O2 ,h0,E1,d2
#ifdef PLS_IMPL_DEVICE_BUFFER
#define J1 struct R0{
#ifdef PLS_IMPL_DEVICE_BUFFER_RASTER_ORDERED
#define A0(c,a) device uint*a[[buffer(c+a4),raster_order_group(0)]]
#define C0(c,a) device uint*a[[buffer(c+a4),raster_order_group(0)]]
#define d3(c,a) device atomic_uint*a[[buffer(c+a4),raster_order_group(0)]]
#else
#define A0(c,a) device uint*a[[buffer(c+a4)]]
#define C0(c,a) device uint*a[[buffer(c+a4)]]
#define d3(c,a) device atomic_uint*a[[buffer(c+a4)]]
#endif
#define K1 };
#define L2 ,R0 k0,uint e2
#define X0 ,k0,e2
#define r0(e) unpackUnorm4x8(k0.e[e2])
#define I0(e) k0.e[e2]
#define i3(e) atomic_load_explicit(&k0.e[e2],memory_order::memory_order_relaxed)
#define y0(e,o) k0.e[e2]=packUnorm4x8(o)
#define K0(e,o) k0.e[e2]=(o)
#define j3(e,o) atomic_store_explicit(&k0.e[e2],o,memory_order::memory_order_relaxed)
#define O1(e)
#define X1(e)
#define p4(e,n0) atomic_fetch_max_explicit(&k0.e[e2],n0,memory_order::memory_order_relaxed)
#define q4(e,n0) atomic_fetch_add_explicit(&k0.e[e2],n0,memory_order::memory_order_relaxed)
#define w1
#define x1
#define P4(a) __attribute__((visibility("default")))fragment a(R0 k0,constant SB&P[[buffer(n3)]],S E[[stage_in]],A3 E1,z3 d2){f h0=E.F0.xy;n1 v=n1(metal::floor(h0));uint e2=v.y*P.Q7+v.x;
#define I8(a) __attribute__((visibility("default")))fragment a(R0 k0,constant SB&P[[buffer(n3)]],constant ZB&X[[buffer(Z3)]],S E[[stage_in]],A3 E1,z3 d2){f h0=E.F0.xy;n1 v=n1(metal::floor(h0));uint e2=v.y*P.Q7+v.x;
#define M1(a) void P4(a)
#define U3(a) void I8(a)
#define k2 }
#define N2(a) i P4(a){i e1;
#define o4(a) i I8(a){i e1;
#define S3 }return e1;k2
#else
#define J1 struct R0{
#define A0(c,a) [[color(c)]]i a
#define C0(c,a) [[color(c)]]uint a
#define d3 C0
#define K1 };
#define L2 ,thread R0&y2,thread R0&k0
#define X0 ,y2,k0
#define r0(e) y2.e
#define I0(e) y2.e
#define i3(e) I0
#define y0(e,o) k0.e=(o)
#define K0(e,o) k0.e=(o)
#define j3(e) K0
#define O1(e) k0.e=y2.e
#define X1(e) k0.e=y2.e
d uint M4(thread uint&p,uint x){uint T0=p;p=metal::max(T0,x);return T0;}
#define p4(e,n0) M4(k0.e,n0)
d uint N4(thread uint&p,uint x){uint T0=p;p=T0+x;return T0;}
#define q4(e,n0) N4(k0.e,n0)
#define w1
#define x1
#define P4(a,...) R0 __attribute__((visibility("default")))fragment a(__VA_ARGS__){f h0[[maybe_unused]]=E.F0.xy;R0 k0;
#define M1(a,...) P4(a,R0 y2,S E[[stage_in]],A3 E1,z3 d2)
#define U3(a) P4(a,R0 y2,S E[[stage_in]],A3 E1,z3 d2,constant ZB&X[[buffer(Z3)]])
#define k2 }return k0;
#define J8(a,...) struct Ma{i Na[[j(0)]];R0 k0;};Ma __attribute__((visibility("default")))fragment a(__VA_ARGS__){f h0[[maybe_unused]]=E.F0.xy;i e1;R0 k0;
#define N2(a) J8(a,R0 y2,S E[[stage_in]],A3 E1,z3 d2)
#define o4(a) J8(a,R0 y2,S E[[stage_in]],A3 E1,z3 d2,__VA_ARGS__ constant ZB&X[[buffer(Z3)]])
#define S3 }return{.Na=e1,.k0=k0};
#endif
#define discard discard_fragment()
using namespace metal;template<int z1>d vec<uint,z1>floatBitsToUint(vec<float,z1>x){return as_type<vec<uint,z1>>(x);}template<int z1>d vec<int,z1>floatBitsToInt(vec<float,z1>x){return as_type<vec<int,z1>>(x);}d uint floatBitsToUint(float x){return as_type<uint>(x);}d int floatBitsToInt(float x){return as_type<int>(x);}template<int z1>d vec<float,z1>uintBitsToFloat(vec<uint,z1>x){return as_type<vec<float,z1>>(x);}d float uintBitsToFloat(uint x){return as_type<float>(x);}d r unpackHalf2x16(uint x){return as_type<r>(x);}d uint packHalf2x16(r x){return as_type<uint>(x);}d i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}d uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}d I inverse(I B0){I R6=I(B0[1][1],-B0[0][1],-B0[1][0],B0[0][0]);float Oa=(R6[0][0]*B0[0][0])+(R6[0][1]*B0[1][0]);return R6*(1/Oa);}d k mix(k l,k b,O6 z0){k K8;for(int A=0;A<3;++A)K8[A]=z0[A]?b[A]:l[A];return K8;}