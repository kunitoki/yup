#ifdef VERTEX
U0(a2)i0(0,c,SB);V0 U0(v2)i0(1,c,TB);V0
#endif
o1 n0 H(0,c,q0);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT H(1,g,j5);
#endif
#ifdef ENABLE_CLIP_RECT
n0 H(2,f,R0);
#endif
p1
#ifdef VERTEX
P2 Q2 N4(PB,a2,c2,v2,w2,n){l0(n,c2,SB,c);l0(n,w2,TB,c);L(q0,c);
#ifdef ENABLE_CLIPPING
L(j5,g);
#endif
#ifdef ENABLE_CLIP_RECT
L(R0,f);
#endif
c J=C0(D1(m0.H6),SB)+m0.S0;q0=TB;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){j5=g7(m0.Z0,q.d5);}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){
#ifndef RENDER_MODE_MSAA
R0=I6(D1(m0.R1),m0.Z1,J);
#else
Ka(D1(m0.R1),m0.Z1,J);
#endif
}
#endif
f Q=F2(J);
#ifdef RENDER_MODE_MSAA
Q.z=S8(m0.X5);
#endif
P(q0);
#ifdef ENABLE_CLIPPING
P(j5);
#endif
#ifdef ENABLE_CLIP_RECT
P(R0);
#endif
h1(Q);}
#endif
#ifdef FRAGMENT
R2 C2(Y5,W8,UB);
#ifdef RENDER_MODE_MSAA
#ifdef ENABLE_ADVANCED_BLEND
C2(U2,Za,PC);
#endif
#endif
S2 p4 G3(X8,B3)q4 w3 x3
#ifndef RENDER_MODE_MSAA
x2 M0(i8,H0);Y0(B5,r1);M0(bb,N3);Y0(k8,x4);y2 R4(NB){N(q0,c);
#ifdef ENABLE_CLIPPING
N(j5,g);
#endif
#ifdef ENABLE_CLIP_RECT
N(R0,f);
#endif
i j=o4(UB,B3,q0);g E=1.;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){g l4=A8(I5(R0));E=clamp(l4,v1(.0),E);}
#endif
h2;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&j5!=.0){G I1=unpackHalf2x16(j1(r1));g k5=I1.y;g Sd=k5==j5?I1.x:v1(.0);E=min(E,Sd);}
#endif
i w1=I0(H0);
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND&&m0.z3!=B8){j.xyz=M4(Y3(j),w1,Q1(m0.z3))*j.w;}
#endif
j*=m0.H2*E;j+=w1*(1.-j.w);T0(H0,j);i2(r1);i2(x4);j2;M2;}
#else
e2(i,NB){N(q0,c);i j=o4(UB,B3,q0)*m0.H2;
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){i w1=d1(PC,c0(floor(y0.xy)));j.xyz=M4(Y3(j),w1,m0.z3);j.xyz*=j.w;}
#endif
f2(j);}
#endif
#endif
