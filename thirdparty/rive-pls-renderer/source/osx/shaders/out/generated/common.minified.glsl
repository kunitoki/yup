#define D4 float(3.141592653589793238)
#ifndef USING_DEPTH_STENCIL
#define j2 float(.5)
#else
#define j2 float(.0)
#endif
p uint X6(uint D){return(D&h9)-1u;}p d q3(d m,d b,float t){return(b-m)*t+m;}p h E4(uint Y6,uint r3){return Y6==0u?.0:unpackHalf2x16((Y6+i9)*r3).x;}p float Z6(d Q0){float a7=.0;if(abs(Q0.x)>abs(Q0.y)){Q0=d(Q0.y,-Q0.x);a7=D4/2.;}return atan(Q0.y,Q0.x)+a7;}p i o2(i f){return M0(f.xyz*f.w,f.w);}p i R3(i f){if(f.w!=.0)f.xyz*=1.0/f.w;return f;}p A V0(g V){return A(V.xy,V.zw);}p h A5(i c7){l0 d7=min(c7.xy,c7.zw);h j9=min(d7.x,d7.y);return j9;}p float n5(d x){return abs(x.x)+abs(x.y);}
#ifdef VERTEX
W3(N2,SB)float D5;float E5;float k9;float e7;uint f7;uint l9;uint d9;uint e9;F5 o4;uint r3;float i2;F4(v)
#define h2(F) g((F).x*v.k9-1.,(F).y*-v.e7+sign(v.e7),.0,1.)
#ifndef USING_DEPTH_STENCIL
p g m4(A W0,d g1,d G5){d H5=abs(W0[0])+abs(W0[1]);if(H5.x!=.0&&H5.y!=.0){d U=1./H5;d O2=h0(W0,G5)+g1;const float m9=.5;return g(O2,-O2)*U.xyxy+U.xyxy+m9;}else{return g1.xyxy;}}
#else
p float I5(uint X3){return 1.-float(X3)*(2./32768.);}
#ifdef ENABLE_CLIP_RECT
p void g7(A W0,d g1,d G5){if(W0!=A(0)){d O2=h0(W0,G5)+g1.xy;gl_ClipDistance[0]=O2.x+1.;gl_ClipDistance[1]=O2.y+1.;gl_ClipDistance[2]=1.-O2.x;gl_ClipDistance[3]=1.-O2.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=g1.x-.5;}}
#endif
#endif
#endif
#ifdef DRAW_IMAGE
W3(v3,ZB)g l5;d K0;float I2;float Ha;g W0;d g1;uint F1;uint Z1;uint X3;F4(J)
#endif
