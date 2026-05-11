#ifdef FRAGMENT
K1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
p0(P2,j0);
#endif
f1(Q2,e0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Q8(d6,z6);
#endif
f1(F6,S0);L1
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
o2(HB)
#else
v1(HB)
#endif
{B(k1,g);
#ifdef DRAW_INTERIOR_TRIANGLES
B(l1,d);
#else
B(I,z2);
#endif
B(z0,d);
#ifdef ENABLE_CLIPPING
B(S1,D);
#endif
#ifdef ENABLE_CLIP_RECT
B(N0,g);
#endif
#ifdef ENABLE_ADVANCED_BLEND
B(Y1,d);
#endif
d q0=
#ifdef DRAW_INTERIOR_TRIANGLES
l1;
#else
Xa(I);
#endif
i F0;d I1;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(!BORROWED_COVERAGE_PASS)
#endif
{F0=M7(k1,1. R2);I1=1.;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d cb=d3(X4(N0));I1=min(cb,I1);}
#endif
}v2;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){h1(S0,packHalf2x16(A2(q0,z0)));
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r2(j0);
#endif
}else
#endif
{D O4=unpackHalf2x16(e1(S0));d h9=O4.y;d P4=h9==z0?O4.x:G0(.0);d Zd=
#ifndef DRAW_INTERIOR_TRIANGLES
P5(I)?max(P4,q0):
#endif
P4+q0;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&S1.x!=.0){D O0=unpackHalf2x16(e1(e0));d F5=O0.y;d db=F5==S1.x?O0.x:G0(.0);I1=min(db,I1);}
#endif
I1=max(I1,.0);d V1=W9(P4,.0,I1);d H1=W9(Zd,.0,I1);
#ifdef ENABLE_DITHER
d E5;if(ENABLE_DITHER){E5=Z9(T.xy,k.v3,k.w3);}
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
i M1=H0(j0);
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){if(Y1!=V5(K5)&&H1!=.0){if(V1==.0){F0.xyz=R4(F0.xyz,M1,W5(Y1));
#ifndef DRAW_INTERIOR_TRIANGLES
if(H1<I1){r P7=F0.xyz;
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){P7+=E5*k.bd;}
#endif
C0(z6,B0(P7,0.0));}
#endif
}else{F0.xyz=H0(z6).xyz;r2(z6);}}F0.xyz*=F0.w;}
#endif
#endif
F0*=K8(V1,H1,F0.w);
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){F0.xyz+=E5;}
#endif
#ifndef DRAW_INTERIOR_TRIANGLES
#ifdef ENABLE_ADVANCED_BLEND
#define ae (!ENABLE_ADVANCED_BLEND||Y1==V5(K5))&&F0.w>=1.
#else
#define ae F0.w>=1.
#endif
od(ae,S0,packHalf2x16(A2(Zd,z0)));
#else
X1(S0);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
nd(F0.w==.0,j0,M1*(1.-F0.w)+F0);
#endif
}X1(e0);w2;
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
m1=F0;i3
#else
c2;
#endif
}
#endif
