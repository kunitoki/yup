#ifdef VERTEX
c1(d0) d1
#endif
l2 E0 V(0,e,a1);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT V(4,C,l1);
#endif
#ifdef ENABLE_ADVANCED_BLEND
OPTIONALLY_FLAT V(6,d,Q0);
#endif
#ifdef ENABLE_MODULATED_IMAGE
E0 V(9,O,r1);
#endif
e2
#ifdef VERTEX
v1(RB,d0,D,p3,F6){T(a1,e);
#ifdef ENABLE_CLIPPING
T(l1,C);
#endif
#ifdef ENABLE_ADVANCED_BLEND
T(Q0,d);
#endif
#ifdef ENABLE_MODULATED_IMAGE
T(r1,O);
#endif
bool A9=(p3&Ag)!=0;bool Ti=(p3&zg)!=0;int Ke=p3&((1<<Ja)-1);int Le=ld(A9);int Ui=Ke>>Le;int Me=Ke&((1<<Le)-1);int Ne=A9?int(vg):int(kd);bool Pb=!A9&&Me==yg;int S3=Pb?0:Me;int G5=min(S3,Ne-1);int T3=Ui*Ne+G5;M C2=p1(TB,q4(T3));uint i0=C2.w;uint x6=max(i0&Ma,1u);M H5=p0(ZC,x6-1u);c S8=uintBitsToFloat(H5.xy);uint a0=H5.z&0xffffu;uint T8=H5.w;Y S0=n1(uintBitsToFloat(p0(LB,a0*4u)));M U3=p0(LB,a0*4u+1u);c m2=uintBitsToFloat(U3.xy);uint B7=i0&Q2;if(B7!=0u&&!A9&&!Pb){S3=S3-1;}if(S3!=G5){int U8=T3+S3-G5;M C7=p1(TB,q4(U8));if((C7.w&(Q2|0xffffu))!=(i0&(Q2|0xffffu))){C2=p1(TB,q4(int(T8)));}else{C2=C7;}i0=(C2.w&~Q2)|B7;}c W8=Pb?S8:uintBitsToFloat(C2.xy);c k0=M0(S0,W8)+m2;O0 G0=l5(WC,a0);uint n2=G0.x&0xfu;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){uint Qb=(n2==o5?G0.y:G0.x)>>16;d X0=l6(Qb,j.T4);if(n2==o5) X0=-X0;l1.x=X0;}
#endif
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND){Q0=float((G0.x>>4)&0xfu);}
#endif
c l0=k0;
#ifdef ENABLE_RENDER_TARGET_BOTTOM_UP
if(j.W9!=0u){l0.y=float(j.X9)-l0.y;}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){Y B3=n1(p0(JB,a0*g2+2u));e P3=p0(JB,a0*g2+3u);Ga(B3,P3.xy,l0 Y4);}
#endif
if(n2==fa){a1=e(unpackUnorm4x8(G0.y));}
#ifdef ENABLE_CLIPPING
else if(ENABLE_CLIPPING&&n2==o5){d E4=l6(G0.x>>16,j.T4);l1.y=E4;}
#endif
else{Y Rb=n1(p0(JB,a0*g2));e W7=p0(JB,a0*g2+1u);a1=Y9(l0,Rb,W7.xy,float(n2),W7.zw,uintBitsToFloat(G0.y));a1.w=-a1.w;}if(Ti){a1=e(.0,.0,.0,.0);}
#ifdef ENABLE_MODULATED_IMAGE
if(ENABLE_MODULATED_IMAGE&&(G0.x&vd)!=0u){Y Sb=n1(p0(JB,a0*g2+4u));e X7=p0(JB,a0*g2+5u);c o3=M0(Sb,l0)+X7.xy;r1=O(o3.x,o3.y,1.+X7.z);}else{r1=O(0.0,0.0,0.0);}
#endif
e I=H3(k0);
#ifdef POST_INVERT_Y
I.y=-I.y;
#endif
M V3=p0(LB,a0*4u+2u);I.z=I8(P1(V3.x),0xffu);Z(a1);
#ifdef ENABLE_CLIPPING
Z(l1);
#endif
#ifdef ENABLE_ADVANCED_BLEND
Z(Q0);
#endif
#ifdef ENABLE_MODULATED_IMAGE
Z(r1);
#endif
w1(I);}
#endif
