#pragma once

#include "advanced_blend.exports.h"

namespace rive {
namespace pls {
namespace glsl {
const char advanced_blend[] = R"===(#ifdef MC
layout(
#ifdef JB
blend_support_all_equations
#else
blend_support_multiply,blend_support_screen,blend_support_overlay,blend_support_darken,blend_support_lighten,blend_support_colordodge,blend_support_colorburn,blend_support_hardlight,blend_support_softlight,blend_support_difference,blend_support_exclusion
#endif
)out;
#endif
#ifdef AB
#ifdef JB
h e5(O e0){return min(min(e0.x,e0.y),e0.z);}h x6(O e0){return max(max(e0.x,e0.y),e0.z);}h f5(O e0){return dot(e0,j0(.30,.59,.11));}h y6(O e0){return x6(e0)-e5(e0);}O C8(O f){h h2=f5(f);h z6=e5(f);h A6=x6(f);if(z6<.0)f=h2+((f-h2)*h2)/(h2-z6);if(A6>1.)f=h2+((f-h2)*(1.-h2))/(A6-h2);return f;}O g5(O i3,O h5){h D8=f5(i3);h E8=f5(h5);h i5=E8-D8;O f=i3+j0(i5,i5,i5);return C8(f);}O B6(O i3,O F8,O h5){h G8=e5(i3);h C6=y6(i3);h H8=y6(F8);O f;if(C6>.0){f=(i3-G8)*H8/C6;}else{f=j0(0,0,0);}return g5(f,h5);}
#endif
#ifdef JB
i E3(i q,i n,M D6)
#else
i F3(i q,i n,M D6)
#endif
{O f0=j0(0,0,0);switch(D6){case I8:f0=q.xyz*n.xyz;break;case J8:f0=q.xyz+n.xyz-q.xyz*n.xyz;break;case K8:{for(int k=0;k<3;++k){if(n[k]<=.5)f0[k]=2.*q[k]*n[k];else f0[k]=1.-2.*(1.-q[k])*(1.-n[k]);}break;}case L8:f0=min(q.xyz,n.xyz);break;case M8:f0=max(q.xyz,n.xyz);break;case N8:f0=mix(min(n.xyz/(1.-q.xyz),j0(1,1,1)),j0(0,0,0),lessThanEqual(n.xyz,j0(0,0,0)));break;case O8:f0=mix(1.-min((1.-n.xyz)/q.xyz,1.),j0(1,1,1),greaterThanEqual(n.xyz,j0(1,1,1)));break;case P8:{for(int k=0;k<3;++k){if(q[k]<=.5)f0[k]=2.*q[k]*n[k];else f0[k]=1.-2.*(1.-q[k])*(1.-n[k]);}break;}case Q8:{for(int k=0;k<3;++k){if(q[k]<=0.5)f0[k]=n[k]-(1.-2.*q[k])*n[k]*(1.-n[k]);else if(n[k]<=.25)f0[k]=n[k]+(2.*q[k]-1.)*n[k]*((16.*n[k]-12.)*n[k]+3.);else f0[k]=n[k]+(2.*q[k]-1.)*(sqrt(n[k])-n[k]);}break;}case R8:f0=abs(n.xyz-q.xyz);break;case S8:f0=q.xyz+n.xyz-2.*q.xyz*n.xyz;break;
#ifdef JB
case T8:q.xyz=clamp(q.xyz,j0(0,0,0),j0(1,1,1));f0=B6(q.xyz,n.xyz,n.xyz);break;case U8:q.xyz=clamp(q.xyz,j0(0,0,0),j0(1,1,1));f0=B6(n.xyz,q.xyz,n.xyz);break;case V8:q.xyz=clamp(q.xyz,j0(0,0,0),j0(1,1,1));f0=g5(q.xyz,n.xyz);break;case W8:q.xyz=clamp(q.xyz,j0(0,0,0),j0(1,1,1));f0=g5(n.xyz,q.xyz);break;
#endif
}O G3=j0(q.w*n.w,q.w*(1.-n.w),(1.-q.w)*n.w);return h0(j5(f0,1,q.xyz,1,n.xyz,1),G3);}
#endif
)===";
} // namespace glsl
} // namespace pls
} // namespace rive