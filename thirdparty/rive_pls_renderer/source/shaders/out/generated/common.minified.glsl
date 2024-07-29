#define L4 float(3.141592653589793238)
#ifndef USING_DEPTH_STENCIL
#define m2 float(.5)
#else
#define m2 float(.0)
#endif
i uint i7(uint E){return(E&B9)-1u;}i d z3(d n,d b,float t){return(b-n)*t+n;}i h M4(uint j7,uint A3){return j7==0u?.0:unpackHalf2x16((j7+C9)*A3).x;}i float k7(d R0){float l7=.0;if(abs(R0.x)>abs(R0.y)){R0=d(R0.y,-R0.x);l7=L4/2.;}return atan(R0.y,R0.x)+l7;}i j r2(j f){return j0(f.xyz*f.w,f.w);}i j X3(j f){if(f.w!=.0)f.xyz*=1.0/f.w;return f;}i B W0(g x){return B(x.xy,x.zw);}i uint f7(N x){return x;}i h K5(j m7){F n7=min(m7.xy,m7.zw);h D9=min(n7.x,n7.y);return D9;}i float z5(d x){return abs(x.x)+abs(x.y);}
#ifdef VERTEX
d4(T2,SB)float N5;float O5;float E9;float o7;uint p7;uint F9;uint x9;uint y9;P5 z4;uint A3;float l2;N4(A)
#define k2(H) g((H).x*A.E9-1.,(H).y*-A.o7+sign(A.o7),.0,1.)
#ifndef USING_DEPTH_STENCIL
i g x4(B X0,d i1,d Q5){d R5=abs(X0[0])+abs(X0[1]);if(R5.x!=.0&&R5.y!=.0){d Q=1./R5;d U2=l0(X0,Q5)+i1;const float G9=.5;return g(U2,-U2)*Q.xyxy+Q.xyxy+G9;}else{return i1.xyxy;}}
#else
i float S5(uint e4){return 1.-float(e4)*(2./32768.);}
#ifdef ENABLE_CLIP_RECT
i void q7(B X0,d i1,d Q5){if(X0!=B(0)){d U2=l0(X0,Q5)+i1.xy;gl_ClipDistance[0]=U2.x+1.;gl_ClipDistance[1]=U2.y+1.;gl_ClipDistance[2]=1.-U2.x;gl_ClipDistance[3]=1.-U2.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=i1.x-.5;}}
#endif
#endif
#endif
#ifdef DRAW_IMAGE
d4(B3,ZB)g x5;d O0;float O2;float Sa;g X0;d i1;uint I1;uint e2;uint e4;N4(L)
#endif
