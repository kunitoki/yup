#ifdef FRAGMENT
#ifdef DRAW_IMAGE_MESH
B3 X2(Z4,T3,AC);
#ifdef ENABLE_ADVANCED_BLEND
g7(LD);
#endif
C3 a5 U3(R5)c5
#endif
Y2(i,IB){
#ifdef DRAW_IMAGE_MESH
B(U0,c);
#else
B(i1,g);
#ifdef ATLAS_BLIT
B(C2,c);
#endif
#ifdef ENABLE_ADVANCED_BLEND
B(Z1,d);
#endif
#endif
#ifdef DRAW_IMAGE_MESH
i j=y7(AC,R5,U0,k.fd)*A0.x4;
#else
d n=
#ifdef ATLAS_BLIT
clamp(m2(UC,I9,C2,.0).x,G0(.0),G0(1.));
#else
1.;
#endif
i j=M7(i1,n S2);
#endif
#if defined(ENABLE_ADVANCED_BLEND)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
#ifdef DRAW_IMAGE_MESH
j.xyz=B6(j);X n2=i2(A0.n2);
#else
X n2=W5(Z1);
#endif
i L1=S8(LD);j.xyz=Q4(j.xyz,L1,n2);j.xyz*=j.w;
#endif
#ifdef NEEDS_GAMMA_CORRECTION
if(NEEDS_GAMMA_CORRECTION){j=k3(j);}
#endif
j.xyz=Q3(j.xyz,S.xy,k.y3,k.z3);G2(j);}
#endif
