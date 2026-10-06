#ifdef FRAGMENT
R1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
B0(K2,n0);
#endif
o1(c3,m0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
sb(o6,O6);
#endif
o1(U6,V0);S1
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
z2(IB)
#else
T1(IB)
#endif
{q(a1,e);
#ifdef ENABLE_MODULATED_IMAGE
q(r1,O);
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
q(m1,d);
#else
q(S,G2);
#endif
q(F0,d);
#ifdef ENABLE_CLIPPING
q(l1,C);
#endif
#ifdef ENABLE_CLIP_RECT
q(R0,e);
#endif
#ifdef ENABLE_ADVANCED_BLEND
q(Q0,d);
#endif
d A0=
#ifdef DRAW_INTERIOR_TRIANGLES
m1;
#else
Ub(S);
#endif
i o0;d N1;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(!BORROWED_COVERAGE_PASS)
#endif
{o0=Y7(
#ifdef ENABLE_MODULATED_IMAGE
r1,
#endif
#ifdef ENABLE_ADVANCED_BLEND
k3(Q0),
#endif
a1 e3);N1=1.;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d Zb=v3(v5(R0));N1=min(Zb,N1);}
#endif
}E2;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){j1(V0,packHalf2x16(H2(A0,F0)));
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
D2(n0);
#endif
}else
#endif
{C d5=unpackHalf2x16(h1(V0));d D9=d5.y;d f5=D9==F0?d5.x:H0(.0);d Ze=
#ifndef DRAW_INTERIOR_TRIANGLES
f6(S)?max(f5,A0):
#endif
f5+A0;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&l1.x!=.0){C T0=unpackHalf2x16(h1(m0));d X5=T0.y;d ac=X5==l1.x?T0.x:H0(.0);N1=min(ac,N1);}
#endif
N1=max(N1,.0);d i2=Aa(f5,.0,N1);d M1=Aa(Ze,.0,N1);
#ifdef ENABLE_DITHER
d W5;if(ENABLE_DITHER){W5=Da(f0.xy,j.L3,j.M3);}
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
i I1=N0(n0);
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND&&Q0!=j6(L4)){if(M1!=.0){if(i2==.0){o0.xyz=h5(o0.xyz,I1,k3(Q0));
#ifndef DRAW_INTERIOR_TRIANGLES
if(M1<N1){v e8=o0.xyz;
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){e8+=W5*j.Wd;}
#endif
y0(O6,I0(e8,0.0));}
#endif
}else{o0.xyz=N0(O6).xyz;D2(O6);}}o0.xyz*=o0.w;}
#endif
#endif
o0*=c9(i2,M1,o0.w);
#ifdef ENABLE_DITHER
o0.xyz=M2(o0.xyz,o0.w,W5);
#endif
#ifndef DRAW_INTERIOR_TRIANGLES
#ifdef ENABLE_ADVANCED_BLEND
#define af (!ENABLE_ADVANCED_BLEND||Q0==j6(L4))&&o0.w>=1.
#else
#define af o0.w>=1.
#endif
je(af,V0,packHalf2x16(H2(Ze,F0)));
#else
Z1(V0);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
ie(o0.x+o0.y+o0.z+o0.w==.0,n0,I1*(1.-o0.w)+o0);
#endif
}Z1(m0);F2;
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
K1=o0;z3
#else
h2;
#endif
}
#endif
