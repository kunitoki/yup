#ifndef fa
#define fa f
#endif
#ifndef J5
#define J5 c
#endif
d float C8(c k,c b){float Qc=dot(k,b);float ga=dot(k,k)*dot(b,b);return(ga==.0)?1.:clamp(Qc*inversesqrt(ga),-1.,1.);}d void Rc(c o0,c p0,c x0,c z0,k1(c)o,k1(c)r,k1(c)L1){L1=p0-o0;c K5=x0-p0;c X6=z0-o0;r=K5-L1;o=-3.*K5+X6;}d S D8(c o0,c p0,c x0,c z0){S t;t[0]=(any(notEqual(o0,p0))?p0:any(notEqual(p0,x0))?x0:z0)-o0;t[1]=z0-(any(notEqual(z0,x0))?x0:any(notEqual(x0,p0))?p0:o0);return t;}d float Sc(c o0,c p0,c x0,c z0,float c1,float Tc){c o,r,L1;Rc(o0,p0,x0,z0,o,r,L1);c L5=3.*(((o*c1)+2.*r)*c1+L1);float ha=length(L5);if(ha==.0){return.0;}L5*=1./ha;float Y6=2.*dot(o,L5);float M5=3.*(Y6*c1+4.*dot(r,L5))*c1+6.*dot(L1,L5);float E8=min(c1,1.-c1);float Uc=(Y6*E8*E8+M5)*E8;float ia=min(Tc,Uc*.9999);float A2;if(Y6==.0){A2=ia/M5;}else{float d0=1./Y6;float b=M5*d0,B0=-ia*d0;float N5=(-1./3.)*b,O5=.5*B0;float ja=O5*O5-N5*N5*N5;if(ja<.0){float Z6=sqrt(N5);float O0=acos(O5/(Z6*Z6*Z6));A2=-2.*Z6*cos(O0*(1./3.)+(-O2*2./3.));}else{float o=pow(abs(O5)+sqrt(ja),1./3.);if(O5<.0)o=-o;A2=o!=.0?o+N5/o:.0;}}A2=abs(A2);f t0011=c1+fa(-A2,-A2,A2,A2);f ka=(o.xyxy*t0011+2.*r.xyxy)*t0011+L1.xyxy;S d2=D8(o0,p0,x0,z0);c Vc=t0011.x<1e-3?d2[0]:ka.xy;c Wc=t0011.z>1.-1e-3?d2[1]:ka.zw;return acos(C8(Vc,Wc));}d float a7(float k,float b){k=b<.0?-k:k;b=abs(b);return k>.0?(k<b?k/b:1.):.0;}float Xc(c o0,c p0,c x0,c z0,k1(float)F8){c la=z0-o0;float ma=length(z0-o0);if(ma==.0){F8=.5;return.0;}c B2=J5(-la.y,la.x)/ma;float na=dot(B2,x0-o0);float C3=dot(B2,p0-o0);float D3=C3-na;
#if 0
float k=3.*D3;float oa=D3+C3;float B0=C3;float V1=sqrt(max(D3*D3+na*C3,.0));if(oa<.0)V1=-V1;V1+=oa;c P5=J5(a7(V1,k),a7(B0,V1));c X4=3.*(P5*(P5*(P5*D3-(C3+D3))+C3));X4=abs(X4);F8=X4.x>X4.y?P5.x:P5.y;return max(X4.x,X4.y);
#else
float pa=3.*D3;float r=-C3-D3;float L1=C3;float t=.5;for(int C=0;C<3;++C){float qa=pa*t;t=a7(qa*t-L1,2.*(qa+r));}F8=t;return abs(t*(t*(t*pa+3.*r)+3.*L1));
#endif
}