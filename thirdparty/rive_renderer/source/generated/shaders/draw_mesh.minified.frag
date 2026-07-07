#ifdef FRAGMENT
#if(defined(FIXED_FUNCTION_COLOR_OUTPUT)&&!defined(ENABLE_CLIPPING))||defined(RENDER_MODE_CLOCKWISE_ATOMIC)
#undef kb
#else
#define kb
#endif
J1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r0(Q2,g0);
#endif
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
k1(R2,d0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r0(d6,g4);
#endif
k1(F6,S0);
#else
r0(R2,d0);
#endif
K1
#ifdef DRAW_IMAGE_MESH
B3 X2(Z4,T3,AC);C3 a5 U3(R5)c5 N3 O3
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#ifdef DRAW_IMAGE_MESH
r4(IB)
#else
o2(IB)
#endif
#else
#ifdef DRAW_IMAGE_MESH
O5(IB)
#else
m1(IB)
#endif
#endif
{
#ifdef ATLAS_BLIT
B(i1,g);B(C2,c);
#endif
#ifdef DRAW_IMAGE_MESH
B(U0,c);
#endif
#ifdef ENABLE_CLIPPING
B(I3,d);
#endif
#ifdef ENABLE_CLIP_RECT
B(N0,g);
#endif
#if defined(ATLAS_BLIT)&&defined(ENABLE_ADVANCED_BLEND)
B(Z1,d);
#endif
#ifdef ATLAS_BLIT
i j=M7(i1,1. S2);d n=clamp(m2(UC,I9,C2,.0).x,G0(.0),G0(1.));
#endif
#ifdef DRAW_IMAGE_MESH
i j=y7(AC,R5,U0,k.fd);d n=1.;
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d U4=max(g3(Y4(N0)),G0(.0));n=min(U4,n);}
#endif
#ifdef kb
v2;
#endif
#if defined(ENABLE_CLIPPING)
if(ENABLE_CLIPPING&&I3!=.0){d r3;
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
D O0=unpackHalf2x16(d1(d0));d A6=O0.y;r3=max(A6==I3?O0.x:G0(.0),G0(.0));
#else
r3=H0(d0).x;
#endif
r3=max(r3,G0(.0));n=min(n,r3);}
#endif
#ifdef DRAW_IMAGE_MESH
n*=A0.x4;
#endif
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)
i L1=H0(g0);
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){
#ifdef ATLAS_BLIT
X n2=W5(Z1);
#endif
#ifdef DRAW_IMAGE_MESH
j.xyz=B6(j);X n2=i2(A0.n2);
#endif
if(n2!=M5){j.xyz=Q4(j.xyz,L1,n2);}j.w*=n;j.xyz*=j.w;}else
#endif
{j*=n;}
#ifdef NEEDS_GAMMA_CORRECTION
if(NEEDS_GAMMA_CORRECTION){j=k3(j);}
#endif
j.xyz=Q3(j.xyz,S.xy,k.y3,k.z3);
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
j=L1*(1.-j.w)+j;
#endif
v0(g0,j);
#endif
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
Y1(d0);Y1(S0);
#else
v0(d0,B0(.0));
#endif
#ifdef kb
w2;
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
j=(j*n);j.xyz=Q3(j.xyz,S.xy,k.y3,k.z3);l1=j;l3
#else
U1;
#endif
}
#endif
