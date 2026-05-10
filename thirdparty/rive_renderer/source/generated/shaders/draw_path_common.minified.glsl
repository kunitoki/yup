#define f7 -2.
#define Ac -1.5
#define Bc .25
#define C8 1e3
#define Cc (C8*C8)
#ifdef VERTEX
P3 Tb(X2,sf,BC);
#ifdef ENABLE_FEATHER
e6(X2,d7,QC);
#endif
Q3 B4 G4(vc,Jf,LB);H5(qb,ve,UC);I5(rb,we,OB);G4(wc,Kf,XC);C4
#endif
#if defined(ENABLE_FEATHER)||defined(ATLAS_BLIT)
V3(d7,T9)
#endif
#ifdef FRAGMENT
y3 U2(X2,xc,DD);
#if defined(ENABLE_FEATHER)||defined(ATLAS_BLIT)
e6(X2,d7,QC);
#endif
#ifdef ATLAS_BLIT
j5(X2,yc,VC);
#endif
U2(Y4,R3,DC);
#if defined(RENDER_MODE_MSAA)&&defined(ENABLE_ADVANCED_BLEND)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
g7(JD);
#endif
z3 V3(xc,wb)
#ifdef ATLAS_BLIT
V3(yc,I9)
#endif
Z4 S3(R5)a5
#endif
#ifdef FRAGMENT
e bool P5(g J){return J.y>=.0;}e bool P5(D J){return J.y>=.0;}
#endif
#if defined(FRAGMENT)&&defined(ENABLE_FEATHER)
e bool yb(g J){return J.x<Ac;}e bool zb(g J){return J.y<Ac;}
#endif
#ifdef VERTEX
g Dc(float la,c D8,float G1){c f6=(1.-D8*abs(G1))*.5;float W3,k5;if(abs(la-T6)<1./C8){W3=.0;k5=.0;}else{float ma=tan(la);W3=sign(T6-la)/max(abs(ma),1./Cc);k5=W3>=.0?f6.y-(1.-f6.x)*ma:f6.y+f6.x*ma;}g J;J.x=max(f6.x,.0)+Bc;J.y=-f6.y+f7;J.z=W3;J.w=k5;return J;}
#endif
#ifdef ENABLE_FEATHER
e d d8(g J C3){d W3=J.z;d k5=max(J.w,.0);d g6=W3>=.0?f5(k5):.0;if(abs(W3)<C8){d x=abs(J.x)-Bc;d y=-J.y+f7;d S2=(y-k5)*0.5984134206;i t=k5+S2*B0(0.20888568955,0.62665706865,1.04442844776,1.46219982687);i u=t*-W3+(y*W3+x);i Lf=B0(f5(u[0]),f5(u[1]),f5(u[2]),f5(u[3]));i Ec=t*5.09593080173+-2.54796540086;i Mf=exp2(-Ec*Ec);g6+=dot(Lf,Mf)*S2;}return g6*sign(J.x);}e d w4(g J C3){float g6=1.;float Nf=(1.-f7)+J.x;g6-=f5(Nf);float Of=1.-J.y;g6-=f5(Of);return g6;}
#endif
#if defined(VERTEX)&&defined(DRAW_PATH)
e U l5(int Fc){return U(Fc&((1<<kc)-1),Fc>>kc);}e float Gc(a0 Y0,c Pf){c e2=Z0(Y0,Pf);return(abs(e2.x)+abs(e2.y))*(1./dot(e2,e2));}e bool p9(g h7,g na,int S,g1(uint)Y2,g1(c)Qf
#ifndef RENDER_MODE_MSAA
,g1(g)N1
#else
,g1(X)i7
#endif
h6){int E8=int(h7.x);float G1=h7.y;float oa=h7.z;int Hc=floatBitsToInt(h7.w)>>2;int j7=floatBitsToInt(h7.w)&3;int pa=min(E8,Hc-1);int H4=S*Hc+pa;D4 m5=F1(BC,l5(H4));uint f0=e5(m5.w);uint F8=max(f0&rc,1u);Q qa=P0(XC,F8-1u);c Ic=uintBitsToFloat(qa.xy);Y2=qa.z&0xffffu;uint Jc=qa.w;a0 Y0=j2(uintBitsToFloat(P0(LB,Y2*4u)));Q I4=P0(LB,Y2*4u+1u);c a2=uintBitsToFloat(I4.xy);float G2=uintBitsToFloat(I4.z);float H2=uintBitsToFloat(I4.w);uint Kc=f0&A3;if(Kc!=0u){E8=int(na.x);G1=na.y;oa=na.z;}if(E8!=pa){int Lc=H4+E8-pa;D4 Mc=F1(BC,l5(Lc));if((e5(Mc.w)&(A3|0xffffu))!=(f0&(A3|0xffffu))){bool Rf=G2==.0||Ic.x!=.0;if(Rf){H4=int(Jc);m5=F1(BC,l5(H4));}}else{H4=Lc;m5=Mc;}f0=(e5(m5.w)&~A3)|Kc;}float j1;
#ifdef ENABLE_FEATHER
float k7;float x1;if((f0&U3)==y8&&j7==B8){uint Nc=e5(m5.z);float X3=float(Nc&0xffffu);float f2=float(Nc>>16);U G8=U(-X3-1.,f2-X3+1.);if((f0&A3)!=0u)G8=-G8;D4 Oc=F1(BC,l5(H4+G8.x));D4 ra=F1(BC,l5(H4+G8.y));if((e5(ra.w)&(A3|0xffffu))!=(e5(Oc.w)&(A3|0xffffu))){ra=F1(BC,l5(int(Jc)));}k7=U5(Oc.z);float Pc=U5(ra.z);x1=Pc-k7;if(abs(x1)>x3)x1-=p8*sign(x1);float sa=f2+1.-float(lc);float Qc=clamp(round(abs(x1)/x3*sa),1.,sa-1.);float l7=sa-Qc;if(X3<=l7){x1=-(x3*sign(x1)-x1);f2=l7;if(X3==l7)G1=-G1;}else if(X3==l7+1.){X3=.0;f2=.0;G1=.0;}else{X3-=l7+2.;f2=Qc;}if(X3==f2){j1=Pc;}else{j1=k7+x1*(X3/f2);}}else
#endif
{j1=U5(m5.z);}c T2=c(sin(j1),-cos(j1));c Rc=U5(m5.xy);c H8=c(0,0);if(H2!=.0){H2=max(H2,(ea/3.)/length(Z0(Y0,T2)));}if(G2!=.0){G1*=sign(determinant(Y0));if((f0&A8)!=0u)G1=min(G1,.0);if((f0&qc)!=0u)G1=max(G1,.0);float J4=H2!=.0?H2:Gc(Y0,T2)*k4;d Sc=1.;if(J4>G2&&H2==.0){Sc=l4(G2)/l4(J4);G2=J4;}c n5=T2*(G2+J4);
#ifndef RENDER_MODE_MSAA
float x=G1*(G2+J4);N1.xy=(1./(J4*2.))*(c(x,-x)+G2)+.5;N1.zw=J6(.0);
#endif
uint ta=f0&U3;if(ta>x8){int m7=2;if((f0&fa)==0u)m7=-m7;if((f0&A3)!=0u)m7=-m7;U Sf=l5(H4+m7);D4 Tf=F1(BC,Sf);float Uf=U5(Tf.z);float n7=abs(Uf-j1);if(n7>x3)n7=p8-n7;bool I8=(f0&fa)!=0u;bool Vf=(f0&A8)!=0u;float Tc=n7*(I8==Vf?-.5:.5)+j1;c J8=c(sin(Tc),-cos(Tc));float ua=Gc(Y0,J8);float o7=cos(n7*.5);float va;if((ta==nf)||(ta==of&&o7>=.25)){float Wf=(f0&z8)!=0u?1.:.25;va=G2*(1./max(o7,Wf));}else{va=G2*o7+ua*.5;}float wa=va+ua*k4;if((f0&pc)!=0u){float Uc=G2+J4;float Xf=J4*.125;if(Uc<=wa*o7+Xf){float Yf=Uc*(1./o7);n5=J8*Yf;}else{c xa=J8*wa;c Zf=c(dot(n5,n5),dot(xa,xa));n5=Z0(Zf,inverse(a0(n5,xa)));}}c ag=abs(G1)*n5;float Vc=(wa-dot(ag,J8))/(ua*(k4*2.));
#ifndef RENDER_MODE_MSAA
if((f0&A8)!=0u)N1.y=Vc;else N1.x=Vc;
#endif
}
#ifndef RENDER_MODE_MSAA
N1.xy*=Sc;N1.y=max(N1.y,1e-4);if(H2!=.0){N1.x=f7-N1.x;}
#endif
H8=Z0(Y0,G1*n5);if(j7!=B8)return false;}else{
#ifndef RENDER_MODE_MSAA
N1=g(oa,-1.,.0,.0);
#ifdef ENABLE_FEATHER
if(H2!=.0){N1.y=f7;N1.z=Cc;N1.w=oa;if((f0&U3)==y8&&j7==B8){if(x1<.0){k7+=x1;x1=-x1;}float Y3=j1-k7;Y3=mod(Y3+T6,p8)-T6;Y3=clamp(Y3,.0,x1);if(Y3>x1*.5){Y3=x1-Y3;}c D8=c(sin(Y3),cos(Y3));
#if 0
float O1=1.+.33*log2(T6/(x3-min(x1,x3-x3/16.)));g bg=Dc(x1,D8,.5*(O1/3.));float cg=d8(bg i1);float dg=Wb(cg);float eg=(.5-dg)*(ea*2.);float fg=O1/max(eg,O1);G1*=fg;
#endif
N1=Dc(x1,D8,G1);}H8=Z0(Y0,(G1*H2)*T2);}else
#endif
{H8=sign(Z0(G1*T2,inverse(Y0)))*k4;}if(bool(f0&A3)!=bool(f0&pf)){N1*=g(-1.,+1.,+1.,+1.);}
#endif
if(j7==tc)Rc=Ic;if((f0&oc)!=0u&&j7!=sc){return false;}}Qf=Z0(Y0,Rc)+H8+a2;
#ifdef RENDER_MODE_MSAA
Q K4=P0(LB,Y2*4u+2u);i7=i2(K4.x);
#else
N1.xy=mix(N1.xy,c(1.,-1.),Ve(k.gg!=0u));
#endif
return true;}
#endif
#if defined(VERTEX)&&defined(DRAW_INTERIOR_TRIANGLES)
e c ob(V i6,g1(uint)Y2
#ifdef RENDER_MODE_MSAA
,g1(X)i7
#else
,g1(d)hg
#endif
h6){Y2=floatBitsToUint(i6.z)&0xffffu;
#ifdef RENDER_MODE_MSAA
Q K4=P0(LB,Y2*4u+2u);i7=i2(K4.x);
#else
hg=U9(floatBitsToInt(i6.z)>>16);
#endif
c j6=i6.xy;a0 Y0=j2(uintBitsToFloat(P0(LB,Y2*4u)));Q I4=P0(LB,Y2*4u+1u);c a2=uintBitsToFloat(I4.xy);j6=Z0(Y0,j6)+a2;return j6;}
#endif
#if defined(VERTEX)&&defined(ATLAS_BLIT)
e c nb(V i6,g1(uint)Y2,
#ifdef RENDER_MODE_MSAA
g1(X)i7,
#endif
g1(c)ig h6){Y2=floatBitsToUint(i6.z)&0xffffu;Q K4=P0(LB,Y2*4u+2u);
#ifdef RENDER_MODE_MSAA
i7=i2(K4.x);
#endif
c j6=i6.xy;V p7=uintBitsToFloat(K4.yzw);ig=(j6*p7.x+p7.yz)*k.jg;return j6;}
#endif
e d K8(d V1,d H1,d l3){return(H1-V1)/max(1.-V1*l3,n9);}
#ifdef RENDER_MODE_CLOCKWISE_ATOMIC
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#define c5 o2
#define T3(o5) m1=o5;i3
#else
#define c5 v1
#define T3(o5) C0(j0,o5);c2;
#endif
e uint ya(W0 k6,uint kg){uint za=(k6.y>>5u)*(kg<<5u)+(k6.x>>5u)*(32u<<5u);za+=((k6.x&0x1fu)>>2u)*(32u<<2u)+((k6.y&0x1fu)>>2u)*(4u<<2u);za+=(k6.y&0x3u)*4u+(k6.x&0x3u);return za;}e d Aa(uint lg){return U9(int((lg&ja)-i5))*ha;}e uint q7(d n){return uint(n*wf+.5);}
#endif
