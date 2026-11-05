#define lb
#ifndef GLSL_VERSION
#define GLSL_VERSION __VERSION__
#endif
#define c vec2
#define M vec3
#define l4 vec3
#define e vec4
#define d mediump float
#define F mediump vec2
#define v mediump vec3
#define i mediump vec4
#define G7 mediump mat3x3
#define H7 mediump mat2x3
#define n5 mediump mat4x4
#define f0 ivec2
#define D6 ivec4
#define R0 uvec2
#define R uvec4
#define O mediump uint
#define m5 bvec2
#define Y6 bvec3
#define l8 bvec4
#define X mat2
#define f
#define p1(H2) out H2
#define p7(H2) inout H2
#ifdef GL_ANGLE_base_vertex_base_instance_shader_builtin
#extension GL_ANGLE_base_vertex_base_instance_shader_builtin:require
#endif
#ifdef ENABLE_KHR_BLEND
#extension GL_KHR_blend_equation_advanced:require
#endif
#ifdef ATLAS_RENDER_TARGET_R32UI_FRAMEBUFFER_FETCH
#extension GL_EXT_shader_framebuffer_fetch:require
#elif defined(ATLAS_RENDER_TARGET_R8_PLS_EXT)
#extension GL_EXT_shader_pixel_local_storage:require
#elif defined(ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
#extension GL_ANGLE_shader_pixel_local_storage:require
#elif defined(ATLAS_RENDER_TARGET_R32I_ATOMIC_TEXTURE)
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#ifdef GL_OES_shader_image_atomic
#extension GL_OES_shader_image_atomic:require
#endif
#endif
#if defined(RENDER_MODE_DEPTH_STENCIL)&&defined(ENABLE_CLIP_RECT)&&defined(GL_ES)&&!defined(DISABLE_CLIP_DISTANCE_FOR_UBERSHADERS)
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#if GLSL_VERSION>=310
#define k8(g,a) layout(binding=g,std140) uniform a{
#else
#define k8(g,a) layout(std140) uniform a{
#endif
#define T9(a) }a;
#define h1(a)
#define K(g,k0,a) layout(location=g) in k0 a
#define i1
#define L(W9,A,a,k0)
#ifdef VERTEX
#if GLSL_VERSION>=310
#define W(g,k0,a) layout(location=g) out k0 a
#else
#define W(g,k0,a) out k0 a
#endif
#else
#if GLSL_VERSION>=310
#define W(g,k0,a) layout(location=g) in k0 a
#else
#define W(g,k0,a) in k0 a
#endif
#endif
#define l3 flat
#define o2
#define P1
#ifdef TARGET_SPIRV
#define I0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define I0 noperspective
#else
#define I0
#endif
#endif
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
#ifdef TARGET_SPIRV
#define v5(h0,g,a) layout(set=h0,binding=g) uniform highp utexture2D a
#define Z6(h0,g,a) layout(set=h0,binding=g) uniform highp texture2D a
#define Z2(h0,g,a) layout(set=h0,binding=g) uniform mediump texture2D a
#define U5(h0,g,a) layout(binding=g) uniform mediump texture2D a
#define oc(h0,g,a) layout(binding=g) uniform highp utexture2D a
#if defined(FRAGMENT)&&defined(RENDER_MODE_DEPTH_STENCIL)
#endif
#elif GLSL_VERSION>=310
#define v5(h0,g,a) layout(binding=g) uniform highp usampler2D a
#define Z6(h0,g,a) layout(binding=g) uniform highp sampler2D a
#define Z2(h0,g,a) layout(binding=g) uniform mediump sampler2D a
#define U5(h0,g,a) layout(binding=g) uniform mediump sampler2D a
#define oc(h0,g,a) layout(binding=g) uniform highp usampler2D a
#else
#define v5(h0,g,a) uniform highp usampler2D a
#define Z6(h0,g,a) uniform highp sampler2D a
#define Z2(h0,g,a) uniform mediump sampler2D a
#define U5(h0,g,a) uniform mediump sampler2D a
#define oc(h0,g,a) uniform highp usampler2D a
#endif
#ifdef TARGET_SPIRV
#define a7(h0,g,a) layout(set=h0,binding=g) uniform mediump sampler a;
#ifdef USE_WEBGPU_SAMPLERS
#define f4(Y5,a) layout(set=Yh,binding=Y5) uniform mediump sampler a;
#define Q6 f4
#define D4(a) a7(J5,Xh,a)
#else
#define f4(Y5,a) layout(set=O2,binding=Y5) uniform mediump sampler a;
#define Q6 f4
#define D4(a) a7(J5,C4,a)
#endif
#define Z5(a,p,o) texture(sampler2D(a,p),o)
#define r2(a,p,o,g1) textureLod(sampler2D(a,p),o,g1)
#define a6(a,p,o,j2) texture(sampler2D(a,p),o,j2)
#if defined(FRAGMENT)&&defined(RENDER_MODE_DEPTH_STENCIL)&&defined(MSAA_DST_COLOR)
#extension GL_OES_sample_variables:require
#endif
#else
#define f4(Y5,a)
#define Q6(Y5,a)
#define a7(h0,g,a)
#define D4(a)
#define Z5(a,p,o) texture(a,o)
#define r2(a,p,o,g1) textureLod(a,o,g1)
#define a6(a,p,o,j2) texture(a,o,j2)
#endif
#define g9(v0,p,o) Z5(v0,p,o)
#define k5(v0,p,o,g1) r2(v0,p,o,g1)
#define m8(v0,p,o,j2) a6(v0,p,o,j2)
#define P6(h0,g,a) U5(h0,g,a)
#define F7(a,p,G,c7,Y9,g1) r2(a,p,c(G,Y9),g1)
#define dj(h0,g,a) v5(h0,g,a)
#define g4
#define r1
#define j1(a,o) texelFetch(a,o,0)
#ifdef TARGET_SPIRV
#elif GLSL_VERSION>=310
#else
#endif
#define i5
#define j5
#define n4
#define p4
#ifdef DISABLE_SHADER_STORAGE_BUFFERS
#define o4(g,a1,a) v5(O2,g,a)
#define c8(g,a1,a) oc(O2,g,a)
#define p6(g,a1,a) Z6(O2,g,a)
#define p0(a,m0) j1(a,f0((m0)&Bb,(m0)>>Ab))
#define Q5(a,m0) j1(a,f0((m0)&Bb,(m0)>>Ab)).x
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define o4(g,a1,a) layout(std430,binding=g) readonly buffer a1{R k2[];}a
#define c8(g,a1,a) layout(std430,binding=g) readonly buffer a1{uint k2[];}a
#define p6(g,a1,a) layout(std430,binding=g) readonly buffer a1{e k2[];}a
#define pc(g,a1,a) layout(std430,binding=g) buffer a1{uint k2[];}a
#define p0(a,m0) a.k2[m0]
#define Q5(a,m0) a.k2[m0]
#define o8(a,m0,G) atomicMax(a.k2[m0],G)
#define qc(a,m0,G) atomicAdd(a.k2[m0],G)
#define ej(a,m0,G) atomicOr(a.k2[m0],G)
#endif
#ifdef PLS_IMPL_STORAGE_BUFFER
#define E1(a) void main(){f0 D=ivec2(floor(Y));int E0=int(S9(uvec2(D),(j.X5+(Ob-1u))&~(Ob-1u)));
#define L1 }
#define r4 ,int E0
#define d2 ,E0
#ifdef TARGET_WGSL
#define Y2(g,a) layout(std430,set=N3,binding=g) buffer a##lf{uint k2[];}a
#elif defined(TARGET_SPIRV)
#define Y2(g,a) layout(std430,set=N3,binding=g) coherent buffer a##lf{uint k2[];}a
#else
#define Y2(g,a) layout(std430,binding=g) coherent buffer a##lf{uint k2[];}a
#endif
#define L4 Y2
#define r3(h) h.k2[E0]
#define v3(h,B) h.k2[E0]=B
#define e4(h) unpackUnorm4x8(r3(h))
#define r5(h,B) v3(h,packUnorm4x8(B))
#define H5(h,G) atomicMax(h.k2[E0],G)
#define I5(h,G) atomicAdd(h.k2[E0],G)
#elif defined(PLS_IMPL_STORAGE_TEXTURE)||defined(USING_PLS_STORAGE_TEXTURES)
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#define E1(a) void main(){f0 D=ivec2(floor(Y));
#define L1 }
#define r4 ,f0 D
#define d2 ,D
#ifdef TARGET_SPIRV
#define L4(g,a) layout(set=N3,binding=g,rgba8) uniform mediump coherent image2D a
#define Y2(g,a) layout(set=N3,binding=g,r32ui) uniform highp coherent uimage2D a
#define rc(g,a) layout(set=N3,binding=g,rgb10_a2) uniform mediump coherent image2D a
#else
#define L4(g,a) layout(binding=g,rgba8) uniform mediump coherent image2D a
#define Y2(g,a) layout(binding=g,r32ui) uniform highp coherent uimage2D a
#define rc(g,a) layout(binding=g,rgb10_a2) uniform mediump coherent image2D a;
#endif
#define r3(h) imageLoad(h,D).x
#define v3(h,B) imageStore(h,D,uvec4(B))
#define e4(h) imageLoad(h,D)
#define r5(h,B) imageStore(h,D,B)
#define H5(h,G) imageAtomicMax(h,D,G)
#define I5(h,G) imageAtomicAdd(h,D,G)
#else
#define E1(a) void main()
#define L1
#define r4
#define d2
#endif
#ifdef PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define I1
#define z0(g,a) layout(binding=g,rgba8) uniform mediump pixelLocalANGLE a
#define x1(g,a) layout(binding=g,r32ui) uniform highp upixelLocalANGLE a
#define J1
#define L0(h) pixelLocalLoadANGLE(h)
#define o1(h) pixelLocalLoadANGLE(h).x
#define y0(h,B) pixelLocalStoreANGLE(h,B)
#define q1(h,B) pixelLocalStoreANGLE(h,uvec4(B))
#define Q2(h)
#define l2(h)
#define R2
#define S2
#endif
#ifdef PLS_IMPL_EXT_NATIVE
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#extension GL_EXT_shader_pixel_local_storage2:require
#else
#extension GL_EXT_shader_pixel_local_storage:require
#endif
#define I1 __pixel_localEXT V1{
#define z0(g,a) layout(rgba8) mediump vec4 a
#define sc(g,a) layout(rgb10_a2) mediump vec4 a
#define x1(g,a) layout(r32ui) highp uint a
#define J1 };
#define L0(h) h
#define o1(h) h
#define y0(h,B) h=(B)
#define q1(h,B) h=(B)
#define Q2(h) h=h
#define l2(h) h=h
#define R2
#define S2
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#define K2(a) layout(location=0,rgba8) out i S1;E1(a)
#endif
#endif
#if defined(PLS_IMPL_STORAGE_TEXTURE)||defined(PLS_IMPL_STORAGE_BUFFER)
#define I1
#define J1
#define z0 L4
#define x1 Y2
#define sc rc
#define L0 e4
#define y0 r5
#define o1 r3
#define q1 v3
#define Q2(h)
#define l2(h)
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define R2 beginInvocationInterlockARB()
#define S2 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define R2 beginFragmentShaderOrderingINTEL()
#define S2
#else
#define R2
#define S2
#endif
#endif
#ifdef PLS_IMPL_SUBPASS_LOAD
#define I1
#define c5(g,a) layout(input_attachment_index=g,binding=g,set=N3) uniform mediump subpassInput p8##a
#define mf(g,a) layout(location=g) out mediump vec4 a
#define z0(g,a) c5(g,a);mf(g,a)
#define x1(g,a) layout(input_attachment_index=g,binding=g,set=N3) uniform highp usubpassInput p8##a;layout(location=g) out highp uvec4 a
#define J1
#define L0(h) subpassLoad(p8##h)
#define o1(h) subpassLoad(p8##h).x
#define y0(h,B) h=(B)
#define q1(h,B) h.x=(B)
#define Q2(h) y0(h,subpassLoad(p8##h))
#define l2(h) q1(h,subpassLoad(p8##h).x)
#define R2
#define S2
#endif
#ifdef PLS_IMPL_NONE
#define I1
#define z0(g,a) layout(location=g) out mediump vec4 a
#define x1(g,a) layout(location=g) out highp uvec4 a
#define J1
#define L0(h) vec4(0)
#define o1(h) 0u
#define y0(h,B) h=(B)
#define q1(h,B) h.x=(B)
#define Q2(h) h=vec4(0)
#define l2(h) h.x=0u
#define R2
#define S2
#endif
#ifndef c5
#define c5 z0
#endif
#ifdef TARGET_SPIRV
#define tc gl_VertexIndex
#ifdef ENABLE_INSTANCE_INDEX
#define q8 gl_InstanceIndex
#else
#define q8 0
#endif
#else
#ifdef ENABLE_BASE_VERTEX
uniform highp int ZE;
#define tc (gl_VertexID+ZE)
#else
#define tc gl_VertexID
#endif
#ifdef ENABLE_INSTANCE_INDEX
#ifdef BASE_INSTANCE_UNIFORM_NAME
uniform highp int BASE_INSTANCE_UNIFORM_NAME;
#define q8 (gl_InstanceID+BASE_INSTANCE_UNIFORM_NAME)
#else
#define q8 (gl_InstanceID+gl_BaseInstance)
#endif
#else
#define q8 0
#endif
#endif
#define T6
#define S3
#define C6
#define I4
#define n1(a,d0,A,c4,H6) void main(){int c4=tc;int H6=q8;
#define O8(a,d0,A,H1,j0,c4,H6) n1(a,d0,A,c4,H6)
#define n7(a,G3,H3,V3,n3,H1,j0,c4) n1(a,G3,H3,c4,H6)
#define V(a,k0)
#define e0(a)
#define q(a,k0)
#define C1(T0) gl_Position=T0;}
#define v2(U0,a) layout(location=0) out U0 fj;void main()
#define d7(U0,a) v2(U0,a)
#define e7 gl_FrontFacing
#define w2(B) fj=B
#define Y gl_FragCoord.xy
#define q7
#define q3
#if defined(PLS_IMPL_STORAGE_TEXTURE)||defined(PLS_IMPL_STORAGE_BUFFER)
#define nf(r8,h,B) if(!(r8)){y0(h,B);}
#define of(r8,h,B) if(!(r8)){q1(h,B);}
#else
#define nf(r8,h,B) y0(h,B);
#define of(r8,h,B) q1(h,B);
#endif
#ifndef K2
#define K2(a) layout(location=0) out i S1;E1(a)
#endif
#define I3 L1
#if defined(TARGET_SPIRV)&&!defined(TARGET_WGSL)
#ifdef MSAA_DST_COLOR
#define K4(a) layout(input_attachment_index=0,binding=B2,set=N3) uniform mediump subpassInputMS a
#define J4(a) wb(mat4(subpassLoad(a,0),subpassLoad(a,1),subpassLoad(a,2),subpassLoad(a,3)),gl_SampleMaskIn[0])
#else
#define K4(a) layout(input_attachment_index=0,binding=B2,set=N3) uniform mediump subpassInput a
#define J4(a) subpassLoad(a)
#endif
#else
#define K4(a) Z2(O2,ve,a)
#define J4(a) texelFetch(a,ivec2(floor(Y.xy)),0)
#endif
#define G0(E,J) ((E)*(J))
precision highp float;precision highp int;
#if GLSL_VERSION<310
f i gj(uint u){R z1=R(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return e(z1)*(1./255.);}
#define unpackUnorm4x8 gj
#endif
