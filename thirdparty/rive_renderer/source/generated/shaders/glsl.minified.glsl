#define r9
#ifndef GLSL_VERSION
#define GLSL_VERSION __VERSION__
#endif
#define c vec2
#define a0 vec3
#define R3 vec3
#define e vec4
#define h mediump float
#define C mediump vec2
#define p mediump vec3
#define i mediump vec4
#define j5 mediump mat3x3
#define k5 mediump mat2x3
#define f0 ivec2
#define p6 ivec4
#define G5 mediump int
#define E0 uvec2
#define D uvec4
#define O mediump uint
#define D4 bvec2
#define L6 bvec3
#define Y mat2
#define d
#define G1(D1) out D1
#define Y3(D1) inout D1
#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin:require
#endif
#ifdef ENABLE_KHR_BLEND
#extension GL_KHR_blend_equation_advanced:require
#endif
#if defined(RENDER_MODE_MSAA)&&defined(ENABLE_CLIP_RECT)&&defined(GL_ES)
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#if GLSL_VERSION>=310
#define G4(f,a) layout(binding=f,std140)uniform a{
#else
#define G4(f,a) layout(std140)uniform a{
#endif
#define l5(a) }a;
#define L0(a)
#define h0(f,U,a) layout(location=f)in U a
#define M0
#define k0(N6,o,a,U)
#ifdef VERTEX
#if GLSL_VERSION>=310
#define H(f,U,a) layout(location=f)out U a
#else
#define H(f,U,a) out U a
#endif
#else
#if GLSL_VERSION>=310
#define H(f,U,a) layout(location=f)in U a
#else
#define H(f,U,a) in U a
#endif
#endif
#define D2 flat
#define o1
#define p1
#ifdef TARGET_VULKAN
#define o0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define o0 noperspective
#else
#define o0
#endif
#endif
#ifdef VERTEX
#define H2
#define I2
#endif
#ifdef FRAGMENT
#define J2
#define L2
#endif
#ifdef TARGET_VULKAN
#define F3(p0,f,a) layout(set=p0,binding=f)uniform highp utexture2D a
#define S4(p0,f,a) layout(set=p0,binding=f)uniform highp texture2D a
#define o2(p0,f,a) layout(set=p0,binding=f)uniform mediump texture2D a
#define G3(p0,f,a) layout(binding=f)uniform mediump texture2D a
#elif GLSL_VERSION>=310
#define F3(p0,f,a) layout(binding=f)uniform highp usampler2D a
#define S4(p0,f,a) layout(binding=f)uniform highp sampler2D a
#define o2(p0,f,a) layout(binding=f)uniform mediump sampler2D a
#define G3(p0,f,a) layout(binding=f)uniform mediump sampler2D a
#else
#define F3(p0,f,a) uniform highp usampler2D a
#define S4(p0,f,a) uniform highp sampler2D a
#define o2(p0,f,a) uniform mediump sampler2D a
#define G3(p0,f,a) uniform mediump sampler2D a
#endif
#define ed(p0,f,a) F3(p0,f,a)
#ifdef TARGET_VULKAN
#define H3(O2,a) layout(set=W9,binding=O2)uniform mediump sampler a;
#define h4(O2,a) layout(set=W9,binding=O2)uniform mediump sampler a;
#define p3(a,E,A) texture(sampler2D(a,E),A)
#define S1(a,E,A,z2) textureLod(sampler2D(a,E),A,z2)
#else
#define H3(O2,a)
#define h4(O2,a)
#define p3(a,E,A) texture(a,A)
#define S1(a,E,A,z2) textureLod(a,A,z2)
#endif
#define x5
#define z1
#define f1(a,A) texelFetch(a,A,0)
#if GLSL_VERSION>=310
#define F6(a,E,A,T4) textureGather(a,(A)*(T4))
#else
#define F6(a,E,A,T4) f2(f1(a,f0(A)+f0(-1,0)).x,f1(a,f0(A)+f0(0,0)).x,f1(a,f0(A)+f0(0,-1)).x,f1(a,f0(A)+f0(-1,-1)).x)
#endif
#define r3
#define v3
#define j3
#define k3
#ifdef DISABLE_SHADER_STORAGE_BUFFERS
#define W3(f,R0,a) F3(K2,f,a)
#define I3(f,R0,a) ed(K2,f,a)
#define X3(f,R0,a) S4(K2,f,a)
#define r0(a,n0) f1(a,f0((n0)&K9,(n0)>>J9))
#define a4(a,n0) f1(a,f0((n0)&K9,(n0)>>J9)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define W3(f,R0,a) layout(std430,binding=f)readonly buffer R0{E0 M3[];}a
#define I3(f,R0,a) layout(std430,binding=f)readonly buffer R0{D M3[];}a
#define X3(f,R0,a) layout(std430,binding=f)readonly buffer R0{e M3[];}a
#define pc(f,R0,a) layout(std430,binding=f)buffer R0{uint M3[];}a
#define r0(a,n0) a.M3[n0]
#define a4(a,n0) a.M3[n0]
#define vc(a,n0) a.M3[n0]
#define W7(a,n0,N) atomicMax(a.M3[n0],N)
#define ba(a,n0,N) atomicAdd(a.M3[n0],N)
#endif
#ifdef PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define k2
#define N0(f,a) layout(binding=f,rgba8)uniform lowp pixelLocalANGLE a
#define P0(f,a) layout(binding=f,r32ui)uniform highp upixelLocalANGLE a
#define l2
#define C0(g) pixelLocalLoadANGLE(g)
#define V0(g) pixelLocalLoadANGLE(g).x
#define H0(g,r) pixelLocalStoreANGLE(g,r)
#define W0(g,r) pixelLocalStoreANGLE(g,uvec4(r))
#define p2(g)
#define X1(g)
#define W1
#define Y1
#endif
#ifdef PLS_IMPL_EXT_NATIVE
#extension GL_EXT_shader_pixel_local_storage:enable
#define k2 __pixel_localEXT j1{
#define N0(f,a) layout(rgba8)lowp vec4 a
#define P0(f,a) layout(r32ui)highp uint a
#define l2 };
#define C0(g) g
#define V0(g) g
#define H0(g,r) g=(r)
#define W0(g,r) g=(r)
#define p2(g) g=g
#define X1(g) g=g
#define W1
#define Y1
#endif
#ifdef PLS_IMPL_FRAMEBUFFER_FETCH
#extension GL_EXT_shader_framebuffer_fetch:require
#define k2
#define N0(f,a) layout(location=f)inout lowp vec4 a
#define P0(f,a) layout(location=f)inout highp uvec4 a
#define l2
#define C0(g) g
#define V0(g) g.x
#define H0(g,r) g=(r)
#define W0(g,r) g.x=(r)
#define p2(g) H0(g,C0(g))
#define X1(g) W0(g,V0(g))
#define W1
#define Y1
#endif
#ifdef PLS_IMPL_STORAGE_TEXTURE
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define W1 beginInvocationInterlockARB()
#define Y1 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define W1 beginFragmentShaderOrderingINTEL()
#define Y1
#else
#define W1
#define Y1
#endif
#define k2
#ifdef TARGET_VULKAN
#define N0(f,a) layout(set=n5,binding=f,rgba8)uniform lowp coherent image2D a
#define P0(f,a) layout(set=n5,binding=f,r32ui)uniform highp coherent uimage2D a
#else
#define N0(f,a) layout(binding=f,rgba8)uniform lowp coherent image2D a
#define P0(f,a) layout(binding=f,r32ui)uniform highp coherent uimage2D a
#endif
#define l2
#define C0(g) imageLoad(g,F)
#define V0(g) imageLoad(g,F).x
#define H0(g,r) imageStore(g,F,r)
#define W0(g,r) imageStore(g,F,uvec4(r))
#define p2(g)
#define X1(g)
#ifndef USING_PLS_STORAGE_TEXTURES
#define USING_PLS_STORAGE_TEXTURES
#endif
#endif
#ifdef PLS_IMPL_SUBPASS_LOAD
#define k2
#define a9(f,a) layout(input_attachment_index=f,binding=f,set=n5)uniform lowp subpassInput H5##a;
#define N0(f,a) a9(f,a);layout(location=f)out lowp vec4 a
#define P0(f,a) layout(input_attachment_index=f,binding=f,set=n5)uniform highp usubpassInput H5##a;layout(location=f)out highp uvec4 a
#define l2
#define C0(g) subpassLoad(H5##g)
#define V0(g) subpassLoad(H5##g).x
#define H0(g,r) g=(r)
#define W0(g,r) g.x=(r)
#define p2(g) H0(g,subpassLoad(H5##g))
#define X1(g) W0(g,subpassLoad(H5##g).x)
#define W1
#define Y1
#endif
#ifdef PLS_IMPL_NONE
#define k2
#define N0(f,a) layout(location=f)out lowp vec4 a
#define P0(f,a) layout(location=f)out highp uvec4 a
#define l2
#define C0(g) vec4(0)
#define V0(g) 0u
#define H0(g,r) g=(r)
#define W0(g,r) g.x=(r)
#define p2(g) g=vec4(1,0,1,1)
#define X1(g) g.x=0u
#define W1
#define Y1
#endif
#ifdef TARGET_VULKAN
#define gl_VertexID gl_VertexIndex
#endif
#ifdef ENABLE_INSTANCE_INDEX
#ifdef TARGET_VULKAN
#define Q6 gl_InstanceIndex
#else
#ifdef ENABLE_SPIRV_CROSS_BASE_INSTANCE
uniform highp int SPIRV_Cross_BaseInstance;
#define Q6 (gl_InstanceID+SPIRV_Cross_BaseInstance)
#else
#define Q6 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define Q6 0
#endif
#define P4
#define g2
#define Z0(a,e0,o,l,I) void main(){int l=gl_VertexID;int I=Q6;
#define U5 Z0
#define v4(a,P1,Q1,h2,i2,l) Z0(a,P1,Q1,l,I)
#define P(a,U)
#define X(a)
#define Z(a,U)
#define U0(S0) gl_Position=S0;}
#define T1(l4,a) layout(location=0)out l4 fd;void main()
#define U1(r) fd=r
#define v0 gl_FragCoord.xy
#define Z4
#define o3
#ifdef USING_PLS_STORAGE_TEXTURES
#ifdef TARGET_VULKAN
#define V3(f,a) layout(set=n5,binding=f,r32ui)uniform highp coherent uimage2D a
#else
#define V3(f,a) layout(binding=f,r32ui)uniform highp coherent uimage2D a
#endif
#define d4(g) imageLoad(g,F).x
#define e4(g,r) imageStore(g,F,uvec4(r))
#define d5(g,N) imageAtomicMax(g,F,N)
#define e5(g,N) imageAtomicAdd(g,F,N)
#define l3 ,f0 F
#define v1 ,F
#define n2(a) void main(){f0 F=ivec2(floor(v0));
#define E2 }
#else
#define l3
#define v1
#define n2(a) void main()
#define E2
#endif
#define z4(a) n2(a)
#define n3(a) layout(location=0)out i A1;n2(a)
#define c5(a) layout(location=0)out i A1;n2(a)
#define x4 E2
#define q0(G,L) ((G)*(L))
precision highp float;precision highp int;
#if GLSL_VERSION<310
d i unpackUnorm4x8(uint u){D k1=D(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return e(k1)*(1./255.);}
#endif
#if GLSL_VERSION>=310&&defined(VERTEX)&&defined(RENDER_MODE_MSAA)&&defined(ENABLE_CLIP_RECT)
out gl_PerVertex{float gl_ClipDistance[4];e gl_Position;};
#endif
