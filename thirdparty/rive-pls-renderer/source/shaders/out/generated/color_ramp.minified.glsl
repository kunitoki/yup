#ifdef VERTEX
T0(P)q0(0,T,RB);U0
#endif
z1 k0 I(0,i,V3);A1
#ifdef VERTEX
N1 O1 U1 V1 i g9(uint f){return M0((T(f,f,f,f)>>T(16,8,0,24))&0xffu)/255.;}e1(GD,P,r,j,L){w0(L,r,RB,T);Q(V3,i);float x=float((j&1)==0?RB.x&0xffffu:RB.x>>16)/65536.;float C5=(j&2)==0?1.:.0;if(v.D5<.0){C5=1.-C5;}V3=g9((j&1)==0?RB.z:RB.w);g B;B.x=x*2.-1.;B.y=(float(RB.y)+C5)*v.D5-sign(v.D5);B.zw=d(0,1);S(V3);f1(B);}
#endif
#ifdef FRAGMENT
q2(i,HD){N(V3,i);r2(V3);}
#endif
