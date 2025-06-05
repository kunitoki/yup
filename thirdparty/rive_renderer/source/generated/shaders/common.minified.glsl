#define O2 3.14159265359
#define e7 6.28318530718
#define S5 1.57079632679
#ifndef RENDER_MODE_MSAA
#define q3 float(.5)
#else
#define q3 float(.0)
#endif
#define F2(l) d7(l,q.bd,q.cd)
#ifdef TESS_TEXTURE_FLOATING_POINT
#define wa(g0,e,a) r4(g0,e,a)
#define H3 f
#define K8(m) m
#define Y4(m) m
#define L8(m) uintBitsToFloat(m)
#define v4(m) floatBitsToUint(m)
#else
#define wa(g0,e,a) I3(g0,e,a)
#define H3 M
#define K8(m) floatBitsToUint(m)
#define Y4(m) uintBitsToFloat(m)
#define L8(m) m
#define v4(m) m
#endif
#define J3(m) T5(JC,M8,m,xa,float(xa),.0).x
#define Z4(m) T5(JC,M8,m,ya,float(ya),.0).x
#ifdef za
d g d4(float x){return x;}d g f7(uint x){return float(x);}d g dd(a0 x){return float(x);}d g N8(int x){return float(x);}d i I5(f xyzw){return xyzw;}d G F6(c xy){return xy;}d i ra(M xyzw){return vec4(xyzw);}d a0 O8(g x){return uint(x);}d a0 Q1(uint x){return x;}
#else
d g d4(float x){return(g)x;}d g f7(uint x){return(g)x;}d g dd(a0 x){return(g)x;}d g N8(int x){return(g)x;}d i I5(f xyzw){return(i)xyzw;}d G F6(c xy){return(G)xy;}d i ra(M xyzw){return(i)xyzw;}d a0 O8(g x){return(a0)x;}d a0 Q1(uint x){return(a0)x;}
#endif
d g v1(g x){return x;}d G Z3(G xy){return xy;}d G Z3(g x,g y){G V;V.x=x,V.y=y;return V;}d G Z3(g x){G V;V.x=x,V.y=x;return V;}d c J5(float x){return c(x,x);}d A K0(g x,g y,g z){A V;V.x=x,V.y=y,V.z=z;return V;}d A K0(g x){A V;V.x=x,V.y=x,V.z=x;return V;}d i E1(g x,g y,g z,g w){i V;V.x=x,V.y=y,V.z=z,V.w=w;return V;}d i E1(A xyz,g w){i V;V.xyz=xyz;V.w=w;return V;}d i E1(g x){i V;V.x=x,V.y=x,V.z=x,V.w=x;return V;}d a5 ed(bool b){return a5(b,b);}d U5 qf(A k,A b,A B0){U5 V;V[0]=k;V[1]=b;V[2]=B0;return V;}d V5 Ic(A k,A b){V5 V;V[0]=k;V[1]=b;return V;}d S D1(f x){return S(x.xy,x.zw);}d uint ea(a0 x){return x;}d uint Aa(uint Y){return(Y&fd)-1u;}d c c5(c k,c b,float t){return(b-k)*t+k;}d g g7(uint Ba,uint d5){return Ba==0u?.0:unpackHalf2x16((Ba+gd)*d5).x;}d float Ca(c M1){M1=normalize(M1);float O0=acos(clamp(M1.x,-1.,1.));return M1.y>=.0?O0:-O0;}d i rf(i j){return E1(j.xyz*j.w,j.w);}d A Y3(i P8){return P8.xyz*(P8.w!=.0?1./P8.w:.0);}d g A8(i Da){G Ea=min(Da.xy,Da.zw);g hd=min(Ea.x,Ea.y);return hd;}d float f8(c x){return abs(x.x)+abs(x.y);}
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
e5(K3,VB)float sa;float Fa;float bd;float cd;uint Ga;uint id;uint Oc;uint Pc;h7 J6;c U4;c Ha;uint Z2;uint d5;float C1;uint jd;W5(q)
#endif
#ifdef VERTEX
d f d7(c Ia,float kd,float Ja){return f(Ia.x*kd-1.,Ia.y*Ja-sign(Ja),0.,1.);}
#ifndef RENDER_MODE_MSAA
d f I6(S R1,c Z1,c Q8){c R8=abs(R1[0])+abs(R1[1]);if(R8.x!=.0&&R8.y!=.0){c d0=1./R8;c w4=C0(R1,Q8)+Z1;const float ld=.5;return f(w4,-w4)*d0.xyxy+d0.xyxy+ld;}else{return Z1.xyxy;}}
#else
d float S8(uint X5){return 1.-float(X5)*(2./32768.);}
#ifdef ENABLE_CLIP_RECT
d void Ka(S R1,c Z1,c Q8){if(R1!=S(0)){c w4=C0(R1,Q8)+Z1.xy;gl_ClipDistance[0]=w4.x+1.;gl_ClipDistance[1]=w4.y+1.;gl_ClipDistance[2]=1.-w4.x;gl_ClipDistance[3]=1.-w4.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=Z1.x-.5;}}
#endif
#endif
#endif
#ifdef DRAW_IMAGE
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
e5(f5,EC)f H6;c S0;float H2;float sf;f R1;c Z1;uint Z0;uint z3;uint X5;W5(m0)
#endif
#endif
