#ifdef FRAGMENT
J1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
r0(Q2,g0);
#endif
r0(R2,d0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Ja(d6,z6);
#endif
K1 N3 Ea(ga,Yd,S0);O3 e void xh(T4(float)o3,d o0,uint T1,e1(uint)p1,e1(d)J3){
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
if(min(o3,o0)>=1.){return;}
#endif
d q;uint be=q7(abs(o0));p1=A7(S0,T1,k.W1|be);if(p1<k.W1){q=o0;
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
J3=o0;
#endif
}else{
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
if((p1&e7)!=0u){p1=A7(S0,T1,k.W1|e7|be);}
#endif
d V1=V5(p1&ja)*ha;d G1=max(V1,o0);q=K8(V1,G1,o3);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
J3=G1;
#endif
}o3*=q;}e void yh(T4(float)o3,d P4,uint T1,e1(uint)p1,e1(d)J3){d q=.0;uint fb=q7(abs(P4));p1=pd(S0,T1);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
if(min(o3,P4)>=1.&&(p1<k.W1||p1>=(k.W1|j5))){return;}
#endif
if(p1<k.W1){uint ce=k.W1|(j5+fb);uint f3=A7(S0,T1,ce);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
p1=f3;
#endif
if(f3<=k.W1){q=P4;
#ifdef DRAW_INTERIOR_TRIANGLES
q=min(q,1.);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
J3=q;
#endif
P4=.0;}else if(f3<ce){uint de=(f3&ja)-j5;d V1=V5(de)*ha;d G1=P4;
#ifdef DRAW_INTERIOR_TRIANGLES
G1=min(G1,1.);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
J3=G1;
#endif
q=K8(V1,G1,o3);fb=de;P4=V1;}}if(P4>.0){uint eb=Fa(S0,T1,fb);d V1=Aa(eb);d G1=V1+P4;V1=clamp(V1,.0,1.);G1=clamp(G1,.0,1.);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
J3=G1;
#endif
q+=(1.-q*o3)*K8(V1,G1,o3);}o3*=q;}d5(IB){B(i1,g);
#ifdef DRAW_INTERIOR_TRIANGLES
Y(j1,d);
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
B(Z1,d);
#endif
B(e3,W0);B(j4,c);i F0=M7(i1,1. S2);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
i gb=H0(g0);
#endif
d o0=
#ifdef DRAW_INTERIOR_TRIANGLES
j1;
#else
cb(I);
#endif
c y6=j4;
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
y6+=(gb.xy+gb.zw)*k.vg;
#endif
y6=floor(y6);uint N7=e3.y;uint T1=e3.x+L8(W0(y6),N7);d H1=1.;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d hb=g3(Y4(N0));H1=min(hb,H1);}
#endif
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&S1.x!=.0){d ib=H0(d0).x;H1=min(ib,H1);}
#endif
H1=max(H1,.0);o0=clamp(o0,.0,H1);uint p1;float J3;
#ifndef DRAW_INTERIOR_TRIANGLES
if(Q5(I)){xh(F0.w,o0,T1,p1,J3);}else
#endif
{yh(F0.w,o0,T1,p1,J3);}
#ifdef ENABLE_DITHER
d G5;if(ENABLE_DITHER){G5=Z9(S.xy,k.y3,k.z3);}
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
if(F0.w>.0){bool zh=p1>=k.W1&&(p1&e7)!=0u;if(!zh){F0.xyz=Q4(F0.xyz,gb,W5(Z1));if(J3<1.){r P7=F0.xyz;
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){P7+=G5*k.gd;}
#endif
Ia(z6,B0(P7,.0));memoryBarrier();xg(S0,T1,e7);}}else{F0.xyz=Ha(z6).xyz;}}
#endif
F0.xyz*=F0.w;
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){F0.xyz+=G5;}
#endif
v0(d0,B0(.0));V3(F0);}
#endif
