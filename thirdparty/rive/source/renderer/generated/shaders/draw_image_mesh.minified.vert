#ifdef VERTEX
A1(h3)p0(0,c,GC);B1 A1(w3)p0(1,c,HC);B1
#endif
h2 J0 c0(0,c,U0);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT c0(1,d,I3);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_MSAA)
J0 c0(2,g,N0);
#endif
a2
#ifdef VERTEX
R3 S3 E6(YB,h3,i3,w3,x3,v){q0(v,i3,GC,c);q0(v,x3,HC,c);Y(U0,c);
#ifdef ENABLE_CLIPPING
Y(I3,d);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_MSAA)
Y(N0,g);
#endif
c i0=Z0(j2(A0.r9),GC)+A0.c2;U0=HC;
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){I3=r8(A0.V0,k.Y5);}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){
#ifndef RENDER_MODE_MSAA
N0=T7(j2(A0.k2),A0.D2,i0 w5);
#else
nc(j2(A0.k2),A0.D2,i0 w5);
#endif
}
#endif
g N=K3(i0);
#ifdef POST_INVERT_Y
N.y=-N.y;
#endif
#ifdef RENDER_MODE_MSAA
N.z=ca(A0.Z6);
#endif
k0(U0);
#ifdef ENABLE_CLIPPING
k0(I3);
#endif
#if defined(ENABLE_CLIP_RECT)&&!defined(RENDER_MODE_MSAA)
k0(N0);
#endif
D1(N);}
#endif
