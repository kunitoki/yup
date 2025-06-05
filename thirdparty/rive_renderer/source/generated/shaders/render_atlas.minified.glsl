#ifdef VERTEX
U0(f0)i0(0,f,LB);i0(1,f,MB);V0
#endif
o1 n0 H(0,f,D);p1
#ifdef VERTEX
q1(PE,f0,B,n,K){l0(n,B,LB,f);l0(n,B,MB,f);L(D,f);f Q;uint R;c J;if(E6(LB,MB,K,R,J,D Y1)){M T3=w0(JB,R*4u+2u);Z n6=uintBitsToFloat(T3.yzw);J=J*n6.x+n6.yz;Q=d7(J,q.Ha.x,q.Ha.y);}else{Q=f(q.C1,q.C1,q.C1,q.C1);}P(D);h1(Q);}
#endif
#ifdef FRAGMENT
#ifdef ATLAS_FEATHERED_FILL
e2(float,RE){N(D,f);f2(S4(D x1));}
#endif
#ifdef ATLAS_FEATHERED_STROKE
e2(float,TE){N(D,f);f2(R6(D x1));}
#endif
#endif
