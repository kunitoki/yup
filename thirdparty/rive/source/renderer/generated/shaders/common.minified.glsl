#define i4 3.14159265359
#define G8 6.28318530718
#define i7 1.57079632679
#ifndef RENDER_MODE_DEPTH_STENCIL
#define H4 float(.5)
#else
#define H4 float(.0)
#endif
#define H3(l) F8(l,j.bg,j.cg)
#define dg(a,l,H8) p1(a,e0(l)+e0(-1,0)) H8,p1(a,e0(l)+e0(0,0)) H8,p1(a,e0(l)+e0(0,-1)) H8,p1(a,e0(l)+e0(-1,-1)) H8
#define A5(F) j7(YC,wa,F,Uc,float(Uc),.0).x
#define Wc(F) j7(YC,wa,F,Vc,float(Vc),.0).x
#ifdef xa
f d d4(float x){return x;}f d j6(uint x){return float(x);}f d eg(Q x){return float(x);}f d ya(int x){return float(x);}f i v5(e xyzw){return xyzw;}f C g8(c xy){return xy;}f i Pc(M xyzw){return vec4(xyzw);}f Q k3(d x){return uint(x);}f Q P1(uint x){return x;}
#else
f d d4(float x){return(d) x;}f d j6(uint x){return(d) x;}f d eg(Q x){return(d) x;}f d ya(int x){return(d) x;}f i v5(e xyzw){return(i) xyzw;}f C g8(c xy){return(C) xy;}f i Pc(M xyzw){return(i) xyzw;}f Q k3(d x){return(Q) x;}f Q P1(uint x){return(Q) x;}
#endif
f d H0(d x){return x;}f C H2(C xy){return xy;}f C H2(d x,d y){C X;X.x=x,X.y=y;return X;}f C H2(d x){C X;X.x=x,X.y=x;return X;}f c Y6(float x){return c(x,x);}f v W0(d x,d y,d z){v X;X.x=x,X.y=y,X.z=z;return X;}f v W0(d x){v X;X.x=x,X.y=x,X.z=x;return X;}f i I0(d x,d y,d z,d w){i X;X.x=x,X.y=y,X.z=z,X.w=w;return X;}f i I0(v xyz,d w){i X;X.xyz=xyz;X.w=w;return X;}f i I0(d x){i X;X.x=x,X.y=x,X.z=x,X.w=x;return X;}f i I0(i x){return x;}f R4 fg(bool b){return R4(b,b);}f k7 hj(v k,v b,v O1){k7 X;X[0]=k;X[1]=b;X[2]=O1;return X;}f l7 ij(v k,v b){l7 X;X[0]=k;X[1]=b;return X;}f S4 jj(i k,i b,i O1,i gg){S4 X;X[0]=k;X[1]=b;X[2]=O1;X[3]=gg;return X;}f Y n1(e x){return Y(x.xy,x.zw);}f uint Bc(Q x){return x;}f c k6(c k,c b,float t){return(b-k)*t+k;}f d l6(uint Xc,uint T4){return Xc==0u?.0:unpackHalf2x16((Xc+hg)*T4).x;}f float Yc(c r2){r2=normalize(r2);float x1=acos(clamp(r2.x,-1.,1.));return r2.y>=.0?x1:-x1;}f i kj(i p){return I0(p.xyz*p.w,p.w);}f v Q6(i za){return za.xyz*(za.w!=.0?1./za.w:.0);}f d v3(C m7){return min(m7.x,m7.y);}f d v3(v Zc){return min(v3(Zc.xy),Zc.z);}f d v3(i ad){C m7=min(ad.xy,ad.zw);d ig=min(m7.x,m7.y);return ig;}f d Y5(C n7){return max(n7.x,n7.y);}f d Y5(v bd){return max(Y5(bd.xy),bd.z);}f d Y5(i cd){C n7=max(cd.xy,cd.zw);d jg=max(n7.x,n7.y);return jg;}f float U9(c x){return abs(x.x)+abs(x.y);}f d Aa(d x,d Ba,d Ca){
#if defined(GL_RENDERER_MALI)||defined(VULKAN_VENDOR_ARM)
#ifdef VULKAN_VENDOR_ARM
if(VULKAN_VENDOR_ARM)
#endif
{if(x<Ca) if(x>Ba) return x;else return Ba;else return Ca;}
#endif
return clamp(x,Ba,Ca);}f d dd(c l0,d I2,d A3){d kg=fract(0.06711056*l0.x+0.00583715*l0.y);d lg=fract(52.9829189*kg);return(lg*I2)+A3;}
#if 0
f d lj(c l0,float I2,float A3){int x=int(l0.x);int y=int(l0.y);int ed=(x^y);int b=(y>>1)&1;b|=(ed&2);b|=(y&1)<<2;b|=(ed&1)<<3;float mg=float(b);d ng=d4(mg)/16.0;return(ng*I2)+A3;}f d mj(c l0,float I2,float A3){l0.y*=0.5;l0.x=fract(l0.x*0.5+l0.y);l0.y=fract(l0.y);float e4=(l0.y*0.5+l0.x);return(e4*I2)+A3;}
#endif
#ifdef ENABLE_DITHER
f d Da(c l0,d I2,d A3){return ENABLE_DITHER?dd(l0,I2,A3):.0;}f v M2(v p,d o7,c l0,d I2,d A3){return(ENABLE_DITHER&&o7!=.0)?(dd(l0,I2,A3)+p):p;}f v M2(v p,d o7,d fd){return(ENABLE_DITHER&&o7!=.0)?(fd+p):p;}
#else
f d Da(c l0,float I2,float A3){return 0.;}f v M2(v p,d o7,c l0,d I2,d A3){return p;}f v M2(v p,d o7,d fd){return p;}
#endif
#ifdef VERTEX
f e F8(c gd,float og,float hd){return e(gd.x*og-1.,gd.y*hd-sign(hd),0.,1.);}
#ifndef RENDER_MODE_DEPTH_STENCIL
f e i8(Y B3,c P3,c Ea){c Fa=abs(B3[0])+abs(B3[1]);if(Fa.x!=.0&&Fa.y!=.0){c R=1./Fa;c B5=M0(B3,Ea)+P3;const float pg=.5;return e(B5,-B5)*R.xyxy+R.xyxy+pg;}else{return P3.xyxy;}}
#else
f float I8(uint qg,uint rg){float id=float((qg<<sg)|rg);
#if defined(xa)&&!defined(TARGET_SPIRV)
return id*uintBitsToFloat(0x34000000u)+uintBitsToFloat(0xbf7fffffu);
#else
return id*uintBitsToFloat(0x33800000u)+uintBitsToFloat(0x33000000u);
#endif
}
#ifdef ENABLE_CLIP_RECT
f void Ga(Y B3,c P3,c Ea p7){
#ifndef DISABLE_CLIP_DISTANCE_FOR_UBERSHADERS
if(any(notEqual(e(B3),e(.0,.0,.0,.0)))){c B5=M0(B3,Ea)+P3.xy;gl_ClipDistance[0]=B5.x+1.;gl_ClipDistance[1]=B5.y+1.;gl_ClipDistance[2]=1.-B5.x;gl_ClipDistance[3]=1.-B5.y;}else{gl_ClipDistance[0]=gl_ClipDistance[1]=gl_ClipDistance[2]=gl_ClipDistance[3]=P3.x-.5;}
#endif
}
#endif
#endif
#endif
#if defined(FRAGMENT)&&defined(RENDER_MODE_DEPTH_STENCIL)&&!defined(FIXED_FUNCTION_COLOR_OUTPUT)
f i Ha(S4 q7,int J8){if(J8==0xf){return(q7[0]+q7[1]+q7[2]+q7[3])*.25;}else{i tg=e(notEqual(J8&m6(1,2,4,8),m6(0,0,0,0)));i X=M0(q7,tg);int K8=(J8&5)+((J8>>1)&5);K8=(K8&3)+(K8>>2);X*=1./float(K8);return X;}}
#endif
