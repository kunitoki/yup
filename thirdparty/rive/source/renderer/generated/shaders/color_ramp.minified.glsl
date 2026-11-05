#ifdef VERTEX
h1(d0)
#ifdef hb
K(0,uint,ZD);K(1,uint,AE);K(2,uint,BE);K(3,uint,CE);
#else
K(0,R,KC);
#endif
i1
#endif
o2 I0 W(0,i,B7);P1
#ifdef VERTEX
A4 B4 i5 j5 i bh(uint k){return Nd((R(k,k,k,k)>>R(16,8,0,24))&0xffu)/255.;}n1(TF,d0,A,C,r){
#ifdef hb
L(r,A,ZD,uint);L(r,A,AE,uint);L(r,A,BE,uint);L(r,A,CE,uint);R KC=R(ZD,AE,BE,CE);
#else
L(r,A,KC,R);
#endif
V(B7,i);int o9=C>>1;float x=float(o9<=1?KC.x&0xffffu:KC.x>>16)/65536.;float ib=(C&1)==0?.0:1.;if(j.Od<.0){ib=1.-ib;}uint C7=KC.y;float y=float(C7&~ch)+ib;if((C7&Pd)!=0u&&o9==0){if((C7&jb)!=0u) x=.0;else x-=l5;}if((C7&Qd)!=0u&&o9==3){if((C7&jb)!=0u) x=1.;else x+=l5;}B7=bh(o9<=1?KC.z:KC.w);e I=p9(c(x,y),2.,j.Od);
#ifdef POST_INVERT_Y
I.y=-I.y;
#endif
e0(B7);C1(I);}
#endif
#ifdef FRAGMENT
X3 Y3 v2(i,UF){q(B7,i);w2(B7);}
#endif
