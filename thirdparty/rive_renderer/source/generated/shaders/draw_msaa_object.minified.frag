#ifdef FRAGMENT
#ifdef DRAW_IMAGE_MESH
y3 U2(Y4,R3,DC);
#ifdef ENABLE_ADVANCED_BLEND
g7(JD);
#endif
z3 Z4 S3(R5)a5
#endif
V2(i,HB){
#ifdef DRAW_IMAGE_MESH
B(U0,c);
#else
B(k1,g);
#ifdef ATLAS_BLIT
B(C2,c);
#endif
#ifdef ENABLE_ADVANCED_BLEND
B(Y1,d);
#endif
#endif
#ifdef DRAW_IMAGE_MESH
i j=y7(DC,R5,U0,k.ad)*A0.y4;
#else
d n=
#ifdef ATLAS_BLIT
clamp(m2(VC,I9,C2,.0).x,G0(.0),G0(1.));
#else
1.;
#endif
i j=M7(k1,n R2);
#endif
#if defined(ENABLE_ADVANCED_BLEND)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
#ifdef DRAW_IMAGE_MESH
j.xyz=B6(j);X n2=i2(A0.n2);
#else
X n2=W5(Y1);
#endif
i M1=S8(JD);j.xyz=R4(j.xyz,M1,n2);j.xyz*=j.w;
#elif defined(SPEC_CONST_NONE)&&defined(FIXED_FUNCTION_COLOR_OUTPUT)&&!defined(DRAW_IMAGE_MESH)
j.xyz*=j.w;
#endif
#ifdef NEEDS_GAMMA_CORRECTION
if(NEEDS_GAMMA_CORRECTION){j=h3(j);}
#endif
j.xyz=M3(j.xyz,T.xy,k.v3,k.w3);F2(j);}
#endif
