#undef C5
#ifdef NEVER_GENERATE_PREMULTIPLIED_PAINT_COLORS
#define C5 false
#elif defined(ENABLE_ADVANCED_BLEND)
#define C5 !ENABLE_ADVANCED_BLEND
#else
#define C5 true
#endif
#undef z2
#ifdef ENABLE_FEATHER
#define z2 g
#else
#define z2 D
#endif
#ifdef VERTEX
A1(c0)
#if defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_BLIT)
r0(0,I3,JB);
#else
r0(0,g,TB);r0(1,g,UB);
#endif
B1
#endif
h2 J0 d0(0,g,k1);
#ifdef ATLAS_BLIT
J0 d0(1,c,C2);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
OPTIONALLY_FLAT d0(1,d,l1);
#else
J0 d0(2,z2,I);
#endif
OPTIONALLY_FLAT d0(3,d,z0);
#endif
#ifdef ENABLE_CLIPPING
#ifdef ATLAS_BLIT
OPTIONALLY_FLAT d0(4,d,F3);
#else
OPTIONALLY_FLAT d0(4,D,S1);
#endif
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_MSAA)
J0 d0(5,g,N0);
#endif
#ifdef ENABLE_ADVANCED_BLEND
OPTIONALLY_FLAT d0(6,d,Y1);
#endif
#ifdef RENDER_MODE_CLOCKWISE_ATOMIC
S4 d0(7,W0,a3);d0(8,c,i4);
#endif
Z1
#ifdef VERTEX
C1(XB,c0,G,v,S){
#if defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_BLIT)
v0(v,G,JB,V);
#else
v0(v,G,TB,g);v0(v,G,UB,g);
#endif
Y(k1,g);
#ifdef ATLAS_BLIT
Y(C2,c);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
Y(l1,d);
#else
Y(I,z2);
#endif
Y(z0,d);
#endif
#ifdef ENABLE_CLIPPING
#ifdef ATLAS_BLIT
Y(F3,d);
#else
Y(S1,D);
#endif
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_MSAA)
Y(N0,g);
#endif
#ifdef ENABLE_ADVANCED_BLEND
Y(Y1,d);
#endif
#ifdef RENDER_MODE_CLOCKWISE_ATOMIC
Y(a3,W0);Y(i4,c);
#endif
bool Pd=false;uint m0;c i0;
#ifdef RENDER_MODE_MSAA
X e9;
#endif
#ifdef ATLAS_BLIT
i0=nb(JB,m0,
#ifdef RENDER_MODE_MSAA
e9,
#endif
C2 p3);
#elif defined(DRAW_INTERIOR_TRIANGLES)
i0=ob(JB,m0
#ifdef RENDER_MODE_MSAA
,e9
#else
,l1
#endif
p3);
#else
g J;Pd=!p9(TB,UB,S,m0,i0
#ifndef RENDER_MODE_MSAA
,J
#else
,e9
#endif
p3);
#ifndef RENDER_MODE_MSAA
#ifdef ENABLE_FEATHER
I=J;
#else
I.xy=R7(J.xy);
#endif
#endif
#endif
W0 r1=J5(UC,m0);
#if!defined(ATLAS_BLIT)&&!defined(RENDER_MODE_MSAA)
z0=r8(m0,k.Y5);if((r1.x&C9)!=0u)z0=-z0;
#endif
uint g3=r1.x&0xfu;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint jh=(g3==Z7?r1.y:r1.x)>>16;d V0=r8(jh,k.Y5);if(g3==Z7)V0=-V0;
#ifdef ATLAS_BLIT
F3=V0;
#else
S1.x=V0;
#endif
}
#endif
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){Y1=float((r1.x>>4)&0xfu);}
#endif
c K0=i0;
#ifdef FRAMEBUFFER_BOTTOM_UP
K0.y=float(k.mg)-K0.y;
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){a0 k2=j2(P0(OB,m0*4u+2u));g D2=P0(OB,m0*4u+3u);
#ifndef RENDER_MODE_MSAA
N0=T7(k2,D2.xy,K0);
#else
ic(k2,D2.xy,K0 r5);
#endif
}
#endif
if(g3==vb){i j=unpackUnorm4x8(r1.y);if(C5)j.xyz*=j.w;k1=g(j);}
#if defined(ENABLE_CLIPPING)&&!defined(ATLAS_BLIT)
else if(ENABLE_CLIPPING&&g3==Z7){d D5=r8(r1.x>>16,k.Y5);S1.y=D5;}
#endif
else{a0 kh=j2(P0(OB,m0*4u));g f9=P0(OB,m0*4u+1u);c W4=Z0(kh,K0)+f9.xy;if(g3==E9||g3==rf){k1.w=-uintBitsToFloat(r1.y);float lh=f9.z;if(lh>.9){k1.z=2.;}else{k1.z=f9.w;}if(g3==E9){k1.y=.0;k1.x=W4.x;}else{k1.z=-k1.z;k1.xy=W4.xy;}}else{float y4=uintBitsToFloat(r1.y);float Wa=f9.z;k1=g(W4.x,W4.y,y4,-2.-Wa);}}g O;if(!Pd){O=H3(i0);
#ifdef POST_INVERT_Y
O.y=-O.y;
#endif
#ifdef RENDER_MODE_MSAA
O.z=ca(e9);
#elif defined(RENDER_MODE_CLOCKWISE_ATOMIC)
Q O4=P0(LB,m0*4u+3u);a3=O4.xy;i4=i0+uintBitsToFloat(O4.zw);
#endif
}else{O=g(k.O2,k.O2,k.O2,k.O2);}l0(k1);
#ifdef ATLAS_BLIT
l0(C2);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
l0(l1);
#else
l0(I);
#endif
l0(z0);
#endif
#ifdef ENABLE_CLIPPING
#ifdef ATLAS_BLIT
l0(F3);
#else
l0(S1);
#endif
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_MSAA)
l0(N0);
#endif
#ifdef ENABLE_ADVANCED_BLEND
l0(Y1);
#endif
#ifdef RENDER_MODE_CLOCKWISE_ATOMIC
l0(a3);l0(i4);
#endif
D1(O);}
#endif
#ifdef FRAGMENT
K3 L3 e i M7(g n3,float n G6){i j;if(n3.w>=.0){j=X4(n3);if(C5)j*=n;else j.w*=n;}else if(n3.w>-1.){float t=n3.z>.0?n3.x:length(n3.xy);t=clamp(t,.0,1.);float Qd=abs(n3.z);float x=Qd>1.?(1.-1./da)*t+(.5/da):(1./da)*t+Qd;float mh=-n3.w;j=m2(DD,wb,c(x,mh),.0);j.w*=n;if(C5)j.xyz*=j.w;}else{d Wa=-n3.w-2.;j=Q6(DC,R5,n3.xy,Wa);d y4=n3.z*n;if(C5)j*=y4;else j=B0(B6(j),j.w*y4);}return j;}
#if!defined(DRAW_INTERIOR_TRIANGLES)&&!defined(ATLAS_BLIT)
e d Rd(z2 J C3){
#ifdef ENABLE_FEATHER
if(ENABLE_FEATHER&&yb(J))return w4(J i1);else
#endif
return min(J.x,J.y);}e d Sd(z2 J C3){
#if defined(ENABLE_FEATHER)
if(ENABLE_FEATHER&&zb(J))return d8(J i1);else
#endif
return J.x;}e d Xa(z2 J C3){if(P5(J))return Rd(J i1);else return Sd(J i1);}e d nh(d P4,z2 J C3){if(P5(J)){d q0=Rd(J i1);return max(q0,P4);}else{d q0=Sd(J i1);return P4+q0;}}
#endif
#endif
