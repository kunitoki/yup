#ifdef FRAGMENT
#ifdef DRAW_IMAGE_MESH
N3 i3(w5,l4,CC);
#ifdef ENABLE_ADVANCED_BLEND
E5(XD);
#endif
O3 x5 m4(r5) y5
#endif
j3(i,IB){
#ifdef DRAW_IMAGE_MESH
q(V5,c);q(Q1,i);
#ifdef ENABLE_ADVANCED_BLEND
q(H1,Q);
#endif
#else
q(a1,e);
#ifdef ENABLE_MODULATED_IMAGE
q(r1,O);
#endif
#ifdef FEATHER_ATLAS_BLIT
q(J2,c);
#endif
#ifdef ENABLE_ADVANCED_BLEND
q(Q0,d);
#endif
#endif
#ifdef DRAW_IMAGE_MESH
i p=K7(CC,r5,V5,j.Vd)*Q1;
#else
d n=
#ifdef FEATHER_ATLAS_BLIT
clamp(o2(FD,ma,J2,.0).x,H0(.0),H0(1.));
#else
1.;
#endif
i p=Y7(
#ifdef ENABLE_MODULATED_IMAGE
r1,
#endif
#ifdef ENABLE_ADVANCED_BLEND
k3(Q0),
#endif
a1 e3);
#endif
#if defined(ENABLE_ADVANCED_BLEND)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
#ifdef DRAW_IMAGE_MESH
p.xyz=Q6(p);Q y3=H1;
#else
Q y3=k3(Q0);
#endif
i I1=I6(XD);p.xyz=h5(p.xyz,I1,y3)*p.w;
#endif
#ifndef DRAW_IMAGE_MESH
p*=n;
#endif
p.xyz=M2(p.xyz,p.w,f0.xy,j.L3,j.M3);P2(p);}
#endif
