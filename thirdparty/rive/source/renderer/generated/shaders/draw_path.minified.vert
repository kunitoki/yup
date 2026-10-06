#undef G2
#ifdef ENABLE_FEATHER
#define G2 e
#else
#define G2 C
#endif
#ifdef VERTEX
c1(d0)
#if defined(DRAW_INTERIOR_TRIANGLES)||defined(FEATHER_ATLAS_BLIT)
K(0,c4,MB);
#else
K(0,e,WB);K(1,e,XB);
#endif
d1
#endif
l2 E0 V(0,e,a1);
#ifdef FEATHER_ATLAS_BLIT
E0 V(1,c,J2);
#elif!defined(RENDER_MODE_DEPTH_STENCIL)
#ifdef DRAW_INTERIOR_TRIANGLES
OPTIONALLY_FLAT V(1,d,m1);
#else
E0 V(2,G2,S);
#endif
OPTIONALLY_FLAT V(3,d,F0);
#endif
#ifdef ENABLE_CLIPPING
#ifdef FEATHER_ATLAS_BLIT
OPTIONALLY_FLAT V(4,d,Y3);
#else
OPTIONALLY_FLAT V(4,C,l1);
#endif
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
E0 V(5,e,R0);
#endif
#ifdef ENABLE_ADVANCED_BLEND
OPTIONALLY_FLAT V(6,d,Q0);
#endif
#ifdef RENDER_MODE_CLOCKWISE_ATOMIC
Z2 V(7,O0,q3);V(8,c,F4);
#endif
#ifdef ENABLE_MODULATED_IMAGE
E0 V(9,O,r1);
#endif
e2
#ifdef VERTEX
v1(RB,d0,D,G,r){
#if defined(DRAW_INTERIOR_TRIANGLES)||defined(FEATHER_ATLAS_BLIT)
L(G,D,MB,O);
#else
L(G,D,WB,e);L(G,D,XB,e);
#endif
T(a1,e);
#if defined(ENABLE_MODULATED_IMAGE)
T(r1,O);
#endif
#ifdef FEATHER_ATLAS_BLIT
T(J2,c);
#elif!defined(RENDER_MODE_DEPTH_STENCIL)
#ifdef DRAW_INTERIOR_TRIANGLES
T(m1,d);
#else
T(S,G2);
#endif
T(F0,d);
#endif
#ifdef ENABLE_CLIPPING
#ifdef FEATHER_ATLAS_BLIT
T(Y3,d);
#else
T(l1,C);
#endif
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
T(R0,e);
#endif
#ifdef ENABLE_ADVANCED_BLEND
T(Q0,d);
#endif
#ifdef RENDER_MODE_CLOCKWISE_ATOMIC
T(q3,O0);T(F4,c);
#endif
bool Oe=false;uint a0;c k0;
#ifdef RENDER_MODE_DEPTH_STENCIL
Q B9;
#endif
#ifdef FEATHER_ATLAS_BLIT
k0=kc(MB,a0,
#ifdef RENDER_MODE_DEPTH_STENCIL
B9,
#endif
J2 G3);
#elif defined(DRAW_INTERIOR_TRIANGLES)
k0=lc(MB,a0
#ifdef RENDER_MODE_DEPTH_STENCIL
,B9
#else
,m1
#endif
G3);
#else
e U;Oe=!K9(WB,XB,r,a0,k0
#ifndef RENDER_MODE_DEPTH_STENCIL
,U
#else
,B9
#endif
G3);
#ifndef RENDER_MODE_DEPTH_STENCIL
#ifdef ENABLE_FEATHER
S=U;
#else
S.xy=g8(U.xy);
#endif
#endif
#endif
O0 G0=l5(WC,a0);
#if!defined(FEATHER_ATLAS_BLIT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
F0=l6(a0,j.T4);if((G0.x&ea)!=0u) F0=-F0;
#endif
uint n2=G0.x&0xfu;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint Qb=(n2==o5?G0.y:G0.x)>>16;d X0=l6(Qb,j.T4);if(n2==o5) X0=-X0;
#ifdef FEATHER_ATLAS_BLIT
Y3=X0;
#else
l1.x=X0;
#endif
}
#endif
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){Q0=float((G0.x>>4)&0xfu);}
#endif
c l0=k0;
#ifdef ENABLE_RENDER_TARGET_BOTTOM_UP
if(j.W9!=0u){l0.y=float(j.X9)-l0.y;}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){Y B3=n1(p0(JB,a0*g2+2u));e P3=p0(JB,a0*g2+3u);
#ifndef RENDER_MODE_DEPTH_STENCIL
R0=i8(B3,P3.xy,l0);
#else
Ga(B3,P3.xy,l0 Y4);
#endif
}
#endif
if(n2==fa){a1=e(unpackUnorm4x8(G0.y));}
#if defined(ENABLE_CLIPPING)&&!defined(FEATHER_ATLAS_BLIT)
else if(ENABLE_CLIPPING&&n2==o5){d E4=l6(G0.x>>16,j.T4);l1.y=E4;}
#endif
else{Y Rb=n1(p0(JB,a0*g2));e W7=p0(JB,a0*g2+1u);a1=Y9(l0,Rb,W7.xy,float(n2),W7.zw,uintBitsToFloat(G0.y));a1.w=-a1.w;}
#if defined(ENABLE_MODULATED_IMAGE)
if(ENABLE_MODULATED_IMAGE&&(G0.x&vd)!=0u){Y Sb=n1(p0(JB,a0*g2+4u));e X7=p0(JB,a0*g2+5u);c o3=M0(Sb,l0)+X7.xy;float Pe=1.+X7.z;if((G0.x&Ig)!=0u){uint a4=(G0.x&Kg)>>Jg;Pe=-(1.+float(a4));}r1=O(o3.x,o3.y,Pe);}else{r1=O(0.0,0.0,0.0);}
#endif
e I;if(!Oe){I=H3(k0);
#ifdef POST_INVERT_Y
I.y=-I.y;
#endif
#ifdef RENDER_MODE_DEPTH_STENCIL
I.z=I8(B9,0xffu);
#elif defined(RENDER_MODE_CLOCKWISE_ATOMIC)
M d5=p0(LB,a0*4u+3u);q3=d5.xy;F4=k0+uintBitsToFloat(d5.zw);
#endif
}else{I=e(j.a3,j.a3,j.a3,j.a3);}Z(a1);
#if defined(ENABLE_MODULATED_IMAGE)
Z(r1);
#endif
#ifdef FEATHER_ATLAS_BLIT
Z(J2);
#elif!defined(RENDER_MODE_DEPTH_STENCIL)
#ifdef DRAW_INTERIOR_TRIANGLES
Z(m1);
#else
Z(S);
#endif
Z(F0);
#endif
#ifdef ENABLE_CLIPPING
#ifdef FEATHER_ATLAS_BLIT
Z(Y3);
#else
Z(l1);
#endif
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
Z(R0);
#endif
#ifdef ENABLE_ADVANCED_BLEND
Z(Q0);
#endif
#ifdef RENDER_MODE_CLOCKWISE_ATOMIC
Z(q3);Z(F4);
#endif
w1(I);}
#endif
#ifdef FRAGMENT
f4 g4 f d Vi(i Tb,uint a4){d Qe=dot(Tb.xyz,W0(.30,.59,.11));if(a4==Lg) return Tb.w;if(a4==Mg) return 1.-Tb.w;if(a4==Ng) return Qe;return 1.-Qe;}f i Y7(
#ifdef ENABLE_MODULATED_IMAGE
O Z7,
#endif
#ifdef ENABLE_ADVANCED_BLEND
Q y3,
#endif
e e5 W6){
#ifdef ENABLE_ADVANCED_BLEND
bool n5=ENABLE_ADVANCED_BLEND&&y3!=L4;
#else
const bool n5=false;
#endif
i p;if(e5.w>=.0){p=v5(e5);}else{e5.w=-e5.w;d ia=d4(fract(e5.w)*(256./255.));e5.w=floor(e5.w)*j.wc+j.xc;c oa=Cc(e5);p=o2(ED,ha,oa,.0);if(!n5){p.xyz*=p.w;p.w*=ia;}}
#if defined(ENABLE_MODULATED_IMAGE)
if(ENABLE_MODULATED_IMAGE&&Z7.z<0.0){return i6(CC,r5,Z7.xy,H0(.0));}if(ENABLE_MODULATED_IMAGE&&Z7.z>0.0){d Wi=Z7.z-1.;i N2=i6(CC,r5,Z7.xy,Wi);if(n5) N2=I0(Q6(N2),N2.w);p*=N2;}
#endif
return p;}
#if!defined(DRAW_INTERIOR_TRIANGLES)&&!defined(FEATHER_ATLAS_BLIT)
f d Re(G2 U R3){
#ifdef ENABLE_FEATHER
if(ENABLE_FEATHER&&yc(U)) return M4(U k1);else
#endif
return min(U.x,U.y);}f d Se(G2 U R3){
#if defined(ENABLE_FEATHER)
if(ENABLE_FEATHER&&zc(U)) return q8(U k1);else
#endif
return U.x;}f d Ub(G2 U R3){if(f6(U)) return Re(U k1);else return Se(U k1);}f d Xi(d f5,G2 U R3){if(f6(U)){d A0=Re(U k1);return max(A0,f5);}else{d A0=Se(U k1);return f5+A0;}}
#endif
#endif
