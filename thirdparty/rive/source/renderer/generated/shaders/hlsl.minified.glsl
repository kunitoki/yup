#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define F half2
#define v half3
#define i half4
#define c float2
#define M float3
#define e float4
#define m5 bool2
#define Y6 bool3
#define l8 bool4
#define R0 uint2
#define R uint4
#define f0 int2
#define D6 int4
#define X float2x2
#define G7 half3x3
#define H7 half2x3
#define n5 half4x4
#endif
typedef M l4;
#ifdef ENABLE_MIN_16_PRECISION
#define O min16uint
#else
#define O uint
#endif
#define f inline
#define p1(H2) out H2
#define p7(H2) inout H2
#define h1(a) struct a{
#define K(g,k0,a) k0 a:a
#define i1 };
#define L(W9,A,a,k0) k0 a=A.a
#define yc(g) register(b##g)
#define k8(g,a) cbuffer a:yc(g){struct{
#define T9(a) }a;}
#define o2 struct l0{
#define I0 noperspective
#define OPTIONALLY_FLAT nointerpolation
#define l3 nointerpolation
#define W(g,k0,a) k0 a:TEXCOORD##g
#ifdef NEEDS_CLIP_DISTANCE
#define P1 e T0:SV_Position;e zc:SV_ClipDistance;};
#else
#define P1 e T0:SV_Position;};
#endif
#define V(a,k0) k0 a
#define e0(a) a0.a=a
#define q(a,k0) k0 a=a0.a
#ifdef VERTEX
#define A4
#define B4
#endif
#ifdef FRAGMENT
#define X3
#define Y3
#endif
#define K5
#define L5
#define v5(h0,g,a) uniform Texture2D<R>a:register(t##g)
#define Z6(h0,g,a) uniform Texture2D<e>a:register(t##g)
#define Z2(h0,g,a) uniform Texture2D<unorm e>a:register(t##g)
#define U5(h0,g,a) uniform Texture2D<d>a:register(t##g)
#define P6(h0,g,a) uniform Texture1DArray<d>a:register(t##g)
#define S4(g,a) SamplerState a:register(s##g);
#define f4 S4
#define Q6 S4
#define a7(h0,g,a) S4(g,a)
#define D4(a) S4(C4,a)
#define j1(a,o) a[o]
#define Z5(a,p,o) a.Sample(p,o)
#define r2(a,p,o,g1) a.SampleLevel(p,o,g1)
#define a6(a,p,o,j2) a.SampleBias(p,o,j2)
#define F7(a,p,G,c7,Y9,g1) a.SampleLevel(p,c(G,c7),g1)
#define g9(v0,p,o) Z5(v0,p,o)
#define k5(v0,p,o,g1) r2(v0,p,o,g1)
#define m8(v0,p,o,j2) a6(v0,p,o,j2)
#define R2
#define S2
#ifdef ENABLE_RASTERIZER_ORDERED_VIEWS
#define f3 RasterizerOrderedTexture2D
#else
#define f3 RWTexture2D
#endif
#define I1
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define z0(g,a) uniform f3<unorm i>a:register(u##g)
#else
#define z0(g,a) uniform f3<uint>a:register(u##g)
#endif
#define c5 z0
#define L4 z0
#define e4 L0
#define r5 y0
#define x1(g,a) uniform f3<uint>a:register(u##g)
#define Y2 x1
#define r3 o1
#define v3 q1
#define J1
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define L0(h) h[D]
#else
#define L0(h) unpackUnorm4x8(h[D])
#endif
#define o1(h) h[D]
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define y0(h,B) h[D]=(B)
#else
#define y0(h,B) h[D]=packUnorm4x8(B)
#endif
#define q1(h,B) h[D]=(B)
f uint c6(f3<uint>P3,f0 D,uint x){uint l1;InterlockedMax(P3[D],x,l1);return l1;}
#define H5(h,G) c6(h,D,G)
f uint d6(f3<uint>P3,f0 D,uint x){uint l1;InterlockedAdd(P3[D],x,l1);return l1;}
#define I5(h,G) d6(h,D,G)
#define Q2(h)
#define l2(h)
#define T6
#define S3
#define g4
#define r1
#ifdef NEEDS_CLIP_DISTANCE
#define C6 ,out e gl_ClipDistance
#define I4 ,a0.zc
#else
#define C6
#define I4
#endif
#ifdef ENABLE_BASE_VERTEX
#define n1(a,d0,A,C,r) cbuffer jj:yc(P7){uint Ac;uint tf;uint a##kj;uint a##lj;}l0 main(d0 A,uint g3:SV_VertexID,uint Q3:SV_InstanceID){uint r=Q3+Ac;uint C=g3+tf;l0 a0;
#else
#define n1(a,d0,A,C,r) cbuffer jj:yc(P7){uint Ac;uint tf;uint a##kj;uint a##lj;}l0 main(d0 A,uint g3:SV_VertexID,uint Q3:SV_InstanceID){uint r=Q3+Ac;uint C=g3;l0 a0;
#endif
#define O8(a,d0,A,H1,j0,C,r) l0 main(d0 A,H1 j0,uint C:SV_VertexID){l0 a0;e T0;
#define n7(a,G3,H3,V3,n3,H1,j0,C) l0 main(G3 H3,V3 n3,H1 j0,uint C:SV_VertexID){l0 a0;e T0;
#define C1(e6) a0.T0=e6;}return a0;
#ifdef RENDER_MODE_DEPTH_STENCIL
#define v2(U0,a) [earlydepthstencil]U0 main(l0 a0):SV_Target{c Y=a0.T0.xy;f0 D=f0(floor(Y));
#else
#define v2(U0,a) U0 main(l0 a0):SV_Target{
#endif
#define d7(U0,a) U0 main(l0 a0,bool e7:SV_IsFrontFace):SV_Target{
#define w2(B) return B;}
#define q7 ,c Y
#define q3 ,Y
#define r4 ,f0 D
#define d2 ,D
#define E1(a) [earlydepthstencil]void main(l0 a0){c Y=a0.T0.xy;f0 D=f0(floor(Y));
#define L1 }
#define K2(a) [earlydepthstencil]i main(l0 a0):SV_Target{c Y=a0.T0.xy;f0 D=f0(floor(Y));i S1;
#define I3 }return S1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(E,J) ((E)==(J))
#define notEqual(E,J) ((E)!=(J))
#define lessThan(E,J) ((E)<(J))
#define greaterThan(E,J) ((E)>(J))
#define G0(E,J) mul(J,E)
#define i5
#define j5
#define n4
#define p4
#define o4(g,a1,a) StructuredBuffer<R>a:register(t##g)
#define c8(g,a1,a) StructuredBuffer<uint>a:register(t##g)
#define p6(g,a1,a) StructuredBuffer<e>a:register(t##g)
#define p0(a,m0) a[m0]
#define Q5(a,m0) a[m0]
f F unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return F(f16tof32(x),f16tof32(y));}f uint packHalf2x16(c F2){uint x=f32tof16(F2.x);uint y=f32tof16(F2.y);return(y<<16)|x;}f i unpackUnorm4x8(uint u){R z1=R(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(z1)*(1./255.);}f c unpackUnorm2x16(uint u){R0 z1=R0(u&0xffffu,u>>16);return c(z1)*(1./65535.);}f uint packUnorm4x8(i k){R z1=(R(saturate(k)*255.)&0xff)<<R(0,8,16,24);z1.xy|=z1.zw;z1.x|=z1.y;return z1.x;}f X inverse(X F1){X Bc=X(F1[1][1],-F1[0][1],-F1[1][0],F1[0][0]);return Bc*(1./determinant(F1));}f float mix(float x,float y,bool s){return s?y:x;}f c mix(c x,c y,m5 s){return s?y:x;}f M mix(M x,M y,Y6 s){return s?y:x;}f e mix(e x,e y,l8 s){return s?y:x;}f d mix(d x,d y,bool s){return s?y:x;}f F mix(F x,F y,m5 s){return s?y:x;}f v mix(v x,v y,Y6 s){return s?y:x;}f i mix(i x,i y,l8 s){return s?y:x;}f float mix(float x,float y,float s){return lerp(x,y,s);}f c mix(c x,c y,c s){return lerp(x,y,s);}f M mix(M x,M y,M s){return lerp(x,y,s);}f e mix(e x,e y,e s){return lerp(x,y,s);}f d mix(d x,d y,d s){return lerp(x,y,s);}f F mix(F x,F y,F s){return lerp(x,y,s);}f v mix(v x,v y,v s){return lerp(x,y,s);}f i mix(i x,i y,i s){return lerp(x,y,s);}f float fract(float x){return frac(x);}f c fract(c x){return frac(x);}f M fract(M x){return frac(x);}f e fract(e x){return frac(x);}f d fract(d x){return frac(x);}f F fract(F x){return F(frac(x));}f v fract(v x){return v(frac(x));}f i fract(i x){return i(frac(x));}f float mod(float x,float y){return fmod(x,y);}f d h3(d x){return sign(x);}f F h3(F x){return F(sign(x));}f v h3(v x){return v(sign(x));}f i h3(i x){return i(sign(x));}f float h3(float x){return sign(x);}f c h3(c x){return sign(x);}f M h3(M x){return sign(x);}f e h3(e x){return sign(x);}
#define sign h3
f d i3(d x){return abs(x);}f F i3(F x){return F(abs(x));}f v i3(v x){return v(abs(x));}f i i3(i x){return i(abs(x));}f float i3(float x){return abs(x);}f c i3(c x){return abs(x);}f M i3(M x){return abs(x);}f e i3(e x){return abs(x);}
#define abs i3
f d j3(d x){return sqrt(x);}f F j3(F x){return F(sqrt(x));}f v j3(v x){return v(sqrt(x));}f i j3(i x){return i(sqrt(x));}f float j3(float x){return sqrt(x);}f c j3(c x){return sqrt(x);}f M j3(M x){return sqrt(x);}f e j3(e x){return sqrt(x);}
#define sqrt j3
