#ifdef VERTEX
h1(d0) K(0,e,YB);K(1,e,ZB);i1
#endif
o2 I0 W(0,e,S);P1
#ifdef VERTEX
n1(FG,d0,A,C,r){L(C,A,YB,e);L(C,A,ZB,e);V(S,e);e I;uint g0;c i0;if(xa(YB,ZB,r,g0,i0,S S3)){R xj=p0(JB,g0*4u+3u);M i8=uintBitsToFloat(xj.xyz);i0=i0*i8.x+i8.yz;I=p9(i0,j.af.x,j.af.y);
#ifdef POST_INVERT_Y
I.y=-I.y;
#endif
}else{I=e(j.m3,j.m3,j.m3,j.m3);}e0(S);C1(I);}
#endif
#ifdef FRAGMENT
#ifdef ATLAS_FEATHERED_FILL
f d f7(e T,bool yj g4){d n=d9(T r1);if(!yj) n=-n;return n;}
#endif
#ifdef ATLAS_RENDER_TARGET_R32UI_FRAMEBUFFER_FETCH
layout(location=0) inout R x0;
#ifdef ATLAS_FEATHERED_FILL
void main(){float n=uintBitsToFloat(x0.x);n+=f7(S,gl_FrontFacing r1);x0.x=floatBitsToUint(n);}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){float n=uintBitsToFloat(x0.x);n=max(n,f5(S));x0.x=floatBitsToUint(n);}
#endif
#elif defined(ATLAS_RENDER_TARGET_R8_PLS_EXT)
__pixel_localEXT V1{layout(r32f) float x0;};
#ifdef ATLAS_FEATHERED_FILL
void main(){x0+=f7(S,gl_FrontFacing r1);}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){x0=max(x0,f5(S));}
#endif
#elif defined(ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui) uniform highp upixelLocalANGLE x0;
#ifdef ATLAS_FEATHERED_FILL
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(x0).x);n+=f7(S,gl_FrontFacing r1);pixelLocalStoreANGLE(x0,R(floatBitsToUint(n)));}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(x0).x);n=max(n,f5(S));pixelLocalStoreANGLE(x0,R(floatBitsToUint(n)));}
#endif
#elif defined(ATLAS_RENDER_TARGET_R32I_ATOMIC_TEXTURE)
layout(binding=0,r32i) uniform highp coherent iimage2D fa;ivec2 xf(){return ivec2(floor(Y));}int yf(float n){return int(n*we);}
#ifdef ATLAS_FEATHERED_FILL
void main(){int n=yf(f7(S,gl_FrontFacing r1));imageAtomicAdd(fa,xf(),n);}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){int n=yf(f5(S));imageAtomicMax(fa,xf(),n);}
#endif
#elif defined(ATLAS_RENDER_TARGET_RGBA8_UNORM)
#ifdef ATLAS_FEATHERED_FILL
d7(i,IF){q(S,e);d n=f7(S,e7 r1);if(abs(n)>Ih-1e-3){w2(n>.0?A0(.0,.0,1./255.,.0):A0(.0,.0,.0,1./255.));}else{n*=1./Nb;w2(A0(max(n,.0),max(-n,.0),.0,.0));}}
#endif
#ifdef ATLAS_FEATHERED_STROKE
v2(i,JF){q(S,e);d n=f5(S r1);n*=1./Nb;w2(A0(n,.0,.0,.0));}
#endif
#else
#ifdef ATLAS_FEATHERED_FILL
d7(float,IF){q(S,e);w2(f7(S,e7 r1));}
#endif
#ifdef ATLAS_FEATHERED_STROKE
v2(float,JF){q(S,e);w2(f5(S r1));}
#endif
#endif
#endif
