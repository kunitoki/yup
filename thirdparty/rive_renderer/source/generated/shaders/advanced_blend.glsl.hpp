#pragma once

#include "advanced_blend.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char advanced_blend[] = R"===(#ifdef EB
#ifdef VD
layout(
#ifdef SB
blend_support_all_equations
#else
blend_support_multiply,blend_support_screen,blend_support_overlay,blend_support_darken,blend_support_lighten,blend_support_colordodge,blend_support_colorburn,blend_support_hardlight,blend_support_softlight,blend_support_difference,blend_support_exclusion
#endif
)out;
#endif
#ifdef FB
#ifdef SB
d gb(r J1){return dot(J1,T0(.30,.59,.11));}r j9(r hb,r k9){d l9=gb(k9);r m9=hb-gb(hb);D ib=A2(l9,1.0-l9)/max(A2(n9),A2(-d3(m9),G5(m9)));d be=min(G0(1.0),min(ib.x,ib.y));return m9*be+l9;}r jb(r Q7,r kb,r k9){float ce=G5(kb)-d3(kb);Q7-=d3(Q7);float de=G5(Q7);float B2=ce/max(n9,de);return j9(Q7*B2,k9);}
#endif
r ee(r k0,i z1,X o9){r o0=B6(z1);r d1;switch(o9){case fe:d1=k0.xyz*o0.xyz;break;case ge:d1=k0.xyz+o0.xyz-k0.xyz*o0.xyz;break;case he:{r C6=k0*o0;d1=2.0*mix(C6,k0+o0-C6-0.5,greaterThan(o0,T0(0.5)));break;}case ie:d1=min(k0.xyz,o0.xyz);break;case je:d1=max(k0.xyz,o0.xyz);break;case ke:{z1.xyz=clamp(z1.xyz,T0(.0),z1.www);r lb=clamp(1.-k0,T0(.0),T0(1.))*z1.w;d1=mix(min(T0(1.),z1.xyz/lb),sign(z1.xyz),equal(lb,T0(.0)));break;}case me:{k0=clamp(k0,T0(.0),T0(1.));z1.xyz=clamp(z1.xyz,T0(.0),z1.www);if(z1.w==.0)z1.w=1.;r mb=z1.w-z1.xyz;d1=1.-mix(min(T0(1.),mb/(k0*z1.w)),sign(mb),equal(k0,T0(.0)));break;}case ne:{r C6=k0*o0;d1=2.0*mix(C6,k0+o0-C6-0.5,greaterThan(k0,T0(0.5)));break;}case oe:{for(int D0=0;D0<3;++D0){if(k0[D0]<=0.5)d1[D0]=(1.0-o0[D0]);else if(o0[D0]<=0.25)d1[D0]=((16.0*o0[D0]-12.0)*o0[D0]+3.0);else d1[D0]=(inversesqrt(o0[D0])-1.0);}d1=o0+o0*(2.0*k0-1.0)*d1;break;}case pe:d1=abs(o0.xyz-k0.xyz);break;case qe:d1=k0.xyz+o0.xyz-2.*k0.xyz*o0.xyz;break;
#ifdef SB
case re:if(SB){k0.xyz=clamp(k0.xyz,T0(.0),T0(1.));d1=jb(k0.xyz,o0.xyz,o0.xyz);}break;case se:if(SB){k0.xyz=clamp(k0.xyz,T0(.0),T0(1.));d1=jb(o0.xyz,k0.xyz,o0.xyz);}break;case te:if(SB){k0.xyz=clamp(k0.xyz,T0(.0),T0(1.));d1=j9(k0.xyz,o0.xyz);}break;case ue:if(SB){k0.xyz=clamp(k0.xyz,T0(.0),T0(1.));d1=j9(o0.xyz,k0.xyz);}break;
#endif
}return d1;}e r R4(r k0,i z1,X o9){r d1=ee(k0,z1,o9);return mix(k0,d1,T0(z1.w));}
#endif
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive