#define z4 3.14159265359
#define D7 6.28318530718
#define E7 1.57079632679
#ifndef RENDER_MODE_DEPTH_STENCIL
#define Z4 float(.5)
#else
#define Z4 float(.0)
#endif
#define T3(o) p9(o,j.dh,j.eh)
#define fh(a,o,q9) j1(a,f0(o)+f0(-1,0)) q9,j1(a,f0(o)+f0(0,0)) q9,j1(a,f0(o)+f0(0,-1)) q9,j1(a,f0(o)+f0(-1,-1)) q9
#define N5(G) F7(BD,kb,G,Rd,float(Rd),.0).x
#define Td(G) F7(BD,kb,G,Sd,float(Sd),.0).x
#ifdef lb
f d a5(float x){return x;}f d z6(uint x){return float(x);}f d gh(O x){return float(x);}f d mb(int x){return float(x);}f i y4(e xyzw){return xyzw;}f F N8(c xy){return xy;}f i Nd(R xyzw){return vec4(xyzw);}f O a3(d x){return uint(x);}f O Z1(uint x){return x;}
#else
f d a5(float x){return(d) x;}f d z6(uint x){return(d) x;}f d gh(O x){return(d) x;}f d mb(int x){return(d) x;}f i y4(e xyzw){return(i) xyzw;}f F N8(c xy){return(F) xy;}f i Nd(R xyzw){return(i) xyzw;}f O a3(d x){return(O) x;}f O Z1(uint x){return(O) x;}
#endif
f d B0(d x){return x;}f F V2(F xy){return xy;}f F V2(d x,d y){F Z;Z.x=x,Z.y=y;return Z;}f F V2(d x){F Z;Z.x=x,Z.y=x;return Z;}f c r7(float x){return c(x,x);}f v e1(d x,d y,d z){v Z;Z.x=x,Z.y=y,Z.z=z;return Z;}f v e1(d x){v Z;Z.x=x,Z.y=x,Z.z=x;return Z;}f i A0(d x,d y,d z,d w){i Z;Z.x=x,Z.y=y,Z.z=z,Z.w=w;return Z;}f i A0(v xyz,d w){i Z;Z.xyz=xyz;Z.w=w;return Z;}f i A0(d x){i Z;Z.x=x,Z.y=x,Z.z=x,Z.w=x;return Z;}f i A0(i x){return x;}f m5 hh(bool b){return m5(b,b);}f G7 vk(v l,v b,v Y1){G7 Z;Z[0]=l;Z[1]=b;Z[2]=Y1;return Z;}f H7 wk(v l,v b){H7 Z;Z[0]=l;Z[1]=b;return Z;}f n5 xk(i l,i b,i Y1,i ih){n5 Z;Z[0]=l;Z[1]=b;Z[2]=Y1;Z[3]=ih;return Z;}f X w1(e x){return X(x.xy,x.zw);}f uint zd(O x){return x;}f c A6(c l,c b,float t){return(b-l)*t+l;}f d r9(uint Ud,uint B6){return Ud==0u?.0:unpackHalf2x16((Ud+jh)*B6).x;}f float nb(c F2){F2=normalize(F2);float k1=acos(clamp(F2.x,-1.,1.));return F2.y>=.0?k1:-k1;}f i yk(i k){return A0(k.xyz*k.w,k.w);}f v o6(i ob){return ob.xyz*(ob.w!=.0?1./ob.w:.0);}f d E3(F I7){return min(I7.x,I7.y);}f d E3(v Vd){return min(E3(Vd.xy),Vd.z);}f d E3(i Wd){F I7=min(Wd.xy,Wd.zw);d kh=min(I7.x,I7.y);return kh;}f d n6(F J7){return max(J7.x,J7.y);}f d n6(v Xd){return max(n6(Xd.xy),Xd.z);}f d n6(i Yd){F J7=max(Yd.xy,Yd.zw);d lh=max(J7.x,J7.y);return lh;}f float Ha(c x){return abs(x.x)+abs(x.y);}f d pb(d x,d qb,d rb){
#if defined(GL_RENDERER_MALI)||defined(VULKAN_VENDOR_ARM)
#ifdef VULKAN_VENDOR_ARM
if(VULKAN_VENDOR_ARM)
#endif
{if(x<rb) if(x>qb) return x;else return qb;else return rb;}
#endif
return clamp(x,qb,rb);}f d Zd(c n0,d W2,d L3){d mh=fract(0.06711056*n0.x+0.00583715*n0.y);d nh=fract(52.9829189*mh);return(nh*W2)+L3;}
#if 0
f d zk(c n0,float W2,float L3){int x=int(n0.x);int y=int(n0.y);int ae=(x^y);int b=(y>>1)&1;b|=(ae&2);b|=(y&1)<<2;b|=(ae&1)<<3;float oh=float(b);d ph=a5(oh)/16.0;return(ph*W2)+L3;}f d Ak(c n0,float W2,float L3){n0.y*=0.5;n0.x=fract(n0.x*0.5+n0.y);n0.y=fract(n0.y);float m4=(n0.y*0.5+n0.x);return(m4*W2)+L3;}
#endif
#ifdef ENABLE_DITHER
f d sb(c n0,d W2,d L3){return ENABLE_DITHER?Zd(n0,W2,L3):.0;}f v M2(v k,d K7,c n0,d W2,d L3){return(ENABLE_DITHER&&K7!=.0)?(Zd(n0,W2,L3)+k):k;}f v M2(v k,d K7,d be){return(ENABLE_DITHER&&K7!=.0)?(be+k):k;}
#else
f d sb(c n0,float W2,float L3){return 0.;}f v M2(v k,d K7,c n0,d W2,d L3){return k;}f v M2(v k,d K7,d be){return k;}
#endif
#ifdef VERTEX
f e p9(c ce,float qh,float de){return e(ce.x*qh-1.,ce.y*de-sign(de),0.,1.);}
#ifndef RENDER_MODE_DEPTH_STENCIL
f e P8(X M3,c Z3,c tb){c ub=abs(M3[0])+abs(M3[1]);if(ub.x!=.0&&ub.y!=.0){c U=1./ub;c O5=G0(M3,tb)+Z3;const float rh=.5;return e(O5,-O5)*U.xyxy+U.xyxy+rh;}else{return Z3.xyxy;}}
#else
f float v9(uint sh,uint th){float ee=float((sh<<uh)|th);
#if defined(lb)&&!defined(TARGET_SPIRV)
return ee*uintBitsToFloat(0x34000000u)+uintBitsToFloat(0xbf7fffffu);
#else
return ee*uintBitsToFloat(0x33800000u)+uintBitsToFloat(0x33000000u);
#endif
}
#ifdef ENABLE_CLIP_RECT
f void vb(X M3,c Z3,c tb C6){
#ifndef DISABLE_CLIP_DISTANCE_FOR_UBERSHADERS
if(any(notEqual(e(M3),e(.0,.0,.0,.0)))){c O5=G0(M3,tb)+Z3.xy;gl_ClipDistance[0]=O5.x+1.;gl_ClipDistance[1]=O5.y+1.;gl_ClipDistance[2]=1.-O5.x;gl_ClipDistance[3]=1.-O5.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=Z3.x-.5;}
#endif
}
#endif
#endif
#endif
#if defined(FRAGMENT)&&defined(RENDER_MODE_DEPTH_STENCIL)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
f i wb(n5 L7,int w9){if(w9==int(0xf)){return(L7[0]+L7[1]+L7[2]+L7[3])*.25;}else{i vh=y4(e(notEqual(w9&D6(1,2,4,8),D6(0,0,0,0))));i Z=G0(L7,vh);int x9=(w9&5)+((w9>>1)&5);x9=(x9&3)+(x9>>2);Z*=B0(1./float(x9));return Z;}}
#endif
#ifdef FRAGMENT
f i Va(i fe,i B1){return fe+B1*(1.-fe.w);}
#endif
