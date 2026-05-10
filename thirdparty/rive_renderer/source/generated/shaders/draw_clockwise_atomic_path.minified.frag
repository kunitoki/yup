#ifdef FRAGMENT
K1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
p0(P2,j0);
#endif
p0(Q2,e0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
md(d6,z6);
#endif
L1 K3 Ea(ga,Td,S0);L3 e void ph(U4(float)l3,d q0,uint T1,g1(uint)p1,g1(d)G3){
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
if(min(l3,q0)>=1.){return;}
#endif
d q;uint Wd=q7(abs(q0));p1=A7(S0,T1,k.W1|Wd);if(p1<k.W1){q=q0;
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
G3=q0;
#endif
}else{
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
if((p1&e7)!=0u){p1=A7(S0,T1,k.W1|e7|Wd);}
#endif
d V1=V5(p1&ja)*ha;d H1=max(V1,q0);q=K8(V1,H1,l3);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
G3=H1;
#endif
}l3*=q;}e void qh(U4(float)l3,d Q4,uint T1,g1(uint)p1,g1(d)G3){d q=.0;uint ab=q7(abs(Q4));p1=kd(S0,T1);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
if(min(l3,Q4)>=1.&&(p1<k.W1||p1>=(k.W1|i5))){return;}
#endif
if(p1<k.W1){uint Xd=k.W1|(i5+ab);uint c3=A7(S0,T1,Xd);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
p1=c3;
#endif
if(c3<=k.W1){q=Q4;
#ifdef DRAW_INTERIOR_TRIANGLES
q=min(q,1.);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
G3=q;
#endif
Q4=.0;}else if(c3<Xd){uint Yd=(c3&ja)-i5;d V1=V5(Yd)*ha;d H1=Q4;
#ifdef DRAW_INTERIOR_TRIANGLES
H1=min(H1,1.);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
G3=H1;
#endif
q=K8(V1,H1,l3);ab=Yd;Q4=V1;}}if(Q4>.0){uint Za=Fa(S0,T1,ab);d V1=Aa(Za);d H1=V1+Q4;V1=clamp(V1,.0,1.);H1=clamp(H1,.0,1.);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
G3=H1;
#endif
q+=(1.-q*l3)*K8(V1,H1,l3);}l3*=q;}c5(HB){B(k1,g);
#ifdef DRAW_INTERIOR_TRIANGLES
Y(l1,d);
#else
Y(I,z2);
#endif
B(z0,d);
#ifdef ENABLE_CLIPPING
B(S1,D);
#endif
#ifdef ENABLE_CLIP_RECT
B(N0,g);
#endif
#ifdef ENABLE_ADVANCED_BLEND
B(Y1,d);
#endif
B(a3,W0);B(i4,c);i F0=M7(k1,1. R2);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
i bb=H0(j0);
#endif
d q0=
#ifdef DRAW_INTERIOR_TRIANGLES
l1;
#else
Xa(I);
#endif
c y6=i4;
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
y6+=(bb.xy+bb.zw)*k.ng;
#endif
y6=floor(y6);uint N7=a3.y;uint T1=a3.x+ya(W0(y6),N7);d I1=1.;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d cb=d3(X4(N0));I1=min(cb,I1);}
#endif
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&S1.x!=.0){d db=H0(e0).x;I1=min(db,I1);}
#endif
I1=max(I1,.0);q0=clamp(q0,.0,I1);uint p1;float G3;
#ifndef DRAW_INTERIOR_TRIANGLES
if(P5(I)){ph(F0.w,q0,T1,p1,G3);}else
#endif
{qh(F0.w,q0,T1,p1,G3);}
#ifdef ENABLE_DITHER
d E5;if(ENABLE_DITHER){E5=Z9(T.xy,k.v3,k.w3);}
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
if(F0.w>.0){bool rh=p1>=k.W1&&(p1&e7)!=0u;if(!rh){F0.xyz=R4(F0.xyz,bb,W5(Y1));if(G3<1.){r P7=F0.xyz;
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){P7+=E5*k.bd;}
#endif
sg(z6,B0(P7,.0));memoryBarrier();pg(S0,T1,e7);}}else{F0.xyz=rg(z6).xyz;}}
#endif
F0.xyz*=F0.w;
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){F0.xyz+=E5;}
#endif
C0(e0,B0(.0));T3(F0);}
#endif
