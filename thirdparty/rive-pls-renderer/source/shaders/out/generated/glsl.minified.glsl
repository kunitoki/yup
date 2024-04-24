#ifndef GLSL_VERSION
#define GLSL_VERSION __VERSION__
#endif
#define d vec2
#define C1 vec3
#define G3 vec3
#define g vec4
#define h mediump float
#define l0 mediump vec2
#define O mediump vec3
#define i mediump vec4
#define E0 float
#define Y1 vec2
#define j0 vec3
#define M0 vec4
#define m0 ivec2
#define F5 ivec4
#define Z5 mediump int
#define y0 uvec2
#define T uvec4
#define M mediump uint
#define O0 uint
#define A mat2
#define i5 mat3x4
#define p
#define x2(g4) out g4
#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin:require
#endif
#ifdef ENABLE_BINDLESS_TEXTURES
#extension GL_ARB_bindless_texture:require
#endif
#ifdef ENABLE_KHR_BLEND
#extension GL_KHR_blend_equation_advanced:require
#endif
#ifdef USING_DEPTH_STENCIL
#ifdef ENABLE_CLIP_RECT
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#endif
#if GLSL_VERSION>=310
#define W3(c,a) layout(binding=c,std140)uniform a{
#else
#define W3(c,a) layout(std140)uniform a{
#endif
#define F4(a) }a;
#define T0(a)
#define q0(c,G,a) layout(location=c)in G a
#define U0
#define w0(a6,r,a,G)
#ifdef VERTEX
#if GLSL_VERSION>=310
#define I(c,G,a) layout(location=c)out G a
#else
#define I(c,G,a) out G a
#endif
#else
#if GLSL_VERSION>=310
#define I(c,G,a) layout(location=c)in G a
#else
#define I(c,G,a) in G a
#endif
#endif
#define c6 flat
#define z1
#define A1
#ifdef TARGET_VULKAN
#define k0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define k0 noperspective
#else
#define k0
#endif
#endif
#ifdef VERTEX
#define N1
#define O1
#endif
#ifdef FRAGMENT
#define E2
#define F2
#endif
#ifdef TARGET_VULKAN
#define P2(c,a) layout(binding=c)uniform highp utexture2D a
#define h4(c,a) layout(binding=c)uniform highp texture2D a
#define w1(c,a) layout(binding=c)uniform mediump texture2D a
#elif GLSL_VERSION>=310
#define P2(c,a) layout(binding=c)uniform highp usampler2D a
#define h4(c,a) layout(binding=c)uniform highp sampler2D a
#define w1(c,a) layout(binding=c)uniform mediump sampler2D a
#else
#define P2(c,a) uniform highp usampler2D a
#define h4(c,a) uniform highp sampler2D a
#define w1(c,a) uniform mediump sampler2D a
#endif
#define Y9(c,a) P2(c,a)
#ifdef TARGET_VULKAN
#define I3(c2,a) layout(binding=c2,set=y7)uniform mediump sampler a;
#define k3(c2,a) layout(binding=c2,set=y7)uniform mediump sampler a;
#define L2(a,p0,F) texture(sampler2D(a,p0),F)
#define Q3(a,p0,F,W2) textureLod(sampler2D(a,p0),F,W2)
#define P3(a,p0,F,X2,Y2) textureGrad(sampler2D(a,p0),F,X2,Y2)
#else
#define I3(c2,a)
#define k3(c2,a)
#define L2(a,p0,F) texture(a,F)
#define Q3(a,p0,F,W2) textureLod(a,F,W2)
#define P3(a,p0,F,X2,Y2) textureGrad(a,F,X2,Y2)
#endif
#define H1(a,F) texelFetch(a,F,0)
#define U1
#define V1
#define K3
#define N3
#ifdef DISABLE_SHADER_STORAGE_BUFFERS
#define L3(c,a1,a) P2(c,a)
#define Q2(c,a1,a) Y9(c,a)
#define M3(c,a1,a) h4(c,a)
#define z0(a,A0) H1(a,m0((A0)&j7,(A0)>>i7))
#define p2(a,A0) H1(a,m0((A0)&j7,(A0)>>i7)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define L3(c,a1,a) layout(std430,binding=c)readonly buffer a1{y0 T4[];}a
#define Q2(c,a1,a) layout(std430,binding=c)readonly buffer a1{T T4[];}a
#define M3(c,a1,a) layout(std430,binding=c)readonly buffer a1{g T4[];}a
#define z0(a,A0) a.T4[A0]
#define p2(a,A0) a.T4[A0]
#endif
#ifdef PLS_IMPL_WEBGL
#extension GL_ANGLE_shader_pixel_local_storage:require
#define D1
#define C0(c,a) layout(binding=c,rgba8)uniform lowp pixelLocalANGLE a
#define D0(c,a) layout(binding=c,r32ui)uniform highp upixelLocalANGLE a
#define E1
#define N0(e) pixelLocalLoadANGLE(e)
#define L0(e) pixelLocalLoadANGLE(e).x
#define F0(e,l) pixelLocalStoreANGLE(e,l)
#define Z0(e,l) pixelLocalStoreANGLE(e,uvec4(l))
#define d0(e)
#define m1
#define n1
#endif
#ifdef PLS_IMPL_EXT_NATIVE
#extension GL_EXT_shader_pixel_local_storage:enable
#extension GL_ARM_shader_framebuffer_fetch:enable
#extension GL_EXT_shader_framebuffer_fetch:enable
#define D1 __pixel_localEXT H0{
#define C0(c,a) layout(rgba8)lowp vec4 a
#define D0(c,a) layout(r32ui)highp uint a
#define E1 };
#define N0(e) e
#define L0(e) e
#define F0(e,l) e=(l)
#define Z0(e,l) e=(l)
#define d0(e)
#define m1
#define n1
#endif
#ifdef PLS_IMPL_FRAMEBUFFER_FETCH
#extension GL_EXT_shader_framebuffer_fetch:require
#define D1
#define C0(c,a) layout(location=c)inout lowp vec4 a
#define D0(c,a) layout(location=c)inout highp uvec4 a
#define E1
#define N0(e) e
#define L0(e) e.x
#define F0(e,l) e=(l)
#define Z0(e,l) e.x=(l)
#define d0(e) e=e
#define m1
#define n1
#endif
#ifdef PLS_IMPL_STORAGE_TEXTURE
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define m1 beginInvocationInterlockARB()
#define n1 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define m1 beginFragmentShaderOrderingINTEL()
#define n1
#else
#define m1
#define n1
#endif
#define D1
#ifdef TARGET_VULKAN
#define C0(c,a) layout(set=J4,binding=c,rgba8)uniform lowp coherent image2D a
#define D0(c,a) layout(set=J4,binding=c,r32ui)uniform highp coherent uimage2D a
#else
#define C0(c,a) layout(binding=c,rgba8)uniform lowp coherent image2D a
#define D0(c,a) layout(binding=c,r32ui)uniform highp coherent uimage2D a
#endif
#define E1
#define N0(e) imageLoad(e,H)
#define L0(e) imageLoad(e,H).x
#define F0(e,l) imageStore(e,H,l)
#define Z0(e,l) imageStore(e,H,uvec4(l))
#define d0(e)
#ifndef USING_PLS_STORAGE_TEXTURES
#define USING_PLS_STORAGE_TEXTURES
#endif
#endif
#ifdef PLS_IMPL_SUBPASS_LOAD
#define D1
#define C0(c,a) layout(input_attachment_index=c,binding=c,set=J4)uniform lowp subpassInput U4##a;layout(location=c)out lowp vec4 a
#define D0(c,a) layout(input_attachment_index=c,binding=c,set=J4)uniform lowp usubpassInput U4##a;layout(location=c)out highp uvec4 a
#define E1
#define N0(e) subpassLoad(U4##e)
#define L0(e) subpassLoad(U4##e).x
#define F0(e,l) e=(l)
#define Z0(e,l) e.x=(l)
#define d0(e) e=subpassLoad(U4##e)
#define m1
#define n1
#endif
#ifdef PLS_IMPL_NONE
#define D1
#define C0(c,a) layout(location=c)out lowp vec4 a
#define D0(c,a) layout(location=c)out highp uvec4 a
#define E1
#define N0(e) vec4(0)
#define L0(e) 0u
#define F0(e,l) e=(l)
#define Z0(e,l) e.x=(l)
#define d0(e)
#define m1
#define n1
#endif
#define C4(e)
#ifdef TARGET_VULKAN
#define gl_VertexID gl_VertexIndex
#endif
#ifdef ENABLE_INSTANCE_INDEX
#ifdef TARGET_VULKAN
#define V4 gl_InstanceIndex
#else
#ifdef ENABLE_SPIRV_CROSS_BASE_INSTANCE
uniform int SPIRV_Cross_BaseInstance;
#define V4 (gl_InstanceID+SPIRV_Cross_BaseInstance)
#else
#define V4 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define V4 0
#endif
#define T2
#define i3
#define e1(a,P,r,j,L) void main(){int j=gl_VertexID;int L=V4;
#define j5 e1
#define n4(a,W1,X1,k2,l2,j) e1(a,W1,X1,j,L)
#define Q(a,G)
#define S(a)
#define N(a,G)
#define f1(p1) gl_Position=p1;}
#define q2(i4,a) layout(location=0)out i4 Z9;void main()
#define r2(l) Z9=l
#define n0 gl_FragCoord.xy
#define v4
#define K2
#ifdef USING_PLS_STORAGE_TEXTURES
#define J3(c,a) layout(binding=c,r32ui)uniform highp coherent uimage2D a
#define n3(e) imageLoad(e,H).x
#define o3(e,l) imageStore(e,H,uvec4(l))
#define A4(e,R0) imageAtomicMax(e,H,R0)
#define B4(e,R0) imageAtomicAdd(e,H,R0)
#define G2 ,m0 H
#define j1 ,H
#define Q1(a) void main(){m0 H=ivec2(floor(n0));
#define a2 }
#else
#define G2
#define j1
#define Q1(a) void main()
#define a2
#endif
#define T3(a) Q1(a)
#define m3(a) layout(location=0)out i P0;Q1(a);
#define y4(a) layout(location=0)out i P0;Q1(a)
#define U3 a2
#define h0(e0,g0) ((e0)*(g0))
#if GLSL_VERSION<310
p i unpackUnorm4x8(uint u){T d2=T(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(d2)*(1./255.);}
#endif
#ifndef TARGET_VULKAN
#define w9
#endif
precision highp float;precision highp int;