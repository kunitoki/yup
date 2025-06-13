#ifdef DRAW_PATH
#ifdef VERTEX
U0(f0)i0(0,f,LB);i0(1,f,MB);V0
#endif
o1
#ifdef ENABLE_FEATHER
n0 H(0,f,D);
#else
n0 H(0,G,D);
#endif
L2 H(1,a0,j0);p1
#ifdef VERTEX
q1(PB,f0,B,n,K){l0(n,B,LB,f);l0(n,B,MB,f);
#ifdef ENABLE_FEATHER
L(D,f);
#else
L(D,G);
#endif
L(j0,a0);f Q;uint R;c J;f O;if(E6(LB,MB,K,R,J,O Y1)){
#ifdef ENABLE_FEATHER
D=O;
#else
D.xy=F6(O.xy);
#endif
j0=Q1(R);Q=F2(J);}else{Q=f(q.C1,q.C1,q.C1,q.C1);}P(D);P(j0);h1(Q);}
#endif
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
#ifdef VERTEX
U0(f0)i0(0,a4,IB);V0
#endif
o1
#ifdef ATLAS_BLIT
n0 H(0,c,Q0);
#else
OPTIONALLY_FLAT H(0,g,i1);
#endif
L2 H(1,a0,j0);p1
#ifdef VERTEX
q1(PB,f0,B,n,K){l0(n,B,IB,Z);
#ifdef ATLAS_BLIT
L(Q0,c);
#else
L(i1,g);
#endif
L(j0,a0);uint R;c J;
#ifdef ATLAS_BLIT
J=a8(IB,R,Q0 Y1);
#else
J=c8(IB,R,i1 Y1);
#endif
j0=Q1(R);f Q=F2(J);
#ifdef ATLAS_BLIT
P(Q0);
#else
P(i1);
#endif
P(j0);h1(Q);}
#endif
#endif
#ifdef DRAW_IMAGE_RECT
#ifdef VERTEX
U0(f0)i0(0,f,YB);V0
#endif
o1 n0 H(0,c,q0);n0 H(1,g,c4);
#ifdef ENABLE_CLIP_RECT
n0 H(2,f,R0);
#endif
p1
#ifdef VERTEX
G6(PB,f0,B,n,K){l0(n,B,YB,f);L(q0,c);L(c4,g);
#ifdef ENABLE_CLIP_RECT
L(R0,f);
#endif
bool d8=YB.z==.0||YB.w==.0;c4=d8?.0:1.;c J=YB.xy;S D0=D1(m0.H6);S A5=transpose(inverse(D0));if(!d8){float e8=q3*f8(A5[1])/dot(D0[1],A5[1]);if(e8>=.5){J.x=.5;c4*=d4(.5/e8);}else{J.x+=e8*YB.z;}float g8=q3*f8(A5[0])/dot(D0[0],A5[0]);if(g8>=.5){J.y=.5;c4*=d4(.5/g8);}else{J.y+=g8*YB.w;}}q0=J;J=C0(D0,J)+m0.S0;if(d8){c e4=C0(A5,YB.zw);e4*=f8(e4)/dot(e4,e4);J+=q3*e4;}
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){R0=I6(D1(m0.R1),m0.Z1,J);}
#endif
f Q=F2(J);P(q0);P(c4);
#ifdef ENABLE_CLIP_RECT
P(R0);
#endif
h1(Q);}
#endif
#elif defined(DRAW_IMAGE_MESH)
#ifdef VERTEX
U0(a2)i0(0,c,SB);V0 U0(v2)i0(1,c,TB);V0
#endif
o1 n0 H(0,c,q0);
#ifdef ENABLE_CLIP_RECT
n0 H(1,f,R0);
#endif
p1
#ifdef VERTEX
N4(PB,a2,c2,v2,w2,n){l0(n,c2,SB,c);l0(n,w2,TB,c);L(q0,c);
#ifdef ENABLE_CLIP_RECT
L(R0,f);
#endif
S D0=D1(m0.H6);c J=C0(D0,SB)+m0.S0;q0=TB;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){R0=I6(D1(m0.R1),m0.Z1,J);}
#endif
f Q=F2(J);P(q0);
#ifdef ENABLE_CLIP_RECT
P(R0);
#endif
h1(Q);}
#endif
#endif
#ifdef DRAW_RENDER_TARGET_UPDATE_BOUNDS
#ifdef VERTEX
U0(f0)V0
#endif
o1 p1
#ifdef VERTEX
q1(PB,f0,B,n,K){c0 S1;S1.x=(n&1)==0?q.J6.x:q.J6.z;S1.y=(n&2)==0?q.J6.y:q.J6.w;f Q=F2(c(S1));h1(Q);}
#endif
#endif
#ifdef DRAW_IMAGE
#endif
#ifdef FRAGMENT
x2
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
#ifdef COLOR_PLANE_IDX_OVERRIDE
#define h8 COLOR_PLANE_IDX_OVERRIDE
#else
#define h8 i8
#endif
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
O4(h8,H0);
#else
M0(h8,H0);
#endif
#endif
#ifdef PLS_BLEND_SRC_OVER
#define r3 i
#define j8 I0
#define K6 E1(.0)
#define V9(m) ((m).w!=.0)
#ifdef ENABLE_CLIPPING
#ifndef RESOLVE_PLS
M0(B5,r1);
#else
O4(B5,r1);
#endif
#endif
#else
#define r3 uint
#define K6 0u
#define j8 j1
#define V9(m) ((m)!=0u)
#ifdef ENABLE_CLIPPING
Y0(B5,r1);
#endif
#endif
f4(k8,v3);y2 w3 g4(l8,W9,DC);h4(m8,X9,KB);x3 d uint Jc(float x){return uint(round(x*n8+o8));}d g L6(uint x){return d4(float(x)*Y9+(-o8*Y9));}
#ifdef ENABLE_CLIPPING
d void Z9(uint Z0,r3 I1,i4(g)E){
#ifdef PLS_BLEND_SRC_OVER
if(all(lessThan(abs(I1.xy-unpackUnorm4x8(Z0).xy),Z3(.25/255.))))E=min(E,I1.z);else E=.0;
#else
if(Z0==I1>>16)E=min(E,unpackHalf2x16(I1).x);else E=.0;
#endif
}
#endif
d void M6(uint R,g F1,k1(i)U
#if defined(ENABLE_CLIPPING)&&!defined(RESOLVE_PLS)
,i4(r3)a1
#endif
C5 y3){N0 F0=k4(DC,R);g E=F1;if((F0.x&(Kc|p8))!=0u){E=abs(E);
#ifdef ENABLE_EVEN_ODD
if(ENABLE_EVEN_ODD&&(F0.x&p8)!=0u){E=1.-abs(fract(E*.5)*2.+-1.);}
#endif
}E=clamp(E,v1(.0),v1(1.));
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint Z0=F0.x>>16u;if(Z0!=0u){Z9(Z0,j8(r1),E);}}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT&&(F0.x&Lc)!=0u){S D0=D1(w0(KB,R*4u+2u));f S0=w0(KB,R*4u+3u);c Mc=C0(D0,y0)+S0.xy;G aa=F6(abs(Mc)*S0.zw-S0.zw);g l4=clamp(min(aa.x,aa.y)+.5,.0,1.);E=min(E,l4);}
#endif
uint J1=F0.x&0xfu;if(J1<=q8){U=unpackUnorm4x8(F0.y);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&J1==N6){
#ifndef RESOLVE_PLS
#ifdef PLS_BLEND_SRC_OVER
a1.xy=U.zw;a1.z=E;a1.w=1.;
#else
a1=F0.y|packHalf2x16(Z3(E,.0));
#endif
#endif
U=E1(.0);}
#endif
}else{S D0=D1(w0(KB,R*4u));f S0=w0(KB,R*4u+1u);c G2=C0(D0,y0)+S0.xy;float t=J1==O6?G2.x:length(G2);t=clamp(t,.0,1.);float x=t*S0.z+S0.w;float y=uintBitsToFloat(F0.y);U=T1(MC,r8,c(x,y),.0);}U.w*=E;
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&defined(ENABLE_ADVANCED_BLEND)
a0 z3;if(ENABLE_ADVANCED_BLEND&&U.w!=.0&&(z3=Q1((F0.x>>4)&0xfu))!=0u){i w1=I0(H0);U.xyz=M4(U.xyz,w1,z3);}
#endif
#ifndef PLS_BLEND_SRC_OVER
U.xyz*=U.w;
#endif
}
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&!defined(COALESCED_PLS_RESOLVE_AND_TRANSFER)
d void P6(i U y3){
#ifndef PLS_BLEND_SRC_OVER
if(U.w==.0)return;float D5=1.-U.w;if(D5!=.0)U+=I0(H0)*D5;
#endif
T0(H0,U);}
#endif
#if defined(ENABLE_CLIPPING)&&!defined(RESOLVE_PLS)
d void v8(r3 a1 y3){
#ifdef PLS_BLEND_SRC_OVER
T0(r1,a1);
#else
if(a1!=0u)l1(r1,a1);
#endif
}
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#define E5 A3
#define ca F5
#define Q4 P4
#else
#define E5 z2
#define ca R4
#define Q4 M2
#endif
#ifdef DRAW_PATH
E5(NB){
#ifdef ENABLE_FEATHER
N(D,f);
#else
N(D,G);
#endif
N(j0,a0);g Q6;
#ifdef ENABLE_FEATHER
if(ENABLE_FEATHER&&w8(D)){Q6=R6(D x1);}else if(ENABLE_FEATHER&&S6(D)){Q6=S4(D x1);}else
#endif
{Q6=min(min(v1(D.x),abs(v1(D.y))),v1(1.));}i U=E1(.0);
#ifdef ENABLE_CLIPPING
r3 a1=K6;
#endif
uint T6=Jc(Q6);uint da=(ea(j0)<<T4)|T6;uint U1=G5(v3,da);a0 N2=Q1(U1>>T4);if(N2==j0){if(!U6(D)){T6+=U1-max(da,U1);T6-=x8;H5(v3,T6);}}else{g F1=L6(U1&V6);M6(N2,F1,U
#ifdef ENABLE_CLIPPING
,a1
#endif
Y2 G1);}
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
K1=U;
#else
P6(U G1);
#endif
#ifdef ENABLE_CLIPPING
v8(a1 G1);
#endif
Q4}
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
E5(NB){
#ifdef ATLAS_BLIT
N(Q0,c);
#else
N(i1,g);
#endif
N(j0,a0);uint U1=m4(v3);a0 N2=Q1(U1>>T4);uint y8;
#ifndef ATLAS_BLIT
if(N2==j0){y8=U1;}else
#endif
{y8=(ea(j0)<<T4)+x8;}g E;
#ifdef ATLAS_BLIT
E=W6(Q0,q.U4 x1);
#else
E=i1;
#endif
int Nc=int(round(E*n8));n4(v3,y8+uint(Nc));i U=E1(.0);
#ifdef ENABLE_CLIPPING
r3 a1=K6;
#endif
#ifndef ATLAS_BLIT
if(N2!=j0)
#endif
{g z8=L6(U1&V6);M6(N2,z8,U
#ifdef ENABLE_CLIPPING
,a1
#endif
Y2 G1);}
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
K1=U;
#else
P6(U G1);
#endif
#ifdef ENABLE_CLIPPING
v8(a1 G1);
#endif
Q4}
#endif
#ifdef DRAW_IMAGE
ca(NB){N(q0,c);
#ifdef DRAW_IMAGE_RECT
N(c4,g);
#endif
#ifdef ENABLE_CLIP_RECT
N(R0,f);
#endif
i V4=o4(UB,B3,q0);g W4=1.;
#ifdef DRAW_IMAGE_RECT
W4=min(c4,W4);
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){g l4=A8(I5(R0));W4=clamp(l4,v1(.0),W4);}
#endif
uint U1=m4(v3);a0 N2=Q1(U1>>T4);g z8=L6(U1&V6);i U;
#ifdef ENABLE_CLIPPING
r3 a1=K6;
#endif
M6(N2,z8,U
#ifdef ENABLE_CLIPPING
,a1
#endif
Y2 G1);
#ifdef PLS_BLEND_SRC_OVER
U.xyz*=U.w;
#endif
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&m0.Z0!=0u){r3 I1=V9(a1)?a1:j8(r1);Z9(m0.Z0,I1,W4);}
#endif
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&defined(ENABLE_ADVANCED_BLEND)
if(ENABLE_ADVANCED_BLEND&&m0.z3!=B8){i w1=I0(H0)*(1.-U.w)+U;V4.xyz=M4(Y3(V4),w1,Q1(m0.z3))*V4.w;}
#endif
V4*=W4*d4(m0.H2);U=U*(1.-V4.w)+V4;
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
K1=U;
#else
P6(U G1);
#endif
#ifdef ENABLE_CLIPPING
v8(a1 G1);
#endif
n4(v3,x8);Q4}
#endif
#ifdef INITIALIZE_PLS
E5(NB){
#ifdef STORE_COLOR_CLEAR
T0(H0,unpackUnorm4x8(q.Oc));
#endif
#ifdef SWIZZLE_COLOR_BGRA_TO_RGBA
i j=I0(H0);T0(H0,j.zyxw);
#endif
n4(v3,q.Pc);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){l1(r1,0u);}
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
discard;
#endif
Q4}
#endif
#ifdef RESOLVE_PLS
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
A3(NB)
#else
E5(NB)
#endif
{uint U1=m4(v3);g F1=L6(U1&V6);a0 N2=Q1(U1>>T4);i U;M6(N2,F1,U Y2 G1);
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
#ifdef PLS_BLEND_SRC_OVER
U.xyz*=U.w;
#endif
float D5=1.-U.w;if(D5!=.0)U+=I0(H0)*D5;K1=U;P4
#else
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
K1=U;
#else
P6(U G1);
#endif
Q4
#endif
}
#endif
#endif
