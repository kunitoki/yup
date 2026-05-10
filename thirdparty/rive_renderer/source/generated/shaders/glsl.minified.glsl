#define Xb
#ifndef GLSL_VERSION
#define GLSL_VERSION __VERSION__
#endif
#define c vec2
#define V vec3
#define I3 vec3
#define g vec4
#define d mediump float
#define D mediump vec2
#define r mediump vec3
#define i mediump vec4
#define V6 mediump mat3x3
#define W6 mediump mat2x3
#define g5 mediump mat4x4
#define U ivec2
#define Z5 ivec4
#define W0 uvec2
#define Q uvec4
#define X mediump uint
#define F4 bvec2
#define n6 bvec3
#define w7 bvec4
#define a0 mat2
#define e
#define g1(g2) out g2
#define U4(g2) inout g2
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
#if defined(RENDER_MODE_MSAA)&&defined(ENABLE_CLIP_RECT)&&defined(GL_ES)&&!defined(DISABLE_CLIP_DISTANCE_FOR_UBERSHADERS)
#ifdef GL_EXT_clip_cull_distance
#extension GL_EXT_clip_cull_distance:require
#elif defined(GL_ANGLE_clip_cull_distance)
#extension GL_ANGLE_clip_cull_distance:require
#endif
#endif
#if GLSL_VERSION>=310
#define l6(f,a) layout(binding=f,std140)uniform a{
#else
#define l6(f,a) layout(std140)uniform a{
#endif
#define v7(a) }a;
#define A1(a)
#define r0(f,W,a) layout(location=f)in W a
#define B1
#define v0(N8,G,a,W)
#ifdef VERTEX
#if GLSL_VERSION>=310
#define d0(f,W,a) layout(location=f)out W a
#else
#define d0(f,W,a) out W a
#endif
#else
#if GLSL_VERSION>=310
#define d0(f,W,a) layout(location=f)in W a
#else
#define d0(f,W,a) in W a
#endif
#endif
#define S4 flat
#define h2
#define Z1
#ifdef TARGET_SPIRV
#define J0
#else
#ifdef GL_NV_shader_noperspective_interpolation
#extension GL_NV_shader_noperspective_interpolation:require
#define J0 noperspective
#else
#define J0
#endif
#endif
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
#ifdef TARGET_SPIRV
#define E4(N,f,a) layout(set=N,binding=f)uniform highp utexture2D a
#define d5(N,f,a) layout(set=N,binding=f)uniform highp texture2D a
#define U2(N,f,a) layout(set=N,binding=f)uniform mediump texture2D a
#define j5(N,f,a) layout(binding=f)uniform mediump texture2D a
#if defined(FRAGMENT)&&defined(RENDER_MODE_MSAA)
#endif
#elif GLSL_VERSION>=310
#define E4(N,f,a) layout(binding=f)uniform highp usampler2D a
#define d5(N,f,a) layout(binding=f)uniform highp sampler2D a
#define U2(N,f,a) layout(binding=f)uniform mediump sampler2D a
#define j5(N,f,a) layout(binding=f)uniform mediump sampler2D a
#else
#define E4(N,f,a) uniform highp usampler2D a
#define d5(N,f,a) uniform highp sampler2D a
#define U2(N,f,a) uniform mediump sampler2D a
#define j5(N,f,a) uniform mediump sampler2D a
#endif
#ifdef TARGET_SPIRV
#define o6(N,f,a) layout(set=N,binding=f)uniform mediump sampler a;
#ifdef USE_WEBGPU_SAMPLERS
#define V3(x7,a) layout(set=vf,binding=x7)uniform mediump sampler a;
#define S3(a) o6(Y4,uf,a)
#else
#define V3(x7,a) layout(set=X2,binding=x7)uniform mediump sampler a;
#define S3(a) o6(Y4,R3,a)
#endif
#define p5(a,p,l) texture(sampler2D(a,p),l)
#define m2(a,p,l,X0) textureLod(sampler2D(a,p),l,X0)
#define q5(a,p,l,P1) texture(sampler2D(a,p),l,P1)
#if defined(FRAGMENT)&&defined(RENDER_MODE_MSAA)
#extension GL_OES_sample_variables:require
#endif
#else
#define V3(x7,a)
#define o6(N,f,a)
#define S3(a)
#define p5(a,p,l) texture(a,l)
#define m2(a,p,l,X0) textureLod(a,l,X0)
#define q5(a,p,l,P1) texture(a,l,P1)
#endif
#define g8(h0,p,l) p5(h0,p,l)
#define Q6(h0,p,l,X0) m2(h0,p,l,X0)
#define y7(h0,p,l,P1) q5(h0,p,l,P1)
#define e6(N,f,a) j5(N,f,a)
#define U6(a,p,q,p6,P8,X0) m2(a,p,c(q,P8),X0)
#define og(N,f,a) E4(N,f,a)
#define C3
#define i1
#define F1(a,l) texelFetch(a,l,0)
#ifdef TARGET_SPIRV
#elif GLSL_VERSION>=310
#else
#endif
#define B4
#define C4
#define K3
#define L3
#ifdef DISABLE_SHADER_STORAGE_BUFFERS
#define H5(f,y1,a) E4(X2,f,a)
#define G4(f,y1,a) og(X2,f,a)
#define I5(f,y1,a) d5(X2,f,a)
#define P0(a,y0) F1(a,U((y0)&nc,(y0)>>mc))
#define J5(a,y0) F1(a,U((y0)&nc,(y0)>>mc)).xy
#else
#ifdef GL_ARB_shader_storage_buffer_object
#extension GL_ARB_shader_storage_buffer_object:require
#endif
#define H5(f,y1,a) layout(std430,binding=f)readonly buffer y1{W0 d4[];}a
#define G4(f,y1,a) layout(std430,binding=f)readonly buffer y1{Q d4[];}a
#define I5(f,y1,a) layout(std430,binding=f)readonly buffer y1{g d4[];}a
#define Ea(f,y1,a) layout(std430,binding=f)buffer y1{uint d4[];}a
#define P0(a,y0) a.d4[y0]
#define J5(a,y0) a.d4[y0]
#define kd(a,y0) a.d4[y0]
#define A7(a,y0,q) atomicMax(a.d4[y0],q)
#define Fa(a,y0,q) atomicAdd(a.d4[y0],q)
#define pg(a,y0,q) atomicOr(a.d4[y0],q)
#endif
#ifdef PLS_IMPL_ANGLE
#extension GL_ANGLE_shader_pixel_local_storage:require
#define K1
#define p0(f,a) layout(binding=f,rgba8)uniform mediump pixelLocalANGLE a
#define f1(f,a) layout(binding=f,r32ui)uniform highp upixelLocalANGLE a
#define L1
#define H0(h) pixelLocalLoadANGLE(h)
#define e1(h) pixelLocalLoadANGLE(h).x
#define C0(h,C) pixelLocalStoreANGLE(h,C)
#define h1(h,C) pixelLocalStoreANGLE(h,uvec4(C))
#define r2(h)
#define X1(h)
#define v2
#define w2
#endif
#ifdef PLS_IMPL_EXT_NATIVE
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#extension GL_EXT_shader_pixel_local_storage2:require
#else
#extension GL_EXT_shader_pixel_local_storage:require
#endif
#define K1 __pixel_localEXT n1{
#define p0(f,a) layout(rgba8)mediump vec4 a
#define Q8(f,a) layout(rgb10_a2)mediump vec4 a
#define f1(f,a) layout(r32ui)highp uint a
#define L1 };
#define H0(h) h
#define e1(h) h
#define C0(h,C) h=(C)
#define h1(h,C) h=(C)
#define r2(h) h=h
#define X1(h) h=h
#define v2
#define w2
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#define o2(a) layout(location=0,rgba8)out i m1;v1(a)
#define v4(a) layout(location=0,rgba8)out i m1;v1(a)
#endif
#endif
#ifdef PLS_IMPL_STORAGE_TEXTURE
#ifdef GL_ARB_shader_image_load_store
#extension GL_ARB_shader_image_load_store:require
#endif
#if defined(GL_ARB_fragment_shader_interlock)
#extension GL_ARB_fragment_shader_interlock:require
#define v2 beginInvocationInterlockARB()
#define w2 endInvocationInterlockARB()
#elif defined(GL_INTEL_fragment_shader_ordering)
#extension GL_INTEL_fragment_shader_ordering:require
#define v2 beginFragmentShaderOrderingINTEL()
#define w2
#else
#define v2
#define w2
#endif
#define K1
#ifdef TARGET_SPIRV
#define p0(f,a) layout(set=B3,binding=f,rgba8)uniform mediump coherent image2D a
#define Q8(f,a) layout(set=B3,binding=f,rgb10_a2)uniform mediump coherent image2D a
#define f1(f,a) layout(set=B3,binding=f,r32ui)uniform highp coherent uimage2D a
#else
#define p0(f,a) layout(binding=f,rgba8)uniform mediump coherent image2D a
#define Q8(f,a) layout(binding=f,rgb10_a2)uniform mediump coherent image2D a
#define f1(f,a) layout(binding=f,r32ui)uniform highp coherent uimage2D a
#endif
#define L1
#define H0(h) imageLoad(h,E)
#define e1(h) imageLoad(h,E).x
#define C0(h,C) imageStore(h,E,C)
#define h1(h,C) imageStore(h,E,uvec4(C))
#define r2(h)
#define X1(h)
#ifndef USING_PLS_STORAGE_TEXTURES
#define USING_PLS_STORAGE_TEXTURES
#endif
#endif
#ifdef PLS_IMPL_SUBPASS_LOAD
#define K1
#define m4(f,a) layout(input_attachment_index=f,binding=f,set=B3)uniform mediump subpassInput B7##a
#define ld(f,a) layout(location=f)out mediump vec4 a
#define p0(f,a) m4(f,a);ld(f,a)
#define f1(f,a) layout(input_attachment_index=f,binding=f,set=B3)uniform highp usubpassInput B7##a;layout(location=f)out highp uvec4 a
#define L1
#define H0(h) subpassLoad(B7##h)
#define e1(h) subpassLoad(B7##h).x
#define C0(h,C) h=(C)
#define h1(h,C) h.x=(C)
#define r2(h) C0(h,subpassLoad(B7##h))
#define X1(h) h1(h,subpassLoad(B7##h).x)
#define v2
#define w2
#endif
#ifdef PLS_IMPL_NONE
#define K1
#define p0(f,a) layout(location=f)out mediump vec4 a
#define f1(f,a) layout(location=f)out highp uvec4 a
#define L1
#define H0(h) vec4(0)
#define e1(h) 0u
#define C0(h,C) h=(C)
#define h1(h,C) h.x=(C)
#define r2(h) h=vec4(0)
#define X1(h) h.x=0u
#define v2
#define w2
#endif
#ifndef m4
#define m4 p0
#endif
#ifdef TARGET_SPIRV
#define gl_VertexID gl_VertexIndex
#endif
#ifdef ENABLE_INSTANCE_INDEX
#ifdef TARGET_SPIRV
#define R8 gl_InstanceIndex
#else
#ifdef BASE_INSTANCE_UNIFORM_NAME
uniform highp int BASE_INSTANCE_UNIFORM_NAME;
#define R8 (gl_InstanceID+BASE_INSTANCE_UNIFORM_NAME)
#else
#define R8 (gl_InstanceID+gl_BaseInstance)
#endif
#endif
#else
#define R8 0
#endif
#define h6
#define p3
#define a7
#define r5
#define C1(a,c0,G,v,S) void main(){int v=gl_VertexID;int S=R8;
#define S7 C1
#define E6(a,e3,f3,q3,r3,v) C1(a,e3,f3,v,S)
#define Y(a,W)
#define l0(a)
#define B(a,W)
#define D1(L0) gl_Position=L0;}
#define V2(Q1,a) layout(location=0)out Q1 qg;void main()
#define q6 V2
#define r6 gl_FrontFacing
#define F2(C) qg=C
#define T gl_FragCoord.xy
#define G6
#define R2
#ifdef USING_PLS_STORAGE_TEXTURES
#ifdef TARGET_SPIRV
#define o4(f,a) layout(set=B3,binding=f,r32ui)uniform highp coherent uimage2D a
#define md(f,a) layout(set=B3,binding=f,rgb10_a2)uniform mediump coherent image2D a
#else
#define o4(f,a) layout(binding=f,r32ui)uniform highp coherent uimage2D a
#define md(f,a) layout(binding=f,rgb10_a2)uniform mediump coherent image2D a;
#endif
#define N3(h) imageLoad(h,E).x
#define O3(h,C) imageStore(h,E,uvec4(C))
#define rg(h) imageLoad(h,E)
#define sg(h,C) imageStore(h,E,C)
#define O5(h,q) imageAtomicMax(h,E,q)
#define Q5(h,q) imageAtomicAdd(h,E,q)
#define r4 ,U E
#define U1 ,E
#define v1(a) void main(){U E=ivec2(floor(T));
#define c2 }
#define nd(C7,h,C) if(!(C7)){C0(h,C);}
#define od(C7,h,C) if(!(C7)){h1(h,C);}
#else
#define r4
#define U1
#define v1(a) void main()
#define c2
#define nd(C7,h,C) C0(h,C);
#define od(C7,h,C) h1(h,C);
#endif
#define M5(a) v1(a)
#ifndef o2
#define o2(a) layout(location=0)out i m1;v1(a)
#endif
#ifndef v4
#define v4(a) layout(location=0)out i m1;v1(a)
#endif
#define i3 c2
#if defined(TARGET_SPIRV)&&!defined(INPUT_ATTACHMENT_NONE)
#define g7(a) layout(input_attachment_index=0,binding=P2,set=B3)uniform mediump subpassInputMS a
#define S8(a) jc(mat4(subpassLoad(a,0),subpassLoad(a,1),subpassLoad(a,2),subpassLoad(a,3)),gl_SampleMaskIn[0])
#else
#define g7(a) U2(X2,tf,a)
#define S8(a) texelFetch(a,ivec2(floor(T.xy)),0)
#endif
#define Z0(A,F) ((A)*(F))
precision highp float;precision highp int;
#if GLSL_VERSION<310
e i tg(uint u){Q R1=Q(u&0xffu,(u>>8)&0xffu,(u>>16)&0xffu,u>>24);return g(R1)*(1./255.);}
#define unpackUnorm4x8 tg
#endif
