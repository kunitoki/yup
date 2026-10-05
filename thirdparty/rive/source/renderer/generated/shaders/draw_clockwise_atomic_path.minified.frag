#ifdef FRAGMENT
R1
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
B0(K2,n0);
#endif
B0(c3,m0);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
rb(o6,O6);
#endif
S1 f4 mb(Na,Te,V0);g4 f d Zi(float n3,d A0,uint d2,i1(uint) z1,i1(d) Z3){
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
if(min(n3,A0)>=1.){return 1.;}
#endif
d F;uint We=G7(abs(A0));z1=M7(V0,d2,j.j2|We);if(z1<j.j2){F=A0;
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Z3=A0;
#endif
}else{
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
if((z1&w7)!=0u){z1=M7(V0,d2,j.j2|w7|We);}
#endif
d i2=j6(z1&Ra)*Pa;d M1=max(i2,A0);F=c9(i2,M1,n3);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Z3=M1;
#endif
}return F;}f d aj(float n3,d g5,uint d2,i1(uint) z1,i1(d) Z3){d F=.0;uint Xb=G7(abs(g5));z1=fe(V0,d2);
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
if(min(n3,g5)>=1.&&(z1<j.j2||z1>=(j.j2|C5))){return 1.;}
#endif
if(z1<j.j2){uint Xe=j.j2|(C5+Xb);uint r3=M7(V0,d2,Xe);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
z1=r3;
#endif
if(r3<=j.j2){F=g5;
#ifdef DRAW_INTERIOR_TRIANGLES
F=min(F,1.);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Z3=F;
#endif
g5=.0;}else if(r3<Xe){uint Ye=(r3&Ra)-C5;d i2=j6(Ye)*Pa;d M1=g5;
#ifdef DRAW_INTERIOR_TRIANGLES
M1=min(M1,1.);
#endif
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Z3=M1;
#endif
F=c9(i2,M1,n3);Xb=Ye;g5=i2;}}if(g5>.0){uint Wb=nb(V0,d2,Xb);d i2=ib(Wb);d M1=i2+g5;i2=clamp(i2,.0,1.);M1=clamp(M1,.0,1.);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
Z3=M1;
#endif
F+=(1.-F*n3)*c9(i2,M1,n3);}return F;}z5(IB){q(a1,e);
#ifdef ENABLE_MODULATED_IMAGE
q(r1,O);
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
T(m1,d);
#else
T(S,G2);
#endif
q(F0,d);
#ifdef ENABLE_CLIPPING
q(l1,C);
#endif
#ifdef ENABLE_CLIP_RECT
q(R0,e);
#endif
#ifdef ENABLE_ADVANCED_BLEND
q(Q0,d);
#endif
q(q3,O0);q(F4,c);i o0=Y7(
#ifdef ENABLE_MODULATED_IMAGE
r1,
#endif
#ifdef ENABLE_ADVANCED_BLEND
k3(Q0),
#endif
a1 e3);
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
i Yb=N0(n0);
#endif
d A0=
#ifdef DRAW_INTERIOR_TRIANGLES
m1;
#else
Ub(S);
#endif
c N6=F4;
#ifndef FIXED_FUNCTION_COLOR_OUTPUT
N6+=(Yb.xy+Yb.zw)*j.Ph;
#endif
N6=floor(N6);uint a8=q3.y;uint d2=q3.x+d9(O0(N6),a8);d N1=1.;
#ifdef ENABLE_CLIP_RECT
if(ENABLE_CLIP_RECT){d Zb=v3(v5(R0));N1=min(Zb,N1);}
#endif
#ifdef ENABLE_CLIPPING
if(ENABLE_CLIPPING&&l1.x!=.0){d ac=N0(m0).x;N1=min(ac,N1);}
#endif
N1=max(N1,.0);A0=clamp(A0,.0,N1);uint z1;d d8;float Z3;
#ifndef DRAW_INTERIOR_TRIANGLES
if(f6(S)){d8=Zi(o0.w,A0,d2,z1,Z3);}else
#endif
{d8=aj(o0.w,A0,d2,z1,Z3);}
#ifdef ENABLE_DITHER
d W5;if(ENABLE_DITHER){W5=Da(f0.xy,j.L3,j.M3);}
#endif
#ifdef FIXED_FUNCTION_COLOR_OUTPUT
o0*=d8;
#else
if(ENABLE_ADVANCED_BLEND&&k3(Q0)!=L4){o0.w*=d8;if(o0.w>.0){bool bj=z1>=j.j2&&(z1&w7)!=0u;if(!bj){o0.xyz=h5(o0.xyz,Yb,k3(Q0));if(Z3<1.){v e8=o0.xyz;
#ifdef ENABLE_DITHER
if(ENABLE_DITHER){e8+=W5*j.Wd;}
#endif
qb(O6,I0(e8,.0));memoryBarrier();Rh(V0,d2,w7);}}else{o0.xyz=pb(O6).xyz;}}o0.xyz*=o0.w;}else{o0*=d8;}
#endif
#ifdef ENABLE_DITHER
o0.xyz=M2(o0.xyz,o0.w,W5);
#endif
y0(m0,I0(.0));n4(o0);}
#endif
