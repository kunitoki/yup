#ifdef VERTEX
C1(LF,a0,G,v,T){g N;N.x=(v!=2)?-1.:3.;N.y=(v!=1)?-1.:3.;N.zw=c(.0,1.);D1(N);}
#endif
#ifdef FRAGMENT
e ivec2 Cd(){return ivec2(floor(gl_FragCoord));}
#ifdef ATLAS_RENDER_TARGET_R32UI_FRAMEBUFFER_FETCH
layout(location=0)inout Q m0;layout(location=1)out i h4;void main(){h4.x=uintBitsToFloat(m0.x);}
#elif defined(ATLAS_RENDER_TARGET_R8_PLS_EXT)
#ifdef CLEAR_COVERAGE
__pixel_local_outEXT n1{layout(r32f)float m0;};
#else
__pixel_local_inEXT n1{layout(r32f)float m0;};layout(location=0)out i h4;
#endif
void main(){
#ifdef CLEAR_COVERAGE
m0=.0;
#else
h4.x=m0;
#endif
}
#elif defined(ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui)uniform highp upixelLocalANGLE m0;layout(location=0)out i h4;void main(){h4.x=uintBitsToFloat(pixelLocalLoadANGLE(m0).x);}
#elif defined(ATLAS_RENDER_TARGET_R32I_ATOMIC_TEXTURE)
layout(binding=0,r32i)uniform highp coherent iimage2D V8;layout(location=0)out i h4;void main(){h4.x=float(imageLoad(V8,Cd()).x)*(1./Ec);}
#elif defined(ATLAS_RENDER_TARGET_RGBA8_UNORM)
X2(a3,0,PE);layout(location=0)out i h4;void main(){i J=v1(PE,Cd());h4.x=(J.x-J.y)*ka+(J.z-J.w)*255.;}
#endif
#endif
