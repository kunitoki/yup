#ifdef VERTEX
c1(w3) K(0,c,PC);d1 c1(J3) K(1,c,QC);d1 c1(A1) K(L9,e,YB);K(M9,e,SB);K(N9,e,PB);K(O9,uint,ZB);K(P9,uint,AC);K(Q9,uint,BC);K(R9,uint,LC);K(Z9,e,HC);d1
#endif
l2 E0 V(0,c,V5);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT V(1,d,Y3);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
E0 V(2,e,R0);
#endif
OPTIONALLY_FLAT V(3,i,Q1);
#ifdef ENABLE_ADVANCED_BLEND
Z2 V(4,Q,H1);
#endif
e2
#ifdef VERTEX
j4 k4 T6(RB,w3,x3,J3,K3,A1,h0,G){L(G,x3,PC,c);L(G,K3,QC,c);L(r,h0,YB,e);L(r,h0,SB,e);L(r,h0,PB,e);L(r,h0,ZB,uint);L(r,h0,AC,uint);L(r,h0,BC,uint);L(r,h0,LC,uint);L(r,h0,HC,e);T(V5,c);
#ifdef ENABLE_CLIPPING
T(Y3,d);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
T(R0,e);
#endif
T(Q1,i);
#ifdef ENABLE_ADVANCED_BLEND
T(H1,Q);
#endif
c k0=M0(n1(YB),PC)+PB.xy;V5=QC*HC.zw+HC.xy;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){Y3=l6(AC,j.T4);}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){
#ifndef RENDER_MODE_DEPTH_STENCIL
R0=i8(n1(SB),PB.zw,k0 Y4);
#else
Ga(n1(SB),PB.zw,k0 Y4);
#endif
}
#endif
e I=H3(k0);
#ifdef POST_INVERT_Y
I.y=-I.y;
#endif
#ifdef RENDER_MODE_DEPTH_STENCIL
I.z=I8(LC,0xffu);
#endif
Q1=unpackUnorm4x8(ZB);
#ifdef ENABLE_ADVANCED_BLEND
H1=P1(BC);
#endif
Z(V5);
#ifdef ENABLE_CLIPPING
Z(Y3);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_DEPTH_STENCIL)
Z(R0);
#endif
Z(Q1);
#ifdef ENABLE_ADVANCED_BLEND
Z(H1);
#endif
w1(I);}
#endif
