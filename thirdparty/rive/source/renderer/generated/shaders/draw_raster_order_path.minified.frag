#ifdef FRAGMENT
R1 B0(K2,n0);o1(c3,m0);B0(o6,B4);o1(U6,R7);S1 T1(IB){q(a1,e);
#ifdef ENABLE_MODULATED_IMAGE
q(r1,O);
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
q(m1,d);
#else
q(S,G2);
#endif
q(F0,d);
#ifdef ENABLE_CLIPPING
q(l1,C);
#endif
#ifdef ENABLE_CLIP_RECT
q(R0,e);
#endif
#ifdef ENABLE_ADVANCED_BLEND
q(Q0,d);
#endif
#if!defined(DRAW_INTERIOR_TRIANGLES)
E2;
#endif
C d5=unpackHalf2x16(h1(R7));d D9=d5.y;d w0=D9==F0?d5.x:H0(.0);
#ifdef DRAW_INTERIOR_TRIANGLES
w0+=m1;Z1(R7);
#else
w0=Xi(w0,S k1);j1(R7,packHalf2x16(H2(w0,F0)));
#endif
d n;
#ifdef CLOCKWISE_FILL
if(CLOCKWISE_FILL){n=Aa(w0,H0(.0),H0(1.));}else
#endif
{n=abs(w0);
#ifdef ENABLE_EVEN_ODD
if(ENABLE_EVEN_ODD&&F0<.0){n=1.-H0(abs(fract(n*.5)*2.+-1.));}
#endif
n=min(n,H0(1.));}
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&l1.x<.0){d X0=-l1.x;
#ifdef ENABLE_NESTED_CLIPPING
if(ENABLE_NESTED_CLIPPING){d E4=l1.y;if(E4!=.0){C T0=unpackHalf2x16(h1(m0));d P6=T0.y;d G4;if(P6!=X0){G4=P6==E4?T0.x:.0;
#ifndef DRAW_INTERIOR_TRIANGLES
y0(B4,I0(G4,.0,.0,.0));
#endif
}else{G4=N0(B4).x;
#ifndef DRAW_INTERIOR_TRIANGLES
D2(B4);
#endif
}n=min(n,G4);}}
#endif
j1(m0,packHalf2x16(H2(n,X0)));D2(n0);}else
#endif
{
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING){d X0=l1.x;if(X0!=.0){C T0=unpackHalf2x16(h1(m0));d P6=T0.y;n=(P6==X0)?min(T0.x,n):H0(.0);}}
#endif
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d m5=v3(v5(R0));n=clamp(m5,H0(.0),n);}
#endif
i p=Y7(
#ifdef ENABLE_MODULATED_IMAGE
r1,
#endif
#ifdef ENABLE_ADVANCED_BLEND
k3(Q0),
#endif
a1 e3);i I1;if(D9!=F0){I1=N0(n0);
#ifndef DRAW_INTERIOR_TRIANGLES
y0(B4,I1);
#endif
}else{I1=N0(B4);
#ifndef DRAW_INTERIOR_TRIANGLES
D2(B4);
#endif
}bool bf=false;
#ifdef ENABLE_MODULATED_IMAGE
bf=ENABLE_MODULATED_IMAGE&&r1.z<.0;
#endif
if(bf){
#ifdef ENABLE_MODULATED_IMAGE
uint dj=uint(-r1.z-1.);d ej=Vi(p,dj);p=I1*mix(H0(1.),ej,n);y0(n0,p);Z1(m0);
#endif
}else{
#ifdef ENABLE_ADVANCED_BLEND
if(ENABLE_ADVANCED_BLEND&&Q0!=j6(L4)){p.xyz=h5(p.xyz,I1,k3(Q0))*p.w;}
#endif
p*=n;d n3=p.w;p+=I1*(1.-n3);p.xyz=M2(p.xyz,n3,f0.xy,j.L3,j.M3);y0(n0,p);Z1(m0);}}
#if!defined(DRAW_INTERIOR_TRIANGLES)
F2;
#endif
h2;}
#endif
