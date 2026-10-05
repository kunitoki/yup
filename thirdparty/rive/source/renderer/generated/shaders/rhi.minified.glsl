#pragma warning(disable:3550)
#pragma warning(disable:4000)
#ifndef _ARE_TOKEN_NAMES_PRESERVED
#define d half
#define C half2
#define v half3
#define i half4
#define Q ushort
#define c float2
#define O float3
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
typedef O c4;
#ifdef ENABLE_MIN_16_PRECISION
#ifdef NEEDS_USHORT_DEFINE
typedef min16uint Q;
#endif
#else
#ifdef NEEDS_USHORT_DEFINE
typedef uint Q;
#endif
#endif
#define te(B,J) B##J
#define f inline
#define i1(w2) out w2
#define V6(w2) inout w2
#define c1(a) struct a{
#define K(g,j0,a) j0 a:te(Wj,g)
#define d1 };
#define L(h9,D,a,j0) j0 a=D.a
#define H7(g,a) cbuffer a{struct{
#define e9(a) }a;}
#define l2 struct v0{
#define E0 noperspective
#define OPTIONALLY_FLAT nointerpolation
#define Z2 nointerpolation
#define V(g,j0,a) j0 a:te(TEXCOORD,g)
#ifdef NEEDS_CLIP_DISTANCE
#define e2 e U0:SV_Position;e hi:SV_ClipDistance;};
#else
#define e2 e U0:SV_Position;};
#endif
#define T(a,j0) j0 a
#define Z(a) g0.a=a
#define q(a,j0) j0 a=g0.a
#ifdef VERTEX
#define j4
#define k4
#endif
#ifdef FRAGMENT
#define N3
#define O3
#endif
#define x5
#define y5
#define V4(c0,g,a) uniform Texture2D<M>a
#define C6(c0,g,a) uniform Texture2D<e>a
#ifdef SOURCE_TEXTURE_MSAA
#define Xf(c0,g,a) uniform Texture2DMS<i>a
#endif
#define i3(c0,g,a) uniform Texture2D<i>a
#define D5(c0,g,a) uniform Texture2D<d>a
#define q6(c0,g,a) uniform Texture2DArray<d>a
#define M5(g,a) SamplerState a;
#define o4 M5
#define D6(c0,g,a) M5(g,a)
#define m4(a) M5(l4,a)
#ifdef Zj
#define D8(a,ii,l) a.ak(l,ii)
#endif
#define p1(a,l) a[l]
#define K5(a,o,l) a.Sample(o,l)
#define o2(a,o,l,Y0) a.SampleLevel(o,l,Y0)
#define L5(a,o,l,Y1) a.SampleBias(o,l,Y1)
#define j7(a,o,F,E6,j9,Y0) a.SampleLevel(o,O(F,0.5,E6),Y0)
#define w8(q0,o,l) K5(q0,o,l)
#define i6(q0,o,l,Y0) o2(q0,o,l,Y0)
#define K7(q0,o,l,Y1) L5(q0,o,l,Y1)
#define E2
#define F2
#ifdef ENABLE_RASTERIZER_ORDERED_VIEWS
#define U2 RasterizerOrderedTexture2D
#else
#define U2 RWTexture2D
#endif
#if defined(FRAGMENT)&&defined(RENDER_MODE_DEPTH_STENCIL)
#ifdef SUPPORTS_SUBPASS_LOAD
#define ji (K2+1)
#define E5(a) [[vk::input_attachment_index(ji)]]SubpassInputMS<i>a
#define I6(a) Ha(S4(a.SubpassLoad(0),a.SubpassLoad(1),a.SubpassLoad(2),a.SubpassLoad(3)),q9)
#elif defined(SUPPORTS_MSAA_DST_TEXEL_FETCH)
#define E5(a) Texture2DMS<i>a
#define I6(a) Ha(S4(a.Load(H,0),a.Load(H,1),a.Load(H,2),a.Load(H,3)),q9)
#else
#define E5(a) Texture2D a
#define I6(a) a[H]
#endif
#endif
#define R1
#define S1
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define B0(g,a) uniform U2<ck i>a
#else
#define B0(g,a) uniform U2<uint>a
#endif
#define I4 B0
#define o1(g,a) uniform U2<uint>a
#define f3 h1
#define g3 j1
#if COMPILER_METAL||FORCE_ATOMIC_BUFFER
#define L2(g,a) uniform RWBuffer<uint>a
#define f3(h) h[K0]
#define g3(h,E) h[K0]=E
#else
#define L2 o1
#define f3 h1
#define g3 j1
#endif
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define N0(h) h[H]
#else
#define N0(h) unpackUnorm4x8(h[H])
#endif
#define h1(h) h[H]
#ifdef ENABLE_TYPED_UAV_LOAD_STORE
#define y0(h,E) h[H]=(E)
#else
#define y0(h,E) h[H]=packUnorm4x8(E)
#endif
#define j1(h,E) h[H]=(E)
#if COMPILER_METAL||FORCE_ATOMIC_BUFFER
f uint N5(RWBuffer<uint>D3,uint K0,uint x){uint e1;InterlockedMax(D3[K0],x,e1);return e1;}
#define p5(h,F) N5(h,K0,F)
f uint O5(RWBuffer<uint>D3,uint K0,uint x){uint e1;InterlockedAdd(D3[K0],x,e1);return e1;}
#define q5(h,F) O5(h,K0,F)
#else
f uint N5(U2<uint>D3,e0 H,uint x){uint e1;InterlockedMax(D3[H],x,e1);return e1;}
#define p5(h,F) N5(h,H,F)
f uint O5(U2<uint>D3,e0 H,uint x){uint e1;InterlockedAdd(D3[H],x,e1);return e1;}
#define q5(h,F) O5(h,H,F)
#endif
#define D2(h)
#define Z1(h)
#define w6
#define G3
#define R3
#define k1
#ifdef SV_INSTANCE_ID_INCLUDES_BASE
#define yb
#define zb(A4) (A4)
#else
#define yb uint baseInstance;
#define zb(A4) ((A4)+baseInstance)
#endif
#if defined(ENABLE_BASE_VERTEX)&&!defined(SV_VERTEX_ID_INCLUDES_BASE)
#define Ab uint baseVertex;
#define Bb(S5) ((S5)+baseVertex)
#else
#define Ab
#define Bb(S5) (S5)
#endif
#ifdef NO_VARYING
#define v1(a,d0,D,G,r) yb Ab e a(d0 D,uint S5:SV_VertexID,uint A4:SV_InstanceID):SV_Position{uint G=Bb(S5);uint r=zb(A4);
#define w1(P5) return P5;}
#else
#define v1(a,d0,D,G,r) yb Ab v0 a(d0 D,uint S5:SV_VertexID,uint A4:SV_InstanceID){uint G=Bb(S5);uint r=zb(A4);v0 g0;
#define h8(a,d0,D,A1,h0,G,r) v0 a(d0 D,A1 h0,uint G:SV_VertexID){v0 g0;e U0;
#define T6(a,w3,x3,J3,K3,A1,h0,G) v0 a(w3 x3,J3 K3,A1 h0,uint G:SV_VertexID){v0 g0;e U0;
#define w1(P5) g0.U0=P5;}return g0;
#endif
#if COMPILER_DXC&&(COMPILER_VULKAN||COMPILER_GLSL_ES3_1)
#define Db(Cb) (Cb)
#else
#define Db(Cb) (!(Cb))
#endif
#ifdef NO_VARYING
#define j3(F1,a) EARLYDEPTHSTENCIL F1 a(e U0:SV_Position):SV_Target{c f0=U0.xy;
#define G6(F1,a) dk F1 a(e U0:SV_Position,uint q9:SV_Coverage,bool Eb:SV_IsFrontFace):SV_Target{c f0=U0.xy;bool H6=Db(Eb);
#else
#define j3(F1,a) EARLYDEPTHSTENCIL F1 a(v0 g0,uint q9:SV_Coverage):SV_Target{c f0=g0.U0.xy;e0 H=e0(floor(f0));uint K0=H.y*j.A6+H.x;
#define G6(F1,a) F1 a(v0 g0,uint q9:SV_Coverage,bool Eb:SV_IsFrontFace):SV_Target{c f0=g0.U0.xy;e0 H=e0(floor(f0));uint K0=H.y*j.A6+H.x;bool H6=Db(Eb);
#endif
#define P2(E) return E;}
#ifdef NEEDS_CLIP_DISTANCE
#define p7 ,out e gl_ClipDistance
#define Y4 ,g0.hi
#else
#define p7
#define Y4
#endif
#define W6 ,c f0
#define e3 ,f0
#define h4 ,e0 H
#define U1 ,H
#define T1(a) EARLYDEPTHSTENCIL void a(v0 g0){c f0=g0.U0.xy;e0 H=e0(floor(f0));uint K0=H.y*j.A6+H.x;
#if defined(FIXED_FUNCTION_COLOR_OUTPUT)&&defined(DRAW_IMAGE_MESH)
#define h2 z3
#else
#define h2 }
#endif
#define z2(a) EARLYDEPTHSTENCIL i a(v0 g0):SV_Target{c f0=g0.U0.xy;e0 H=e0(floor(f0));uint K0=H.y*j.A6+H.x;i K1;
#define z3 }return K1;
#define uintBitsToFloat asfloat
#define floatBitsToInt asint
#define floatBitsToUint asuint
#define inversesqrt rsqrt
#define equal(B,J) ((B)==(J))
#define notEqual(B,J) ((B)!=(J))
#define lessThan(B,J) ((B)<(J))
#define greaterThan(B,J) ((B)>(J))
#define M0(B,J) mul(J,B)
#define P4
#define Q4
#define f4
#define g4
#define Z5(g,E1,a) StructuredBuffer<O0>a
#define W4(g,E1,a) StructuredBuffer<M>a
#define a6(g,E1,a) StructuredBuffer<e>a
#define p0(a,D0) a[D0]
#define l5(a,D0) a[D0]
f C unpackHalf2x16(uint u){uint y=(u>>16);uint x=u&0xffffu;return C(f16tof32(x),f16tof32(y));}f uint packHalf2x16(c r2){uint x=f32tof16(r2.x);uint y=f32tof16(r2.y);return(y<<16)|x;}f i unpackUnorm4x8(uint u){M q1=M(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return i(q1)*(1./255.);}f c unpackUnorm2x16(uint u){O0 q1=O0(u&0xffffu,u>>16);return c(q1)*(1./65535.);}f uint packUnorm4x8(i p){M q1=(M(saturate(p)*255.)&0xff)<<M(0,8,16,24);q1.xy|=q1.zw;q1.x|=q1.y;return q1.x;}f Y inverse(Y y1){Y ub=Y(y1[1][1],-y1[0][1],-y1[1][0],y1[0][0]);return ub*(1./determinant(y1));}f float mix(float x,float y,float s){return lerp(x,y,s);}f c mix(c x,c y,c s){return lerp(x,y,s);}f O mix(O x,O y,O s){return lerp(x,y,s);}f e mix(e x,e y,e s){return lerp(x,y,s);}f float fract(float x){return frac(x);}f c fract(c x){return frac(x);}f O fract(O x){return frac(x);}f e fract(e x){return frac(x);}f float mod(float x,float y){return fmod(x,y);}f float V2(float x){return sign(x);}f c V2(c x){return sign(x);}f O V2(O x){return sign(x);}f e V2(e x){return sign(x);}
#define sign V2
f float W2(float x){return abs(x);}f c W2(c x){return abs(x);}f O W2(O x){return abs(x);}f e W2(e x){return abs(x);}
#define abs W2
f float X2(float x){return sqrt(x);}f c X2(c x){return sqrt(x);}f O X2(O x){return sqrt(x);}f e X2(e x){return sqrt(x);}
#define sqrt X2
