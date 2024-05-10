#ifdef VERTEX
U0(X1)q0(0,d,WB);V0 U0(l2)q0(1,d,XB);V0
#endif
A1 k0 I(0,d,B0);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT I(1,h,G0);
#endif
#ifdef ENABLE_CLIP_RECT
k0 I(2,g,i0);
#endif
B1
#ifdef VERTEX
O1 P1 o4(OB,X1,Y1,l2,m2,j){v0(j,Y1,WB,d);v0(j,m2,XB,d);Q(B0,d);
#ifdef ENABLE_CLIPPING
Q(G0,h);
#endif
#ifdef ENABLE_CLIP_RECT
Q(i0,g);
#endif
d R=h0(W0(J.m5),WB)+J.K0;B0=XB;
#ifdef ENABLE_CLIPPING
G0=F4(J.G1,v.v3);
#endif
#ifdef ENABLE_CLIP_RECT
#ifndef USING_DEPTH_STENCIL
i0=n4(W0(J.X0),J.i1,R);
#else
i7(W0(J.X0),J.i1,R);
#endif
#endif
g B=i2(R);
#ifdef USING_DEPTH_STENCIL
B.z=J5(J.Y3);
#endif
S(B0);
#ifdef ENABLE_CLIPPING
S(G0);
#endif
#ifdef ENABLE_CLIP_RECT
S(i0);
#endif
h1(B);}
#endif
#ifdef FRAGMENT
F2 x1(k3,MB);
#ifdef USING_DEPTH_STENCIL
#ifdef ENABLE_ADVANCED_BLEND
x1(z7,EC);
#endif
#endif
G2 l3(k3,n2)L3 O3
#ifndef USING_DEPTH_STENCIL
E1 C0(v5,c0);D0(w5,Q0);D0(x5,w0);C0(v7,w2);F1 U3(IB){N(B0,d);
#ifdef ENABLE_CLIPPING
N(G0,h);
#endif
#ifdef ENABLE_CLIP_RECT
N(i0,g);
#endif
i f=M2(MB,n2,B0);h C=1.;
#ifdef ENABLE_CLIP_RECT
h K2=B5(M0(i0));C=clamp(K2,E0(0),C);
#endif
n1;
#ifdef ENABLE_CLIPPING
if(G0!=.0){l0 Z0=unpackHalf2x16(L0(w0));h y3=Z0.y;h I2=y3==G0?Z0.x:E0(0);C=min(C,I2);}
#endif
f.w*=J.J2*C;i y1=N0(c0);
#ifdef ENABLE_ADVANCED_BLEND
if(J.a2!=0u){
#ifdef ENABLE_HSL_BLEND_MODES
f=E3(
#else
f=F3(
#endif
f,S3(y1),O0(J.a2));}else
#endif
{f.xyz*=f.w;f=f+y1*(1.-f.w);}F0(c0,f);y0(Q0);y0(w0);o1;c2;}
#else
r2(i,IB){N(B0,d);i f=M2(MB,n2,B0);f.w*=J.J2;
#ifdef ENABLE_ADVANCED_BLEND
i y1=I1(EC,m0(floor(n0.xy)));
#ifdef ENABLE_HSL_BLEND_MODES
f=E3(
#else
f=F3(
#endif
f,S3(y1),O0(J.a2));
#else
f=p2(f);
#endif
v2(f);}
#endif
#endif
