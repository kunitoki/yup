#ifndef Dc
#define Dc e
#endif
#ifndef Y6
#define Y6 c
#endif
f float x8(c k,c b){float Pf=dot(k,b);float Ec=dot(k,k)*dot(b,b);return(Ec==.0)?1.:clamp(Pf*inversesqrt(Ec),-1.,1.);}f void Qf(c z0,c C0,c J0,c P0,i1(c) B,i1(c) J,i1(c) p2){p2=C0-z0;c Z6=J0-C0;c y8=P0-z0;J=Z6-p2;B=-3.*Z6+y8;}f Y qa(c z0,c C0,c J0,c P0){Y t;t[0]=(any(notEqual(z0,C0))?C0:any(notEqual(C0,J0))?J0:P0)-z0;t[1]=P0-(any(notEqual(P0,J0))?J0:any(notEqual(J0,C0))?C0:z0);return t;}f float Rf(c z0,c C0,c J0,c P0,float C1,float Sf){c B,J,p2;Qf(z0,C0,J0,P0,B,J,p2);c a7=3.*(((B*C1)+2.*J)*C1+p2);float Fc=length(a7);if(Fc==.0){return.0;}a7*=1./Fc;float z8=2.*dot(B,a7);float c7=3.*(z8*C1+4.*dot(J,a7))*C1+6.*dot(p2,a7);float ra=min(C1,1.-C1);float Tf=(z8*ra*ra+c7)*ra;float Gc=min(Sf,Tf*.9999);float h3;if(z8==.0){h3=Gc/c7;}else{float R=1./z8;float b=c7*R,O1=-Gc*R;float d7=(-1./3.)*b,e7=.5*O1;float Hc=e7*e7-d7*d7*d7;if(Hc<.0){float A8=sqrt(d7);float x1=acos(e7/(A8*A8*A8));h3=-2.*A8*cos(x1*(1./3.)+(-i4*2./3.));}else{float B=pow(abs(e7)+sqrt(Hc),1./3.);if(e7<.0) B=-B;h3=B!=.0?B+d7/B:.0;}}h3=abs(h3);e t0011=C1+Dc(-h3,-h3,h3,h3);e Ic=(B.xyxy*t0011+2.*J.xyxy)*t0011+p2.xyxy;Y q2=qa(z0,C0,J0,P0);c Uf=t0011.x<1e-3?q2[0]:Ic.xy;c Vf=t0011.z>1.-1e-3?q2[1]:Ic.zw;return acos(x8(Uf,Vf));}f float B8(float k,float b){k=b<.0?-k:k;b=abs(b);return k>.0?(k<b?k/b:1.):.0;}float Wf(c z0,c C0,c J0,c P0,i1(float) sa){c Jc=P0-z0;float Kc=length(P0-z0);if(Kc==.0){sa=.5;return.0;}c O2=Y6(-Jc.y,Jc.x)/Kc;float Lc=dot(O2,J0-z0);float N4=dot(O2,C0-z0);float O4=N4-Lc;
#if 0
float k=3.*O4;float Mc=O4+N4;float O1=N4;float B2=sqrt(max(O4*O4+Lc*N4,.0));if(Mc<.0) B2=-B2;B2+=Mc;c f7=Y6(B8(B2,k),B8(O1,B2));c h6=3.*(f7*(f7*(f7*O4-(N4+O4))+N4));h6=abs(h6);sa=h6.x>h6.y?f7.x:f7.y;return max(h6.x,h6.y);
#else
float Nc=3.*O4;float J=-N4-O4;float p2=N4;float t=.5;for(int L0=0;L0<3;++L0){float Oc=Nc*t;t=B8(Oc*t-p2,2.*(Oc+J));}sa=t;return abs(t*(t*(t*Nc+3.*J)+3.*p2));
#endif
}