#ifdef FRAGMENT
I1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
z0(B2,q0);
#endif
x1(o3,o0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
sc(F6,j7);
#endif
x1(o7,d1);J1
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
K2(HB)
#else
E1(HB)
#endif
{q(N0,e);
#ifdef ENABLE_MODULATED_IMAGE
q(O0,M);
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
q(v1,d);
#else
q(S,U2);
#endif
q(J0,d);
#ifdef ENABLE_CLIPPING
q(m2,F);
#endif
#ifdef ENABLE_CLIP_RECT
q(Y0,e);
#endif
#ifdef ENABLE_ADVANCED_BLEND
q(S0,d);
#endif
d F0=
#ifdef DRAW_INTERIOR_TRIANGLES
v1;
#else
Xc(S);
#endif
i r0;d X1;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(!BORROWED_COVERAGE_PASS)
#endif
{r0=G8(
#ifdef ENABLE_MODULATED_IMAGE
O0,
#endif
#ifdef ENABLE_ADVANCED_BLEND
a3(S0),
#endif
N0 q3);X1=1.;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d cd=E3(y4(Y0));X1=min(cd,X1);}
#endif
}R2;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){q1(d1,packHalf2x16(V2(F0,J0)));
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Q2(q0);
#endif
}else
#endif
{F B5=unpackHalf2x16(o1(d1));d qa=B5.y;d C5=qa==J0?B5.x:B0(.0);d bg=
#ifndef DRAW_INTERIOR_TRIANGLES
w6(S)?max(C5,F0):
#endif
C5+F0;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&m2.x!=.0){F Z0=unpackHalf2x16(o1(o0));d m6=Z0.y;d dd=m6==m2.x?Z0.x:B0(.0);X1=min(dd,X1);}
#endif
X1=max(X1,.0);d y2=pb(C5,.0,X1);d U1=pb(bg,.0,X1);
#ifdef ENABLE_DITHER
d l6;if(ENABLE_DITHER){l6=sb(Y.xy,j.J3,j.K3);}
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
i X0=L0(q0);
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND&&S0!=z6(W3)){if(U1!=.0){if(y2==.0){r0.xyz=Y4(r0.xyz,X0,a3(S0));
#ifndef DRAW_INTERIOR_TRIANGLES
if(U1<X1){v L8=r0.xyz;
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){L8+=l6*j.cf;}
#endif
y0(j7,A0(L8,0.0));}
#endif
}else{r0.xyz=L0(j7).xyz;Q2(j7);}}r0.xyz*=r0.w;}
#endif
#endif
r0*=R9(y2,U1,r0.w);
#ifdef ENABLE_DITHER
r0.xyz=M2(r0.xyz,r0.w,l6);
#endif
#ifndef DRAW_INTERIOR_TRIANGLES
#ifdef ENABLE_ADVANCED_BLEND
#define cg (!ENABLE_ADVANCED_BLEND||S0==z6(W3))&&r0.w>=1.
#else
#define cg r0.w>=1.
#endif
of(cg,d1,packHalf2x16(V2(bg,J0)));
#else
l2(d1);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
nf(r0.x+r0.y+r0.z+r0.w==.0,q0,X0*(1.-r0.w)+r0);
#endif
}l2(o0);S2;
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
S1=r0;I3
#else
L1;
#endif
}
#endif
