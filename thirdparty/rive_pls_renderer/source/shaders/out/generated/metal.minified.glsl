#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define h half
#define l0 half2
#define O half3
#define i half4
#define a6 short
#define M ushort
#define d float2
#define D1 float3
#define H3 packed_float3
#define g float4
#define c8 bool3
#define x0 uint2
#define T uint4
#define m0 int2
#define G5 int4
#define M ushort
#define A float2x2
#define e6 half3x4
#endif
#define M0 i
#define j0 O
#define Z1 l0
#define E0 h
#define O0 M
#define p inline
#define y2(h4) thread h4&
#define notEqual(d0,g0) ((d0)!=(g0))
#define lessThanEqual(d0,g0) ((d0)<=(g0))
#define greaterThanEqual(d0,g0) ((d0)>=(g0))
#define h0(d0,g0) ((d0)*(g0))
#define atan atan2
#define inversesqrt rsqrt
#define X3(c,a) struct a{
#define G4(a) };
#define U0(a) struct a{
#define q0(c,G,a) G a
#define V0 };
#define v0(c6,r,a,G) G a=r[c6].a
#define A1 struct X{
#define I(c,G,a) G a
#define d6 [[flat]]
#define k0 [[center_no_perspective]]
#define OPTIONALLY_FLAT
#define B1 g q1[[position]][[invariant]];};
#define Q(a,G) thread G&a=K.a
#define S(a)
#define N(a,G) G a=K.a
#define V1 struct ja{
#define W1 };
#define L3 struct a3{
#define O3 };
#define M3(c,d1,a) constant x0*a[[buffer(c)]]
#define R2(c,d1,a) constant T*a[[buffer(c)]]
#define N3(c,d1,a) constant g*a[[buffer(c)]]
#define z0(a,A0) M1.a[A0]
#define q2(a,A0) M1.a[A0]
#define O1 struct ka{
#define P1 };
#define F2 struct c3{
#define G2 };
#define Q2(c,a) [[texture(c)]]texture2d<uint>a
#define i4(c,a) [[texture(c)]]texture2d<float>a
#define x1(c,a) [[texture(c)]]texture2d<h>a
#define J3(d2,a) constexpr sampler a(filter::linear,mip_filter::none);
#define l3(d2,a) constexpr sampler a(filter::linear,mip_filter::linear);
#define I1(d3,F) r1.d3.read(x0(F))
#define M2(d3,p0,F) r1.d3.sample(p0,F)
#define R3(d3,p0,F,X2) r1.d3.sample(p0,F,level(X2))
#define Q3(d3,p0,F,Y2,Z2) r1.d3.sample(p0,F,gradient2d(Y2,Z2))
#define U2 ,ka r1,ja M1
#define j3 ,r1,M1
#ifdef ENABLE_INSTANCE_INDEX
#define g1(a,P,r,j,L) __attribute__((visibility("default")))X vertex a(uint j[[vertex_id]],uint L[[instance_id]],constant uint&la[[buffer(y7)]],constant RB&v[[buffer(O2)]],constant P*r[[buffer(0)]]U2){L+=la;X K;
#else
#define g1(a,P,r,j,L) __attribute__((visibility("default")))X vertex a(uint j[[vertex_id]],uint L[[instance_id]],constant RB&v[[buffer(O2)]],constant P*r[[buffer(0)]]U2){X K;
#endif
#define k5(a,P,r,j,L) __attribute__((visibility("default")))X vertex a(uint j[[vertex_id]],constant RB&v[[buffer(O2)]],constant YB&J[[buffer(w3)]],constant P*r[[buffer(0)]]U2){X K;
#define o4(a,X1,Y1,l2,m2,j) __attribute__((visibility("default")))X vertex a(uint j[[vertex_id]],constant RB&v[[buffer(O2)]],constant YB&J[[buffer(w3)]],constant X1*Y1[[buffer(0)]],constant l2*m2[[buffer(1)]]){X K;
#define h1(i6) K.q1=i6;}return K;
#define r2(j4,a) j4 __attribute__((visibility("default")))fragment a(X K[[stage_in]]){
#define v2(l) return l;}
#define w4 ,d n0,c3 r1,a3 M1
#define L2 ,n0,r1,M1
#ifdef PLS_IMPL_DEVICE_BUFFER
#define E1 struct H0{
#ifdef PLS_IMPL_DEVICE_BUFFER_RASTER_ORDERED
#define C0(c,a) device uint*a[[buffer(c+x3),raster_order_group(0)]]
#define D0(c,a) device uint*a[[buffer(c+x3),raster_order_group(0)]]
#define K3(c,a) device atomic_uint*a[[buffer(c+x3),raster_order_group(0)]]
#else
#define C0(c,a) device uint*a[[buffer(c+x3)]]
#define D0(c,a) device uint*a[[buffer(c+x3)]]
#define K3(c,a) device atomic_uint*a[[buffer(c+x3)]]
#endif
#define F1 };
#define H2 ,H0 Y,uint N1
#define l1 ,Y,N1
#define N0(e) unpackUnorm4x8(Y.e[N1])
#define L0(e) Y.e[N1]
#define o3(e) atomic_load_explicit(&Y.e[N1],memory_order::memory_order_relaxed)
#define F0(e,l) Y.e[N1]=packUnorm4x8(l)
#define a1(e,l) Y.e[N1]=(l)
#define p3(e,l) atomic_store_explicit(&Y.e[N1],l,memory_order::memory_order_relaxed)
#define y0(e)
#define D4(e)
#define B4(e,S0) atomic_fetch_max_explicit(&Y.e[N1],S0,memory_order::memory_order_relaxed)
#define C4(e,S0) atomic_fetch_add_explicit(&Y.e[N1],S0,memory_order::memory_order_relaxed)
#define n1
#define o1
#define k4(a) __attribute__((visibility("default")))fragment a(H0 Y,constant RB&v[[buffer(O2)]],X K[[stage_in]],c3 r1,a3 M1){d n0=K.q1.xy;x0 H=x0(metal::floor(n0));uint N1=H.y*v.h7+H.x;
#define f8(a) __attribute__((visibility("default")))fragment a(H0 Y,constant RB&v[[buffer(O2)]],constant YB&J[[buffer(w3)]],X K[[stage_in]],c3 r1,a3 M1){d n0=K.q1.xy;x0 H=x0(metal::floor(n0));uint N1=H.y*v.h7+H.x;
#define R1(a) void k4(a)
#define U3(a) void f8(a)
#define c2 }
#define n3(a) i k4(a){i P0;
#define z4(a) i f8(a){i P0;
#define V3 }return P0;c2
#else
#define E1 struct H0{
#define C0(c,a) [[color(c)]]i a
#define D0(c,a) [[color(c)]]uint a
#define K3 D0
#define F1 };
#define H2 ,thread H0&f2,thread H0&Y
#define l1 ,f2,Y
#define N0(e) f2.e
#define L0(e) f2.e
#define o3(e) L0
#define F0(e,l) Y.e=(l)
#define a1(e,l) Y.e=(l)
#define p3(e) a1
#define y0(e) Y.e=f2.e
#define D4(e) f2.e=Y.e
p uint f6(thread uint&n,uint x){uint L1=n;n=metal::max(L1,x);return L1;}
#define B4(e,S0) f6(Y.e,S0)
p uint h6(thread uint&n,uint x){uint L1=n;n=L1+x;return L1;}
#define C4(e,S0) h6(Y.e,S0)
#define n1
#define o1
#define k4(a,...) H0 __attribute__((visibility("default")))fragment a(__VA_ARGS__){d n0[[maybe_unused]]=K.q1.xy;H0 Y;
#define R1(a,...) k4(a,H0 f2,X K[[stage_in]],c3 r1,a3 M1)
#define U3(a) k4(a,H0 f2,X K[[stage_in]],c3 r1,a3 M1,constant YB&J[[buffer(w3)]])
#define c2 }return Y;
#define g8(a,...) struct ma{i na[[f(0)]];H0 Y;};ma __attribute__((visibility("default")))fragment a(__VA_ARGS__){d n0[[maybe_unused]]=K.q1.xy;i P0;H0 Y;
#define n3(a) g8(a,H0 f2,X K[[stage_in]],c3 r1,a3 M1)
#define z4(a) g8(a,H0 f2,X K[[stage_in]],c3 r1,a3 M1,__VA_ARGS__ constant YB&J[[buffer(w3)]])
#define V3 }return{.na=P0,.Y=Y};
#endif
#define discard discard_fragment()
using namespace metal;template<int z1>p vec<uint,z1>floatBitsToUint(vec<float,z1>x){return as_type<vec<uint,z1>>(x);}template<int z1>p vec<int,z1>floatBitsToInt(vec<float,z1>x){return as_type<vec<int,z1>>(x);}p uint floatBitsToUint(float x){return as_type<uint>(x);}p int floatBitsToInt(float x){return as_type<int>(x);}template<int z1>p vec<float,z1>uintBitsToFloat(vec<uint,z1>x){return as_type<vec<float,z1>>(x);}p float uintBitsToFloat(uint x){return as_type<float>(x);}p l0 unpackHalf2x16(uint x){return as_type<l0>(x);}p uint packHalf2x16(l0 x){return as_type<uint>(x);}p i unpackUnorm4x8(uint x){return unpack_unorm4x8_to_half(x);}p uint packUnorm4x8(i x){return pack_half_to_unorm4x8(x);}p A inverse(A e1){A j6=A(e1[1][1],-e1[0][1],-e1[1][0],e1[0][0]);float oa=(j6[0][0]*e1[0][0])+(j6[0][1]*e1[1][0]);return j6*(1/oa);}p O mix(O m,O b,c8 e0){O h8;for(int k=0;k<3;++k)h8[k]=e0[k]?b[k]:m[k];return h8;}p e6 j5(O m,h b,O e0,h pa,O k6,h f0){return e6(m.x,m.y,m.z,b,e0.x,e0.y,e0.z,pa,k6.x,k6.y,k6.z,f0);}