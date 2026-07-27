#ifdef FRAGMENT
J1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r0(Q2,g0);
#endif
k1(R2,d0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Ka(d6,g4);
#endif
k1(F6,S0);K1 m1(IB){B(S1,D);d V0=-S1.x;
#ifdef DRAW_INTERIOR_TRIANGLES
B(j1,d);d o0=j1;
#else
B(I,z2);d o0=I.x;
#endif
v2;D O0;d H5,r3;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){r3=o0;}else
#endif
{O0=unpackHalf2x16(d1(d0));H5=O0.y;d O4=H5==V0?O0.x:G0(.0);r3=O4+o0;}
#ifdef ENABLE_NESTED_CLIPPING
d F5=S1.y;if(ENABLE_NESTED_CLIPPING&&F5!=.0){d k4=.0;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){O0=unpackHalf2x16(d1(d0));H5=O0.y;}
#endif
if(H5!=V0){k4=H5==F5?O0.x:.0;f1(S0,packHalf2x16(A2(k4,vf)));}else{k4=unpackHalf2x16(d1(S0)).x;Y1(S0);}r3=min(r3,k4);}else
#endif
{Y1(S0);}f1(d0,packHalf2x16(A2(r3,V0)));
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r2(g0);
#endif
w2;U1;}
#endif
