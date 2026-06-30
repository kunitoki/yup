#ifndef Cb
#define Cb g
#endif
#ifndef J6
#define J6 c
#endif
e float K9(c o,c b){float Fe=dot(o,b);float Db=dot(o,o)*dot(b,b);return(Db==.0)?1.:clamp(Fe*inversesqrt(Db),-1.,1.);}e void Ge(c w0,c x0,c E0,c I0,g1(c)A,g1(c)F,g1(c)d2){d2=x0-w0;c K6=E0-x0;c h8=I0-w0;F=K6-d2;A=-3.*K6+h8;}e a0 L9(c w0,c x0,c E0,c I0){a0 t;t[0]=(any(notEqual(w0,x0))?x0:any(notEqual(x0,E0))?E0:I0)-w0;t[1]=I0-(any(notEqual(I0,E0))?E0:any(notEqual(E0,x0))?x0:w0);return t;}e float He(c w0,c x0,c E0,c I0,float w1,float Ie){c A,F,d2;Ge(w0,x0,E0,I0,A,F,d2);c L6=3.*(((A*w1)+2.*F)*w1+d2);float Eb=length(L6);if(Eb==.0){return.0;}L6*=1./Eb;float i8=2.*dot(A,L6);float M6=3.*(i8*w1+4.*dot(F,L6))*w1+6.*dot(d2,L6);float M9=min(w1,1.-w1);float Je=(i8*M9*M9+M6)*M9;float Fb=min(Ie,Je*.9999);float S2;if(i8==.0){S2=Fb/M6;}else{float H=1./i8;float b=M6*H,J1=-Fb*H;float N6=(-1./3.)*b,O6=.5*J1;float Gb=O6*O6-N6*N6*N6;if(Gb<.0){float j8=sqrt(N6);float j1=acos(O6/(j8*j8*j8));S2=-2.*j8*cos(j1*(1./3.)+(-x3*2./3.));}else{float A=pow(abs(O6)+sqrt(Gb),1./3.);if(O6<.0)A=-A;S2=A!=.0?A+N6/A:.0;}}S2=abs(S2);g t0011=w1+Cb(-S2,-S2,S2,S2);g Hb=(A.xyxy*t0011+2.*F.xyxy)*t0011+d2.xyxy;a0 E2=L9(w0,x0,E0,I0);c Ke=t0011.x<1e-3?E2[0]:Hb.xy;c Le=t0011.z>1.-1e-3?E2[1]:Hb.zw;return acos(K9(Ke,Le));}e float k8(float o,float b){o=b<.0?-o:o;b=abs(b);return o>.0?(o<b?o/b:1.):.0;}float Me(c w0,c x0,c E0,c I0,g1(float)N9){c Ib=I0-w0;float Jb=length(I0-w0);if(Jb==.0){N9=.5;return.0;}c T2=J6(-Ib.y,Ib.x)/Jb;float Kb=dot(T2,E0-w0);float z4=dot(T2,x0-w0);float A4=z4-Kb;
#if 0
float o=3.*A4;float Lb=A4+z4;float J1=z4;float q2=sqrt(max(A4*A4+Kb*z4,.0));if(Lb<.0)q2=-q2;q2+=Lb;c P6=J6(k8(q2,o),k8(J1,q2));c T5=3.*(P6*(P6*(P6*A4-(z4+A4))+z4));T5=abs(T5);N9=T5.x>T5.y?P6.x:P6.y;return max(T5.x,T5.y);
#else
float Mb=3.*A4;float F=-z4-A4;float d2=z4;float t=.5;for(int D0=0;D0<3;++D0){float Nb=Mb*t;t=k8(Nb*t-d2,2.*(Nb+F));}N9=t;return abs(t*(t*(t*Mb+3.*F)+3.*d2));
#endif
}