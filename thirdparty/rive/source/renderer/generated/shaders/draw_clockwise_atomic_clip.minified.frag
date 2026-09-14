#ifdef FRAGMENT
J1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r0(Q2,g0);
#endif
rd(R2,d0);K1
#ifdef NESTED_CLIP_UPDATE_ONLY
N3 Ea(ga,Yd,S0);O3
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#define d5 o2
#define V3(p5) l1=p5;l3
#else
#define d5 m1
#define V3(p5) v0(g0,p5);U1;
#endif
d5(IB){
#ifdef DRAW_INTERIOR_TRIANGLES
B(j1,d);d o0=j1;
#else
B(I,z2);d o0=I.x;
#endif
#ifdef NESTED_CLIP_UPDATE_ONLY
if(NESTED_CLIP_UPDATE_ONLY){B(e3,W0);B(j4,c);uint N7=e3.y;uint T1=e3.x+L8(W0(floor(j4)),N7);uint p1=pd(S0,T1);d db;if(o0>=1.&&(p1<k.W1||p1>=(k.W1|j5))){db=.0;}else{d ae=o0;d g9=o0;if(p1<k.W1){uint O7=k.W1|(j5+q7(abs(o0)));uint f3=A7(S0,T1,O7);if(f3<=k.W1){g9=.0;}else if(f3<O7){g9=Aa(f3);}}if(g9>.0){uint eb=Fa(S0,T1,q7(abs(g9)));ae=Aa(eb)+o0;}db=1.-ae;}v0(d0,B0(db));V3(B0(1.))}else
#endif
{v0(d0,B0(o0));V3(B0(.0))}}
#endif
