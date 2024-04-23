#ifdef VERTEX
T0(W1)q0(0,d,XB);U0 T0(k2)q0(1,d,YB);U0
#endif
z1 k0 I(0,d,B0);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT I(1,h,G0);
#endif
#ifdef ENABLE_CLIP_RECT
k0 I(2,g,i0);
#endif
A1
#ifdef VERTEX
N1 O1 n4(PB,W1,X1,k2,l2,j){w0(j,X1,XB,d);w0(j,l2,YB,d);Q(B0,d);
#ifdef ENABLE_CLIPPING
Q(G0,h);
#endif
#ifdef ENABLE_CLIP_RECT
Q(i0,g);
#endif
d R=h0(V0(J.l5),XB)+J.K0;B0=YB;
#ifdef ENABLE_CLIPPING
G0=E4(J.F1,v.r3);
#endif
#ifdef ENABLE_CLIP_RECT
#ifndef USING_DEPTH_STENCIL
i0=m4(V0(J.W0),J.g1,R);
#else
g7(V0(J.W0),J.g1,R);
#endif
#endif
g B=h2(R);
#ifdef USING_DEPTH_STENCIL
B.z=I5(J.X3);
#endif
S(B0);
#ifdef ENABLE_CLIPPING
S(G0);
#endif
#ifdef ENABLE_CLIP_RECT
S(i0);
#endif
f1(B);}
#endif
#ifdef FRAGMENT
E2 w1(j3,NB);
#ifdef USING_DEPTH_STENCIL
#ifdef ENABLE_ADVANCED_BLEND
w1(x7,FC);
#endif
#endif
F2 k3(j3,m2)K3 N3
#ifndef USING_DEPTH_STENCIL
D1 C0(r5,X);D0(v5,x0);D0(w5,r0);C0(q7,v2);E1 T3(JB){N(B0,d);
#ifdef ENABLE_CLIPPING
N(G0,h);
#endif
#ifdef ENABLE_CLIP_RECT
N(i0,g);
#endif
i f=L2(NB,m2,B0);h C=1.;
#ifdef ENABLE_CLIP_RECT
h J2=A5(M0(i0));C=clamp(J2,E0(0),C);
#endif
m1;
#ifdef ENABLE_CLIPPING
if(G0!=.0){l0 Y0=unpackHalf2x16(L0(r0));h x3=Y0.y;h H2=x3==G0?Y0.x:E0(0);C=min(C,H2);}
#endif
f.w*=J.I2*C;i x1=N0(X);
#ifdef ENABLE_ADVANCED_BLEND
if(J.Z1!=0u){
#ifdef ENABLE_HSL_BLEND_MODES
f=D3(
#else
f=E3(
#endif
f,R3(x1),O0(J.Z1));}else
#endif
{f.xyz*=f.w;f=f+x1*(1.-f.w);}F0(X,f);d0(x0);d0(r0);n1;a2;}
#else
q2(i,JB){N(B0,d);i f=L2(NB,m2,B0);f.w*=J.I2;
#ifdef ENABLE_ADVANCED_BLEND
i x1=H1(FC,m0(floor(n0.xy)));
#ifdef ENABLE_HSL_BLEND_MODES
f=D3(
#else
f=E3(
#endif
f,R3(x1),O0(J.Z1));
#else
f=o2(f);
#endif
r2(f);}
#endif
#endif
