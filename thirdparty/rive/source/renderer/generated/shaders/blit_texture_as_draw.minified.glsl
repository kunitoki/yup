l2
#ifdef USE_FILTERING
E0 V(0,c,f2);
#endif
e2
#ifdef VERTEX
j4 k4 P4 Q4 c1(d0) d1 v1(LF,d0,D,G,r){c y2;y2.x=(G&1)==0?-1.:1.;y2.y=(G&2)==0?-1.:1.;
#ifdef USE_FILTERING
T(f2,c);f2.x=y2.x*.5+.5;f2.y=y2.y*-.5+.5;Z(f2);
#endif
e I=e(y2,0,1);w1(I);}
#endif
#ifdef FRAGMENT
N3
#ifdef SOURCE_TEXTURE_MSAA
Xf(w5,l4,IC);
#else
i3(w5,l4,IC);
#endif
O3
#ifdef USE_FILTERING
x5 m4(Yf) y5
#endif
j3(i,PE){i C8;
#ifdef USE_FILTERING
q(f2,c);C8=i6(IC,Yf,f2,.0);
#elif defined(SOURCE_TEXTURE_MSAA)
C8=(D8(IC,0,e0(floor(f0.xy)))+D8(IC,1,e0(floor(f0.xy)))+D8(IC,2,e0(floor(f0.xy)))+D8(IC,3,e0(floor(f0.xy))))*0.25;
#else
C8=p1(IC,e0(floor(f0.xy)));
#endif
P2(C8);}
#endif
