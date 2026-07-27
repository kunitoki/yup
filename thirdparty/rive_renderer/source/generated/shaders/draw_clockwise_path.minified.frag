#ifdef FRAGMENT
J1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r0(Q2,g0);
#endif
k1(R2,d0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Ka(d6,z6);
#endif
k1(F6,S0);K1
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
o2(IB)
#else
m1(IB)
#endif
{B(i1,g);
#ifdef DRAW_INTERIOR_TRIANGLES
B(j1,d);
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
B(Z1,d);
#endif
d o0=
#ifdef DRAW_INTERIOR_TRIANGLES
j1;
#else
cb(I);
#endif
i F0;d H1;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(!BORROWED_COVERAGE_PASS)
#endif
{F0=M7(i1,1. S2);H1=1.;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d hb=g3(Y4(N0));H1=min(hb,H1);}
#endif
}v2;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){f1(S0,packHalf2x16(A2(o0,z0)));
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r2(g0);
#endif
}else
#endif
{D N4=unpackHalf2x16(d1(S0));d h9=N4.y;d O4=h9==z0?N4.x:G0(.0);d ee=
#ifndef DRAW_INTERIOR_TRIANGLES
Q5(I)?max(O4,o0):
#endif
O4+o0;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&S1.x!=.0){D O0=unpackHalf2x16(d1(d0));d H5=O0.y;d ib=H5==S1.x?O0.x:G0(.0);H1=min(ib,H1);}
#endif
H1=max(H1,.0);d V1=W9(O4,.0,H1);d G1=W9(ee,.0,H1);
#ifdef ENABLE_DITHER
d G5;if(ENABLE_DITHER){G5=Z9(S.xy,k.y3,k.z3);}
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
i L1=H0(g0);
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){if(Z1!=V5(M5)&&G1!=.0){if(V1==.0){F0.xyz=Q4(F0.xyz,L1,W5(Z1));
#ifndef DRAW_INTERIOR_TRIANGLES
if(G1<H1){r P7=F0.xyz;
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){P7+=G5*k.gd;}
#endif
v0(z6,B0(P7,0.0));}
#endif
}else{F0.xyz=H0(z6).xyz;r2(z6);}}F0.xyz*=F0.w;}
#endif
#endif
F0*=K8(V1,G1,F0.w);
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){F0.xyz+=G5;}
#endif
#ifndef DRAW_INTERIOR_TRIANGLES
#ifdef ENABLE_ADVANCED_BLEND
#define fe (!ENABLE_ADVANCED_BLEND||Z1==V5(M5))&&F0.w>=1.
#else
#define fe F0.w>=1.
#endif
td(fe,S0,packHalf2x16(A2(ee,z0)));
#else
Y1(S0);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
sd(F0.w==.0,g0,L1*(1.-F0.w)+F0);
#endif
}Y1(d0);w2;
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
l1=F0;l3
#else
U1;
#endif
}
#endif
