#ifdef VERTEX
A1(a0)p0(0,g,SB);p0(1,g,TB);B1
#endif
h2 J0 c0(0,g,I);a2
#ifdef VERTEX
C1(KF,a0,G,v,T){q0(v,G,SB,g);q0(v,G,TB,g);Y(I,g);g N;uint l0;c i0;if(p9(SB,TB,T,l0,i0,I v3)){Q J4=P0(MB,l0*4u+2u);V p7=uintBitsToFloat(J4.yzw);i0=i0*p7.x+p7.yz;N=o8(i0,k.ed.x,k.ed.y);
#ifdef POST_INVERT_Y
N.y=-N.y;
#endif
}else{N=g(k.P2,k.P2,k.P2,k.P2);}k0(I);D1(N);}
#endif
#ifdef FRAGMENT
#ifdef ATLAS_FEATHERED_FILL
e d v6(g J,bool Ig F3){d n=d8(J g1);if(!Ig)n=-n;return n;}
#endif
#ifdef ATLAS_RENDER_TARGET_R32UI_FRAMEBUFFER_FETCH
layout(location=0)inout Q m0;
#ifdef ATLAS_FEATHERED_FILL
void main(){float n=uintBitsToFloat(m0.x);n+=v6(I,gl_FrontFacing g1);m0.x=floatBitsToUint(n);}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){float n=uintBitsToFloat(m0.x);n=max(n,v4(I));m0.x=floatBitsToUint(n);}
#endif
#elif defined(ATLAS_RENDER_TARGET_R8_PLS_EXT)
__pixel_localEXT n1{layout(r32f)float m0;};
#ifdef ATLAS_FEATHERED_FILL
void main(){m0+=v6(I,gl_FrontFacing g1);}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){m0=max(m0,v4(I));}
#endif
#elif defined(ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui)uniform highp upixelLocalANGLE m0;
#ifdef ATLAS_FEATHERED_FILL
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(m0).x);n+=v6(I,gl_FrontFacing g1);pixelLocalStoreANGLE(m0,Q(floatBitsToUint(n)));}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){float n=uintBitsToFloat(pixelLocalLoadANGLE(m0).x);n=max(n,v4(I));pixelLocalStoreANGLE(m0,Q(floatBitsToUint(n)));}
#endif
#elif defined(ATLAS_RENDER_TARGET_R32I_ATOMIC_TEXTURE)
layout(binding=0,r32i)uniform highp coherent iimage2D V8;ivec2 Ad(){return ivec2(floor(S));}int Bd(float n){return int(n*Ec);}
#ifdef ATLAS_FEATHERED_FILL
void main(){int n=Bd(v6(I,gl_FrontFacing g1));imageAtomicAdd(V8,Ad(),n);}
#endif
#ifdef ATLAS_FEATHERED_STROKE
void main(){int n=Bd(v4(I));imageAtomicMax(V8,Ad(),n);}
#endif
#elif defined(ATLAS_RENDER_TARGET_RGBA8_UNORM)
#ifdef ATLAS_FEATHERED_FILL
q6(i,NE){B(I,g);d n=v6(I,r6 g1);if(abs(n)>qf-1e-3){G2(n>.0?B0(.0,.0,1./255.,.0):B0(.0,.0,.0,1./255.));}else{n*=1./ka;G2(B0(max(n,.0),max(-n,.0),.0,.0));}}
#endif
#ifdef ATLAS_FEATHERED_STROKE
Y2(i,OE){B(I,g);d n=v4(I g1);n*=1./ka;G2(B0(n,.0,.0,.0));}
#endif
#else
#ifdef ATLAS_FEATHERED_FILL
q6(float,NE){B(I,g);G2(v6(I,r6 g1));}
#endif
#ifdef ATLAS_FEATHERED_STROKE
Y2(float,OE){B(I,g);G2(v4(I g1));}
#endif
#endif
#endif
