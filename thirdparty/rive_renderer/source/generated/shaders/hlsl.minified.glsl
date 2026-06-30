#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define D half2
#define r half3
#define i half4
#define c float2
#define V float3
#define g float4
#define F4 bool2
#define n6 bool3
#define w7 bool4
#define W0 uint2
#define Q uint4
#define U int2
#define Z5 int4
#define a0 float2x2
#define V6 half3x3
#define W6 half2x3
#define g5 half4x4
#endif
typedef V I3;
#ifdef ENABLE_MIN_16_PRECISION
#define X min16uint
#else
#define X uint
#endif
#define e inline
#define g1(g2) out g2
#define U4(g2) inout g2
#define A1(a) struct a{
#define r0(f,W,a) W a:a
#define B1 };
#define v0(N8,G,a,W) W a=G.a
#define rd(f) register(b##f)
#define l6(f,a) cbuffer a:rd(f){struct{
#define v7(a) }a;}
#define h2 struct g0{
#define J0 noperspective
#define OPTIONALLY_FLAT nointerpolation
#define S4 nointerpolation
#define d0(f,W,a) W a:TEXCOORD##f
#define Z1 g L0:SV_Position;};
#define Y(a,W) W a
#define l0(a) R.a=a
#define B(a,W) W a=R.a
#ifdef VERTEX
#define P3
#define Q3
#endif
#ifdef FRAGMENT
#define y3
#define z3
#endif
#define Z4
#define a5
#define E4(N,f,a) uniform Texture2D<Q>a:register(t##f)
#define d5(N,f,a) uniform Texture2D<g>a:register(t##f)
#define U2(N,f,a) uniform Texture2D<unorm g>a:register(t##f)
#define j5(N,f,a) uniform Texture2D<d>a:register(t##f)
#define e6(N,f,a) uniform Texture1DArray<d>a:register(t##f)
#define v5(f,a) SamplerState a:register(s##f);
#define V3 v5
#define o6(N,f,a) v5(f,a)
#define S3(a) v5(R3,a)
#define F1(a,l) a[l]
#define p5(a,p,l) a.Sample(p,l)
#define m2(a,p,l,X0) a.SampleLevel(p,l,X0)
#define q5(a,p,l,P1) a.SampleBias(p,l,P1)
#define U6(a,p,q,p6,P8,X0) a.SampleLevel(p,c(q,p6),X0)
#define g8(h0,p,l) p5(h0,p,l)
#define Q6(h0,p,l,X0) m2(h0,p,l,X0)
#define y7(h0,p,l,P1) q5(h0,p,l,P1)
#define v2
#define w2
#ifdef ENABLE_RASTERIZER_ORDERED_VIEWS
#define J2 RasterizerOrderedTexture2D
#else
#define J2 RWTexture2D
#endif
#define K1
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define p0(f,a) uniform J2<unorm i>a:register(u##f)
#else
#define p0(f,a) uniform J2<uint>a:register(u##f)
#endif
#define m4 p0
#define f1(f,a) uniform J2<uint>a:register(u##f)
#define o4 f1
#define N3 e1
#define O3 h1
#define L1
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define H0(h) h[E]
#else
#define H0(h) unpackUnorm4x8(h[E])
#endif
#define e1(h) h[E]
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define C0(h,C) h[E]=(C)
#else
#define C0(h,C) h[E]=packUnorm4x8(C)
#endif
#define h1(h,C) h[E]=(C)
e uint w5(J2<uint>m3,U E,uint x){uint a1;InterlockedMax(m3[E],x,a1);return a1;}
#define O5(h,q) w5(h,E,q)
e uint x5(J2<uint>m3,U E,uint x){uint a1;InterlockedAdd(m3[E],x,a1);return a1;}
#define Q5(h,q) x5(h,E,q)
#define r2(h)
#define X1(h)
#define h6
#define p3
#define C3
#define i1
#define a7
#define r5
#define C1(a,c0,G,v,S) cbuffer ei:rd(uc){uint ug;uint a##fi;uint a##gi;uint a##hi;}g0 main(c0 G,uint v:SV_VertexID,uint D7:SV_InstanceID){uint S=D7+ug;g0 R;
#define S7(a,c0,G,v,S) g0 main(c0 G,uint v:SV_VertexID){g0 R;g L0;
#define E6(a,e3,f3,q3,r3,v) g0 main(e3 f3,q3 r3,uint v:SV_VertexID){g0 R;g L0;
#define D1(y5) R.L0=y5;}return R;
#define V2(Q1,a) Q1 main(g0 R):SV_Target{
#define q6(Q1,a) Q1 main(g0 R,bool r6:SV_IsFrontFace):SV_Target{
#define F2(C) return C;}
#define G6 ,c T
#define R2 ,T
#define r4 ,U E
#define U1 ,E
#define v1(a) [earlydepthstencil]void main(g0 R){c T=R.L0.xy;U E=U(floor(T));
#define M5(a) v1(a)
#define c2 }
#define o2(a) [earlydepthstencil]i main(g0 R):SV_Target{c T=R.L0.xy;U E=U(floor(T));i m1;
#define v4(a) o2(a)
#define i3 }return m1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(A,F) ((A)==(F))
#define notEqual(A,F) ((A)!=(F))
#define lessThan(A,F) ((A)<(F))
#define greaterThan(A,F) ((A)>(F))
#define Z0(A,F) mul(F,A)
#define B4
#define C4
#define K3
#define L3
#define H5(f,y1,a) StructuredBuffer<W0>a:register(t##f)
#define G4(f,y1,a) StructuredBuffer<Q>a:register(t##f)
#define I5(f,y1,a) StructuredBuffer<g>a:register(t##f)
#define P0(a,y0) a[y0]
#define J5(a,y0) a[y0]
e D unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return D(f16tof32(x),f16tof32(y));}e uint packHalf2x16(c e2){uint x=f32tof16(e2.x);uint y=f32tof16(e2.y);return(y<<16)|x;}e i unpackUnorm4x8(uint u){Q R1=Q(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(R1)*(1./255.);}e uint packUnorm4x8(i j){Q R1=(Q(j*255.)&0xff)<<Q(0,8,16,24);R1.xy|=R1.zw;R1.x|=R1.y;return R1.x;}e a0 inverse(a0 o1){a0 Ga=a0(o1[1][1],-o1[0][1],-o1[1][0],o1[0][0]);return Ga*(1./determinant(o1));}e float mix(float x,float y,bool s){return s?y:x;}e c mix(c x,c y,F4 s){return s?y:x;}e V mix(V x,V y,n6 s){return s?y:x;}e g mix(g x,g y,w7 s){return s?y:x;}e d mix(d x,d y,bool s){return s?y:x;}e D mix(D x,D y,F4 s){return s?y:x;}e r mix(r x,r y,n6 s){return s?y:x;}e i mix(i x,i y,w7 s){return s?y:x;}e float mix(float x,float y,float s){return lerp(x,y,s);}e c mix(c x,c y,c s){return lerp(x,y,s);}e V mix(V x,V y,V s){return lerp(x,y,s);}e g mix(g x,g y,g s){return lerp(x,y,s);}e d mix(d x,d y,d s){return lerp(x,y,s);}e D mix(D x,D y,D s){return lerp(x,y,s);}e r mix(r x,r y,r s){return lerp(x,y,s);}e i mix(i x,i y,i s){return lerp(x,y,s);}e float fract(float x){return frac(x);}e c fract(c x){return frac(x);}e V fract(V x){return frac(x);}e g fract(g x){return frac(x);}e d fract(d x){return frac(x);}e D fract(D x){return D(frac(x));}e r fract(r x){return r(frac(x));}e i fract(i x){return i(frac(x));}e float mod(float x,float y){return fmod(x,y);}e d K2(d x){return sign(x);}e D K2(D x){return D(sign(x));}e r K2(r x){return r(sign(x));}e i K2(i x){return i(sign(x));}e float K2(float x){return sign(x);}e c K2(c x){return sign(x);}e V K2(V x){return sign(x);}e g K2(g x){return sign(x);}
#define sign K2
e d L2(d x){return abs(x);}e D L2(D x){return D(abs(x));}e r L2(r x){return r(abs(x));}e i L2(i x){return i(abs(x));}e float L2(float x){return abs(x);}e c L2(c x){return abs(x);}e V L2(V x){return abs(x);}e g L2(g x){return abs(x);}
#define abs L2
e d M2(d x){return sqrt(x);}e D M2(D x){return D(sqrt(x));}e r M2(r x){return r(sqrt(x));}e i M2(i x){return i(sqrt(x));}e float M2(float x){return sqrt(x);}e c M2(c x){return sqrt(x);}e V M2(V x){return sqrt(x);}e g M2(g x){return sqrt(x);}
#define sqrt M2
