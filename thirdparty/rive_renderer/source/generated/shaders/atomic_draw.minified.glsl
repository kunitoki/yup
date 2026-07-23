#ifdef DRAW_PATH
#ifdef VERTEX
A1(a0)p0(0,g,SB);p0(1,g,TB);B1
#endif
h2
#ifdef ENABLE_FEATHER
J0 c0(0,g,I);
#else
J0 c0(0,D,I);
#endif
R4 c0(1,X,z0);a2
#ifdef VERTEX
C1(YB,a0,G,v,T){q0(v,G,SB,g);q0(v,G,TB,g);
#ifdef ENABLE_FEATHER
Y(I,g);
#else
Y(I,D);
#endif
Y(z0,X);g N;uint l0;c i0;g J;if(p9(SB,TB,T,l0,i0,J v3)){
#ifdef ENABLE_FEATHER
I=J;
#else
I.xy=R7(J.xy);
#endif
z0=i2(l0);N=K3(i0);}else{N=g(k.P2,k.P2,k.P2,k.P2);}k0(I);k0(z0);D1(N);}
#endif
#endif
#if defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_BLIT)
#ifdef VERTEX
A1(a0)p0(0,L3,KB);B1
#endif
h2
#ifdef ATLAS_BLIT
J0 c0(0,c,C2);
#else
OPTIONALLY_FLAT c0(0,d,j1);
#endif
R4 c0(1,X,z0);a2
#ifdef VERTEX
C1(YB,a0,G,v,T){q0(v,G,KB,V);
#ifdef ATLAS_BLIT
Y(C2,c);
#else
Y(j1,d);
#endif
Y(z0,X);uint l0;c i0;
#ifdef ATLAS_BLIT
i0=sb(KB,l0,C2 v3);
#else
i0=tb(KB,l0,j1 v3);
#endif
z0=i2(l0);g N=K3(i0);
#ifdef ATLAS_BLIT
k0(C2);
#else
k0(j1);
#endif
k0(z0);D1(N);}
#endif
#endif
#ifdef DRAW_IMAGE_RECT
#ifdef VERTEX
A1(a0)p0(0,g,ZB);B1
#endif
h2 J0 c0(0,c,U0);J0 c0(1,d,S4);
#ifdef ENABLE_CLIP_RECT
J0 c0(2,g,N0);
#endif
a2
#ifdef VERTEX
S7(YB,a0,G,v,T){q0(v,G,ZB,g);Y(U0,c);Y(S4,d);
#ifdef ENABLE_CLIP_RECT
Y(N0,g);
#endif
bool q9=ZB.z==.0||ZB.w==.0;S4=q9?.0:1.;c i0=ZB.xy;Z Y0=j2(A0.r9);Z D6=transpose(inverse(Y0));if(!q9){float v9=l4*w9(D6[1])/dot(Y0[1],D6[1]);if(v9>=.5){i0.x=.5;S4*=m4(.5/v9);}else{i0.x+=v9*ZB.z;}float x9=l4*w9(D6[0])/dot(Y0[0],D6[0]);if(x9>=.5){i0.y=.5;S4*=m4(.5/x9);}else{i0.y+=x9*ZB.w;}}U0=i0;i0=Z0(Y0,i0)+A0.c2;if(q9){c M3=Z0(D6,ZB.zw);M3*=w9(M3)/dot(M3,M3);i0+=l4*M3;}
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){N0=T7(j2(A0.k2),A0.D2,i0);}
#endif
g N=K3(i0);k0(U0);k0(S4);
#ifdef ENABLE_CLIP_RECT
k0(N0);
#endif
D1(N);}
#endif
#elif defined(DRAW_IMAGE_MESH)
#ifdef VERTEX
A1(h3)p0(0,c,GC);B1 A1(w3)p0(1,c,HC);B1
#endif
h2 J0 c0(0,c,U0);
#ifdef ENABLE_CLIP_RECT
J0 c0(1,g,N0);
#endif
a2
#ifdef VERTEX
E6(YB,h3,i3,w3,x3,v){q0(v,i3,GC,c);q0(v,x3,HC,c);Y(U0,c);
#ifdef ENABLE_CLIP_RECT
Y(N0,g);
#endif
Z Y0=j2(A0.r9);c i0=Z0(Y0,GC)+A0.c2;U0=HC;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){N0=T7(j2(A0.k2),A0.D2,i0);}
#endif
g N=K3(i0);k0(U0);
#ifdef ENABLE_CLIP_RECT
k0(N0);
#endif
D1(N);}
#endif
#endif
#ifdef DRAW_RENDER_TARGET_UPDATE_BOUNDS
#ifdef VERTEX
A1(a0)B1
#endif
h2 a2
#ifdef VERTEX
C1(YB,a0,G,v,T){U l2;l2.x=(v&1)==0?k.U7.x:k.U7.z;l2.y=(v&2)==0?k.U7.y:k.U7.w;g N=K3(c(l2));D1(N);}
#endif
#endif
#ifdef DRAW_IMAGE
#endif
#if defined(INITIALIZE_PLS)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
#endif
#ifdef FRAGMENT
J1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
#ifdef COLOR_PLANE_IDX_OVERRIDE
#define y9 COLOR_PLANE_IDX_OVERRIDE
#else
#define y9 Q2
#endif
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
n4(y9,g0);
#else
r0(y9,g0);
#endif
#endif
#ifdef PLS_BLEND_SRC_OVER
#define o4 i
#define z9 H0
#define V7 B0(.0)
#define ub(q) ((q).w!=.0)
#ifdef ENABLE_CLIPPING
#ifndef RESOLVE_PLS
r0(R2,d0);
#else
n4(R2,d0);
#endif
#endif
#else
#define o4 uint
#define V7 0u
#define z9 d1
#define ub(q) ((q)!=0u)
#ifdef ENABLE_CLIPPING
k1(R2,d0);
#endif
#endif
E2(F6,p4);K1 N3 J5(vb,Be,TC);K5(wb,Ce,PB);O3 e uint De(float x){return uint(round(x*A9+B9));}e d W7(uint x){return m4(float(x)*xb+(-B9*xb));}X X7(X l0){
#ifdef NEEDS_PATH_ID_CLAMP_WORKAROUND
l0=min(l0,k.Ee);
#endif
return l0;}
#ifdef ENABLE_CLIPPING
e void yb(uint V0,o4 O0,T4(d)n){
#ifdef PLS_BLEND_SRC_OVER
if(all(lessThan(abs(O0.xy-unpackUnorm4x8(V0).xy),A2(.25/255.))))n=min(n,O0.z);else n=.0;
#else
if(V0==O0>>16)n=min(n,unpackHalf2x16(O0).x);else n=.0;
#endif
}
#endif
e void Y7(uint l0,d m0,e1(i)P
#if defined(ENABLE_CLIPPING)&&!defined(RESOLVE_PLS)
,T4(o4)q1
#endif
G6 P3){W0 r1=L5(TC,l0);d n=m0;if((r1.x&(Fe|C9))!=0u){n=abs(n);
#ifdef ENABLE_EVEN_ODD
if(ENABLE_EVEN_ODD&&(r1.x&C9)!=0u){n=1.-abs(fract(n*.5)*2.+-1.);}
#endif
}n=clamp(n,G0(.0),G0(1.));
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint V0=r1.x>>16u;if(V0!=0u){yb(V0,z9(d0),n);}}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT&&(r1.x&Ge)!=0u){Z Y0=j2(P0(PB,l0*4u+2u));g c2=P0(PB,l0*4u+3u);c He=Z0(Y0,S)+c2.xy;D zb=R7(abs(He)*c2.zw-c2.zw);d U4=clamp(min(zb.x,zb.y)+.5,.0,1.);n=min(n,U4);}
#endif
uint j3=r1.x&0xfu;if(j3<=Ab){P=unpackUnorm4x8(r1.y);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&j3==Z7){
#ifndef RESOLVE_PLS
#ifdef PLS_BLEND_SRC_OVER
q1.xy=P.zw;q1.z=n;q1.w=1.;
#else
q1=r1.y|packHalf2x16(A2(n,.0));
#endif
#endif
P=B0(.0);}
#endif
}else{Z Y0=j2(P0(PB,l0*4u));g c2=P0(PB,l0*4u+1u);c V4=Z0(Y0,S)+c2.xy;float t=j3==E9?V4.x:length(V4);t=clamp(t,.0,1.);float x=t*c2.z+c2.w;float y=uintBitsToFloat(r1.y);P=m2(DD,Bb,c(x,y),.0);}P.w*=n;
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&defined(ENABLE_ADVANCED_BLEND)
X n2;if(ENABLE_ADVANCED_BLEND&&P.w!=.0&&(n2=i2((r1.x>>4)&0xfu))!=M5){i L1=H0(g0);P.xyz=Q4(P.xyz,L1,n2);}
#endif
#if defined(NEEDS_GAMMA_CORRECTION)&&(defined(FIXED_FUNCTION_COLOR_OUTPUT)||defined(RESOLVE_PLS))
P=k3(P);
#endif
P.xyz*=P.w;}
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&!defined(COALESCED_PLS_RESOLVE_AND_TRANSFER)
e void a8(i P P3){
#ifndef PLS_BLEND_SRC_OVER
if(P.w==.0)return;float H6=1.-P.w;if(H6!=.0)P+=H0(g0)*H6;
#endif
v0(g0,P);}
#endif
#if defined(ENABLE_CLIPPING)&&!defined(RESOLVE_PLS)
e void F9(o4 q1 P3){
#ifdef PLS_BLEND_SRC_OVER
v0(d0,q1);
#else
if(q1!=0u)f1(d0,q1);
#endif
}
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#define I6 o2
#define Cb r4
#define N5 l3
#else
#define I6 m1
#define Cb O5
#define N5 U1
#endif
#ifdef DRAW_PATH
I6(IB){
#ifdef ENABLE_FEATHER
B(I,g);
#else
B(I,D);
#endif
B(z0,X);d c8;
#ifdef ENABLE_FEATHER
if(ENABLE_FEATHER&&Db(I)){c8=v4(I g1);}else if(ENABLE_FEATHER&&Eb(I)){c8=d8(I g1);}else
#endif
{c8=min(min(G0(I.x),abs(G0(I.y))),G0(1.));}i P=B0(.0);
#ifdef ENABLE_CLIPPING
o4 q1=V7;
#endif
uint e8=De(c8);uint Fb=(Gb(z0)<<P5)|e8;uint p2=W4(p4,Fb);X E1=i2(p2>>P5);E1=X7(E1);if(E1==z0){if(!Q5(I)){e8+=p2-max(Fb,p2);e8-=G9;X4(p4,e8);}}else{d m0=W7(p2&f8);Y7(E1,m0,P
#ifdef ENABLE_CLIPPING
,q1
#endif
S2 M1);}P.xyz=Q3(P.xyz,S.xy,k.y3,k.z3);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
l1=P;
#else
a8(P M1);
#endif
#ifdef ENABLE_CLIPPING
F9(q1 M1);
#endif
N5}
#endif
#if defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_BLIT)
I6(IB){
#ifdef ATLAS_BLIT
B(C2,c);
#else
B(j1,d);
#endif
B(z0,X);uint p2=T2(p4);X E1=i2(p2>>P5);E1=X7(E1);uint H9;
#ifndef ATLAS_BLIT
if(E1==z0){H9=p2;}else
#endif
{H9=(Gb(z0)<<P5)+G9;}d n;
#ifdef ATLAS_BLIT
n=clamp(m2(UC,I9,C2,.0).x,G0(.0),G0(1.));
#else
n=j1;
#endif
int Ie=int(round(n*A9));U2(p4,H9+uint(Ie));i P=B0(.0);
#ifdef ENABLE_CLIPPING
o4 q1=V7;
#endif
#ifndef ATLAS_BLIT
if(E1!=z0)
#endif
{d J9=W7(p2&f8);Y7(E1,J9,P
#ifdef ENABLE_CLIPPING
,q1
#endif
S2 M1);}P.xyz=Q3(P.xyz,S.xy,k.y3,k.z3);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
l1=P;
#else
a8(P M1);
#endif
#ifdef ENABLE_CLIPPING
F9(q1 M1);
#endif
N5}
#endif
#ifdef DRAW_IMAGE
Cb(IB){B(U0,c);
#ifdef DRAW_IMAGE_RECT
B(S4,d);
#endif
#ifdef ENABLE_CLIP_RECT
B(N0,g);
#endif
i w4=g8(AC,R5,U0);d S5=1.;
#ifdef DRAW_IMAGE_RECT
S5=min(S4,S5);
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d U4=g3(Y4(N0));S5=clamp(U4,G0(.0),S5);}
#endif
uint p2=T2(p4);X E1=i2(p2>>P5);E1=X7(E1);d J9=W7(p2&f8);i P;
#ifdef ENABLE_CLIPPING
o4 q1=V7;
#endif
Y7(E1,J9,P
#ifdef ENABLE_CLIPPING
,q1
#endif
S2 M1);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&A0.V0!=0u){o4 O0=ub(q1)?q1:z9(d0);yb(A0.V0,O0,S5);}
#endif
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&defined(ENABLE_ADVANCED_BLEND)
if(ENABLE_ADVANCED_BLEND&&A0.n2!=M5){i L1=H0(g0)*(1.-P.w)+P;w4.xyz=Q4(B6(w4),L1,i2(A0.n2))*w4.w;}
#endif
w4*=S5*m4(A0.x4);
#if defined(NEEDS_GAMMA_CORRECTION)
w4=k3(w4);
#endif
P=P*(1.-w4.w)+w4;P.xyz=Q3(P.xyz,S.xy,k.y3,k.z3);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
l1=P;
#else
a8(P M1);
#endif
#ifdef ENABLE_CLIPPING
F9(q1 M1);
#endif
U2(p4,G9);N5}
#endif
#ifdef INITIALIZE_PLS
I6(IB){
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
#ifdef STORE_COLOR_CLEAR
if(STORE_COLOR_CLEAR){v0(g0,unpackUnorm4x8(k.Je));}
#endif
#ifdef LOAD_COLOR_FROM_DST_TEXTURE
if(LOAD_COLOR_FROM_DST_TEXTURE){v0(g0,v1(AC,E));}
#endif
#ifdef SWIZZLE_COLOR_BGRA_TO_RGBA
i j=H0(g0);v0(g0,j.zyxw);
#endif
#endif
U2(p4,k.Ke);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){f1(d0,0u);}
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
discard;
#endif
N5}
#endif
#ifdef RESOLVE_PLS
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
o2(IB)
#else
I6(IB)
#endif
{uint p2=T2(p4);d m0=W7(p2&f8);X E1=i2(p2>>P5);E1=X7(E1);i P;Y7(E1,m0,P S2 M1);
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
float H6=1.-P.w;if(H6!=.0)P+=H0(g0)*H6;l1=P;l3
#else
P.xyz=Q3(P.xyz,S.xy,k.y3,k.z3);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
l1=P;
#else
a8(P M1);
#endif
N5
#endif
}
#endif
#endif
