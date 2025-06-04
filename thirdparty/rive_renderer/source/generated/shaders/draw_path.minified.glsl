#ifdef ENABLE_ADVANCED_BLEND
#define o6 !ENABLE_ADVANCED_BLEND
#else
#define o6 true
#endif
#ifdef VERTEX
U0(f0)
#ifdef DRAW_INTERIOR_TRIANGLES
i0(0,a4,IB);
#else
i0(0,f,LB);i0(1,f,MB);
#endif
V0
#endif
o1 n0 H(0,f,H1);
#ifdef ATLAS_BLIT
n0 H(1,c,Q0);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
OPTIONALLY_FLAT H(1,g,i1);
#elif defined(ENABLE_FEATHER)
n0 H(2,f,D);
#else
n0 H(2,G,D);
#endif
OPTIONALLY_FLAT H(3,g,j0);
#endif
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT H(4,G,i3);
#endif
#ifdef ENABLE_CLIP_RECT
n0 H(5,f,R0);
#endif
#ifdef ENABLE_ADVANCED_BLEND
OPTIONALLY_FLAT H(6,g,U3);
#endif
p1
#ifdef VERTEX
q1(PB,f0,B,n,K){
#ifdef DRAW_INTERIOR_TRIANGLES
l0(n,B,IB,Z);
#else
l0(n,B,LB,f);l0(n,B,MB,f);
#endif
L(H1,f);
#ifdef ATLAS_BLIT
L(Q0,c);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
L(i1,g);
#elif defined(ENABLE_FEATHER)
L(D,f);
#else
L(D,G);
#endif
L(j0,g);
#endif
#ifdef ENABLE_CLIPPING
L(i3,G);
#endif
#ifdef ENABLE_CLIP_RECT
L(R0,f);
#endif
#ifdef ENABLE_ADVANCED_BLEND
L(U3,g);
#endif
bool Eb=false;uint R;c J;
#ifdef RENDER_MODE_MSAA
a0 z7;
#endif
#ifdef ATLAS_BLIT
J=a8(IB,R,
#ifdef RENDER_MODE_MSAA
z7,
#endif
Q0 Y1);
#elif defined(DRAW_INTERIOR_TRIANGLES)
J=c8(IB,R
#ifdef RENDER_MODE_MSAA
,z7
#else
,i1
#endif
Y1);
#else
f O;Eb=!E6(LB,MB,K,R,J
#ifndef RENDER_MODE_MSAA
,O
#else
,z7
#endif
Y1);
#ifndef RENDER_MODE_MSAA
#ifdef ENABLE_FEATHER
D=O;
#else
D.xy=F6(O.xy);
#endif
#endif
#endif
N0 F0=k4(DC,R);
#if!defined(ATLAS_BLIT)&&!defined(RENDER_MODE_MSAA)
j0=g7(R,q.d5);if((F0.x&p8)!=0u)j0=-j0;
#endif
uint J1=F0.x&0xfu;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint te=(J1==N6?F0.y:F0.x)>>16;g Z0=g7(te,q.d5);if(J1==N6)Z0=-Z0;i3.x=Z0;}
#endif
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){U3=float((F0.x>>4)&0xfu);}
#endif
c p6=J;
#ifdef FRAMEBUFFER_BOTTOM_UP
p6.y=float(q.id)-p6.y;
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){S R1=D1(w0(KB,R*4u+2u));f Z1=w0(KB,R*4u+3u);
#ifndef RENDER_MODE_MSAA
R0=I6(R1,Z1.xy,p6);
#else
Ka(R1,Z1.xy,p6);
#endif
}
#endif
if(J1==q8){i j=unpackUnorm4x8(F0.y);if(o6)j.xyz*=j.w;H1=f(j);}
#ifdef ENABLE_CLIPPING
else if(ENABLE_CLIPPING&&J1==N6){g A7=g7(F0.x>>16,q.d5);i3.y=A7;}
#endif
else{S ue=D1(w0(KB,R*4u));f B7=w0(KB,R*4u+1u);c G2=C0(ue,p6)+B7.xy;if(J1==O6||J1==td){H1.w=-uintBitsToFloat(F0.y);float ve=B7.z;if(ve>.9){H1.z=2.;}else{H1.z=B7.w;}if(J1==O6){H1.y=.0;H1.x=G2.x;}else{H1.z=-H1.z;H1.xy=G2.xy;}}else{float H2=uintBitsToFloat(F0.y);float d6=B7.z;H1=f(G2.x,G2.y,H2,-2.-d6);}}f Q;if(!Eb){Q=F2(J);
#ifdef POST_INVERT_Y
Q.y=-Q.y;
#endif
#ifdef RENDER_MODE_MSAA
Q.z=S8(z7);
#endif
}else{Q=f(q.C1,q.C1,q.C1,q.C1);}P(H1);
#ifdef ATLAS_BLIT
P(Q0);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
P(i1);
#elif defined(ENABLE_FEATHER)
P(D);
#else
P(D);
#endif
P(j0);
#endif
#ifdef ENABLE_CLIPPING
P(i3);
#endif
#ifdef ENABLE_CLIP_RECT
P(R0);
#endif
#ifdef ENABLE_ADVANCED_BLEND
P(U3);
#endif
h1(Q);}
#endif
#ifdef FRAGMENT
w3 x3 d i Fb(f J2,float E C5){i j;if(J2.w>=.0){j=I5(J2);if(o6)j*=E;else j.w*=E;}else if(J2.w>-1.){float t=J2.z>.0?J2.x:length(J2.xy);t=clamp(t,.0,1.);float Gb=abs(J2.z);float x=Gb>1.?(1.-1./T8)*t+(.5/T8):(1./T8)*t+Gb;float we=-J2.w;j=T1(MC,r8,c(x,we),.0);j.w*=E;if(o6)j.xyz*=j.w;}else{g d6=-J2.w-2.;j=C7(UB,B3,J2.xy,d6);g H2=J2.z*E;if(o6)j*=H2;else j=E1(Y3(j),j.w*H2);}return j;}
#ifndef RENDER_MODE_MSAA
x2 M0(i8,H0);Y0(B5,r1);M0(bb,N3);Y0(k8,x4);y2 z2(NB){N(H1,f);
#ifdef ATLAS_BLIT
N(Q0,c);
#elif!defined(RENDER_MODE_MSAA)
#ifdef DRAW_INTERIOR_TRIANGLES
N(i1,g);
#elif defined(ENABLE_FEATHER)
N(D,f);
#else
N(D,G);
#endif
N(j0,g);
#endif
#ifdef ENABLE_CLIPPING
N(i3,G);
#endif
#ifdef ENABLE_CLIP_RECT
N(R0,f);
#endif
#ifdef ENABLE_ADVANCED_BLEND
N(U3,g);
#endif
#if!defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_BLIT)
h2;
#endif
g E;
#ifdef ATLAS_BLIT
E=W6(Q0,q.U4 x1);
#else
G L3=unpackHalf2x16(j1(x4));g Hb=L3.y;g F1=Hb==j0?L3.x:v1(.0);
#ifdef DRAW_INTERIOR_TRIANGLES
F1+=i1;i2(x4);
#else
if(U6(D)){g r0;
#ifdef ENABLE_FEATHER
if(ENABLE_FEATHER&&w8(D)){r0=R6(D x1);}else
#endif
{r0=min(D.x,D.y);}F1=max(r0,F1);}else{g r0;
#if defined(ENABLE_FEATHER)
if(ENABLE_FEATHER&&S6(D)){r0=S4(D x1);}else
#endif
{r0=D.x;}F1+=r0;}l1(x4,packHalf2x16(Z3(F1,j0)));
#endif
#ifdef CLOCKWISE_FILL
if(CLOCKWISE_FILL){
#ifdef VULKAN_VENDOR_ID
if(VULKAN_VENDOR_ID==yd){if(F1<.0)E=.0;else if(F1<=1.)E=F1;else E=1.;}else
#endif
{E=clamp(F1,v1(.0),v1(1.));}}else
#endif
{E=abs(F1);
#ifdef ENABLE_EVEN_ODD
if(ENABLE_EVEN_ODD&&j0<.0){E=1.-v1(abs(fract(E*.5)*2.+-1.));}
#endif
E=min(E,v1(1.));}
#endif
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&i3.x<.0){g Z0=-i3.x;
#ifdef ENABLE_NESTED_CLIPPING
if(ENABLE_NESTED_CLIPPING){g A7=i3.y;if(A7!=.0){G I1=unpackHalf2x16(j1(r1));g k5=I1.y;g D7;if(k5!=Z0){D7=k5==A7?I1.x:.0;
#ifndef DRAW_INTERIOR_TRIANGLES
T0(N3,E1(D7,.0,.0,.0));
#endif
}else{D7=I0(N3).x;
#ifndef DRAW_INTERIOR_TRIANGLES
E2(N3);
#endif
}E=min(E,D7);}}
#endif
l1(r1,packHalf2x16(Z3(E,Z0)));E2(H0);}else
#endif
{
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){g Z0=i3.x;if(Z0!=.0){G I1=unpackHalf2x16(j1(r1));g k5=I1.y;E=(k5==Z0)?min(I1.x,E):v1(.0);}}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){g l4=A8(I5(R0));E=clamp(l4,v1(.0),E);}
#endif
i j=Fb(H1,E Y2);i w1;
#ifdef ATLAS_BLIT
w1=I0(H0);
#else
if(Hb!=j0){w1=I0(H0);
#ifndef DRAW_INTERIOR_TRIANGLES
T0(N3,w1);
#endif
}else{w1=I0(N3);
#ifndef DRAW_INTERIOR_TRIANGLES
E2(N3);
#endif
}
#endif
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){if(U3!=f7(B8)){j.xyz=M4(j.xyz,w1,O8(U3));}j.xyz*=j.w;}
#endif
j+=w1*(1.-j.w);T0(H0,j);i2(r1);}
#if!defined(DRAW_INTERIOR_TRIANGLES)||defined(ATLAS_BLIT)
j2;
#endif
M2;}
#else
e2(i,NB){N(H1,f);
#ifdef ATLAS_BLIT
N(Q0,c);
#endif
#ifdef ENABLE_ADVANCED_BLEND
N(U3,g);
#endif
g E=
#ifdef ATLAS_BLIT
W6(Q0,q.U4 x1);
#else
1.;
#endif
i j=Fb(H1,E Y2);
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){i w1=d1(PC,c0(floor(y0.xy)));j.xyz=M4(j.xyz,w1,O8(U3));j.xyz*=j.w;}
#endif
f2(j);}
#endif
#endif
