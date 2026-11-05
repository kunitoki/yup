#ifdef FRAGMENT
#if(defined(FIXED_FUNCTION_COLOR_OUTPUT)&&!defined(ENABLE_CLIPPING))||defined(RENDER_MODE_CLOCKWISE_ATOMIC)
#undef fd
#else
#define fd
#endif
I1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
z0(B2,q0);
#endif
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
x1(o3,o0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
z0(F6,T4);
#endif
x1(o7,d1);
#else
z0(o3,o0);
#endif
J1
#ifdef DRAW_IMAGE_MESH
X3 Z2(J5,C4,SB);Y3 K5 D4(x4) L5 n4 p4
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#ifdef DRAW_IMAGE_MESH
K2(HB)
#else
K2(HB)
#endif
#else
#ifdef DRAW_IMAGE_MESH
E1(HB)
#else
E1(HB)
#endif
#endif
{
#ifdef FEATHER_ATLAS_BLIT
q(N0,e);
#if defined(ENABLE_MODULATED_IMAGE)
q(O0,M);
#endif
q(X2,c);
#endif
#ifdef ENABLE_CLIPPING
q(j4,d);
#endif
#ifdef ENABLE_CLIP_RECT
q(Y0,e);
#endif
#if defined(FEATHER_ATLAS_BLIT)&&defined(ENABLE_ADVANCED_BLEND)
q(S0,d);
#endif
#ifdef DRAW_IMAGE_MESH
q(j6,c);q(a2,i);
#ifdef ENABLE_ADVANCED_BLEND
q(Q1,O);
#endif
#endif
#ifdef FEATHER_ATLAS_BLIT
i k=G8(
#ifdef ENABLE_MODULATED_IMAGE
O0,
#endif
#ifdef ENABLE_ADVANCED_BLEND
a3(S0),
#endif
N0 q3);d n=clamp(r2(ID,Za,X2,.0).x,B0(.0),B0(1.));
#endif
#ifdef DRAW_IMAGE_MESH
i k=m8(SB,x4,j6,j.bf);d n=1.;
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d G5=max(E3(y4(Y0)),B0(.0));n=min(G5,n);}
#endif
#ifdef fd
R2;
#endif
#if defined(ENABLE_CLIPPING)
if(ENABLE_CLIPPING&&j4!=.0){d R3;
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
F Z0=unpackHalf2x16(o1(o0));d k7=Z0.y;R3=max(k7==j4?Z0.x:B0(.0),B0(.0));
#else
R3=L0(o0).x;
#endif
R3=max(R3,B0(.0));n=min(n,R3);}
#endif
#ifdef DRAW_IMAGE_MESH
k*=a2;
#endif
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)
i X0=L0(q0);
#ifdef ENABLE_ADVANCED_BLEND
#ifdef FEATHER_ATLAS_BLIT
O c2=a3(S0);
#endif
#ifdef DRAW_IMAGE_MESH
O c2=Q1;
#endif
if(ENABLE_ADVANCED_BLEND&&c2!=W3){
#ifdef DRAW_IMAGE_MESH
k.xyz=o6(k);
#endif
k.xyz=Y4(k.xyz,X0,c2)*k.w;}
#endif
k*=n;k.xyz=M2(k.xyz,k.w,Y.xy,j.J3,j.K3);
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
k=X0*(1.-k.w)+k;
#endif
y0(q0,k);
#endif
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
l2(o0);l2(d1);
#else
y0(o0,A0(.0));
#endif
#ifdef fd
S2;
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
k=(k*n);k.xyz=M2(k.xyz,k.w,Y.xy,j.J3,j.K3);S1=k;I3
#else
L1;
#endif
}
#endif
