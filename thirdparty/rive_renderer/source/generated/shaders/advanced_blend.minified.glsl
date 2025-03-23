#ifdef FRAGMENT
#ifdef ENABLE_KHR_BLEND
layout(
#ifdef ENABLE_HSL_BLEND_MODES
blend_support_all_equations
#else
blend_support_multiply,blend_support_screen,blend_support_overlay,blend_support_darken,blend_support_lighten,blend_support_colordodge,blend_support_colorburn,blend_support_hardlight,blend_support_softlight,blend_support_difference,blend_support_exclusion
#endif
)out;
#endif
#ifdef ENABLE_ADVANCED_BLEND
#ifdef ENABLE_HSL_BLEND_MODES
h a7(p D0){return min(min(D0.x,D0.y),D0.z);}h Q8(p D0){return max(max(D0.x,D0.y),D0.z);}h c7(p D0){return dot(D0,K0(.30,.59,.11));}h R8(p D0){return Q8(D0)-a7(D0);}p db(p j){h f3=c7(j);h S8=a7(j);h T8=Q8(j);if(S8<.0)j=f3+((j-f3)*f3)/(f3-S8);if(T8>1.)j=f3+((j-f3)*(1.-f3))/(T8-f3);return j;}p d7(p r4,p e7){h eb=c7(r4);h fb=c7(e7);h gb=fb-eb;p j=r4+K0(gb);return db(j);}p U8(p r4,p hb,p e7){h ib=a7(r4);h V8=R8(r4);h jb=R8(hb);p j;if(V8>.0){j=(r4-ib)*jb/V8;}else{j=K0(.0);}return d7(j,e7);}
#endif
p W8(p W,p B,O W4){p y0;switch(W4){case kb:y0=W.xyz*B.xyz;break;case lb:y0=W.xyz+B.xyz-W.xyz*B.xyz;break;case mb:{for(int v=0;v<3;++v){if(B[v]<=.5)y0[v]=2.*W[v]*B[v];else y0[v]=1.-2.*(1.-W[v])*(1.-B[v]);}break;}case nb:y0=min(W.xyz,B.xyz);break;case ob:y0=max(W.xyz,B.xyz);break;case pb:y0=mix(min(B.xyz/(1.-W.xyz),K0(1.)),K0(.0),lessThanEqual(B.xyz,K0(.0)));break;case qb:y0=mix(1.-min((1.-B.xyz)/W.xyz,1.),K0(1.),greaterThanEqual(B.xyz,K0(1.)));break;case rb:{for(int v=0;v<3;++v){if(W[v]<=.5)y0[v]=2.*W[v]*B[v];else y0[v]=1.-2.*(1.-W[v])*(1.-B[v]);}break;}case sb:{for(int v=0;v<3;++v){if(W[v]<=0.5)y0[v]=B[v]-(1.-2.*W[v])*B[v]*(1.-B[v]);else if(B[v]<=.25)y0[v]=B[v]+(2.*W[v]-1.)*B[v]*((16.*B[v]-12.)*B[v]+3.);else y0[v]=B[v]+(2.*W[v]-1.)*(sqrt(B[v])-B[v]);}break;}case tb:y0=abs(B.xyz-W.xyz);break;case ub:y0=W.xyz+B.xyz-2.*W.xyz*B.xyz;break;
#ifdef ENABLE_HSL_BLEND_MODES
case vb:if(ENABLE_HSL_BLEND_MODES){W.xyz=clamp(W.xyz,K0(.0),K0(1.));y0=U8(W.xyz,B.xyz,B.xyz);break;}case wb:if(ENABLE_HSL_BLEND_MODES){W.xyz=clamp(W.xyz,K0(.0),K0(1.));y0=U8(B.xyz,W.xyz,B.xyz);break;}case xb:if(ENABLE_HSL_BLEND_MODES){W.xyz=clamp(W.xyz,K0(.0),K0(1.));y0=d7(W.xyz,B.xyz);break;}case yb:if(ENABLE_HSL_BLEND_MODES){W.xyz=clamp(W.xyz,K0(.0),K0(1.));y0=d7(B.xyz,W.xyz);break;}
#endif
}return y0;}d i R5(i W,i B,O W4){p y0=W8(W.xyz,B.xyz,W4);h f7=W.w*B.w;p P3=K0(f7,W.w-f7,B.w-f7);return f2(q0(zb(y0,W.xyz,B.xyz),P3),B.w*(1.-W.w)+W.w);}d p X8(p W,i B,O W4){p y0=W8(W,B.xyz,W4);C P3=Q3(B.w,1.-B.w);return q0(Ab(y0,W),P3);}
#endif
#endif
