#ifdef DRAW_PATH
#ifdef VERTEX
L0(e0)h0(0,e,LB);h0(1,e,MB);M0
#endif
o1
#ifdef ENABLE_FEATHER
o0 H(0,e,q);
#else
o0 H(0,C,q);
#endif
D2 H(1,O,i0);p1
#ifdef VERTEX
Z0(PB,e0,o,l,I){k0(l,o,LB,e);k0(l,o,MB,e);
#ifdef ENABLE_FEATHER
P(q,e);
#else
P(q,C);
#endif
P(i0,O);e Q;uint d0;c K;e M;if(S5(LB,MB,I,d0,K,M g2)){
#ifdef ENABLE_FEATHER
q=M;
#else
q.xy=T5(M.xy);
#endif
i0=M1(d0);Q=v2(K);}else{Q=e(m.q1,m.q1,m.q1,m.q1);}X(q);X(i0);U0(Q);}
#endif
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
#ifdef VERTEX
L0(e0)h0(0,R3,IB);M0
#endif
o1
#ifdef ATLAS_COVERAGE
o0 H(0,c,N1);
#else
OPTIONALLY_FLAT H(0,h,a1);
#endif
D2 H(1,O,i0);p1
#ifdef VERTEX
Z0(PB,e0,o,l,I){k0(l,o,IB,a0);
#ifdef ATLAS_COVERAGE
P(N1,c);
#else
P(a1,h);
#endif
P(i0,O);uint d0;c K;
#ifdef ATLAS_COVERAGE
K=Y8(IB,d0,N1 g2);
#else
K=g7(IB,d0,a1 g2);
#endif
i0=M1(d0);e Q=v2(K);
#ifdef ATLAS_COVERAGE
X(N1);
#else
X(a1);
#endif
X(i0);U0(Q);}
#endif
#endif
#ifdef DRAW_IMAGE_RECT
#ifdef VERTEX
L0(e0)h0(0,e,YB);M0
#endif
o1 o0 H(0,c,A0);o0 H(1,h,S3);
#ifdef ENABLE_CLIP_RECT
o0 H(2,e,F0);
#endif
p1
#ifdef VERTEX
U5(PB,e0,o,l,I){k0(l,o,YB,e);P(A0,c);P(S3,h);
#ifdef ENABLE_CLIP_RECT
P(F0,e);
#endif
bool h7=YB.z==.0||YB.w==.0;S3=h7?.0:1.;c K=YB.xy;Y w0=r1(l0.V5);Y X4=transpose(inverse(w0));if(!h7){float i7=g3*j7(X4[1])/dot(w0[1],X4[1]);if(i7>=.5){K.x=.5;S3*=T3(.5/i7);}else{K.x+=i7*YB.z;}float k7=g3*j7(X4[0])/dot(w0[0],X4[0]);if(k7>=.5){K.y=.5;S3*=T3(.5/k7);}else{K.y+=k7*YB.w;}}A0=K;K=q0(w0,K)+l0.G0;if(h7){c U3=q0(X4,YB.zw);U3*=j7(U3)/dot(U3,U3);K+=g3*U3;}
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){F0=W5(r1(l0.F1),l0.O1,K);}
#endif
e Q=v2(K);X(A0);X(S3);
#ifdef ENABLE_CLIP_RECT
X(F0);
#endif
U0(Q);}
#endif
#elif defined(DRAW_IMAGE_MESH)
#ifdef VERTEX
L0(P1)h0(0,c,SB);M0 L0(h2)h0(1,c,TB);M0
#endif
o1 o0 H(0,c,A0);
#ifdef ENABLE_CLIP_RECT
o0 H(1,e,F0);
#endif
p1
#ifdef VERTEX
v4(PB,P1,Q1,h2,i2,l){k0(l,Q1,SB,c);k0(l,i2,TB,c);P(A0,c);
#ifdef ENABLE_CLIP_RECT
P(F0,e);
#endif
Y w0=r1(l0.V5);c K=q0(w0,SB)+l0.G0;A0=TB;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){F0=W5(r1(l0.F1),l0.O1,K);}
#endif
e Q=v2(K);X(A0);
#ifdef ENABLE_CLIP_RECT
X(F0);
#endif
U0(Q);}
#endif
#endif
#ifdef DRAW_RENDER_TARGET_UPDATE_BOUNDS
#ifdef VERTEX
L0(e0)M0
#endif
o1 p1
#ifdef VERTEX
Z0(PB,e0,o,l,I){f0 j2;j2.x=(l&1)==0?m.X5.x:m.X5.z;j2.y=(l&2)==0?m.X5.y:m.X5.w;e Q=v2(c(j2));U0(Q);}
#endif
#endif
#ifdef DRAW_IMAGE
#endif
#ifdef FRAGMENT
k2
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
#ifdef COLOR_PLANE_IDX_OVERRIDE
N0(COLOR_PLANE_IDX_OVERRIDE,B0);
#else
N0(l7,B0);
#endif
#endif
#ifdef PLS_BLEND_SRC_OVER
#define h3 i
#define m7 C0
#define Y5 f2(.0)
#define Z8(N) ((N).w!=.0)
#ifdef ENABLE_CLIPPING
#ifndef RESOLVE_PLS
N0(Y4,c1);
#else
a9(Y4,c1);
#endif
#endif
#else
#define h3 uint
#define Y5 0u
#define m7 V0
#define Z8(N) ((N)!=0u)
#ifdef ENABLE_CLIPPING
P0(Y4,c1);
#endif
#endif
V3(n7,i3);l2 j3 W3(o7,c9,CC);X3(p7,d9,JB);k3 d uint Bb(float x){return uint(round(x*q7+r7));}d h Z5(uint x){return T3(float(x)*e9+(-r7*e9));}
#ifdef ENABLE_CLIPPING
d void f9(uint m2,h3 x1,Y3(h)J){
#ifdef PLS_BLEND_SRC_OVER
if(all(lessThan(abs(x1.xy-unpackUnorm4x8(m2).xy),Q3(.25/255.))))J=min(J,x1.z);else J=.0;
#else
if(m2==x1>>16)J=min(J,unpackHalf2x16(x1).x);else J=.0;
#endif
}
#endif
d void a6(uint d0,h R1,G1(i)c0
#if defined(ENABLE_CLIPPING)&&!defined(RESOLVE_PLS)
,Y3(h3)Q0
#endif
Z4 l3){E0 z0=a4(CC,d0);h J=R1;if((z0.x&(Cb|v7))!=0u){J=abs(J);
#ifdef ENABLE_EVEN_ODD
if(ENABLE_EVEN_ODD&&(z0.x&v7)!=0u){J=1.-abs(fract(J*.5)*2.+-1.);}
#endif
}J=clamp(J,d1(.0),d1(1.));
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint m2=z0.x>>16u;if(m2!=0u){f9(m2,m7(c1),J);}}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT&&(z0.x&Db)!=0u){Y w0=r1(r0(JB,d0*4u+2u));e G0=r0(JB,d0*4u+3u);c Eb=q0(w0,v0)+G0.xy;C g9=T5(abs(Eb)*G0.zw-G0.zw);h c4=clamp(min(g9.x,g9.y)+.5,.0,1.);J=min(J,c4);}
#endif
uint y1=z0.x&0xfu;if(y1<=w7){c0=unpackUnorm4x8(z0.y);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&y1==c6){
#ifndef RESOLVE_PLS
#ifdef PLS_BLEND_SRC_OVER
Q0.xy=c0.zw;Q0.z=J;Q0.w=1.;
#else
Q0=z0.y|packHalf2x16(Q3(J,.0));
#endif
#endif
c0=f2(.0);}
#endif
}else{Y w0=r1(r0(JB,d0*4u));e G0=r0(JB,d0*4u+1u);c w2=q0(w0,v0)+G0.xy;float t=y1==d6?w2.x:length(w2);t=clamp(t,.0,1.);float x=t*G0.z+G0.w;float y=uintBitsToFloat(z0.y);c0=S1(KC,x7,c(x,y),.0);}c0.w*=J;
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&defined(ENABLE_ADVANCED_BLEND)
O m3;if(ENABLE_ADVANCED_BLEND&&c0.w!=.0&&(m3=M1((z0.x>>4)&0xfu))!=0u){i e1=C0(B0);c0.xyz=X8(c0.xyz,w4(e1),m3);}
#endif
#ifndef PLS_BLEND_SRC_OVER
c0.xyz*=c0.w;
#endif
}
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
d void e6(i c0 l3){
#ifndef PLS_BLEND_SRC_OVER
if(c0.w==.0)return;float i9=1.-c0.w;if(i9!=.0)c0=C0(B0)*i9+c0;
#endif
H0(B0,c0);}
#endif
#if defined(ENABLE_CLIPPING)&&!defined(RESOLVE_PLS)
d void y7(h3 Q0 l3){
#ifdef PLS_BLEND_SRC_OVER
H0(c1,Q0);
#else
if(Q0!=0u)W0(c1,Q0);
#endif
}
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#define a5 n3
#define j9 c5
#define y4 x4
#else
#define a5 n2
#define j9 z4
#define y4 E2
#endif
#ifdef DRAW_PATH
a5(NB){
#ifdef ENABLE_FEATHER
Z(q,e);
#else
Z(q,C);
#endif
Z(i0,O);h f6;
#ifdef ENABLE_FEATHER
if(ENABLE_FEATHER&&z7(q)){f6=g6(q z1);}else if(ENABLE_FEATHER&&h6(q)){f6=A4(q z1);}else
#endif
{f6=min(min(d1(q.x),abs(d1(q.y))),d1(1.));}uint i6=Bb(f6);uint k9=(l9(i0)<<B4)|i6;uint H1=d5(i3,k9);O F2=M1(H1>>B4);if(F2==i0){if(!j6(q)){i6+=H1-max(k9,H1);i6-=A7;e5(i3,i6);}discard;}h R1=Z5(H1&k6);i c0;
#ifdef ENABLE_CLIPPING
h3 Q0=Y5;
#endif
a6(F2,R1,c0
#ifdef ENABLE_CLIPPING
,Q0
#endif
o3 v1);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
A1=c0;
#else
e6(c0 v1);
#endif
#ifdef ENABLE_CLIPPING
y7(Q0 v1);
#endif
y4}
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
a5(NB){
#ifdef ATLAS_COVERAGE
Z(N1,c);
#else
Z(a1,h);
#endif
Z(i0,O);uint H1=d4(i3);O F2=M1(H1>>B4);uint B7;
#ifndef ATLAS_COVERAGE
if(F2==i0){B7=H1;}else
#endif
{B7=(l9(i0)<<B4)+A7;}h J;
#ifdef ATLAS_COVERAGE
J=C7(N1,m.f5 z1);
#else
J=a1;
#endif
int Fb=int(round(J*q7));e4(i3,B7+uint(Fb));
#ifndef ATLAS_COVERAGE
if(F2==i0){discard;}
#endif
h D7=Z5(H1&k6);i c0;
#ifdef ENABLE_CLIPPING
h3 Q0=Y5;
#endif
a6(F2,D7,c0
#ifdef ENABLE_CLIPPING
,Q0
#endif
o3 v1);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
A1=c0;
#else
e6(c0 v1);
#endif
#ifdef ENABLE_CLIPPING
y7(Q0 v1);
#endif
y4}
#endif
#ifdef DRAW_IMAGE
j9(NB){Z(A0,c);
#ifdef DRAW_IMAGE_RECT
Z(S3,h);
#endif
#ifdef ENABLE_CLIP_RECT
Z(F0,e);
#endif
i f4=p3(UB,q3,A0);h C4=1.;
#ifdef DRAW_IMAGE_RECT
C4=min(S3,C4);
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){h c4=E7(g5(F0));C4=clamp(c4,d1(.0),C4);}
#endif
uint H1=d4(i3);O F2=M1(H1>>B4);h D7=Z5(H1&k6);i c0;
#ifdef ENABLE_CLIPPING
h3 Q0=Y5;
#endif
a6(F2,D7,c0
#ifdef ENABLE_CLIPPING
,Q0
#endif
o3 v1);
#ifdef PLS_BLEND_SRC_OVER
c0.xyz*=c0.w;
#endif
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&l0.m2!=0u){h3 x1=Z8(Q0)?Q0:m7(c1);f9(l0.m2,x1,C4);}
#endif
f4.w*=C4*T3(l0.G2);
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&defined(ENABLE_ADVANCED_BLEND)
if(ENABLE_ADVANCED_BLEND&&l0.m3!=F7){i e1=C0(B0)*(1.-c0.w)+c0;f4.xyz=X8(f4.xyz,w4(e1),M1(l0.m3));}
#endif
f4.xyz*=f4.w;c0=c0*(1.-f4.w)+f4;
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
A1=c0;
#else
e6(c0 v1);
#endif
#ifdef ENABLE_CLIPPING
y7(Q0 v1);
#endif
e4(i3,A7);y4}
#endif
#ifdef INITIALIZE_PLS
a5(NB){
#ifdef STORE_COLOR_CLEAR
H0(B0,unpackUnorm4x8(m.Gb));
#endif
#ifdef SWIZZLE_COLOR_BGRA_TO_RGBA
i j=C0(B0);H0(B0,j.zyxw);
#endif
e4(i3,m.Hb);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){W0(c1,0u);}
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
discard;
#endif
y4}
#endif
#ifdef RESOLVE_PLS
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
n3(NB)
#else
a5(NB)
#endif
{uint H1=d4(i3);h R1=Z5(H1&k6);O F2=M1(H1>>B4);i c0;a6(F2,R1,c0 o3 v1);
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
A1=C0(B0)*(1.-c0.w)+c0;x4
#else
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
A1=c0;
#else
e6(c0 v1);
#endif
y4
#endif
}
#endif
#endif
