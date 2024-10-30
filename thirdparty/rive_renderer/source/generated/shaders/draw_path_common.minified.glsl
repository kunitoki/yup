#ifdef VERTEX
R1 R2(p2,da,EC);S1 f2 S2(c8,pa,TB);N3(p7,E9,GC);O3(q7,F9,HB);S2(d8,qa,IC);g2
#ifdef DRAW_PATH
d e0 m5(int o8){return e0(o8&((1<<S7)-1),o8>>S7);}d float p8(I p0,f ra){f L0=m0(p0,ra);return(abs(L0.x)+abs(L0.y))*(1./dot(L0,L0));}d bool k7(g E4,g v6,int H,j2(M)q3,j2(f)sa
#ifndef USING_DEPTH_STENCIL
,j2(r)r3
#else
,j2(M)ta
#endif
T2){int n5=int(E4.x);float m2=E4.y;float o5=E4.z;int q8=floatBitsToInt(E4.w)>>2;int w6=floatBitsToInt(E4.w)&3;int x6=min(n5,q8-1);int p5=H*q8+x6;G d4=N1(EC,m5(p5));uint R=d4.w;G y6=E0(IC,I7(R));f r8=uintBitsToFloat(y6.xy);q3=W1(y6.z&0xffffu);uint ua=y6.w;I p0=j1(uintBitsToFloat(E0(TB,q3*2u)));G F4=E0(TB,q3*2u+1u);f a1=uintBitsToFloat(F4.xy);float Z1=uintBitsToFloat(F4.z);
#ifdef USING_DEPTH_STENCIL
ta=W1(F4.w);
#endif
uint v8=R&p3;if(v8!=0u){n5=int(v6.x);m2=v6.y;o5=v6.z;}if(n5!=x6){p5+=n5-x6;G w8=N1(EC,m5(p5));if((w8.w&(p3|0xffffu))!=(R&(p3|0xffffu))){bool va=Z1==.0||r8.x!=.0;if(va){d4=N1(EC,m5(int(ua)));}}else{d4=w8;}R=(d4.w&~p3)|v8;}float y1=uintBitsToFloat(d4.z);f v3=f(sin(y1),-cos(y1));f x8=uintBitsToFloat(d4.xy);f z6;if(Z1!=.0){m2*=sign(determinant(p0));if((R&i5)!=0u)m2=min(m2,.0);if((R&X7)!=0u)m2=max(m2,.0);float U2=p8(p0,v3)*G2;h y8=1.;if(U2>Z1){y8=o2(Z1)/o2(U2);Z1=U2;}f w3=m0(v3,Z1+U2);
#ifndef USING_DEPTH_STENCIL
float x=m2*(Z1+U2);r3=Z5((1./(U2*2.))*(f(x,-x)+Z1)+.5);
#endif
uint A6=R&A4;if(A6!=0u){int G4=2;if((R&q6)==0u)G4=-G4;if((R&p3)!=0u)G4=-G4;e0 wa=m5(p5+G4);G xa=N1(EC,wa);float ya=uintBitsToFloat(xa.z);float H4=abs(ya-y1);if(H4>e5)H4=2.*e5-H4;bool q5=(R&q6)!=0u;bool za=(R&i5)!=0u;float z8=H4*(q5==za?-.5:.5)+y1;f r5=f(sin(z8),-cos(z8));float B6=p8(p0,r5);float I4=cos(H4*.5);float C6;if((A6==Z9)||(A6==aa&&I4>=.25)){float Aa=(R&h5)!=0u?1.:.25;C6=Z1*(1./max(I4,Aa));}else{C6=Z1*I4+B6*.5;}float D6=C6+B6*G2;if((R&W7)!=0u){float A8=Z1+U2;float Ba=U2*.125;if(A8<=D6*I4+Ba){float Ca=A8*(1./I4);w3=r5*Ca;}else{f E6=r5*D6;f Da=f(dot(w3,w3),dot(E6,E6));w3=m0(Da,inverse(I(w3,E6)));}}f Ea=abs(m2)*w3;float B8=(D6-dot(Ea,r5))/(B6*(G2*2.));
#ifndef USING_DEPTH_STENCIL
if((R&i5)!=0u)r3.y=o2(B8);else r3.x=o2(B8);
#endif
}
#ifndef USING_DEPTH_STENCIL
r3*=y8;r3.y=max(r3.y,d1(1e-4));
#endif
z6=m0(p0,m2*w3);if(w6!=Y7)return false;}else{if(w6==a8)x8=r8;z6=sign(m0(m2*v3,inverse(p0)))*G2;if(bool(R&p3)!=bool(R&ba)){o5=-o5;}
#ifndef USING_DEPTH_STENCIL
r3=D2(o5,-1.);
#endif
if((R&V7)!=0u&&w6!=Z7)return false;}sa=m0(p0,x8)+z6+a1;return true;}
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
d f l7(Q F6,j2(M)q3,j2(h)Fa T2){q3=W1(floatBitsToUint(F6.z)&0xffffu);I p0=j1(uintBitsToFloat(E0(TB,q3*2u)));G F4=E0(TB,q3*2u+1u);f a1=uintBitsToFloat(F4.xy);Fa=H7(floatBitsToInt(F6.z)>>16)*sign(determinant(p0));return m0(p0,F6.xy)+a1;}
#endif
#endif
