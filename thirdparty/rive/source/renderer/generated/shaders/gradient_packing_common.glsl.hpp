#pragma once

#include "gradient_packing_common.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char gradient_packing_common[] = R"===(#define uc 8.0
#ifdef CB
f e La(c n0,X ba,c C2,c v8,uint x5,float w4,float n){e W1;W1.xy=G0(ba,n0)+C2;bool v4=(v8.x<0.0);v8.x=max(v8.x,0.0);W1.z=v8.y+(float(x5)/uc)+(v8.x*(l5/uc));W1.z=(v4)?-W1.z:W1.z;W1.w=(n*-0.5-0.25)-round(w4*255.0);return W1;}
#endif
#ifdef EB
f c pf(float x5,c w8,c e3,bool v4){
#ifdef JE
if(JE&&x5==float(Qh)){float t=nb(w8)*(1.0/D7);return c(t,e3.y);}else
#endif
{float t=(x5==float(Ph))?w8.x:length(w8.xy);t=clamp(t,0.0,1.0);float hj=v4?(1.0-l5):l5;float x=t*hj+e3.x;return c(x,e3.y);}}f c Mg(float x5,c w8,c qf,bool v4,float vc,float wc){const float xc=0.5*l5;c e3=c(qf.x*l5+xc,qf.y*vc+wc);return pf(x5,w8,e3,v4);}f c cb(e W1,float vc,float wc){const float xc=0.5*l5;bool v4=W1.z<0.0;W1.z=abs(W1.z);c e3;e3.y=floor(W1.z);e3.x=(W1.z-e3.y)*uc+xc;e3.y=e3.y*vc+wc;float x5=floor(e3.x);return pf(x5,W1.xy,e3,v4);}f d Ci(e W1){return fract(W1.w)*-2.0+1.5;}f d Ee(e W1){const float ij=-1.0/255.0;return ceil(W1.w)*ij;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive