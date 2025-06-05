o1
#ifndef USE_TEXEL_FETCH_WITH_FRAG_COORD
n0 H(0,c,q0);
#endif
p1
#ifdef VERTEX
P2 Q2 E3 F3 q1(ZD,f0,B,n,K){c S1;S1.x=(n&1)==0?-1.:1.;S1.y=(n&2)==0?-1.:1.;
#ifndef USE_TEXEL_FETCH_WITH_FRAG_COORD
L(q0,c);q0.x=S1.x*.5+.5;q0.y=S1.y*-.5+.5;P(q0);
#endif
f Q=f(S1,0,1);h1(Q);}
#endif
#ifdef FRAGMENT
R2 C2(0,0,VC);S2
#ifndef USE_TEXEL_FETCH_WITH_FRAG_COORD
p4 G3(0,Yc)q4
#endif
e2(i,MD){i G8;
#ifndef USE_TEXEL_FETCH_WITH_FRAG_COORD
N(q0,c);G8=T1(VC,Yc,q0,.0);
#else
G8=d1(VC,c0(floor(y0.xy)));
#endif
f2(G8);}
#endif
