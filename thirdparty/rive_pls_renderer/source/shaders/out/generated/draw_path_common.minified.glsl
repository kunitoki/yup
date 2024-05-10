#ifdef VERTEX
O1 Q2(w9,AC);P1 V1 R2(w7,B9,SB);M3(G6,X8,PB);N3(H6,Y8,HB);R2(x7,C9,FC);W1
#ifdef DRAW_PATH
p m0 N4(int H7){return m0(H7&((1<<j7)-1),H7>>j7);}p float I7(A a0,d D9){d R0=h0(a0,D9);return(abs(R0.x)+abs(R0.y))*(1./dot(R0,R0));}p bool E6(g c4,g P5,int L,y2(M)S2,y2(d)E9
#ifndef USING_DEPTH_STENCIL
,y2(l0)T2
#else
,y2(M)F9
#endif
U2){int O4=int(c4.x);float S1=c4.y;float P4=c4.z;int J7=floatBitsToInt(c4.w)>>2;int Q5=floatBitsToInt(c4.w)&3;int R5=min(O4,J7-1);int Q4=L*J7+R5;T A3=I1(AC,N4(Q4));uint D=A3.w;T S5=z0(FC,Z6(D));d K7=uintBitsToFloat(S5.xy);S2=O0(S5.z&0xffffu);uint G9=S5.w;A a0=W0(uintBitsToFloat(z0(SB,S2*2u)));T d4=z0(SB,S2*2u+1u);d K0=uintBitsToFloat(d4.xy);float K1=uintBitsToFloat(d4.z);
#ifdef USING_DEPTH_STENCIL
F9=O0(d4.w);
#endif
uint L7=D&I4;if(L7!=0u){O4=int(P5.x);S1=P5.y;P4=P5.z;}if(O4!=R5){Q4+=O4-R5;T M7=I1(AC,N4(Q4));if((M7.w&0xffffu)!=(D&0xffffu)){bool H9=K1==.0||K7.x!=.0;if(H9){A3=I1(AC,N4(int(G9)));}}else{A3=M7;}D=A3.w|L7;}float p1=uintBitsToFloat(A3.z);d V2=d(sin(p1),-cos(p1));d N7=uintBitsToFloat(A3.xy);d T5;if(K1!=.0){S1*=sign(determinant(a0));if((D&J4)!=0u)S1=min(S1,.0);if((D&o7)!=0u)S1=max(S1,.0);float z2=I7(a0,V2)*k2;h O7=1.;if(z2>K1){O7=E0(K1)/E0(z2);K1=z2;}d W2=h0(V2,K1+z2);
#ifndef USING_DEPTH_STENCIL
float x=S1*(K1+z2);T2=Z1((1./(z2*2.))*(d(x,-x)+K1)+.5);
#endif
uint U5=D&Z3;if(U5!=0u){int e4=2;if((D&L5)==0u)e4=-e4;if((D&I4)!=0u)e4=-e4;m0 I9=N4(Q4+e4);T J9=I1(AC,I9);float K9=uintBitsToFloat(J9.z);float f4=abs(K9-p1);if(f4>E4)f4=2.*E4-f4;bool R4=(D&L5)!=0u;bool L9=(D&J4)!=0u;float P7=f4*(R4==L9?-.5:.5)+p1;d S4=d(sin(P7),-cos(P7));float V5=I7(a0,S4);float g4=cos(f4*.5);float W5;if((U5==r9)||(U5==v9&&g4>=.25)){float M9=(D&H4)!=0u?1.:.25;W5=K1*(1./max(g4,M9));}else{W5=K1*g4+V5*.5;}float X5=W5+V5*k2;if((D&n7)!=0u){float Q7=K1+z2;float N9=z2*.125;if(Q7<=X5*g4+N9){float O9=Q7*(1./g4);W2=S4*O9;}else{d Y5=S4*X5;d P9=d(dot(W2,W2),dot(Y5,Y5));W2=h0(P9,inverse(A(W2,Y5)));}}d Q9=abs(S1)*W2;float R7=(X5-dot(Q9,S4))/(V5*(k2*2.));
#ifndef USING_DEPTH_STENCIL
if((D&J4)!=0u)T2.y=E0(R7);else T2.x=E0(R7);
#endif
}
#ifndef USING_DEPTH_STENCIL
T2*=O7;T2.y=max(T2.y,E0(1e-4));
#endif
T5=h0(a0,S1*W2);if(Q5!=p7)return false;}else{if(Q5==r7)N7=K7;T5=sign(h0(S1*V2,inverse(a0)))*k2;if((D&I4)!=0u)P4=-P4;
#ifndef USING_DEPTH_STENCIL
T2=Z1(P4,-1);
#endif
if((D&m7)!=0u&&Q5!=q7)return false;}E9=h0(a0,N7)+T5+K0;return true;}
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
p d F6(D1 Z5,y2(M)S2,y2(h)R9 U2){S2=O0(floatBitsToUint(Z5.z)&0xffffu);A a0=W0(uintBitsToFloat(z0(SB,S2*2u)));T d4=z0(SB,S2*2u+1u);d K0=uintBitsToFloat(d4.xy);R9=float(floatBitsToInt(Z5.z)>>16)*sign(determinant(a0));return h0(a0,Z5.xy)+K0;}
#endif
#endif
