#define f7 -2.
#define Fc -1.5
#define Gc .25
#define C8 1e3
#define Hc (C8*C8)
#ifdef VERTEX
R3 Yb(a3,xf,DC);
#ifdef ENABLE_FEATHER
f6(a3,d7,QC);
#endif
S3 A4 F4(Ac,Rf,MB);J5(vb,Be,TC);K5(wb,Ce,PB);F4(Bc,Sf,XC);B4
#endif
#if defined(ENABLE_FEATHER)||defined(ATLAS_BLIT)
X3(d7,T9)
#endif
#ifdef FRAGMENT
B3 X2(a3,Cc,DD);
#if defined(ENABLE_FEATHER)||defined(ATLAS_BLIT)
f6(a3,d7,QC);
#endif
#ifdef ATLAS_BLIT
k5(a3,Dc,UC);
#endif
X2(Z4,T3,AC);
#if defined(RENDER_MODE_MSAA)&&defined(ENABLE_ADVANCED_BLEND)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
g7(LD);
#endif
C3 X3(Cc,Bb)
#ifdef ATLAS_BLIT
X3(Dc,I9)
#endif
a5 U3(R5)c5
#endif
#ifdef FRAGMENT
e bool Q5(g J){return J.y>=.0;}e bool Q5(D J){return J.y>=.0;}
#endif
#if defined(FRAGMENT)&&defined(ENABLE_FEATHER)
e bool Db(g J){return J.x<Fc;}e bool Eb(g J){return J.y<Fc;}
#endif
#ifdef VERTEX
g Ic(float ma,c D8,float F1){c g6=(1.-D8*abs(F1))*.5;float Y3,l5;if(abs(ma-T6)<1./C8){Y3=.0;l5=.0;}else{float na=tan(ma);Y3=sign(T6-ma)/max(abs(na),1./Hc);l5=Y3>=.0?g6.y-(1.-g6.x)*na:g6.y+g6.x*na;}g J;J.x=max(g6.x,.0)+Gc;J.y=-g6.y+f7;J.z=Y3;J.w=l5;return J;}
#endif
#ifdef ENABLE_FEATHER
e d d8(g J F3){d Y3=J.z;d l5=max(J.w,.0);d h6=Y3>=.0?g5(l5):.0;if(abs(Y3)<C8){d x=abs(J.x)-Gc;d y=-J.y+f7;d V2=(y-l5)*0.5984134206;i t=l5+V2*B0(0.20888568955,0.62665706865,1.04442844776,1.46219982687);i u=t*-Y3+(y*Y3+x);i Tf=B0(g5(u[0]),g5(u[1]),g5(u[2]),g5(u[3]));i Jc=t*5.09593080173+-2.54796540086;i Uf=exp2(-Jc*Jc);h6+=dot(Tf,Uf)*V2;}return h6*sign(J.x);}e d v4(g J F3){float h6=1.;float Vf=(1.-f7)+J.x;h6-=g5(Vf);float Wf=1.-J.y;h6-=g5(Wf);return h6;}
#endif
#if defined(VERTEX)&&defined(DRAW_PATH)
e U m5(int Kc){return U(Kc&((1<<pc)-1),Kc>>pc);}e float Lc(Z Y0,c Xf){c e2=Z0(Y0,Xf);return(abs(e2.x)+abs(e2.y))*(1./dot(e2,e2));}e bool p9(g h7,g oa,int T,e1(uint) c3,e1(c) Yf
#ifndef RENDER_MODE_MSAA
,e1(g) N1
#else
,e1(X) i7
#endif
i6){int E8=int(h7.x);float F1=h7.y;float pa=h7.z;int Mc=floatBitsToInt(h7.w)>>2;int j7=floatBitsToInt(h7.w)&3;int qa=min(E8,Mc-1);int G4=T*Mc+qa;C4 n5=v1(DC,m5(G4));uint e0=f5(n5.w);uint F8=max(e0&wc,1u);Q ra=P0(XC,F8-1u);c Nc=uintBitsToFloat(ra.xy);c3=ra.z&0xffffu;uint Oc=ra.w;Z Y0=j2(uintBitsToFloat(P0(MB,c3*4u)));Q H4=P0(MB,c3*4u+1u);c c2=uintBitsToFloat(H4.xy);float H2=uintBitsToFloat(H4.z);float I2=uintBitsToFloat(H4.w);uint Pc=e0&D3;if(Pc!=0u){E8=int(oa.x);F1=oa.y;pa=oa.z;}if(E8!=qa){int Qc=G4+E8-qa;C4 Rc=v1(DC,m5(Qc));if((f5(Rc.w)&(D3|0xffffu))!=(e0&(D3|0xffffu))){bool Zf=H2==.0||Nc.x!=.0;if(Zf){G4=int(Oc);n5=v1(DC,m5(G4));}}else{G4=Qc;n5=Rc;}e0=(f5(n5.w)&~D3)|Pc;}float h1;
#ifdef ENABLE_FEATHER
float k7;float x1;if((e0&W3)==y8&&j7==B8){uint Sc=f5(n5.z);float Z3=float(Sc&0xffffu);float f2=float(Sc>>16);U G8=U(-Z3-1.,f2-Z3+1.);if((e0&D3)!=0u)G8=-G8;C4 Tc=v1(DC,m5(G4+G8.x));C4 sa=v1(DC,m5(G4+G8.y));if((f5(sa.w)&(D3|0xffffu))!=(f5(Tc.w)&(D3|0xffffu))){sa=v1(DC,m5(int(Oc)));}k7=U5(Tc.z);float Uc=U5(sa.z);x1=Uc-k7;if(abs(x1)>A3)x1-=p8*sign(x1);float ta=f2+1.-float(qc);float Vc=clamp(round(abs(x1)/A3*ta),1.,ta-1.);float l7=ta-Vc;if(Z3<=l7){x1=-(A3*sign(x1)-x1);f2=l7;if(Z3==l7)F1=-F1;}else if(Z3==l7+1.){Z3=.0;f2=.0;F1=.0;}else{Z3-=l7+2.;f2=Vc;}if(Z3==f2){h1=Uc;}else{h1=k7+x1*(Z3/f2);}}else
#endif
{h1=U5(n5.z);}c W2=c(sin(h1),-cos(h1));c Wc=U5(n5.xy);c H8=c(0,0);if(I2!=.0){I2=max(I2,(ea/3.)/length(Z0(Y0,W2)));}if(H2!=.0){F1*=sign(determinant(Y0));if((e0&A8)!=0u)F1=min(F1,.0);if((e0&vc)!=0u)F1=max(F1,.0);float I4=I2!=.0?I2:Lc(Y0,W2)*l4;d Xc=1.;if(I4>H2&&I2==.0){Xc=m4(H2)/m4(I4);H2=I4;}c o5=W2*(H2+I4);
#ifndef RENDER_MODE_MSAA
float x=F1*(H2+I4);N1.xy=(1./(I4*2.))*(c(x,-x)+H2)+.5;N1.zw=J6(.0);
#endif
uint ua=e0&W3;if(ua>x8){int m7=2;if((e0&fa)==0u)m7=-m7;if((e0&D3)!=0u)m7=-m7;U ag=m5(G4+m7);C4 bg=v1(DC,ag);float cg=U5(bg.z);float n7=abs(cg-h1);if(n7>A3)n7=p8-n7;bool I8=(e0&fa)!=0u;bool dg=(e0&A8)!=0u;float Yc=n7*(I8==dg?-.5:.5)+h1;c J8=c(sin(Yc),-cos(Yc));float va=Lc(Y0,J8);float o7=cos(n7*.5);float wa;if((ua==sf)||(ua==tf&&o7>=.25)){float eg=(e0&z8)!=0u?1.:.25;wa=H2*(1./max(o7,eg));}else{wa=H2*o7+va*.5;}float xa=wa+va*l4;if((e0&uc)!=0u){float Zc=H2+I4;float fg=I4*.125;if(Zc<=xa*o7+fg){float gg=Zc*(1./o7);o5=J8*gg;}else{c ya=J8*xa;c hg=c(dot(o5,o5),dot(ya,ya));o5=Z0(hg,inverse(Z(o5,ya)));}}c ig=abs(F1)*o5;float ad=(xa-dot(ig,J8))/(va*(l4*2.));
#ifndef RENDER_MODE_MSAA
if((e0&A8)!=0u)N1.y=ad;else N1.x=ad;
#endif
}
#ifndef RENDER_MODE_MSAA
N1.xy*=Xc;N1.y=max(N1.y,1e-4);if(I2!=.0){N1.x=f7-N1.x;}
#endif
H8=Z0(Y0,F1*o5);if(j7!=B8)return false;}else{
#ifndef RENDER_MODE_MSAA
N1=g(pa,-1.,.0,.0);
#ifdef ENABLE_FEATHER
if(I2!=.0){N1.y=f7;N1.z=Hc;N1.w=pa;if((e0&W3)==y8&&j7==B8){if(x1<.0){k7+=x1;x1=-x1;}float a4=h1-k7;a4=mod(a4+T6,p8)-T6;a4=clamp(a4,.0,x1);if(a4>x1*.5){a4=x1-a4;}c D8=c(sin(a4),cos(a4));
#if 0
float O1=1.+.33*log2(T6/(A3-min(x1,A3-A3/16.)));g jg=Ic(x1,D8,.5*(O1/3.));float kg=d8(jg g1);float lg=bc(kg);float mg=(.5-lg)*(ea*2.);float ng=O1/max(mg,O1);F1*=ng;
#endif
N1=Ic(x1,D8,F1);}H8=Z0(Y0,(F1*I2)*W2);}else
#endif
{H8=sign(Z0(F1*W2,inverse(Y0)))*l4;}if(bool(e0&D3)!=bool(e0&uf)){N1*=g(-1.,+1.,+1.,+1.);}
#endif
if(j7==yc)Wc=Nc;if((e0&tc)!=0u&&j7!=xc){return false;}}Yf=Z0(Y0,Wc)+H8+c2;
#ifdef RENDER_MODE_MSAA
Q J4=P0(MB,c3*4u+2u);i7=i2(J4.x);
#else
N1.xy=mix(N1.xy,c(1.,-1.),bf(k.og!=0u));
#endif
return true;}
#endif
#if defined(VERTEX)&&defined(DRAW_INTERIOR_TRIANGLES)
e c tb(V j6,e1(uint) c3
#ifdef RENDER_MODE_MSAA
,e1(X) i7
#else
,e1(d) pg
#endif
i6){c3=floatBitsToUint(j6.z)&0xffffu;
#ifdef RENDER_MODE_MSAA
Q J4=P0(MB,c3*4u+2u);i7=i2(J4.x);
#else
pg=U9(floatBitsToInt(j6.z)>>16);
#endif
c k6=j6.xy;Z Y0=j2(uintBitsToFloat(P0(MB,c3*4u)));Q H4=P0(MB,c3*4u+1u);c c2=uintBitsToFloat(H4.xy);k6=Z0(Y0,k6)+c2;return k6;}
#endif
#if defined(VERTEX)&&defined(ATLAS_BLIT)
e c sb(V j6,e1(uint) c3,
#ifdef RENDER_MODE_MSAA
e1(X) i7,
#endif
e1(c) qg i6){c3=floatBitsToUint(j6.z)&0xffffu;Q J4=P0(MB,c3*4u+2u);
#ifdef RENDER_MODE_MSAA
i7=i2(J4.x);
#endif
c k6=j6.xy;V p7=uintBitsToFloat(J4.yzw);qg=(k6*p7.x+p7.yz)*k.rg;return k6;}
#endif
e d K8(d V1,d G1,d o3){return(G1-V1)/max(1.-V1*o3,n9);}e uint L8(W0 l6,uint sg){uint za=(l6.y>>e6)*(sg<<e6)+((l6.x>>e6)<<(e6<<1));za+=((l6.x&0x1cu)<<e6)+((l6.y&0x1cu)<<2);za+=((l6.y&0x3u)<<2)+(l6.x&0x3u);return za;}
#ifdef RENDER_MODE_CLOCKWISE_ATOMIC
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
#define d5 o2
#define V3(p5) l1=p5;l3
#else
#define d5 m1
#define V3(p5) v0(g0,p5);U1;
#endif
e d Aa(uint tg){return U9(int((tg&ja)-j5))*ha;}e uint q7(d n){return uint(n*Bf+.5);}
#endif
