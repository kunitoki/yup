#ifdef VERTEX
L0(e0)
#ifdef G7
h0(0,uint,TC);h0(1,uint,UC);h0(2,uint,VC);h0(3,uint,WC);
#else
h0(0,D,ZB);
#endif
M0
#endif
o1 o0 H(0,i,h5);p1
#ifdef VERTEX
H2 I2 r3 v3 i Jb(uint j){return m9((D(j,j,j,j)>>D(16,8,0,24))&0xffu)/255.;}Z0(XD,e0,o,l,I){
#ifdef G7
k0(I,o,TC,uint);k0(I,o,UC,uint);k0(I,o,VC,uint);k0(I,o,WC,uint);D ZB=D(TC,UC,VC,WC);
#else
k0(I,o,ZB,D);
#endif
P(h5,i);int l6=l>>1;float x=float(l6<=1?ZB.x&0xffffu:ZB.x>>16)/65536.;float H7=(l&1)==0?.0:1.;if(m.n9<.0){H7=1.-H7;}uint i5=ZB.y;float y=float(i5&~Kb)+H7;if((i5&o9)!=0u&&l6==0){if((i5&I7)!=0u)x=.0;else x-=p9;}if((i5&q9)!=0u&&l6==3){if((i5&I7)!=0u)x=1.;else x+=p9;}h5=Jb(l6<=1?ZB.z:ZB.w);e Q=m6(c(x,y),2.,m.n9);X(h5);U0(Q);}
#endif
#ifdef FRAGMENT
J2 L2 T1(i,YD){Z(h5,i);U1(h5);}
#endif
