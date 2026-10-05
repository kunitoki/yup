#ifdef VERTEX
c1(d0) K(0,e,WB);K(1,e,XB);d1
#endif
l2 E0 V(0,e,S);e2
#ifdef VERTEX
v1(YF,d0,D,G,r){L(G,D,WB,e);L(G,D,XB,e);T(S,e);e I;uint a0;c k0;if(K9(WB,XB,r,a0,k0,S G3)){M V3=p0(LB,a0*4u+2u);O F7=uintBitsToFloat(V3.yzw);k0=k0*F7.x+F7.yz;I=F8(k0,j.Ud.x,j.Ud.y);
#ifdef POST_INVERT_Y
I.y=-I.y;
#endif
}else{I=e(j.a3,j.a3,j.a3,j.a3);}Z(S);w1(I);}
#endif
#ifdef FRAGMENT
#ifdef ATLAS_FEATHERED_FILL
f d K6(e U,bool gi R3){d n=q8(U k1);if(!gi) n=-n;return n;}
#endif
#ifdef ATLAS_RENDER_TARGET_R32UI_FRAMEBUFFER_FETCH
layout(location=0) inout M w0;
#ifdef ATLAS_FEATHERED_FILL
void main(){float n=uintBitsToFloat(w0.x);n+=K6(S,gl_FrontFacing k1);w0.x=floatBitsToUint(n);}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){float n=uintBitsToFloat(w0.x);n=max(n,M4(S));w0.x=floatBitsToUint(n);}
#endif
#elif defined(ATLAS_RENDER_TARGET_R8_PLS_EXT)
__pixel_localEXT a2{layout(r32f) float w0;};
#ifdef ATLAS_FEATHERED_FILL
void main(){w0+=K6(S,gl_FrontFacing k1);}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){w0=max(w0,M4(S));}
#endif
#elif defined(ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui) uniform highp upixelLocalANGLE w0;
#ifdef ATLAS_FEATHERED_FILL
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(w0).x);n+=K6(S,gl_FrontFacing k1);pixelLocalStoreANGLE(w0,M(floatBitsToUint(n)));}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(w0).x);n=max(n,M4(S));pixelLocalStoreANGLE(w0,M(floatBitsToUint(n)));}
#endif
#elif defined(ATLAS_RENDER_TARGET_R32I_ATOMIC_TEXTURE)
layout(binding=0,r32i) uniform highp coherent iimage2D p9;ivec2 qe(){return ivec2(floor(f0));}int re(float n){return int(n*Bd);}
#ifdef ATLAS_FEATHERED_FILL
void main(){int n=re(K6(S,gl_FrontFacing k1));imageAtomicAdd(p9,qe(),n);}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){int n=re(M4(S));imageAtomicMax(p9,qe(),n);}
#endif
#elif defined(ATLAS_RENDER_TARGET_RGBA8_UNORM)
#ifdef ATLAS_FEATHERED_FILL
G6(i,BF){q(S,e);d n=K6(S,H6 k1);if(abs(n)>Cg-1e-3){P2(n>.0?I0(.0,.0,1./255.,.0):I0(.0,.0,.0,1./255.));}else{n*=1./Sa;P2(I0(max(n,.0),max(-n,.0),.0,.0));}}
#endif
#ifdef ATLAS_FEATHERED_STROKE
j3(i,CF){q(S,e);d n=M4(S k1);n*=1./Sa;P2(I0(n,.0,.0,.0));}
#endif
#else
#ifdef ATLAS_FEATHERED_FILL
G6(float,BF){q(S,e);P2(K6(S,H6 k1));}
#endif
#ifdef ATLAS_FEATHERED_STROKE
j3(float,CF){q(S,e);P2(M4(S k1));}
#endif
#endif
#endif
