#ifdef VERTEX
L0(e0)
#ifdef DRAW_INTERIOR_TRIANGLES
h0(0,R3,IB);
#else
h0(0,e,LB);h0(1,e,MB);
#endif
M0
#endif
o1 o0 H(0,e,X0);
#ifdef ATLAS_COVERAGE
o0 H(1,c,N1);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
OPTIONALLY_FLAT H(1,h,a1);
#elif defined(ENABLE_FEATHER)
o0 H(2,e,q);
#else
o0 H(2,C,q);
#endif
OPTIONALLY_FLAT H(3,h,i0);
#endif
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT H(4,h,h1);
#endif
#ifdef ENABLE_CLIP_RECT
o0 H(5,e,F0);
#endif
#ifdef ENABLE_ADVANCED_BLEND
OPTIONALLY_FLAT H(6,h,E3);
#endif
p1
#ifdef VERTEX
Z0(PB,e0,o,l,I){
#ifdef DRAW_INTERIOR_TRIANGLES
k0(l,o,IB,a0);
#else
k0(l,o,LB,e);k0(l,o,MB,e);
#endif
P(X0,e);
#ifdef ATLAS_COVERAGE
P(N1,c);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
P(a1,h);
#elif defined(ENABLE_FEATHER)
P(q,e);
#else
P(q,C);
#endif
P(i0,h);
#endif
#ifdef ENABLE_CLIPPING
P(h1,h);
#endif
#ifdef ENABLE_CLIP_RECT
P(F0,e);
#endif
#ifdef ENABLE_ADVANCED_BLEND
P(E3,h);
#endif
bool fa=false;uint d0;c K;
#ifdef RENDER_MODE_MSAA
O z6;
#endif
#ifdef ATLAS_COVERAGE
K=Y8(IB,d0,
#ifdef RENDER_MODE_MSAA
z6,
#endif
N1 g2);
#elif defined(DRAW_INTERIOR_TRIANGLES)
K=g7(IB,d0
#ifdef RENDER_MODE_MSAA
,z6
#else
,a1
#endif
g2);
#else
e M;fa=!S5(LB,MB,I,d0,K
#ifndef RENDER_MODE_MSAA
,M
#else
,z6
#endif
g2);
#ifndef RENDER_MODE_MSAA
#ifdef ENABLE_FEATHER
q=M;
#else
q.xy=T5(M.xy);
#endif
#endif
#endif
E0 z0=a4(CC,d0);
#if!defined(ATLAS_COVERAGE)&&!defined(RENDER_MODE_MSAA)
i0=o6(d0,m.F4);if((z0.x&v7)!=0u)i0=-i0;
#endif
uint y1=z0.x&0xfu;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint zc=(y1==c6?z0.y:z0.x)>>16;h1=o6(zc,m.F4);if(y1==c6)h1=-h1;}
#endif
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){E3=float((z0.x>>4)&0xfu);}
#endif
c r5=K;
#ifdef FRAMEBUFFER_BOTTOM_UP
r5.y=float(m.Tb)-r5.y;
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){Y F1=r1(r0(JB,d0*4u+2u));e O1=r0(JB,d0*4u+3u);
#ifndef RENDER_MODE_MSAA
F0=W5(F1,O1.xy,r5);
#else
G9(F1,O1.xy,r5);
#endif
}
#endif
if(y1==w7){i j=unpackUnorm4x8(z0.y);X0=e(j);}
#ifdef ENABLE_CLIPPING
else if(ENABLE_CLIPPING&&y1==c6){h A6=o6(z0.x>>16,m.F4);X0=e(A6,0,0,0);}
#endif
else{Y Ac=r1(r0(JB,d0*4u));e B6=r0(JB,d0*4u+1u);c w2=q0(Ac,r5)+B6.xy;if(y1==d6||y1==cc){X0.w=-uintBitsToFloat(z0.y);float Bc=B6.z;if(Bc>.9){X0.z=2.;}else{X0.z=B6.w;}if(y1==d6){X0.y=.0;X0.x=w2.x;}else{X0.z=-X0.z;X0.xy=w2.xy;}}else{float G2=uintBitsToFloat(z0.y);float q5=B6.z;X0=e(w2.x,w2.y,G2,-2.-q5);}}e Q;if(!fa){Q=v2(K);
#ifdef POST_INVERT_Y
Q.y=-Q.y;
#endif
#ifdef RENDER_MODE_MSAA
Q.z=N7(z6);
#endif
}else{Q=e(m.q1,m.q1,m.q1,m.q1);}X(X0);
#ifdef ATLAS_COVERAGE
X(N1);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
X(a1);
#elif defined(ENABLE_FEATHER)
X(q);
#else
X(q);
#endif
X(i0);
#endif
#ifdef ENABLE_CLIPPING
X(h1);
#endif
#ifdef ENABLE_CLIP_RECT
X(F0);
#endif
#ifdef ENABLE_ADVANCED_BLEND
X(E3);
#endif
U0(Q);}
#endif
#ifdef FRAGMENT
j3 k3 d i ga(e x2 Z4){if(x2.w>=.0){return g5(x2);}else if(x2.w>-1.){float t=x2.z>.0?x2.x:length(x2.xy);t=clamp(t,.0,1.);float ha=abs(x2.z);float x=ha>1.?(1.-1./O7)*t+(.5/O7):(1./O7)*t+ha;float Cc=-x2.w;return S1(KC,x7,c(x,Cc),.0);}else{h q5=-x2.w-2.;i j=S1(UB,q3,x2.xy,q5);h G2=x2.z;j.w*=G2;return j;}}
#ifndef RENDER_MODE_MSAA
k2 N0(l7,B0);P0(Y4,c1);N0(X9,D3);P0(n7,i4);l2 n2(NB){Z(X0,e);
#ifdef ATLAS_COVERAGE
Z(N1,c);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
Z(a1,h);
#elif defined(ENABLE_FEATHER)
Z(q,e);
#else
Z(q,C);
#endif
Z(i0,h);
#endif
#ifdef ENABLE_CLIPPING
Z(h1,h);
#endif
#ifdef ENABLE_CLIP_RECT
Z(F0,e);
#endif
#ifdef ENABLE_ADVANCED_BLEND
Z(E3,h);
#endif
#if!defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_COVERAGE)
W1;
#endif
h J;
#ifdef ATLAS_COVERAGE
J=C7(N1,m.f5 z1);
#else
C A3=unpackHalf2x16(V0(i4));h ia=A3.y;h R1=ia==i0?A3.x:d1(.0);
#ifdef DRAW_INTERIOR_TRIANGLES
R1+=a1;X1(i4);
#else
if(j6(q)){h m0;
#ifdef ENABLE_FEATHER
if(ENABLE_FEATHER&&z7(q)){m0=g6(q z1);}else
#endif
{m0=min(q.x,q.y);}R1=max(m0,R1);}else{h m0;
#if defined(ENABLE_FEATHER)
if(ENABLE_FEATHER&&h6(q)){m0=A4(q z1);}else
#endif
{m0=q.x;}R1+=m0;}W0(i4,packHalf2x16(Q3(R1,i0)));
#endif
#ifdef CLOCKWISE_FILL
if(CLOCKWISE_FILL){J=clamp(R1,d1(.0),d1(1.));}else
#endif
{J=abs(R1);
#ifdef ENABLE_EVEN_ODD
if(ENABLE_EVEN_ODD&&i0<.0){J=1.-d1(abs(fract(J*.5)*2.+-1.));}
#endif
J=min(J,d1(1.));}
#endif
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&h1<.0){h m2=-h1;
#ifdef ENABLE_NESTED_CLIPPING
if(ENABLE_NESTED_CLIPPING){h A6=X0.x;if(A6!=.0){C x1=unpackHalf2x16(V0(c1));h M4=x1.y;h C6;if(M4!=m2){C6=M4==A6?x1.x:.0;
#ifndef DRAW_INTERIOR_TRIANGLES
H0(D3,f2(C6,.0,.0,.0));
#endif
}else{C6=C0(D3).x;
#ifndef DRAW_INTERIOR_TRIANGLES
p2(D3);
#endif
}J=min(J,C6);}}
#endif
W0(c1,packHalf2x16(Q3(J,m2)));p2(B0);}else
#endif
{
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){if(h1!=.0){C x1=unpackHalf2x16(V0(c1));h M4=x1.y;J=(M4==h1)?min(x1.x,J):d1(.0);}}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){h c4=E7(g5(F0));J=clamp(c4,d1(.0),J);}
#endif
i j=ga(X0 o3);j.w*=J;i e1;
#ifdef ATLAS_COVERAGE
e1=C0(B0);
#else
if(ia!=i0){e1=C0(B0);
#ifndef DRAW_INTERIOR_TRIANGLES
H0(D3,e1);
#endif
}else{e1=C0(D3);
#ifndef DRAW_INTERIOR_TRIANGLES
p2(D3);
#endif
}
#endif
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND&&E3!=n6(F7)){j=R5(j,w4(e1),K7(E3));}else
#endif
{j.xyz*=j.w;j=j+e1*(1.-j.w);}H0(B0,j);X1(c1);}
#if!defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_COVERAGE)
Y1;
#endif
E2;}
#else
T1(i,NB){Z(X0,e);
#ifdef ATLAS_COVERAGE
Z(N1,c);
#endif
#ifdef ENABLE_ADVANCED_BLEND
Z(E3,h);
#endif
i j=ga(X0);
#ifdef ATLAS_COVERAGE
j.w*=C7(N1,m.f5 z1);
#endif
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){i e1=f1(MC,f0(floor(v0.xy)));j=R5(j,w4(e1),K7(E3));}else
#endif
{j=y9(j);}U1(j);}
#endif
#endif
