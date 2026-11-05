#ifdef FRAGMENT
I1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
z0(B2,q0);
#endif
x1(o3,o0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
sc(F6,T4);
#endif
x1(o7,d1);J1 E1(HB){q(m2,F);d D1=-m2.x;
#ifdef DRAW_INTERIOR_TRIANGLES
q(v1,d);d F0=v1;
#else
q(S,U2);d F0=S.x;
#endif
R2;F Z0;d m6,R3;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){R3=F0;}else
#endif
{Z0=unpackHalf2x16(o1(o0));m6=Z0.y;d C5=m6==D1?Z0.x:B0(.0);R3=C5+F0;}
#ifdef ENABLE_NESTED_CLIPPING
d k6=m2.y;if(ENABLE_NESTED_CLIPPING&&k6!=.0){d X4=.0;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){Z0=unpackHalf2x16(o1(o0));m6=Z0.y;}
#endif
if(m6!=D1){X4=m6==k6?Z0.x:.0;q1(d1,packHalf2x16(V2(X4,Oh)));}else{X4=unpackHalf2x16(o1(d1)).x;l2(d1);}R3=min(R3,X4);}else
#endif
{l2(d1);}q1(o0,packHalf2x16(V2(R3,D1)));
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Q2(q0);
#endif
S2;L1;}
#endif
