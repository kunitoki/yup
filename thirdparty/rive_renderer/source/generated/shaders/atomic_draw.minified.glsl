#ifdef DRAW_PATH
#ifdef VERTEX
A1(c0)r0(0,g,TB);r0(1,g,UB);B1
#endif
h2
#ifdef ENABLE_FEATHER
J0 d0(0,g,I);
#else
J0 d0(0,D,I);
#endif
S4 d0(1,X,z0);Z1
#ifdef VERTEX
C1(XB,c0,G,v,S){v0(v,G,TB,g);v0(v,G,UB,g);
#ifdef ENABLE_FEATHER
Y(I,g);
#else
Y(I,D);
#endif
Y(z0,X);g O;uint m0;c i0;g J;if(p9(TB,UB,S,m0,i0,J p3)){
#ifdef ENABLE_FEATHER
I=J;
#else
I.xy=R7(J.xy);
#endif
z0=i2(m0);O=H3(i0);}else{O=g(k.O2,k.O2,k.O2,k.O2);}l0(I);l0(z0);D1(O);}
#endif
#endif
#if defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_BLIT)
#ifdef VERTEX
A1(c0)r0(0,I3,JB);B1
#endif
h2
#ifdef ATLAS_BLIT
J0 d0(0,c,C2);
#else
OPTIONALLY_FLAT d0(0,d,l1);
#endif
S4 d0(1,X,z0);Z1
#ifdef VERTEX
C1(XB,c0,G,v,S){v0(v,G,JB,V);
#ifdef ATLAS_BLIT
Y(C2,c);
#else
Y(l1,d);
#endif
Y(z0,X);uint m0;c i0;
#ifdef ATLAS_BLIT
i0=nb(JB,m0,C2 p3);
#else
i0=ob(JB,m0,l1 p3);
#endif
z0=i2(m0);g O=H3(i0);
#ifdef ATLAS_BLIT
l0(C2);
#else
l0(l1);
#endif
l0(z0);D1(O);}
#endif
#endif
#ifdef DRAW_IMAGE_RECT
#ifdef VERTEX
A1(c0)r0(0,g,YB);B1
#endif
h2 J0 d0(0,c,U0);J0 d0(1,d,T4);
#ifdef ENABLE_CLIP_RECT
J0 d0(2,g,N0);
#endif
Z1
#ifdef VERTEX
S7(XB,c0,G,v,S){v0(v,G,YB,g);Y(U0,c);Y(T4,d);
#ifdef ENABLE_CLIP_RECT
Y(N0,g);
#endif
bool q9=YB.z==.0||YB.w==.0;T4=q9?.0:1.;c i0=YB.xy;a0 Y0=j2(A0.r9);a0 D6=transpose(inverse(Y0));if(!q9){float v9=k4*w9(D6[1])/dot(Y0[1],D6[1]);if(v9>=.5){i0.x=.5;T4*=l4(.5/v9);}else{i0.x+=v9*YB.z;}float x9=k4*w9(D6[0])/dot(Y0[0],D6[0]);if(x9>=.5){i0.y=.5;T4*=l4(.5/x9);}else{i0.y+=x9*YB.w;}}U0=i0;i0=Z0(Y0,i0)+A0.a2;if(q9){c J3=Z0(D6,YB.zw);J3*=w9(J3)/dot(J3,J3);i0+=k4*J3;}
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){N0=T7(j2(A0.k2),A0.D2,i0);}
#endif
g O=H3(i0);l0(U0);l0(T4);
#ifdef ENABLE_CLIP_RECT
l0(N0);
#endif
D1(O);}
#endif
#elif defined(DRAW_IMAGE_MESH)
#ifdef VERTEX
A1(e3)r0(0,c,FC);B1 A1(q3)r0(1,c,GC);B1
#endif
h2 J0 d0(0,c,U0);
#ifdef ENABLE_CLIP_RECT
J0 d0(1,g,N0);
#endif
Z1
#ifdef VERTEX
E6(XB,e3,f3,q3,r3,v){v0(v,f3,FC,c);v0(v,r3,GC,c);Y(U0,c);
#ifdef ENABLE_CLIP_RECT
Y(N0,g);
#endif
a0 Y0=j2(A0.r9);c i0=Z0(Y0,FC)+A0.a2;U0=GC;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){N0=T7(j2(A0.k2),A0.D2,i0);}
#endif
g O=H3(i0);l0(U0);
#ifdef ENABLE_CLIP_RECT
l0(N0);
#endif
D1(O);}
#endif
#endif
#ifdef DRAW_RENDER_TARGET_UPDATE_BOUNDS
#ifdef VERTEX
A1(c0)B1
#endif
h2 Z1
#ifdef VERTEX
C1(XB,c0,G,v,S){U l2;l2.x=(v&1)==0?k.U7.x:k.U7.z;l2.y=(v&2)==0?k.U7.y:k.U7.w;g O=H3(c(l2));D1(O);}
#endif
#endif
#ifdef DRAW_IMAGE
#endif
#ifdef FRAGMENT
K1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
#ifdef COLOR_PLANE_IDX_OVERRIDE
#define y9 COLOR_PLANE_IDX_OVERRIDE
#else
#define y9 P2
#endif
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
m4(y9,j0);
#else
p0(y9,j0);
#endif
#endif
#ifdef PLS_BLEND_SRC_OVER
#define n4 i
#define z9 H0
#define V7 B0(.0)
#define pb(q) ((q).w!=.0)
#ifdef ENABLE_CLIPPING
#ifndef RESOLVE_PLS
p0(Q2,e0);
#else
m4(Q2,e0);
#endif
#endif
#else
#define n4 uint
#define V7 0u
#define z9 e1
#define pb(q) ((q)!=0u)
#ifdef ENABLE_CLIPPING
f1(Q2,e0);
#endif
#endif
o4(F6,p4);L1 K3 H5(qb,ve,UC);I5(rb,we,OB);L3 e uint xe(float x){return uint(round(x*A9+B9));}e d W7(uint x){return l4(float(x)*sb+(-B9*sb));}X X7(X m0){
#ifdef NEEDS_PATH_ID_CLAMP_WORKAROUND
m0=min(m0,k.ye);
#endif
return m0;}
#ifdef ENABLE_CLIPPING
e void tb(uint V0,n4 O0,U4(d)n){
#ifdef PLS_BLEND_SRC_OVER
if(all(lessThan(abs(O0.xy-unpackUnorm4x8(V0).xy),A2(.25/255.))))n=min(n,O0.z);else n=.0;
#else
if(V0==O0>>16)n=min(n,unpackHalf2x16(O0).x);else n=.0;
#endif
}
#endif
e void Y7(uint m0,d n0,g1(i)P
#if defined(ENABLE_CLIPPING)&&!defined(RESOLVE_PLS)
,U4(n4)q1
#endif
G6 r4){W0 r1=J5(UC,m0);d n=n0;if((r1.x&(ze|C9))!=0u){n=abs(n);
#ifdef ENABLE_EVEN_ODD
if(ENABLE_EVEN_ODD&&(r1.x&C9)!=0u){n=1.-abs(fract(n*.5)*2.+-1.);}
#endif
}n=clamp(n,G0(.0),G0(1.));
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint V0=r1.x>>16u;if(V0!=0u){tb(V0,z9(e0),n);}}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT&&(r1.x&Ae)!=0u){a0 Y0=j2(P0(OB,m0*4u+2u));g a2=P0(OB,m0*4u+3u);c Be=Z0(Y0,T)+a2.xy;D ub=R7(abs(Be)*a2.zw-a2.zw);d V4=clamp(min(ub.x,ub.y)+.5,.0,1.);n=min(n,V4);}
#endif
uint g3=r1.x&0xfu;if(g3<=vb){P=unpackUnorm4x8(r1.y);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&g3==Z7){
#ifndef RESOLVE_PLS
#ifdef PLS_BLEND_SRC_OVER
q1.xy=P.zw;q1.z=n;q1.w=1.;
#else
q1=r1.y|packHalf2x16(A2(n,.0));
#endif
#endif
P=B0(.0);}
#endif
}else{a0 Y0=j2(P0(OB,m0*4u));g a2=P0(OB,m0*4u+1u);c W4=Z0(Y0,T)+a2.xy;float t=g3==E9?W4.x:length(W4);t=clamp(t,.0,1.);float x=t*a2.z+a2.w;float y=uintBitsToFloat(r1.y);P=m2(DD,wb,c(x,y),.0);}P.w*=n;
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&defined(ENABLE_ADVANCED_BLEND)
X n2;if(ENABLE_ADVANCED_BLEND&&P.w!=.0&&(n2=i2((r1.x>>4)&0xfu))!=K5){i M1=H0(j0);P.xyz=R4(P.xyz,M1,n2);}
#endif
#if defined(NEEDS_GAMMA_CORRECTION)&&(defined(FIXED_FUNCTION_COLOR_OUTPUT)||defined(RESOLVE_PLS))
P=h3(P);
#endif
P.xyz*=P.w;}
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&!defined(COALESCED_PLS_RESOLVE_AND_TRANSFER)
e void a8(i P r4){
#ifndef PLS_BLEND_SRC_OVER
if(P.w==.0)return;float H6=1.-P.w;if(H6!=.0)P+=H0(j0)*H6;
#endif
C0(j0,P);}
#endif
#if defined(ENABLE_CLIPPING)&&!defined(RESOLVE_PLS)
e void F9(n4 q1 r4){
#ifdef PLS_BLEND_SRC_OVER
C0(e0,q1);
#else
if(q1!=0u)h1(e0,q1);
#endif
}
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#define I6 o2
#define xb v4
#define L5 i3
#else
#define I6 v1
#define xb M5
#define L5 c2
#endif
#ifdef DRAW_PATH
I6(HB){
#ifdef ENABLE_FEATHER
B(I,g);
#else
B(I,D);
#endif
B(z0,X);d c8;
#ifdef ENABLE_FEATHER
if(ENABLE_FEATHER&&yb(I)){c8=w4(I i1);}else if(ENABLE_FEATHER&&zb(I)){c8=d8(I i1);}else
#endif
{c8=min(min(G0(I.x),abs(G0(I.y))),G0(1.));}i P=B0(.0);
#ifdef ENABLE_CLIPPING
n4 q1=V7;
#endif
uint e8=xe(c8);uint Ab=(Bb(z0)<<N5)|e8;uint p2=O5(p4,Ab);X E1=i2(p2>>N5);E1=X7(E1);if(E1==z0){if(!P5(I)){e8+=p2-max(Ab,p2);e8-=G9;Q5(p4,e8);}}else{d n0=W7(p2&f8);Y7(E1,n0,P
#ifdef ENABLE_CLIPPING
,q1
#endif
R2 U1);}P.xyz=M3(P.xyz,T.xy,k.v3,k.w3);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
m1=P;
#else
a8(P U1);
#endif
#ifdef ENABLE_CLIPPING
F9(q1 U1);
#endif
L5}
#endif
#if defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_BLIT)
I6(HB){
#ifdef ATLAS_BLIT
B(C2,c);
#else
B(l1,d);
#endif
B(z0,X);uint p2=N3(p4);X E1=i2(p2>>N5);E1=X7(E1);uint H9;
#ifndef ATLAS_BLIT
if(E1==z0){H9=p2;}else
#endif
{H9=(Bb(z0)<<N5)+G9;}d n;
#ifdef ATLAS_BLIT
n=clamp(m2(VC,I9,C2,.0).x,G0(.0),G0(1.));
#else
n=l1;
#endif
int Ce=int(round(n*A9));O3(p4,H9+uint(Ce));i P=B0(.0);
#ifdef ENABLE_CLIPPING
n4 q1=V7;
#endif
#ifndef ATLAS_BLIT
if(E1!=z0)
#endif
{d J9=W7(p2&f8);Y7(E1,J9,P
#ifdef ENABLE_CLIPPING
,q1
#endif
R2 U1);}P.xyz=M3(P.xyz,T.xy,k.v3,k.w3);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
m1=P;
#else
a8(P U1);
#endif
#ifdef ENABLE_CLIPPING
F9(q1 U1);
#endif
L5}
#endif
#ifdef DRAW_IMAGE
xb(HB){B(U0,c);
#ifdef DRAW_IMAGE_RECT
B(T4,d);
#endif
#ifdef ENABLE_CLIP_RECT
B(N0,g);
#endif
i x4=g8(DC,R5,U0);d S5=1.;
#ifdef DRAW_IMAGE_RECT
S5=min(T4,S5);
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d V4=d3(X4(N0));S5=clamp(V4,G0(.0),S5);}
#endif
uint p2=N3(p4);X E1=i2(p2>>N5);E1=X7(E1);d J9=W7(p2&f8);i P;
#ifdef ENABLE_CLIPPING
n4 q1=V7;
#endif
Y7(E1,J9,P
#ifdef ENABLE_CLIPPING
,q1
#endif
R2 U1);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&A0.V0!=0u){n4 O0=pb(q1)?q1:z9(e0);tb(A0.V0,O0,S5);}
#endif
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&defined(ENABLE_ADVANCED_BLEND)
if(ENABLE_ADVANCED_BLEND&&A0.n2!=K5){i M1=H0(j0)*(1.-P.w)+P;x4.xyz=R4(B6(x4),M1,i2(A0.n2))*x4.w;}
#endif
x4*=S5*l4(A0.y4);
#if defined(NEEDS_GAMMA_CORRECTION)
x4=h3(x4);
#endif
P=P*(1.-x4.w)+x4;P.xyz=M3(P.xyz,T.xy,k.v3,k.w3);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
m1=P;
#else
a8(P U1);
#endif
#ifdef ENABLE_CLIPPING
F9(q1 U1);
#endif
O3(p4,G9);L5}
#endif
#ifdef INITIALIZE_PLS
I6(HB){
#ifdef STORE_COLOR_CLEAR
C0(j0,unpackUnorm4x8(k.De));
#endif
#ifdef SWIZZLE_COLOR_BGRA_TO_RGBA
i j=H0(j0);C0(j0,j.zyxw);
#endif
O3(p4,k.Ee);
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){h1(e0,0u);}
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
discard;
#endif
L5}
#endif
#ifdef RESOLVE_PLS
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
o2(HB)
#else
I6(HB)
#endif
{uint p2=N3(p4);d n0=W7(p2&f8);X E1=i2(p2>>N5);E1=X7(E1);i P;Y7(E1,n0,P R2 U1);
#ifdef COALESCED_PLS_RESOLVE_AND_TRANSFER
float H6=1.-P.w;if(H6!=.0)P+=H0(j0)*H6;m1=P;i3
#else
P.xyz=M3(P.xyz,T.xy,k.v3,k.w3);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
m1=P;
#else
a8(P U1);
#endif
L5
#endif
}
#endif
#endif
