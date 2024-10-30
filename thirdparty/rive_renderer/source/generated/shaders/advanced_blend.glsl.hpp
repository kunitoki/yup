#pragma once

#include "advanced_blend.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char advanced_blend[] = R"===(#ifdef EB
#ifdef ZC
layout(
#ifdef LB
blend_support_all_equations
#else
blend_support_multiply,blend_support_screen,blend_support_overlay,blend_support_darken,blend_support_lighten,blend_support_colordodge,blend_support_colorburn,blend_support_hardlight,blend_support_softlight,blend_support_difference,blend_support_exclusion
#endif
)out;
#endif
#ifdef BB
#ifdef LB
h I5(k z0){return min(min(z0.x,z0.y),z0.z);}h c7(k z0){return max(max(z0.x,z0.y),z0.z);}h J5(k z0){return dot(z0,H0(.30,.59,.11));}h d7(k z0){return c7(z0)-I5(z0);}k d9(k j){h B2=J5(j);h e7=I5(j);h f7=c7(j);if(e7<.0)j=B2+((j-B2)*B2)/(B2-e7);if(f7>1.)j=B2+((j-B2)*(1.-B2))/(f7-B2);return j;}k K5(k G3,k L5){h e9=J5(G3);h f9=J5(L5);h g9=f9-e9;k j=G3+H0(g9);return d9(j);}k g7(k G3,k h9,k L5){h i9=I5(G3);h h7=d7(G3);h j9=d7(h9);k j;if(h7>.0){j=(G3-i9)*j9/h7;}else{j=H0(.0);}return K5(j,L5);}
#endif
k i7(k F,k p,M h4){k l0;switch(h4){case k9:l0=F.xyz*p.xyz;break;case l9:l0=F.xyz+p.xyz-F.xyz*p.xyz;break;case m9:{for(int A=0;A<3;++A){if(p[A]<=.5)l0[A]=2.*F[A]*p[A];else l0[A]=1.-2.*(1.-F[A])*(1.-p[A]);}break;}case n9:l0=min(F.xyz,p.xyz);break;case o9:l0=max(F.xyz,p.xyz);break;case p9:l0=mix(min(p.xyz/(1.-F.xyz),H0(1.)),H0(.0),lessThanEqual(p.xyz,H0(.0)));break;case q9:l0=mix(1.-min((1.-p.xyz)/F.xyz,1.),H0(1.,1.,1.),greaterThanEqual(p.xyz,H0(1.)));break;case r9:{for(int A=0;A<3;++A){if(F[A]<=.5)l0[A]=2.*F[A]*p[A];else l0[A]=1.-2.*(1.-F[A])*(1.-p[A]);}break;}case v9:{for(int A=0;A<3;++A){if(F[A]<=0.5)l0[A]=p[A]-(1.-2.*F[A])*p[A]*(1.-p[A]);else if(p[A]<=.25)l0[A]=p[A]+(2.*F[A]-1.)*p[A]*((16.*p[A]-12.)*p[A]+3.);else l0[A]=p[A]+(2.*F[A]-1.)*(sqrt(p[A])-p[A]);}break;}case w9:l0=abs(p.xyz-F.xyz);break;case x9:l0=F.xyz+p.xyz-2.*F.xyz*p.xyz;break;
#ifdef LB
case y9:if(LB){F.xyz=clamp(F.xyz,H0(.0),H0(1.));l0=g7(F.xyz,p.xyz,p.xyz);break;}case z9:if(LB){F.xyz=clamp(F.xyz,H0(.0),H0(1.));l0=g7(p.xyz,F.xyz,p.xyz);break;}case A9:if(LB){F.xyz=clamp(F.xyz,H0(.0),H0(1.));l0=K5(F.xyz,p.xyz);break;}case B9:if(LB){F.xyz=clamp(F.xyz,H0(.0),H0(1.));l0=K5(p.xyz,F.xyz);break;}
#endif
}return l0;}d i S4(i F,i p,M h4){k l0=i7(F.xyz,p.xyz,h4);h M5=F.w*p.w;k X2=H0(M5,F.w-M5,p.w-M5);return C2(m0(C9(l0,F.xyz,p.xyz),X2),p.w*(1.-F.w)+F.w);}d k j7(k F,i p,M h4){k l0=i7(F,p.xyz,h4);r X2=D2(p.w,1.-p.w);return m0(D9(l0,F),X2);}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive