#ifdef FRAGMENT
#if(defined(FIXED_FUNCTION_COLOR_OUTPUT)&&!defined(ENABLE_CLIPPING))||defined(RENDER_MODE_CLOCKWISE_ATOMIC)
#undef cc
#else
#define cc
#endif
R1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
B0(K2,n0);
#endif
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
o1(c3,m0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
B0(o6,B4);
#endif
o1(U6,V0);
#else
B0(c3,m0);
#endif
S1
#ifdef DRAW_IMAGE_MESH
N3 i3(w5,l4,CC);O3 x5 m4(r5) y5 f4 g4
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#ifdef DRAW_IMAGE_MESH
z2(IB)
#else
z2(IB)
#endif
#else
#ifdef DRAW_IMAGE_MESH
T1(IB)
#else
T1(IB)
#endif
#endif
{
#ifdef FEATHER_ATLAS_BLIT
q(a1,e);
#if defined(ENABLE_MODULATED_IMAGE)
q(r1,O);
#endif
q(J2,c);
#endif
#ifdef ENABLE_CLIPPING
q(Y3,d);
#endif
#ifdef ENABLE_CLIP_RECT
q(R0,e);
#endif
#if defined(FEATHER_ATLAS_BLIT)&&defined(ENABLE_ADVANCED_BLEND)
q(Q0,d);
#endif
#ifdef DRAW_IMAGE_MESH
q(V5,c);q(Q1,i);
#ifdef ENABLE_ADVANCED_BLEND
q(H1,Q);
#endif
#endif
#ifdef FEATHER_ATLAS_BLIT
i p=Y7(
#ifdef ENABLE_MODULATED_IMAGE
r1,
#endif
#ifdef ENABLE_ADVANCED_BLEND
k3(Q0),
#endif
a1 e3);d n=clamp(o2(FD,ma,J2,.0).x,H0(.0),H0(1.));
#endif
#ifdef DRAW_IMAGE_MESH
i p=K7(CC,r5,V5,j.Vd);d n=1.;
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d m5=max(v3(v5(R0)),H0(.0));n=min(m5,n);}
#endif
#ifdef cc
E2;
#endif
#if defined(ENABLE_CLIPPING)
if(ENABLE_CLIPPING&&Y3!=.0){d F3;
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
C T0=unpackHalf2x16(h1(m0));d P6=T0.y;F3=max(P6==Y3?T0.x:H0(.0),H0(.0));
#else
F3=N0(m0).x;
#endif
F3=max(F3,H0(.0));n=min(n,F3);}
#endif
#ifdef DRAW_IMAGE_MESH
p*=Q1;
#endif
#if!defined(FIXED_FUNCTION_COLOR_OUTPUT)
i I1=N0(n0);
#ifdef ENABLE_ADVANCED_BLEND
#ifdef FEATHER_ATLAS_BLIT
Q y3=k3(Q0);
#endif
#ifdef DRAW_IMAGE_MESH
Q y3=H1;
#endif
if(ENABLE_ADVANCED_BLEND&&y3!=L4){
#ifdef DRAW_IMAGE_MESH
p.xyz=Q6(p);
#endif
p.xyz=h5(p.xyz,I1,y3)*p.w;}
#endif
p*=n;p.xyz=M2(p.xyz,p.w,f0.xy,j.L3,j.M3);
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
p=I1*(1.-p.w)+p;
#endif
y0(n0,p);
#endif
#ifndef RENDER_MODE_CLOCKWISE_ATOMIC
Z1(m0);Z1(V0);
#else
y0(m0,I0(.0));
#endif
#ifdef cc
F2;
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
p=(p*n);p.xyz=M2(p.xyz,p.w,f0.xy,j.L3,j.M3);K1=p;z3
#else
h2;
#endif
}
#endif
