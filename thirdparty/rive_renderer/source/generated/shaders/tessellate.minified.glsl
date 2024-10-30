#define Ya 10
#ifdef VERTEX
Y0(V)a0(0,g,JC);a0(1,g,KC);a0(2,g,BC);
#ifdef h6
a0(3,uint,VC);a0(4,uint,WC);a0(5,uint,XC);a0(6,uint,YC);
#else
a0(3,G,JB);
#endif
Z0
#endif
H1 w0 W(0,g,f4);w0 W(1,g,g4);w0 W(2,g,V2);w0 W(3,Q,C3);H3 W(4,uint,Q4);I1 d I L8(f g1,f G0,f U0,f p1){I t;t[0]=(any(notEqual(g1,G0))?G0:any(notEqual(G0,U0))?U0:p1)-g1;t[1]=p1-(any(notEqual(p1,U0))?U0:any(notEqual(U0,G0))?G0:g1);return t;}
#ifdef VERTEX
R1 S1 f2 S2(c8,pa,TB);S2(d8,qa,IC);g2 float M8(f l,f b){float Za=dot(l,b);float N8=dot(l,l)*dot(b,b);return(N8==.0)?1.:clamp(Za*inversesqrt(N8),-1.,1.);}h1(FE,V,q,n,H){f0(H,q,JC,g);f0(H,q,KC,g);f0(H,q,BC,g);
#ifdef h6
f0(H,q,VC,uint);f0(H,q,WC,uint);f0(H,q,XC,uint);f0(H,q,YC,uint);G JB=G(VC,WC,XC,YC);
#else
f0(H,q,JB,G);
#endif
c0(f4,g);c0(g4,g);c0(V2,g);c0(C3,Q);c0(Q4,uint);f g1=JC.xy;f G0=JC.zw;f U0=KC.xy;f p1=KC.zw;bool O8=n<4;float y=O8?BC.z:BC.w;int T6=int(O8?JB.x:JB.y);
#ifdef F7
int P8=T6<<16;if(JB.z==0xffffffffu){--P8;}float B5=float(P8>>16);
#else
float B5=float(T6<<16>>16);
#endif
float C5=float(T6>>16);f r1=f((n&1)==0?B5:C5,(n&2)==0?y+1.:y);uint F1=JB.z&0x3ffu;uint Q8=(JB.z>>10)&0x3ffu;uint D3=JB.z>>20;uint R=JB.w;if(C5<B5){R|=p3;}if((C5-B5)*P.l6<.0){r1.y=2.*y+1.-r1.y;}if((R&Y9)!=0u){uint ab=E0(IC,I7(R)).z;I R8=j1(uintBitsToFloat(E0(TB,ab*2u)));f S8=m0(R8,-2.*G0+U0+g1);f T8=m0(R8,-2.*U0+p1+G0);float B0=max(dot(S8,S8),dot(T8,T8));float a3=max(ceil(sqrt(.75*4.*sqrt(B0))),1.);F1=min(uint(a3),F1);}uint D5=F1+Q8+D3-1u;I n2=L8(g1,G0,U0,p1);float y1=acos(M8(n2[0],n2[1]));float z2=y1/float(Q8);float U6=determinant(I(U0-g1,p1-G0));if(U6==.0)U6=determinant(n2);if(U6<.0)z2=-z2;f4=g(g1,G0);g4=g(U0,p1);V2=g(float(D5)-abs(C5-r1.x),float(D5),(D3<<10)|F1,z2);if(D3>1u){I V6=I(n2[1],BC.xy);float bb=acos(M8(V6[0],V6[1]));float U8=float(D3);if((R&(A4|h5))==h5){U8-=2.;}float W6=bb/U8;if(determinant(V6)<.0)W6=-W6;C3.xy=BC.xy;C3.z=W6;}Q4=R;g T;T.x=r1.x*(2./X9)-1.;T.y=r1.y*P.l6-sign(P.l6);T.zw=f(0,1);d0(f4);d0(g4);d0(V2);d0(C3);d0(Q4);i1(T);}
#endif
#ifdef FRAGMENT
q2(G,GE){Y(f4,g);Y(g4,g);Y(V2,g);Y(C3,Q);Y(Q4,uint);f g1=f4.xy;f G0=f4.zw;f U0=g4.xy;f p1=g4.zw;I n2=L8(g1,G0,U0,p1);float cb=max(floor(V2.x),.0);float D5=V2.y;uint V8=uint(V2.z);float F1=float(V8&0x3ffu);float D3=float(V8>>10);float z2=V2.w;uint R=Q4;float E3=D5-D3;float G1=cb;if(G1<=E3){R&=~A4;}else{g1=G0=U0=p1;n2=I(n2[1],C3.xy);F1=1.;G1-=E3;E3=D3;if((R&A4)!=0u){if(G1<2.5)R|=q6;if(G1>1.5&&G1<3.5)R|=W7;}else if((R&h5)!=0u){E3-=2.;G1--;}z2=C3.z;R|=z2<.0?i5:X7;}f E5;float y1=.0;if(G1==.0||G1==E3||(R&A4)!=0u){bool q5=G1<E3*.5;E5=q5?g1:p1;y1=K7(q5?n2[0]:n2[1]);}else if((R&V7)!=0u){E5=G0;}else{float c2,F3;if(F1==E3){c2=G1/F1;F3=.0;}else{f L,O,v5=G0-g1;f C8=p1-g1;f W8=U0-G0;O=W8-v5;L=-3.*W8+C8;f db=O*(F1*2.);f eb=v5*(F1*F1);float F5=.0;float fb=min(F1-1.,G1);f X6=normalize(n2[0]);float gb=-abs(z2);float hb=(1.+G1)*abs(z2);for(int X2=Ya-1;X2>=0;--X2){float R4=F5+exp2(float(X2));if(R4<=fb){f Y6=R4*L+db;Y6=R4*Y6+eb;float ib=dot(normalize(Y6),X6);float Z6=R4*gb+hb;Z6=min(Z6,e5);if(ib>=cos(Z6))F5=R4;}}float jb=F5/F1;float X8=G1-F5;float G5=acos(clamp(X6.x,-1.,1.));G5=X6.y>=.0?G5:-G5;y1=X8*z2+G5;f v3=f(sin(y1),-cos(y1));float l=dot(v3,L),H5=dot(v3,O),z0=dot(v3,v5);float kb=max(H5*H5-l*z0,.0);float W2=sqrt(kb);if(H5>.0)W2=-W2;W2-=H5;float Y8=-.5*W2*l;f a7=(abs(W2*W2+Y8)<abs(l*z0+Y8))?f(W2,l):f(z0,W2);F3=(a7.y!=.0)?a7.x/a7.y:.0;F3=clamp(F3,.0,1.);if(X8==.0)F3=.0;c2=max(jb,F3);}f lb=W3(g1,G0,c2);f Z8=W3(G0,U0,c2);f mb=W3(U0,p1,c2);f a9=W3(lb,Z8,c2);f c9=W3(Z8,mb,c2);E5=W3(a9,c9,c2);if(c2!=F3)y1=K7(c9-a9);}r2(G(floatBitsToUint(Q(E5,y1)),R));}
#endif
