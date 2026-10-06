#ifdef FRAGMENT
R1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
B0(K2,n0);
#endif
o1(c3,m0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
sb(o6,B4);
#endif
o1(U6,V0);S1 T1(IB){q(l1,C);d X0=-l1.x;
#ifdef DRAW_INTERIOR_TRIANGLES
q(m1,d);d A0=m1;
#else
q(S,G2);d A0=S.x;
#endif
E2;C T0;d X5,F3;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){F3=A0;}else
#endif
{T0=unpackHalf2x16(h1(m0));X5=T0.y;d f5=X5==X0?T0.x:H0(.0);F3=f5+A0;}
#ifdef ENABLE_NESTED_CLIPPING
d E4=l1.y;if(ENABLE_NESTED_CLIPPING&&E4!=.0){d G4=.0;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){T0=unpackHalf2x16(h1(m0));X5=T0.y;}
#endif
if(X5!=X0){G4=X5==E4?T0.x:.0;j1(V0,packHalf2x16(H2(G4,Hg)));}else{G4=unpackHalf2x16(h1(V0)).x;Z1(V0);}F3=min(F3,G4);}else
#endif
{Z1(V0);}j1(m0,packHalf2x16(H2(F3,X0)));
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
D2(n0);
#endif
F2;h2;}
#endif
