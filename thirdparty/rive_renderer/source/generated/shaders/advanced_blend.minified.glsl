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
g V7(A B0){return min(min(B0.x,B0.y),B0.z);}g N9(A B0){return max(max(B0.x,B0.y),B0.z);}g W7(A B0){return dot(B0,K0(.30,.59,.11));}g O9(A B0){return N9(B0)-V7(B0);}A kc(A j){g p3=W7(j);g P9=V7(j);g Q9=N9(j);if(P9<.0)j=p3+((j-p3)*p3)/(p3-P9);if(Q9>1.)j=p3+((j-p3)*(1.-p3))/(Q9-p3);return j;}A X7(A L4,A Y7){g lc=W7(L4);g mc=W7(Y7);g nc=mc-lc;A j=L4+K0(nc);return kc(j);}A R9(A L4,A oc,A Y7){g pc=V7(L4);g S9=O9(L4);g qc=O9(oc);A j;if(S9>.0){j=(L4-pc)*qc/S9;}else{j=K0(.0);}return X7(j,Y7);}
#endif
A rc(A e0,i X0,a0 Z7){A h0=Y3(X0);A L0;switch(Z7){case sc:L0=e0.xyz*h0.xyz;break;case tc:L0=e0.xyz+h0.xyz-e0.xyz*h0.xyz;break;case uc:{for(int C=0;C<3;++C){if(h0[C]<=.5)L0[C]=2.*e0[C]*h0[C];else L0[C]=1.-2.*(1.-e0[C])*(1.-h0[C]);}break;}case vc:L0=min(e0.xyz,h0.xyz);break;case wc:L0=max(e0.xyz,h0.xyz);break;case xc:{X0.xyz=clamp(X0.xyz,K0(.0),X0.www);A T9=clamp(1.-e0,K0(.0),K0(1.))*X0.w;L0=mix(min(K0(1.),X0.xyz/T9),sign(X0.xyz),equal(T9,K0(.0)));break;}case zc:{e0=clamp(e0,K0(.0),K0(1.));X0.xyz=clamp(X0.xyz,K0(.0),X0.www);if(X0.w==.0)X0.w=1.;A U9=X0.w-X0.xyz;L0=1.-mix(min(K0(1.),U9/(e0*X0.w)),sign(U9),equal(e0,K0(.0)));break;}case Ac:{for(int C=0;C<3;++C){if(e0[C]<=.5)L0[C]=2.*e0[C]*h0[C];else L0[C]=1.-2.*(1.-e0[C])*(1.-h0[C]);}break;}case Bc:{for(int C=0;C<3;++C){if(e0[C]<=0.5)L0[C]=h0[C]-(1.-2.*e0[C])*h0[C]*(1.-h0[C]);else if(h0[C]<=.25)L0[C]=h0[C]+(2.*e0[C]-1.)*h0[C]*((16.*h0[C]-12.)*h0[C]+3.);else L0[C]=h0[C]+(2.*e0[C]-1.)*(sqrt(h0[C])-h0[C]);}break;}case Cc:L0=abs(h0.xyz-e0.xyz);break;case Dc:L0=e0.xyz+h0.xyz-2.*e0.xyz*h0.xyz;break;
#ifdef ENABLE_HSL_BLEND_MODES
case Ec:if(ENABLE_HSL_BLEND_MODES){e0.xyz=clamp(e0.xyz,K0(.0),K0(1.));L0=R9(e0.xyz,h0.xyz,h0.xyz);}break;case Fc:if(ENABLE_HSL_BLEND_MODES){e0.xyz=clamp(e0.xyz,K0(.0),K0(1.));L0=R9(h0.xyz,e0.xyz,h0.xyz);}break;case Gc:if(ENABLE_HSL_BLEND_MODES){e0.xyz=clamp(e0.xyz,K0(.0),K0(1.));L0=X7(e0.xyz,h0.xyz);}break;case Hc:if(ENABLE_HSL_BLEND_MODES){e0.xyz=clamp(e0.xyz,K0(.0),K0(1.));L0=X7(h0.xyz,e0.xyz);}break;
#endif
}return L0;}d A M4(A e0,i X0,a0 Z7){A L0=rc(e0,X0,Z7);G z5=Z3(X0.w,1.-X0.w);return C0(Ic(L0,e0),z5);}
#endif
#endif
