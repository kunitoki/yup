#ifdef VERTEX
Y0(V)
#ifdef DRAW_INTERIOR_TRIANGLES
a0(0,I3,KB);
#else
a0(0,g,VB);a0(1,g,WB);
#endif
Z0
#endif
H1 w0 W(0,g,M0);
#ifndef USING_DEPTH_STENCIL
#ifdef DRAW_INTERIOR_TRIANGLES
OPTIONALLY_FLAT W(1,h,Q1);
#else
w0 W(2,r,V0);
#endif
OPTIONALLY_FLAT W(3,h,j0);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT W(4,h,P0);
#endif
#ifdef ENABLE_CLIP_RECT
w0 W(5,g,x0);
#endif
#endif
#ifdef ENABLE_ADVANCED_BLEND
OPTIONALLY_FLAT W(6,h,Q2);
#endif
I1
#ifdef VERTEX
h1(PB,V,q,n,H){
#ifdef DRAW_INTERIOR_TRIANGLES
f0(n,q,KB,Q);
#else
f0(n,q,VB,g);f0(n,q,WB,g);
#endif
c0(M0,g);
#ifndef Cb
#ifdef DRAW_INTERIOR_TRIANGLES
c0(Q1,h);
#else
c0(V0,r);
#endif
c0(j0,h);
#ifdef ENABLE_CLIPPING
c0(P0,h);
#endif
#ifdef ENABLE_CLIP_RECT
c0(x0,g);
#endif
#endif
#ifdef ENABLE_ADVANCED_BLEND
c0(Q2,h);
#endif
bool i8=false;M l1;f g0;
#ifdef USING_DEPTH_STENCIL
M j8;
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
g0=l7(KB,l1,Q1 Y2);
#else
i8=!k7(VB,WB,H,l1,g0
#ifndef USING_DEPTH_STENCIL
,V0
#else
,j8
#endif
Y2);
#endif
n1 J0=Q3(GC,l1);
#ifndef USING_DEPTH_STENCIL
j0=f5(l1,P.X3);if((J0.x&w7)!=0u)j0=-j0;
#endif
uint V1=J0.x&0xfu;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint la=(V1==a5?J0.y:J0.x)>>16;P0=f5(la,P.X3);if(V1==a5)P0=-P0;}
#endif
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){Q2=float((J0.x>>4)&0xfu);}
#endif
f D4=g0;
#ifdef ma
D4.y=float(P.V9)-D4.y;
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){I k1=j1(E0(HB,l1*4u+2u));g q1=E0(HB,l1*4u+3u);
#ifndef USING_DEPTH_STENCIL
x0=U4(k1,q1.xy,D4);
#else
R7(k1,q1.xy,D4);
#endif
}
#endif
if(V1==y7){i j=unpackUnorm4x8(J0.y);M0=g(j);}
#ifdef ENABLE_CLIPPING
else if(ENABLE_CLIPPING&&V1==a5){h j5=f5(J0.x>>16,P.X3);M0=g(j5,0,0,0);}
#endif
else{I na=j1(E0(HB,l1*4u));g k5=E0(HB,l1*4u+1u);f g3=m0(na,D4)+k5.xy;if(V1==a6||V1==ca){M0.w=-uintBitsToFloat(J0.y);if(k5.z>.9){M0.z=2.;}else{M0.z=k5.w;}if(V1==a6){M0.y=.0;M0.x=g3.x;}else{M0.z=-M0.z;M0.xy=g3.xy;}}else{float m3=uintBitsToFloat(J0.y);float r6=k5.z;M0=g(g3.x,g3.y,m3,-2.-r6);}}g T;if(!i8){T=E2(g0);
#ifdef USING_DEPTH_STENCIL
T.z=o6(j8);
#endif
}else{T=g(P.F2,P.F2,P.F2,P.F2);}d0(M0);
#ifndef USING_DEPTH_STENCIL
#ifdef DRAW_INTERIOR_TRIANGLES
d0(Q1);
#else
d0(V0);
#endif
d0(j0);
#ifdef ENABLE_CLIPPING
d0(P0);
#endif
#ifdef ENABLE_CLIP_RECT
d0(x0);
#endif
#endif
#ifdef ENABLE_ADVANCED_BLEND
d0(Q2);
#endif
i1(T);}
#endif
#ifdef FRAGMENT
H2 v1(p2,W4,FC);v1(S5,J3,QB);
#ifdef USING_DEPTH_STENCIL
#ifdef ENABLE_ADVANCED_BLEND
v1(p2,f8,HC);
#endif
#endif
I2 K3(W4,T5)c3(J3,L3)M3 P3 d i k8(g Y1 m4){if(Y1.w>=.0){return r4(Y1);}else if(Y1.w>-1.){float t=Y1.z>.0?Y1.x:length(Y1.xy);t=clamp(t,.0,1.);float l8=abs(Y1.z);float x=l8>1.?(1.-1./p6)*t+(.5/p6):(1./p6)*t+l8;float oa=-Y1.w;return h3(FC,T5,f(x,oa),.0);}else{h r6=-Y1.w-2.;i j=h3(QB,L3,Y1.xy,r6);h m3=Y1.z;j.w*=m3;return j;}}
#ifndef USING_DEPTH_STENCIL
J1 A0(U5,q0);
#if defined(ENABLE_CLIPPING)||defined(PLS_IMPL_ANGLE)
C0(k4,O0);
#endif
A0(h8,P2);C0(W5,C4);K1 M1(IB){Y(M0,g);
#ifdef DRAW_INTERIOR_TRIANGLES
Y(Q1,h);
#else
Y(V0,r);
#endif
Y(j0,h);
#ifdef ENABLE_CLIPPING
Y(P0,h);
#endif
#ifdef ENABLE_CLIP_RECT
Y(x0,g);
#endif
#ifdef ENABLE_ADVANCED_BLEND
Y(Q2,h);
#endif
#ifndef DRAW_INTERIOR_TRIANGLES
w1;
#endif
r m8=unpackHalf2x16(I0(C4));h n8=m8.y;h m1=n8==j0?m8.x:d1(.0);
#ifdef DRAW_INTERIOR_TRIANGLES
m1+=Q1;
#else
if(V0.y>=.0)m1=max(min(V0.x,V0.y),m1);else m1+=V0.x;K0(C4,packHalf2x16(D2(m1,j0)));
#endif
#ifdef CLOCKWISE_FILL
h N=clamp(m1,d1(.0),d1(1.));
#else
h N=abs(m1);
#ifdef ENABLE_EVEN_ODD
if(ENABLE_EVEN_ODD&&j0<.0){N=1.-d1(abs(fract(N*.5)*2.+-1.));}
#endif
N=min(N,d1(1.));
#endif
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&P0<.0){h L1=-P0;
#ifdef ENABLE_NESTED_CLIPPING
if(ENABLE_NESTED_CLIPPING){h j5=M0.x;if(j5!=.0){r c1=unpackHalf2x16(I0(O0));h c4=c1.y;h l5;if(c4!=L1){l5=c4==j5?c1.x:.0;
#ifndef DRAW_INTERIOR_TRIANGLES
y0(P2,C2(l5,.0,.0,.0));
#endif
}else{l5=r0(P2).x;
#ifndef DRAW_INTERIOR_TRIANGLES
O1(P2);
#endif
}N=min(N,l5);}}
#endif
K0(O0,packHalf2x16(D2(N,L1)));O1(q0);}else
#endif
{
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){if(P0!=.0){r c1=unpackHalf2x16(I0(O0));h c4=c1.y;N=(c4==P0)?min(c1.x,N):d1(.0);}X1(O0);}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){h f3=f6(r4(x0));N=clamp(f3,d1(.0),N);}
#endif
i j=k8(M0 O2);j.w*=N;i W0;if(n8!=j0){W0=r0(q0);
#ifndef DRAW_INTERIOR_TRIANGLES
y0(P2,W0);
#endif
}else{W0=r0(P2);
#ifndef DRAW_INTERIOR_TRIANGLES
O1(P2);
#endif
}
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND&&Q2!=G7(g6)){j=S4(j,R3(W0),k6(Q2));}else
#endif
{j.xyz*=j.w;j=j+W0*(1.-j.w);}y0(q0,j);}
#ifndef DRAW_INTERIOR_TRIANGLES
x1;
#endif
k2;}
#else
q2(i,IB){Y(M0,g);
#ifdef ENABLE_ADVANCED_BLEND
Y(Q2,h);
#endif
i j=k8(M0);
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){i W0=N1(HC,e0(floor(h0.xy)));j=S4(j,R3(W0),k6(Q2));}else
#endif
{j=M7(j);}r2(j);}
#endif
#endif
