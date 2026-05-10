#ifdef VERTEX
A1(e3)r0(0,c,FC);B1 A1(q3)r0(1,c,GC);B1
#endif
h2 J0 d0(0,c,U0);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT d0(1,d,F3);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_MSAA)
J0 d0(2,g,N0);
#endif
Z1
#ifdef VERTEX
P3 Q3 E6(XB,e3,f3,q3,r3,v){v0(v,f3,FC,c);v0(v,r3,GC,c);Y(U0,c);
#ifdef ENABLE_CLIPPING
Y(F3,d);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_MSAA)
Y(N0,g);
#endif
c i0=Z0(j2(A0.r9),FC)+A0.a2;U0=GC;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){F3=r8(A0.V0,k.Y5);}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){
#ifndef RENDER_MODE_MSAA
N0=T7(j2(A0.k2),A0.D2,i0 r5);
#else
ic(j2(A0.k2),A0.D2,i0 r5);
#endif
}
#endif
g O=H3(i0);
#ifdef POST_INVERT_Y
O.y=-O.y;
#endif
#ifdef RENDER_MODE_MSAA
O.z=ca(A0.Z6);
#endif
l0(U0);
#ifdef ENABLE_CLIPPING
l0(F3);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_MSAA)
l0(N0);
#endif
D1(O);}
#endif
