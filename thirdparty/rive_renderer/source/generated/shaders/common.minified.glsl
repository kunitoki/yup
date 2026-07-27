#define A3 3.14159265359
#define p8 6.28318530718
#define T6 1.57079632679
#ifndef RENDER_MODE_MSAA
#define l4 float(.5)
#else
#define l4 float(.0)
#endif
#define K3(l) o8(l,k.Xe,k.Ye)
#ifdef TESS_TEXTURE_FLOATING_POINT
#define Yb(M,f,a) e5(M,f,a)
#define C4 g
#define R9(q) q
#define U5(q) q
#define S9(q) uintBitsToFloat(q)
#define f5(q) floatBitsToUint(q)
#else
#define Yb(M,f,a) D4(M,f,a)
#define C4 Q
#define R9(q) floatBitsToUint(q)
#define U5(q) uintBitsToFloat(q)
#define S9(q) q
#define f5(q) q
#endif
#define Ze(a,l,q8) v1(a,U(l)+U(-1,0))q8,v1(a,U(l)+U(0,0))q8,v1(a,U(l)+U(0,-1))q8,v1(a,U(l)+U(-1,-1))q8
#define g5(q) U6(QC,T9,q,Zb,float(Zb),.0).x
#define bc(q) U6(QC,T9,q,ac,float(ac),.0).x
#ifdef cc
e d m4(float x){return x;}e d V5(uint x){return float(x);}e d af(X x){return float(x);}e d U9(int x){return float(x);}e i Y4(g xyzw){return xyzw;}e D R7(c xy){return xy;}e i Tb(Q xyzw){return vec4(xyzw);}e X W5(d x){return uint(x);}e X i2(uint x){return x;}
#else
e d m4(float x){return(d)x;}e d V5(uint x){return(d)x;}e d af(X x){return(d)x;}e d U9(int x){return(d)x;}e i Y4(g xyzw){return(i)xyzw;}e D R7(c xy){return(D)xy;}e i Tb(Q xyzw){return(i)xyzw;}e X W5(d x){return(X)x;}e X i2(uint x){return(X)x;}
#endif
e d G0(d x){return x;}e D A2(D xy){return xy;}e D A2(d x,d y){D L;L.x=x,L.y=y;return L;}e D A2(d x){D L;L.x=x,L.y=x;return L;}e c J6(float x){return c(x,x);}e r T0(d x,d y,d z){r L;L.x=x,L.y=y,L.z=z;return L;}e r T0(d x){r L;L.x=x,L.y=x,L.z=x;return L;}e i B0(d x,d y,d z,d w){i L;L.x=x,L.y=y,L.z=z,L.w=w;return L;}e i B0(r xyz,d w){i L;L.xyz=xyz;L.w=w;return L;}e i B0(d x){i L;L.x=x,L.y=x,L.z=x,L.w=x;return L;}e i B0(i x){return x;}e E4 bf(bool b){return E4(b,b);}e V6 Dh(r o,r b,r I1){V6 L;L[0]=o;L[1]=b;L[2]=I1;return L;}e W6 Eh(r o,r b){W6 L;L[0]=o;L[1]=b;return L;}e h5 Fh(i o,i b,i I1,i cf){h5 L;L[0]=o;L[1]=b;L[2]=I1;L[3]=cf;return L;}e Z j2(g x){return Z(x.xy,x.zw);}e uint Gb(X x){return x;}e c X5(c o,c b,float t){return(b-o)*t+o;}e d r8(uint dc,uint Y5){return dc==0u?.0:unpackHalf2x16((dc+df)*Y5).x;}e float ec(c e2){e2=normalize(e2);float h1=acos(clamp(e2.x,-1.,1.));return e2.y>=.0?h1:-h1;}e i Gh(i j){return B0(j.xyz*j.w,j.w);}e r B6(i V9){return V9.xyz*(V9.w!=.0?1./V9.w:.0);}e d g3(D X6){return min(X6.x,X6.y);}e d g3(r fc){return min(g3(fc.xy),fc.z);}e d g3(i gc){D X6=min(gc.xy,gc.zw);d ef=min(X6.x,X6.y);return ef;}e d I5(D Y6){return max(Y6.x,Y6.y);}e d I5(r hc){return max(I5(hc.xy),hc.z);}e d I5(i ic){D Y6=max(ic.xy,ic.zw);d ff=max(Y6.x,Y6.y);return ff;}e float w9(c x){return abs(x.x)+abs(x.y);}e d W9(d x,d X9,d Y9){
#if defined(GL_RENDERER_MALI)||defined(VULKAN_VENDOR_ARM)
#ifdef VULKAN_VENDOR_ARM
if(VULKAN_VENDOR_ARM)
#endif
{if(x<Y9)if(x>X9)return x;else return X9;else return Y9;}
#endif
return clamp(x,X9,Y9);}e d jc(c K0,d B2,d m3){d gf=fract(0.06711056*K0.x+0.00583715*K0.y);d hf=fract(52.9829189*gf);return(hf*B2)+m3;}
#if 0
e d Hh(c K0,float B2,float m3){int x=int(K0.x);int y=int(K0.y);int kc=(x^y);int b=(y>>1)&1;b|=(kc&2);b|=(y&1)<<2;b|=(kc&1)<<3;float jf=float(b);d kf=m4(jf)/16.0;return(kf*B2)+m3;}e d Ih(c K0,float B2,float m3){K0.y*=0.5;K0.x=fract(K0.x*0.5+K0.y);K0.y=fract(K0.y);float M3=(K0.y*0.5+K0.x);return(M3*B2)+m3;}
#endif
#ifdef ENABLE_DITHER
e d Z9(c K0,d B2,d m3){return ENABLE_DITHER?jc(K0,B2,m3):.0;}e r Q3(r j,c K0,d B2,d m3){return ENABLE_DITHER?(jc(K0,B2,m3)+j):j;}
#else
e d Z9(c K0,float B2,float m3){return 0.;}e r Q3(r j,c K0,d B2,d m3){return j;}
#endif
#ifdef VERTEX
e g o8(c lc,float lf,float mc){return g(lc.x*lf-1.,lc.y*mc-sign(mc),0.,1.);}
#ifndef RENDER_MODE_MSAA
e g T7(Z k2,c D2,c aa){c ba=abs(k2[0])+abs(k2[1]);if(ba.x!=.0&&ba.y!=.0){c H=1./ba;c i5=Z0(k2,aa)+D2;const float mf=.5;return g(i5,-i5)*H.xyxy+H.xyxy+mf;}else{return D2.xyxy;}}
#else
e float ca(uint Z6){return 1.-float(Z6)*(2./32768.);}
#ifdef ENABLE_CLIP_RECT
e void nc(Z k2,c D2,c aa a7){
#ifndef DISABLE_CLIP_DISTANCE_FOR_UBERSHADERS
if(any(notEqual(g(k2),g(.0,.0,.0,.0)))){c i5=Z0(k2,aa)+D2.xy;gl_ClipDistance[0]=i5.x+1.;gl_ClipDistance[1]=i5.y+1.;gl_ClipDistance[2]=1.-i5.x;gl_ClipDistance[3]=1.-i5.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=D2.x-.5;}
#endif
}
#endif
#endif
#endif
#ifdef FRAGMENT
#ifdef NEEDS_GAMMA_CORRECTION
e d k3(d j){return(j<=0.04045)?j/12.92:pow(abs((j+0.055)/1.055),2.4);}e r k3(r j){return T0(k3(j.x),k3(j.y),k3(j.z));}e i k3(i j){return B0(k3(j.xyz),j.w);}
#endif
#endif
#if defined(FRAGMENT)&&defined(RENDER_MODE_MSAA)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
e i oc(h5 c7,int v8){if(v8==0xf){return(c7[0]+c7[1]+c7[2]+c7[3])*.25;}else{i nf=g(notEqual(v8&Z5(1,2,4,8),Z5(0)));i L=Z0(c7,nf);int w8=(v8&5)+((v8>>1)&5);w8=(w8&3)+(w8>>2);L*=1./float(w8);return L;}}
#endif
