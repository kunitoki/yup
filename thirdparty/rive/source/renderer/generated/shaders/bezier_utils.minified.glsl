#ifndef Bd
#define Bd e
#endif
#ifndef r7
#define r7 c
#endif
f float h9(c l,c b){float Rg=dot(l,b);float Cd=dot(l,l)*dot(b,b);return(Cd==.0)?1.:clamp(Rg*inversesqrt(Cd),-1.,1.);}f void Sg(c D0,c H0,c M0,c V0,p1(c) E,p1(c) J,p1(c) D2){D2=H0-D0;c v7=M0-H0;c i9=V0-D0;J=v7-D2;E=-3.*v7+i9;}f X eb(c D0,c H0,c M0,c V0){X t;t[0]=(any(notEqual(D0,H0))?H0:any(notEqual(H0,M0))?M0:V0)-D0;t[1]=V0-(any(notEqual(V0,M0))?M0:any(notEqual(M0,H0))?H0:D0);return t;}f float Tg(c D0,c H0,c M0,c V0,float M1,float Ug){c E,J,D2;Sg(D0,H0,M0,V0,E,J,D2);c w7=3.*(((E*M1)+2.*J)*M1+D2);float Dd=length(w7);if(Dd==.0){return.0;}w7*=1./Dd;float j9=2.*dot(E,w7);float x7=3.*(j9*M1+4.*dot(J,w7))*M1+6.*dot(D2,w7);float fb=min(M1,1.-M1);float Vg=(j9*fb*fb+x7)*fb;float Ed=min(Ug,Vg*.9999);float w3;if(j9==.0){w3=Ed/x7;}else{float U=1./j9;float b=x7*U,Y1=-Ed*U;float y7=(-1./3.)*b,z7=.5*Y1;float Fd=z7*z7-y7*y7*y7;if(Fd<.0){float k9=sqrt(y7);float k1=acos(z7/(k9*k9*k9));w3=-2.*k9*cos(k1*(1./3.)+(-z4*2./3.));}else{float E=pow(abs(z7)+sqrt(Fd),1./3.);if(z7<.0) E=-E;w3=E!=.0?E+y7/E:.0;}}w3=abs(w3);e t0011=M1+Bd(-w3,-w3,w3,w3);e Gd=(E.xyxy*t0011+2.*J.xyxy)*t0011+D2.xyxy;X E2=eb(D0,H0,M0,V0);c Wg=t0011.x<1e-3?E2[0]:Gd.xy;c Xg=t0011.z>1.-1e-3?E2[1]:Gd.zw;return acos(h9(Wg,Xg));}f float l9(float l,float b){l=b<.0?-l:l;b=abs(b);return l>.0?(l<b?l/b:1.):.0;}float Yg(c D0,c H0,c M0,c V0,p1(float) gb){c Hd=V0-D0;float Id=length(V0-D0);if(Id==.0){gb=.5;return.0;}c N1=r7(-Hd.y,Hd.x)/Id;float Jd=dot(N1,M0-D0);float g5=dot(N1,H0-D0);float h5=g5-Jd;
#if 0
float l=3.*h5;float Kd=h5+g5;float Y1=g5;float N2=sqrt(max(h5*h5+Jd*g5,.0));if(Kd<.0) N2=-N2;N2+=Kd;c A7=r7(l9(N2,l),l9(Y1,N2));c y6=3.*(A7*(A7*(A7*h5-(g5+h5))+g5));y6=abs(y6);gb=y6.x>y6.y?A7.x:A7.y;return max(y6.x,y6.y);
#else
float Ld=3.*h5;float J=-g5-h5;float D2=g5;float t=.5;for(int P0=0;P0<3;++P0){float Md=Ld*t;t=l9(Md*t-D2,2.*(Md+J));}gb=t;return abs(t*(t*(t*Ld+3.*J)+3.*D2));
#endif
}