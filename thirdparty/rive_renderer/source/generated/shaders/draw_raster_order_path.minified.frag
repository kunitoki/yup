#ifdef FRAGMENT
J1 r0(Q2,g0);k1(R2,d0);r0(d6,g4);k1(F6,H7);K1 m1(IB){B(i1,g);
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
#if!defined(DRAW_INTERIOR_TRIANGLES)
v2;
#endif
D N4=unpackHalf2x16(d1(H7));d h9=N4.y;d m0=h9==z0?N4.x:G0(.0);
#ifdef DRAW_INTERIOR_TRIANGLES
m0+=j1;Y1(H7);
#else
m0=vh(m0,I g1);f1(H7,packHalf2x16(A2(m0,z0)));
#endif
d n;
#ifdef CLOCKWISE_FILL
if(CLOCKWISE_FILL){n=W9(m0,G0(.0),G0(1.));}else
#endif
{n=abs(m0);
#ifdef ENABLE_EVEN_ODD
if(ENABLE_EVEN_ODD&&z0<.0){n=1.-G0(abs(fract(n*.5)*2.+-1.));}
#endif
n=min(n,G0(1.));}
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&S1.x<.0){d V0=-S1.x;
#ifdef ENABLE_NESTED_CLIPPING
if(ENABLE_NESTED_CLIPPING){d F5=S1.y;if(F5!=.0){D O0=unpackHalf2x16(d1(d0));d A6=O0.y;d k4;if(A6!=V0){k4=A6==F5?O0.x:.0;
#ifndef DRAW_INTERIOR_TRIANGLES
v0(g4,B0(k4,.0,.0,.0));
#endif
}else{k4=H0(g4).x;
#ifndef DRAW_INTERIOR_TRIANGLES
r2(g4);
#endif
}n=min(n,k4);}}
#endif
f1(d0,packHalf2x16(A2(n,V0)));r2(g0);}else
#endif
{
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){d V0=S1.x;if(V0!=.0){D O0=unpackHalf2x16(d1(d0));d A6=O0.y;n=(A6==V0)?min(O0.x,n):G0(.0);}}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d U4=g3(Y4(N0));n=clamp(U4,G0(.0),n);}
#endif
i j=M7(i1,n S2);i L1;if(h9!=z0){L1=H0(g0);
#ifndef DRAW_INTERIOR_TRIANGLES
v0(g4,L1);
#endif
}else{L1=H0(g4);
#ifndef DRAW_INTERIOR_TRIANGLES
r2(g4);
#endif
}
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){if(Z1!=V5(M5)){j.xyz=Q4(j.xyz,L1,W5(Z1));}j.xyz*=j.w;}
#endif
#ifdef NEEDS_GAMMA_CORRECTION
if(NEEDS_GAMMA_CORRECTION){j=k3(j);}
#endif
j+=L1*(1.-j.w);j.xyz=Q3(j.xyz,S.xy,k.y3,k.z3);v0(g0,j);Y1(d0);}
#if!defined(DRAW_INTERIOR_TRIANGLES)
w2;
#endif
U1;}
#endif
