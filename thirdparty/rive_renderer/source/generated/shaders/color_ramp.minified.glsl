#ifdef VERTEX
U0(f0)
#ifdef H8
i0(0,uint,WC);i0(1,uint,XC);i0(2,uint,YC);i0(3,uint,ZC);
#else
i0(0,M,AC);
#endif
V0
#endif
o1 n0 H(0,i,Q5);p1
#ifdef VERTEX
P2 Q2 E3 F3 i Zc(uint j){return ra((M(j,j,j,j)>>M(16,8,0,24))&0xffu)/255.;}q1(AE,f0,B,n,K){
#ifdef H8
l0(K,B,WC,uint);l0(K,B,XC,uint);l0(K,B,YC,uint);l0(K,B,ZC,uint);M AC=M(WC,XC,YC,ZC);
#else
l0(K,B,AC,M);
#endif
L(Q5,i);int c7=n>>1;float x=float(c7<=1?AC.x&0xffffu:AC.x>>16)/65536.;float I8=(n&1)==0?.0:1.;if(q.sa<.0){I8=1.-I8;}uint R5=AC.y;float y=float(R5&~ad)+I8;if((R5&ta)!=0u&&c7==0){if((R5&J8)!=0u)x=.0;else x-=ua;}if((R5&va)!=0u&&c7==3){if((R5&J8)!=0u)x=1.;else x+=ua;}Q5=Zc(c7<=1?AC.z:AC.w);f Q=d7(c(x,y),2.,q.sa);P(Q5);h1(Q);}
#endif
#ifdef FRAGMENT
R2 S2 e2(i,BE){N(Q5,i);f2(Q5);}
#endif
