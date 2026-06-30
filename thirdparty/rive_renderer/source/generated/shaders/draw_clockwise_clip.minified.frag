#ifdef FRAGMENT
K1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
p0(P2,j0);
#endif
f1(Q2,e0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Q8(d6,f4);
#endif
f1(F6,S0);L1 v1(HB){B(S1,D);d V0=-S1.x;
#ifdef DRAW_INTERIOR_TRIANGLES
B(l1,d);d q0=l1;
#else
B(I,z2);d q0=I.x;
#endif
v2;D O0;d F5,o3;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){o3=q0;}else
#endif
{O0=unpackHalf2x16(e1(e0));F5=O0.y;d P4=F5==V0?O0.x:G0(.0);o3=P4+q0;}
#ifdef ENABLE_NESTED_CLIPPING
d D5=S1.y;if(ENABLE_NESTED_CLIPPING&&D5!=.0){d j4=.0;
#if defined(DRAW_INTERIOR_TRIANGLES)&&defined(BORROWED_COVERAGE_PASS)
if(BORROWED_COVERAGE_PASS){O0=unpackHalf2x16(e1(e0));F5=O0.y;}
#endif
if(F5!=V0){j4=F5==D5?O0.x:.0;h1(S0,packHalf2x16(A2(j4,qf)));}else{j4=unpackHalf2x16(e1(S0)).x;X1(S0);}o3=min(o3,j4);}else
#endif
{X1(S0);}h1(e0,packHalf2x16(A2(o3,V0)));
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r2(j0);
#endif
w2;c2;}
#endif
