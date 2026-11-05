#ifdef FRAGMENT
#ifdef DRAW_IMAGE_MESH
X3 Z2(J5,C4,SB);
#if defined(ENABLE_ADVANCED_BLEND)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)&&!defined(RENDER_TO_UAV)
K4(LD);
#endif
Y3
#ifdef RENDER_TO_UAV
I1 L4(B2,LC);J1
#endif
K5 D4(x4) L5
#endif
#ifdef RENDER_TO_UAV
E1(HB)
#else
v2(i,HB)
#endif
{
#ifdef DRAW_IMAGE_MESH
q(j6,c);q(a2,i);
#ifdef ENABLE_ADVANCED_BLEND
q(Q1,O);
#endif
#else
q(N0,e);
#ifdef ENABLE_MODULATED_IMAGE
q(O0,M);
#endif
#ifdef FEATHER_ATLAS_BLIT
q(X2,c);
#endif
#ifdef ENABLE_ADVANCED_BLEND
q(S0,d);
#endif
#endif
#ifdef DRAW_IMAGE_MESH
i k=m8(SB,x4,j6,j.bf)*a2;
#else
d n=
#ifdef FEATHER_ATLAS_BLIT
clamp(r2(ID,Za,X2,.0).x,B0(.0),B0(1.));
#else
1.;
#endif
i k=G8(
#ifdef ENABLE_MODULATED_IMAGE
O0,
#endif
#ifdef ENABLE_ADVANCED_BLEND
a3(S0),
#endif
N0 q3);
#endif
#if defined(ENABLE_ADVANCED_BLEND)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
#ifdef DRAW_IMAGE_MESH
k.xyz=o6(k);O c2=Q1;
#else
O c2=a3(S0);
#endif
#ifdef RENDER_TO_UAV
i X0=e4(LC);
#else
i X0=J4(LD);
#endif
k.xyz=Y4(k.xyz,X0,c2)*k.w;
#elif defined(RENDER_TO_UAV)
i X0=e4(LC);
#endif
#ifndef DRAW_IMAGE_MESH
k*=n;
#endif
k.xyz=M2(k.xyz,k.w,Y.xy,j.J3,j.K3);
#ifdef RENDER_TO_UAV
if(any(notEqual(k,A0(.0)))){r5(LC,Va(k,X0));}
#endif
#ifdef RENDER_TO_UAV
L1
#else
w2(k);
#endif
}
#endif
