#define e6 -2.
#define ib -1.5
#define jb .25
#define p7 1e3
#define kb (p7*p7)
#ifdef VERTEX
P2 wa(U2,wd,BC);
#ifdef ENABLE_FEATHER
l5(U2,Z5,JC);
#endif
Q2 E3 O3(Va,Td,JB);g4(l8,W9,DC);h4(m8,X9,KB);O3(Wa,Ud,QC);F3
#endif
#if defined(ENABLE_FEATHER)||defined(ATLAS_BLIT)
P3(Z5,M8)
#endif
#ifdef FRAGMENT
R2 C2(U2,Xa,MC);
#if defined(ENABLE_FEATHER)||defined(ATLAS_BLIT)
l5(U2,Z5,JC);
#endif
#ifdef ATLAS_BLIT
y4(Y5,Ya,ND);
#endif
C2(Y5,W8,UB);
#if defined(RENDER_MODE_MSAA)&&defined(ENABLE_ADVANCED_BLEND)
C2(U2,Za,PC);
#endif
S2 P3(Xa,r8)
#ifdef ATLAS_BLIT
P3(Ya,Vd)
#endif
p4 G3(X8,B3)q4
#endif
#ifdef FRAGMENT
d bool U6(f O){return O.y>=.0;}d bool U6(G O){return O.y>=.0;}
#endif
#if defined(FRAGMENT)&&defined(ENABLE_FEATHER)
d bool w8(f O){return O.x<ib;}d bool S6(f O){return O.y<ib;}
#endif
#ifdef VERTEX
f lb(float f9,c q7,float m1){c m5=(1.-q7*abs(m1))*.5;float f3,z4;if(abs(f9-S5)<1./p7){f3=.0;z4=.0;}else{float g9=tan(f9);f3=sign(S5-f9)/max(abs(g9),1./kb);z4=f3>=.0?m5.y-(1.-m5.x)*g9:m5.y+m5.x*g9;}f O;O.x=max(m5.x,.0)+jb;O.y=-m5.y+e6;O.z=f3;O.w=z4;return O;}
#endif
#ifdef ENABLE_FEATHER
d g S4(f O n5){g f3=O.z;g z4=max(O.w,.0);g o5=f3>=.0?J3(z4):.0;if(abs(f3)<p7){g x=abs(O.x)-jb;g y=-O.y+e6;g A2=(y-z4)*0.5984134206;i t=z4+A2*E1(0.20888568955,0.62665706865,1.04442844776,1.46219982687);i u=t*-f3+(y*f3+x);i Wd=E1(J3(u[0]),J3(u[1]),J3(u[2]),J3(u[3]));i mb=t*5.09593080173+-2.54796540086;i Xd=exp2(-mb*mb);o5+=dot(Wd,Xd)*A2;}return o5*sign(O.x);}d g R6(f O n5){float o5=1.;float Yd=(1.-e6)+O.x;o5-=J3(Yd);float Zd=1.-O.y;o5-=J3(Zd);return o5;}
#endif
#if defined(FRAGMENT)&&defined(ATLAS_BLIT)
d g W6(c h9,c U4 n5){c i9=round(h9);i O=p5(ND,Vd,i9,U4);O=E1(Z4(O.x),Z4(O.y),Z4(O.z),Z4(O.w));O.xw=mix(O.xw,O.yz,v1(h9.x+.5-i9.x));O.x=mix(O.w,O.x,v1(h9.y+.5-i9.y));return J3(O.x);}
#endif
#if defined(VERTEX)&&defined(DRAW_PATH)
d c0 A4(int nb){return c0(nb&((1<<La)-1),nb>>La);}d float ob(S D0,c ae){c M1=C0(D0,ae);return(abs(M1.x)+abs(M1.y))*(1./dot(M1,M1));}d bool E6(f f6,f j9,int K,k1(uint)D2,k1(c)be
#ifndef RENDER_MODE_MSAA
,k1(f)n1
#else
,k1(a0)g6
#endif
q5){int r7=int(f6.x);float m1=f6.y;float k9=f6.z;int pb=floatBitsToInt(f6.w)>>2;int h6=floatBitsToInt(f6.w)&3;int l9=min(r7,pb-1);int Q3=K*pb+l9;H3 B4=d1(BC,A4(Q3));uint Y=v4(B4.w);M m9=w0(QC,Aa(Y));c qb=uintBitsToFloat(m9.xy);D2=m9.z&0xffffu;uint rb=m9.w;S D0=D1(uintBitsToFloat(w0(JB,D2*4u)));M R3=w0(JB,D2*4u+1u);c S0=uintBitsToFloat(R3.xy);float k2=uintBitsToFloat(R3.z);float l2=uintBitsToFloat(R3.w);uint sb=Y&T2;if(sb!=0u){r7=int(j9.x);m1=j9.y;k9=j9.z;}if(r7!=l9){int tb=Q3+r7-l9;H3 ub=d1(BC,A4(tb));if((v4(ub.w)&(T2|0xffffu))!=(Y&(T2|0xffffu))){bool ce=k2==.0||qb.x!=.0;if(ce){Q3=int(rb);B4=d1(BC,A4(Q3));}}else{Q3=tb;B4=ub;}Y=(v4(B4.w)&~T2)|sb;}float O0;
#ifdef ENABLE_FEATHER
float i6;float e1;if((Y&a3)==j7&&h6==m7){uint vb=v4(B4.z);float g3=float(vb&0xffffu);float O1=float(vb>>16);c0 v7=c0(-g3-1.,O1-g3+1.);if((Y&T2)!=0u)v7=-v7;H3 wb=d1(BC,A4(Q3+v7.x));H3 n9=d1(BC,A4(Q3+v7.y));if((v4(n9.w)&(T2|0xffffu))!=(v4(wb.w)&(T2|0xffffu))){n9=d1(BC,A4(int(rb)));}i6=Y4(wb.z);float xb=Y4(n9.z);e1=xb-i6;if(abs(e1)>O2)e1-=e7*sign(e1);float o9=O1+1.-float(Ma);float yb=clamp(round(abs(e1)/O2*o9),1.,o9-1.);float j6=o9-yb;if(g3<=j6){e1=-(O2*sign(e1)-e1);O1=j6;if(g3==j6)m1=-m1;}else if(g3==j6+1.){g3=.0;O1=.0;m1=.0;}else{g3-=j6+2.;O1=yb;}if(g3==O1){O0=xb;}else{O0=i6+e1*(g3/O1);}}else
#endif
{O0=Y4(B4.z);}c B2=c(sin(O0),-cos(O0));c zb=Y4(B4.xy);c w7=c(0,0);if(l2!=.0){l2=max(l2,(U8/3.)/length(C0(D0,B2)));}if(k2!=.0){m1*=sign(determinant(D0));if((Y&l7)!=0u)m1=min(m1,.0);if((Y&Ra)!=0u)m1=max(m1,.0);float S3=l2!=.0?l2:ob(D0,B2)*q3;g Ab=1.;if(S3>k2&&l2==.0){Ab=d4(k2)/d4(S3);k2=S3;}c C4=B2*(k2+S3);
#ifndef RENDER_MODE_MSAA
float x=m1*(k2+S3);n1.xy=(1./(S3*2.))*(c(x,-x)+k2)+.5;n1.zw=J5(.0);
#endif
uint p9=Y&a3;if(p9>i7){int k6=2;if((Y&V8)==0u)k6=-k6;if((Y&T2)!=0u)k6=-k6;c0 de=A4(Q3+k6);H3 ee=d1(BC,de);float fe=Y4(ee.z);float l6=abs(fe-O0);if(l6>O2)l6=e7-l6;bool x7=(Y&V8)!=0u;bool ge=(Y&l7)!=0u;float Bb=l6*(x7==ge?-.5:.5)+O0;c y7=c(sin(Bb),-cos(Bb));float q9=ob(D0,y7);float m6=cos(l6*.5);float r9;if((p9==pd)||(p9==qd&&m6>=.25)){float he=(Y&k7)!=0u?1.:.25;r9=k2*(1./max(m6,he));}else{r9=k2*m6+q9*.5;}float v9=r9+q9*q3;if((Y&Qa)!=0u){float Cb=k2+S3;float ie=S3*.125;if(Cb<=v9*m6+ie){float je=Cb*(1./m6);C4=y7*je;}else{c w9=y7*v9;c ke=c(dot(C4,C4),dot(w9,w9));C4=C0(ke,inverse(S(C4,w9)));}}c le=abs(m1)*C4;float Db=(v9-dot(le,y7))/(q9*(q3*2.));
#ifndef RENDER_MODE_MSAA
if((Y&l7)!=0u)n1.y=Db;else n1.x=Db;
#endif
}
#ifndef RENDER_MODE_MSAA
n1.xy*=Ab;n1.y=max(n1.y,1e-4);if(l2!=.0){n1.x=e6-n1.x;}
#endif
w7=C0(D0,m1*C4);if(h6!=m7)return false;}else{
#ifndef RENDER_MODE_MSAA
n1=f(k9,-1.,.0,.0);
#ifdef ENABLE_FEATHER
if(l2!=.0){n1.y=e6;n1.z=kb;n1.w=k9;if((Y&a3)==j7&&h6==m7){if(e1<.0){i6+=e1;e1=-e1;}float h3=O0-i6;h3=mod(h3+S5,e7)-S5;h3=clamp(h3,.0,e1);if(h3>e1*.5){h3=e1-h3;}c q7=c(sin(h3),cos(h3));
#if 0
float y1=1.+.33*log2(S5/(O2-min(e1,O2-O2/16.)));f me=lb(e1,q7,.5*(y1/3.));float ne=S4(me x1);float oe=Z4(ne);float pe=(.5-oe)*(U8*2.);float qe=y1/max(pe,y1);m1*=qe;
#endif
n1=lb(e1,q7,m1);}w7=C0(D0,(m1*l2)*B2);}else
#endif
{w7=sign(C0(m1*B2,inverse(D0)))*q3;}if(bool(Y&T2)!=bool(Y&rd)){n1.x=-n1.x;}
#endif
if(h6==Ta)zb=qb;if((Y&Pa)!=0u&&h6!=Sa){return false;}}be=C0(D0,zb)+w7+S0;
#ifdef RENDER_MODE_MSAA
M T3=w0(JB,D2*4u+2u);g6=Q1(T3.x);
#else
n1.xy=mix(n1.xy,c(1.,-1.),ed(q.jd!=0u));
#endif
return true;}
#endif
#if defined(VERTEX)&&defined(DRAW_INTERIOR_TRIANGLES)
d c c8(Z r5,k1(uint)D2
#ifdef RENDER_MODE_MSAA
,k1(a0)g6
#else
,k1(g)re
#endif
q5){D2=floatBitsToUint(r5.z)&0xffffu;
#ifdef RENDER_MODE_MSAA
M T3=w0(JB,D2*4u+2u);g6=Q1(T3.x);
#else
re=N8(floatBitsToInt(r5.z)>>16);
#endif
c v5=r5.xy;S D0=D1(uintBitsToFloat(w0(JB,D2*4u)));M R3=w0(JB,D2*4u+1u);c S0=uintBitsToFloat(R3.xy);v5=C0(D0,v5)+S0;return v5;}
#endif
#if defined(VERTEX)&&defined(ATLAS_BLIT)
d c a8(Z r5,k1(uint)D2,
#ifdef RENDER_MODE_MSAA
k1(a0)g6,
#endif
k1(c)se q5){D2=floatBitsToUint(r5.z)&0xffffu;M T3=w0(JB,D2*4u+2u);
#ifdef RENDER_MODE_MSAA
g6=Q1(T3.x);
#endif
c v5=r5.xy;Z n6=uintBitsToFloat(T3.yzw);se=v5*n6.x+n6.yz;return v5;}
#endif
