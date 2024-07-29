#ifdef DRAW_PATH
#ifdef VERTEX
U0(R)v0(0,g,VB);v0(1,g,WB);V0
#endif
D1 n0 K(0,F,N0);OPTIONALLY_FLAT K(1,N,Z);E1
#ifdef VERTEX
g1(PB,R,v,k,O){z0(k,v,VB,g);z0(k,v,WB,g);S(N0,F);S(Z,N);g C;d T;if(M6(VB,WB,O,Z,T,N0 o3)){C=k2(T);}else{C=g(A.l2,A.l2,A.l2,A.l2);}U(N0);U(Z);h1(C);}
#endif
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
#ifdef VERTEX
U0(R)v0(0,M3,LB);V0
#endif
D1 OPTIONALLY_FLAT K(0,h,F1);OPTIONALLY_FLAT K(1,N,Z);E1
#ifdef VERTEX
g1(PB,R,v,k,O){z0(k,v,LB,i0);S(F1,h);S(Z,N);d T=N6(LB,Z,F1 o3);g C=k2(T);U(F1);U(Z);h1(C);}
#endif
#endif
#ifdef DRAW_IMAGE
#ifdef DRAW_IMAGE_RECT
#ifdef VERTEX
U0(R)v0(0,g,NB);V0
#endif
D1 n0 K(0,d,E0);n0 K(1,h,I2);
#ifdef ENABLE_CLIP_RECT
n0 K(2,g,m0);
#endif
E1
#ifdef VERTEX
Q1 R1 Y1 Z1 v5(PB,R,v,k,O){z0(k,v,NB,g);S(E0,d);S(I2,h);
#ifdef ENABLE_CLIP_RECT
S(m0,g);
#endif
bool w5=NB.z==.0||NB.w==.0;I2=w5?.0:1.;d T=NB.xy;B d0=W0(L.x5);B N3=transpose(inverse(d0));if(!w5){float y5=m2*z5(N3[1])/dot(d0[1],N3[1]);if(y5>=.5){T.x=.5;I2*=V(.5/y5);}else{T.x+=y5*NB.z;}float A5=m2*z5(N3[0])/dot(d0[0],N3[0]);if(A5>=.5){T.y=.5;I2*=V(.5/A5);}else{T.y+=A5*NB.w;}}E0=T;T=l0(d0,T)+L.O0;if(w5){d J2=l0(N3,NB.zw);J2*=z5(J2)/dot(J2,J2);T+=m2*J2;}
#ifdef ENABLE_CLIP_RECT
m0=x4(W0(L.X0),L.i1,T);
#endif
g C=k2(T);U(E0);U(I2);
#ifdef ENABLE_CLIP_RECT
U(m0);
#endif
h1(C);}
#endif
#else
#ifdef VERTEX
U0(a2)v0(0,d,XB);V0 U0(n2)v0(1,d,YB);V0
#endif
D1 n0 K(0,d,E0);
#ifdef ENABLE_CLIP_RECT
n0 K(1,g,m0);
#endif
E1
#ifdef VERTEX
y4(PB,a2,c2,n2,o2,k){z0(k,c2,XB,d);z0(k,o2,YB,d);S(E0,d);
#ifdef ENABLE_CLIP_RECT
S(m0,g);
#endif
B d0=W0(L.x5);d T=l0(d0,XB)+L.O0;E0=YB;
#ifdef ENABLE_CLIP_RECT
m0=x4(W0(L.X0),L.i1,T);
#endif
g C=k2(T);U(E0);
#ifdef ENABLE_CLIP_RECT
U(m0);
#endif
h1(C);}
#endif
#endif
#endif
#ifdef DRAW_RENDER_TARGET_UPDATE_BOUNDS
#ifdef VERTEX
U0(R)V0
#endif
D1 E1
#ifdef VERTEX
Q1 R1 Y1 Z1 g1(PB,R,v,k,O){o0 j1;j1.x=(k&1)==0?A.z4.x:A.z4.z;j1.y=(k&2)==0?A.z4.y:A.z4.w;g C=k2(d(j1));h1(C);}
#endif
#endif
#ifdef ENABLE_BINDLESS_TEXTURES
#define B5
#endif
#ifdef DRAW_IMAGE
#define B5
#endif
#ifdef FRAGMENT
K2 z1(d2,A4,EC);
#ifdef B5
z1(C5,p3,OB);
#endif
L2 O3(A4,D5)
#ifdef B5
q3(p3,p2)
#endif
G1
#ifdef ENABLE_ADVANCED_BLEND
#ifdef FRAMEBUFFER_PLANE_IDX_OVERRIDE
F0(PC,e0);
#else
F0(E5,e0);
#endif
#endif
P3(F5,Y0);
#ifdef ENABLE_CLIPPING
G0(G5,A0);
#endif
H1 Q3 R3(O6,n9,QB);S3(P6,o9,IB);T3 uint Q6(float x){return uint(x*R6+U3);}h B4(uint x){return V(float(x)*S6+(-U3*S6));}j C4(h Z0,B0 G,uint k1 D4 M2){h D=abs(Z0);
#ifdef ENABLE_EVEN_ODD
if((G.x&T6)!=0u)D=1.-abs(fract(D*.5)*2.+-1.);
#endif
D=min(D,V(1));
#ifdef ENABLE_CLIPPING
uint I1=G.x>>16u;if(I1!=0u){uint a1=H0(A0);h N2=I1==(a1>>16u)?unpackHalf2x16(a1).x:.0;D=min(D,N2);}
#endif
j f=j0(0,0,0,0);uint J1=G.x&0xfu;switch(J1){case U6:f=unpackUnorm4x8(G.y);
#ifdef ENABLE_CLIPPING
A1(A0);
#endif
break;case E4:case V6:
#ifdef ENABLE_BINDLESS_TEXTURES
case W6:
#endif
{B d0=W0(C0(IB,k1*4u));g O0=C0(IB,k1*4u+1u);d q2=l0(d0,p0)+O0.xy;
#ifdef ENABLE_BINDLESS_TEXTURES
if(J1==W6){f=V3(sampler2D(floatBitsToUint(O0.zw)),p2,q2,d0[0],d0[1]);float O2=uintBitsToFloat(G.y);f.w*=O2;}else
#endif
{float t=J1==E4?q2.x:length(q2);t=clamp(t,.0,1.);float x=t*O0.z+O0.w;float y=uintBitsToFloat(G.y);f=j0(W3(EC,D5,d(x,y),.0));}
#ifdef ENABLE_CLIPPING
A1(A0);
#endif
break;}
#ifdef ENABLE_CLIPPING
case F4:P0(A0,G.y|packHalf2x16(l1(D,0)));break;
#endif
}
#ifdef ENABLE_CLIP_RECT
if((G.x&p9)!=0u){B d0=W0(C0(IB,k1*4u+2u));g O0=C0(IB,k1*4u+3u);d q9=l0(d0,p0)+O0.xy;F X6=l1(abs(q9)*O0.zw-O0.zw);h P2=clamp(min(X6.x,X6.y)+.5,.0,1.);D=min(D,P2);}
#endif
f.w*=D;return f;}j Y6(j Z6,j S1){return Z6+S1*(1.-Z6.w);}
#ifdef ENABLE_ADVANCED_BLEND
j H5(j a7,j S1,N e2){if(e2!=c7){
#ifdef ENABLE_HSL_BLEND_MODES
return J3(
#else
return K3(
#endif
a7,X3(S1),e2);}else{return Y6(r2(a7),S1);}}j d7(j f,B0 G M2){j S1=I0(e0);N e2=J0((G.x>>4)&0xfu);return H5(f,S1,e2);}void I5(j f,B0 G M2){if(f.w!=.0){j r9=d7(f,G m1);w0(e0,r9);}else{n1(e0);}}
#endif
#ifdef ENABLE_ADVANCED_BLEND
#define Y3 T1
#define e7 Z3
#define r3 f2
#else
#define Y3 v3
#define e7 G4
#define r3 a4
#endif
#ifdef DRAW_PATH
Y3(JB){P(N0,F);P(Z,N);
#ifndef ENABLE_ADVANCED_BLEND
Q0=j0(0,0,0,0);
#endif
h D=min(min(N0.x,abs(N0.y)),V(1));uint H4=Q6(D);uint J5=(f7(Z)<<16)|H4;uint o1=I4(Y0,J5);N c1=J0(o1>>16);if(c1!=Z){h Z0=B4(o1&0xffffu);B0 G=v2(QB,c1);j f=C4(Z0,G,c1 Q2 m1);
#ifdef ENABLE_ADVANCED_BLEND
I5(f,G m1);
#else
Q0=r2(f);
#endif
}else{if(N0.y<.0){if(o1<J5){H4+=o1-J5;}H4-=uint(U3);J4(Y0,H4);}discard;}r3}
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
Y3(JB){P(F1,h);P(Z,N);
#ifndef ENABLE_ADVANCED_BLEND
Q0=j0(0,0,0,0);
#endif
h D=F1;uint o1=w3(Y0);N c1=J0(o1>>16);h g7=B4(o1&0xffffu);if(c1!=Z){B0 G=v2(QB,c1);j f=C4(g7,G,c1 Q2 m1);
#ifdef ENABLE_ADVANCED_BLEND
I5(f,G m1);
#else
Q0=r2(f);
#endif
}else{D+=g7;}x3(Y0,(f7(Z)<<16)|Q6(D));if(c1==Z){discard;}r3}
#endif
#ifdef DRAW_IMAGE
e7(JB){P(E0,d);
#ifdef DRAW_IMAGE_RECT
P(I2,h);
#endif
#ifdef ENABLE_CLIP_RECT
P(m0,g);
#endif
j y3=j0(R2(OB,p2,E0));h S2=1.;
#ifdef DRAW_IMAGE_RECT
S2=min(I2,S2);
#endif
#ifdef ENABLE_CLIP_RECT
h P2=K5(j0(m0));S2=clamp(P2,V(0),S2);
#endif
#ifdef DRAW_IMAGE_MESH
p1;
#endif
uint o1=w3(Y0);h Z0=B4(o1&0xffffu);N c1=J0(o1>>16);B0 h7=v2(QB,c1);j L5=C4(Z0,h7,c1 Q2 m1);
#ifdef ENABLE_CLIPPING
if(L.I1!=0u){K4(A0);uint a1=H0(A0);uint I1=a1>>16;h N2=I1==L.I1?unpackHalf2x16(a1).x:.0;S2=min(S2,N2);}
#endif
y3.w*=S2*V(L.O2);
#ifdef ENABLE_ADVANCED_BLEND
if(L5.w!=.0||y3.w!=.0){j S1=I0(e0);N v9=J0((h7.x>>4)&0xfu);N w9=J0(L.e2);S1=H5(L5,S1,v9);y3=H5(y3,S1,w9);w0(e0,y3);}else{n1(e0);}
#else
Q0=Y6(r2(y3),r2(L5));
#endif
x3(Y0,uint(U3));
#ifdef DRAW_IMAGE_MESH
q1;
#endif
r3}
#endif
#ifdef INITIALIZE_PLS
Y3(JB){
#ifndef ENABLE_ADVANCED_BLEND
Q0=j0(0,0,0,0);
#endif
#ifdef STORE_COLOR_CLEAR
w0(e0,unpackUnorm4x8(A.x9));
#endif
#ifdef SWIZZLE_COLOR_BGRA_TO_RGBA
j f=I0(e0);w0(e0,f.zyxw);
#endif
x3(Y0,A.y9);
#ifdef ENABLE_CLIPPING
P0(A0,0u);
#endif
r3}
#endif
#ifdef RESOLVE_PLS
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
v3(JB)
#else
Y3(JB)
#endif
{uint o1=w3(Y0);h Z0=B4(o1&0xffffu);N c1=J0(o1>>16);B0 G=v2(QB,c1);j f=C4(Z0,G,c1 Q2 m1);
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
Q0=d7(f,G m1);a4
#else
#ifdef ENABLE_ADVANCED_BLEND
I5(f,G m1);
#else
Q0=r2(f);
#endif
r3
#endif
}
#endif
#endif
