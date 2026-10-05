#ifdef VERTEX
v1(ZF,d0,D,G,r){e I;I.x=(G!=2)?-1.:3.;I.y=(G!=1)?-1.:3.;I.zw=c(.0,1.);w1(I);}
#endif
#ifdef FRAGMENT
f ivec2 se(){return ivec2(floor(gl_FragCoord));}
#ifdef ATLAS_RENDER_TARGET_R32UI_FRAMEBUFFER_FETCH
layout(location=0) inout M w0;layout(location=1) out i C4;void main(){C4.x=uintBitsToFloat(w0.x);}
#elif defined(ATLAS_RENDER_TARGET_R8_PLS_EXT)
#ifdef CLEAR_COVERAGE
__pixel_local_outEXT a2{layout(r32f) float w0;};
#else
__pixel_local_inEXT a2{layout(r32f) float w0;};layout(location=0) out i C4;
#endif
void main(){
#ifdef CLEAR_COVERAGE
w0=.0;
#else
C4.x=w0;
#endif
}
#elif defined(ATLAS_RENDER_TARGET_R32UI_PLS_ANGLE)
layout(binding=0,r32ui) uniform highp upixelLocalANGLE w0;layout(location=0) out i C4;void main(){C4.x=uintBitsToFloat(pixelLocalLoadANGLE(w0).x);}
#elif defined(ATLAS_RENDER_TARGET_R32I_ATOMIC_TEXTURE)
layout(binding=0,r32i) uniform highp coherent iimage2D p9;layout(location=0) out i C4;void main(){C4.x=float(imageLoad(p9,se()).x)*(1./Bd);}
#elif defined(ATLAS_RENDER_TARGET_RGBA8_UNORM)
i3(l3,0,DF);layout(location=0) out i C4;void main(){i U=p1(DF,se());C4.x=(U.x-U.y)*Sa+(U.z-U.w)*255.;}
#endif
#endif
