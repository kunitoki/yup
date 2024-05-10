#ifdef VERTEX
U0(P)
#ifdef DRAW_INTERIOR_TRIANGLES
q0(0,H3,KB);
#else
q0(0,g,UB);q0(1,g,VB);
#endif
V0
#endif
A1 k0 I(0,g,o0);
#ifndef USING_DEPTH_STENCIL
#ifdef DRAW_INTERIOR_TRIANGLES
OPTIONALLY_FLAT I(1,h,C1);
#else
k0 I(2,l0,J0);
#endif
OPTIONALLY_FLAT I(3,h,W);
#ifdef ENABLE_CLIPPING
OPTIONALLY_FLAT I(4,h,G0);
#endif
#ifdef ENABLE_CLIP_RECT
k0 I(5,g,i0);
#endif
#endif
#ifdef ENABLE_ADVANCED_BLEND
OPTIONALLY_FLAT I(6,h,x2);
#endif
B1
#ifdef VERTEX
g1(OB,P,r,j,L){
#ifdef DRAW_INTERIOR_TRIANGLES
v0(j,r,KB,D1);
#else
v0(j,r,UB,g);v0(j,r,VB,g);
#endif
Q(o0,g);
#ifndef Ua
#ifdef DRAW_INTERIOR_TRIANGLES
Q(C1,h);
#else
Q(J0,l0);
#endif
Q(W,h);
#ifdef ENABLE_CLIPPING
Q(G0,h);
#endif
#ifdef ENABLE_CLIP_RECT
Q(i0,g);
#endif
#endif
#ifdef ENABLE_ADVANCED_BLEND
Q(x2,h);
#endif
bool B7=false;M k1;d R;
#ifdef USING_DEPTH_STENCIL
M C7;
#endif
#ifdef DRAW_INTERIOR_TRIANGLES
R=F6(KB,k1,C1 j3);
#else
B7=!E6(UB,VB,L,k1,R
#ifndef USING_DEPTH_STENCIL
,J0
#else
,C7
#endif
j3);
#endif
x0 E=q2(PB,k1);
#ifndef USING_DEPTH_STENCIL
W=F4(k1,v.v3);if((E.x&L6)!=0u)W=-W;
#endif
uint H1=E.x&0xfu;
#ifdef ENABLE_CLIPPING
uint x9=(H1==y4?E.y:E.x)>>16;G0=F4(x9,v.v3);if(H1==y4)G0=-G0;
#endif
#ifdef ENABLE_ADVANCED_BLEND
x2=float((E.x>>4)&0xfu);
#endif
d a4=R;
#ifdef y9
a4.y=float(v.n9)-a4.y;
#endif
#ifdef ENABLE_CLIP_RECT
A X0=W0(z0(HB,k1*4u+2u));g i1=z0(HB,k1*4u+3u);
#ifndef USING_DEPTH_STENCIL
i0=n4(X0,i1.xy,a4);
#else
i7(X0,i1.xy,a4);
#endif
#endif
if(H1==M6){i f=unpackUnorm4x8(E.y);o0=g(f);}
#ifdef ENABLE_CLIPPING
else if(H1==y4){h L4=F4(E.x>>16,v.v3);o0=g(L4,0,0,0);}
#endif
else{A z9=W0(z0(HB,k1*4u));g M5=z0(HB,k1*4u+1u);d o2=h0(z9,a4)+M5.xy;if(H1==x4||H1==N6){o0.w=-uintBitsToFloat(E.y);if(M5.z>.9){o0.z=2.;}else{o0.z=M5.w;}if(H1==x4){o0.y=.0;o0.x=o2.x;}else{o0.z=-o0.z;o0.xy=o2.xy;}}else{float J2=uintBitsToFloat(E.y);o0=g(o2.x,o2.y,J2,-2.);}}g B;if(!B7){B=i2(R);
#ifdef USING_DEPTH_STENCIL
B.z=J5(C7);
#endif
}else{B=g(v.j2,v.j2,v.j2,v.j2);}S(o0);
#ifndef USING_DEPTH_STENCIL
#ifdef DRAW_INTERIOR_TRIANGLES
S(C1);
#else
S(J0);
#endif
S(W);
#ifdef ENABLE_CLIPPING
S(G0);
#endif
#ifdef ENABLE_CLIP_RECT
S(i0);
#endif
#endif
#ifdef ENABLE_ADVANCED_BLEND
S(x2);
#endif
h1(B);}
#endif
#ifdef FRAGMENT
F2 x1(q4,DC);x1(k3,MB);
#ifdef USING_DEPTH_STENCIL
#ifdef ENABLE_ADVANCED_BLEND
x1(z7,EC);
#endif
#endif
G2 J3(q4,r5)l3(k3,n2)L3 O3 p i D7(g J1
#ifdef TARGET_VULKAN
,d N5,d O5
#endif
w4){if(J1.w>=.0){return M0(J1);}else if(J1.w>-1.){float t=J1.z>.0?J1.x:length(J1.xy);t=clamp(t,.0,1.);float E7=abs(J1.z);float x=E7>1.?(1.-1./K5)*t+(.5/K5):(1./K5)*t+E7;float A9=-J1.w;return M0(R3(DC,r5,d(x,A9),.0));}else{i f;
#ifdef TARGET_VULKAN
f=Q3(MB,n2,J1.xy,N5,O5);
#else
f=M2(MB,n2,J1.xy);
#endif
f.w*=J1.z;return f;}}
#ifndef USING_DEPTH_STENCIL
E1 C0(v5,c0);D0(w5,Q0);
#ifdef ENABLE_CLIPPING
D0(x5,w0);
#endif
C0(v7,w2);F1 R1(IB){N(o0,g);
#ifdef DRAW_INTERIOR_TRIANGLES
N(C1,h);
#else
N(J0,l0);
#endif
N(W,h);
#ifdef ENABLE_CLIPPING
N(G0,h);
#endif
#ifdef ENABLE_CLIP_RECT
N(i0,g);
#endif
#ifdef ENABLE_ADVANCED_BLEND
N(x2,h);
#endif
#ifdef TARGET_VULKAN
d N5=dFdx(o0.xy);d O5=dFdy(o0.xy);
#endif
#ifndef DRAW_INTERIOR_TRIANGLES
n1;
#endif
l0 F7=unpackHalf2x16(L0(Q0));h G7=F7.y;h Y0=G7==W?F7.x:E0(0);
#ifdef DRAW_INTERIOR_TRIANGLES
Y0+=C1;
#else
if(J0.y>=.0)Y0=max(min(J0.x,J0.y),Y0);else Y0+=J0.x;a1(Q0,packHalf2x16(Z1(Y0,W)));
#endif
h C=abs(Y0);
#ifdef ENABLE_EVEN_ODD
if(W<.0)C=1.-abs(fract(C*.5)*2.+-1.);
#endif
C=min(C,E0(1));
#ifdef ENABLE_CLIPPING
if(G0<.0){h G1=-G0;
#ifdef ENABLE_NESTED_CLIPPING
h L4=o0.x;if(L4!=.0){l0 Z0=unpackHalf2x16(L0(w0));h y3=Z0.y;h M4;if(y3!=G1){M4=y3==L4?Z0.x:.0;
#ifndef DRAW_INTERIOR_TRIANGLES
F0(w2,M0(M4,0,0,0));
#endif
}else{M4=N0(w2).x;
#ifndef DRAW_INTERIOR_TRIANGLES
y0(w2);
#endif
}C=min(C,M4);}
#endif
a1(w0,packHalf2x16(Z1(C,G1)));y0(c0);}else
#endif
{
#ifdef ENABLE_CLIPPING
if(G0!=.0){l0 Z0=unpackHalf2x16(L0(w0));h y3=Z0.y;h I2=y3==G0?Z0.x:E0(0);C=min(C,I2);}y0(w0);
#endif
#ifdef ENABLE_CLIP_RECT
h K2=B5(M0(i0));C=clamp(K2,E0(0),C);
#endif
i f=D7(o0
#ifdef TARGET_VULKAN
,N5,O5
#endif
L2);f.w*=C;i y1;if(G7!=W){y1=N0(c0);
#ifndef DRAW_INTERIOR_TRIANGLES
F0(w2,y1);
#endif
}else{y1=N0(w2);
#ifndef DRAW_INTERIOR_TRIANGLES
y0(w2);
#endif
}
#ifdef ENABLE_ADVANCED_BLEND
if(x2!=E0(T6)){
#ifdef ENABLE_HSL_BLEND_MODES
f=E3(
#else
f=F3(
#endif
f,S3(y1),O0(x2));}else
#endif
{
#ifndef PLS_IMPL_NONE
f.xyz*=f.w;f=f+y1*(1.-f.w);
#endif
}F0(c0,f);}
#ifndef DRAW_INTERIOR_TRIANGLES
o1;
#endif
c2;}
#else
r2(i,IB){N(o0,g);
#ifdef ENABLE_ADVANCED_BLEND
N(x2,h);
#endif
i f=D7(o0);
#ifdef ENABLE_ADVANCED_BLEND
i y1=I1(EC,m0(floor(n0.xy)));
#ifdef ENABLE_HSL_BLEND_MODES
f=E3(
#else
f=F3(
#endif
f,S3(y1),O0(x2));
#else
f=p2(f);
#endif
v2(f);}
#endif
#endif
