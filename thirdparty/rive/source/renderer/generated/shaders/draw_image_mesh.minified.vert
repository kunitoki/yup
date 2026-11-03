#ifdef VERTEX
h1(G3) K(0,c,SC);i1 h1(V3) K(1,c,TC);i1 h1(H1) K(ya,e,AC);K(za,e,VB);K(Aa,e,QB);K(Ba,uint,BC);K(Ca,uint,CC);K(Da,uint,DC);K(Ea,uint,NC);K(Ma,e,IC);i1
#endif
o2 I0 W(0,c,j6);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT W(1,d,j4);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
I0 W(2,e,Y0);
#endif
OPTIONALLY_FLAT W(3,i,a2);
#ifdef ENABLE_ADVANCED_BLEND
l3 W(4,O,Q1);
#endif
P1
#ifdef VERTEX
A4 B4 n7(UB,G3,H3,V3,n3,H1,j0,C){L(C,H3,SC,c);L(C,n3,TC,c);L(r,j0,AC,e);L(r,j0,VB,e);L(r,j0,QB,e);L(r,j0,BC,uint);L(r,j0,CC,uint);L(r,j0,DC,uint);L(r,j0,NC,uint);L(r,j0,IC,e);V(j6,c);
#ifdef ENABLE_CLIPPING
V(j4,d);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
V(Y0,e);
#endif
V(a2,i);
#ifdef ENABLE_ADVANCED_BLEND
V(Q1,O);
#endif
c i0=G0(w1(AC),SC)+QB.xy;j6=TC*IC.zw+IC.xy;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){j4=r9(CC,j.B6);}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){
#ifndef RENDER_MODE_DEPTH_STENCIL
Y0=P8(w1(VB),QB.zw,i0 I4);
#else
vb(w1(VB),QB.zw,i0 I4);
#endif
}
#endif
e I=T3(i0);
#ifdef POST_INVERT_Y
I.y=-I.y;
#endif
#ifdef RENDER_MODE_DEPTH_STENCIL
I.z=v9(NC,0xffu);
#endif
a2=unpackUnorm4x8(BC);
#ifdef ENABLE_ADVANCED_BLEND
Q1=Z1(DC);
#endif
e0(j6);
#ifdef ENABLE_CLIPPING
e0(j4);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
e0(Y0);
#endif
e0(a2);
#ifdef ENABLE_ADVANCED_BLEND
e0(Q1);
#endif
C1(I);}
#endif
