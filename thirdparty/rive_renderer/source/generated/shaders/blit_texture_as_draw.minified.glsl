h2
#ifdef USE_FILTERING
J0 d0(0,c,U0);
#endif
Z1
#ifdef VERTEX
P3 Q3 B4 C4 A1(c0)B1 C1(TE,c0,G,v,S){c l2;l2.x=(v&1)==0?-1.:1.;l2.y=(v&2)==0?-1.:1.;
#ifdef USE_FILTERING
Y(U0,c);U0.x=l2.x*.5+.5;U0.y=l2.y*-.5+.5;l0(U0);
#endif
g O=g(l2,0,1);D1(O);}
#endif
#ifdef FRAGMENT
y3
#ifdef SOURCE_TEXTURE_MSAA
Ne(Y4,R3,ZB);
#else
U2(Y4,R3,ZB);
#endif
z3
#ifdef USE_FILTERING
Z4 S3(Oe)a5
#endif
V2(i,YD){i l8;
#ifdef USE_FILTERING
B(U0,c);l8=Q6(ZB,Oe,U0,.0);
#elif defined(SOURCE_TEXTURE_MSAA)
l8=(m8(ZB,0,U(floor(T.xy)))+m8(ZB,1,U(floor(T.xy)))+m8(ZB,2,U(floor(T.xy)))+m8(ZB,3,U(floor(T.xy))))*0.25;
#else
l8=F1(ZB,U(floor(T.xy)));
#endif
F2(l8);}
#endif
