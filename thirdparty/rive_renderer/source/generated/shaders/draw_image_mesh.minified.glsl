#ifdef VERTEX
L0(P1)h0(0,c,SB);M0 L0(h2)h0(1,c,TB);M0
#endif
o1 o0 H(0,c,A0);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT H(1,h,h1);
#endif
#ifdef ENABLE_CLIP_RECT
o0 H(2,e,F0);
#endif
p1
#ifdef VERTEX
H2 I2 v4(PB,P1,Q1,h2,i2,l){k0(l,Q1,SB,c);k0(l,i2,TB,c);P(A0,c);
#ifdef ENABLE_CLIPPING
P(h1,h);
#endif
#ifdef ENABLE_CLIP_RECT
P(F0,e);
#endif
c K=q0(r1(l0.V5),SB)+l0.G0;A0=TB;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){h1=o6(l0.m2,m.F4);}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){
#ifndef RENDER_MODE_MSAA
F0=W5(r1(l0.F1),l0.O1,K);
#else
G9(r1(l0.F1),l0.O1,K);
#endif
}
#endif
e Q=v2(K);
#ifdef RENDER_MODE_MSAA
Q.z=N7(l0.m5);
#endif
X(A0);
#ifdef ENABLE_CLIPPING
X(h1);
#endif
#ifdef ENABLE_CLIP_RECT
X(F0);
#endif
U0(Q);}
#endif
#ifdef FRAGMENT
J2 o2(w6,I4,UB);
#ifdef RENDER_MODE_MSAA
#ifdef ENABLE_ADVANCED_BLEND
o2(K2,V9,MC);
#endif
#endif
L2 h4(I4,q3)j3 k3
#ifndef RENDER_MODE_MSAA
k2 N0(l7,B0);P0(Y4,c1);N0(X9,D3);P0(n7,i4);l2 z4(NB){Z(A0,c);
#ifdef ENABLE_CLIPPING
Z(h1,h);
#endif
#ifdef ENABLE_CLIP_RECT
Z(F0,e);
#endif
i j=p3(UB,q3,A0);h J=1.;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){h c4=E7(g5(F0));J=clamp(c4,d1(.0),J);}
#endif
W1;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&h1!=.0){C x1=unpackHalf2x16(V0(c1));h M4=x1.y;h yc=M4==h1?x1.x:d1(.0);J=min(J,yc);}
#endif
j.w*=l0.G2*J;i e1=C0(B0);
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND&&l0.m3!=F7){j=R5(j,w4(e1),M1(l0.m3));}else
#endif
{j.xyz*=j.w;j=j+e1*(1.-j.w);}H0(B0,j);X1(c1);X1(i4);Y1;E2;}
#else
T1(i,NB){Z(A0,c);i j=p3(UB,q3,A0);j.w*=l0.G2;
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){i e1=f1(MC,f0(floor(v0.xy)));j=R5(j,w4(e1),l0.m3);}else
#endif
{j=y9(j);}U1(j);}
#endif
#endif
