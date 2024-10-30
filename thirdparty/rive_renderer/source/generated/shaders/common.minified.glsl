#define e5 float(3.141592653589793238)
#ifndef USING_DEPTH_STENCIL
#define G2 float(.5)
#else
#define G2 float(.0)
#endif
#ifdef F7
d h o2(float x){return x;}d h G7(uint x){return float(x);}d h Q9(M x){return float(x);}d h H7(int x){return float(x);}d i r4(g xyzw){return xyzw;}d r Z5(f xy){return xy;}d i E7(G xyzw){return vec4(xyzw);}d M k6(h x){return uint(x);}d M W1(uint x){return x;}
#else
d h o2(float x){return(h)x;}d h G7(uint x){return(h)x;}d h Q9(M x){return(h)x;}d h H7(int x){return(h)x;}d i r4(g xyzw){return(i)xyzw;}d r Z5(f xy){return(r)xy;}d i E7(G xyzw){return(i)xyzw;}d M k6(h x){return(M)x;}d M W1(uint x){return(M)x;}
#endif
d h d1(h x){return x;}d r D2(r xy){return xy;}d r D2(h x,h y){r C;C.x=x,C.y=y;return C;}d r D2(h x){r C;C.x=x,C.y=x;return C;}d k H0(h x,h y,h z){k C;C.x=x,C.y=y,C.z=z;return C;}d k H0(h x){k C;C.x=x,C.y=x,C.z=x;return C;}d i C2(h x,h y,h z,h w){i C;C.x=x,C.y=y,C.z=z,C.w=w;return C;}d i C2(k xyz,h w){i C;C.xyz=xyz;C.w=w;return C;}d i C2(h x){i C;C.x=x,C.y=x,C.z=x,C.w=x;return C;}d w4 C9(k l,k b,k z0){w4 C;C[0]=l;C[1]=b;C[2]=z0;return C;}d x4 D9(k l,k b){x4 C;C[0]=l;C[1]=b;return C;}d I j1(g x){return I(x.xy,x.zw);}d uint D7(M x){return x;}d uint I7(uint R){return(R&R9)-1u;}d f W3(f l,f b,float t){return(b-l)*t+l;}d h f5(uint J7,uint X3){return J7==0u?.0:unpackHalf2x16((J7+S9)*X3).x;}d float K7(f L0){float L7=.0;if(abs(L0.x)>abs(L0.y)){L0=f(L0.y,-L0.x);L7=e5/2.;}return atan(L0.y,L0.x)+L7;}d i M7(i j){return C2(j.xyz*j.w,j.w);}d i R3(i j){if(j.w!=.0)j.xyz*=1.0/j.w;return j;}d h f6(i N7){r O7=min(N7.xy,N7.zw);h T9=min(O7.x,O7.y);return T9;}d float Q5(f x){return abs(x.x)+abs(x.y);}
#ifdef VERTEX
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
Y3(n3,SB)float j6;float l6;float U9;float P7;uint Q7;uint V9;uint M9;uint N9;g5 V4;uint X3;float F2;y4(P)
#endif
#define E2(K) g((K).x*P.U9-1.,(K).y*-P.P7+sign(P.P7),.0,1.)
#ifndef USING_DEPTH_STENCIL
d g U4(I k1,f q1,f m6){f n6=abs(k1[0])+abs(k1[1]);if(n6.x!=.0&&n6.y!=.0){f Z=1./n6;f o3=m0(k1,m6)+q1;const float W9=.5;return g(o3,-o3)*Z.xyxy+Z.xyxy+W9;}else{return q1.xyxy;}}
#else
d float o6(uint z4){return 1.-float(z4)*(2./32768.);}
#ifdef ENABLE_CLIP_RECT
d void R7(I k1,f q1,f m6){if(k1!=I(0)){f o3=m0(k1,m6)+q1.xy;gl_ClipDistance[0]=o3.x+1.;gl_ClipDistance[1]=o3.y+1.;gl_ClipDistance[2]=1.-o3.x;gl_ClipDistance[3]=1.-o3.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=q1.x-.5;}}
#endif
#endif
#endif
#ifdef DRAW_IMAGE
#ifndef UNIFORM_DEFINITIONS_AUTO_GENERATED
Y3(Z3,ZB)g O5;f a1;float m3;float ob;g k1;f q1;uint L1;uint M2;uint z4;y4(X)
#endif
#endif
