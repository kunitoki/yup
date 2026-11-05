o2
#ifdef USE_FILTERING
I0 W(0,c,p2);
#endif
P1
#ifdef VERTEX
A4 B4 i5 j5 h1(d0) i1 n1(RF,d0,A,C,r){c I2;I2.x=(C&1)==0?-1.:1.;I2.y=(C&2)==0?-1.:1.;
#ifdef USE_FILTERING
V(p2,c);p2.x=I2.x*.5+.5;p2.y=I2.y*-.5+.5;e0(p2);
#endif
e I=e(I2,0,1);C1(I);}
#endif
#ifdef FRAGMENT
X3
#ifdef SOURCE_TEXTURE_MSAA
Zg(J5,C4,JC);
#else
Z2(J5,C4,JC);
#endif
Y3
#ifdef USE_FILTERING
K5 D4(ah) L5
#endif
v2(i,SF){i m9;
#ifdef USE_FILTERING
q(p2,c);m9=k5(JC,ah,p2,.0);
#elif defined(SOURCE_TEXTURE_MSAA)
m9=(n9(JC,0,f0(floor(Y.xy)))+n9(JC,1,f0(floor(Y.xy)))+n9(JC,2,f0(floor(Y.xy)))+n9(JC,3,f0(floor(Y.xy))))*0.25;
#else
m9=j1(JC,f0(floor(Y.xy)));
#endif
w2(m9);}
#endif
