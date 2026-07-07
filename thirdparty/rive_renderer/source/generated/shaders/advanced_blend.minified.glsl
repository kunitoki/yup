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
d lb(r I1){return dot(I1,T0(.30,.59,.11));}r j9(r mb,r k9){d l9=lb(k9);r m9=mb-lb(mb);D nb=A2(l9,1.0-l9)/max(A2(n9),A2(-g3(m9),I5(m9)));d ge=min(G0(1.0),min(nb.x,nb.y));return m9*ge+l9;}r ob(r Q7,r pb,r k9){float he=I5(pb)-g3(pb);Q7-=g3(Q7);float ie=I5(Q7);float B2=he/max(n9,ie);return j9(Q7*B2,k9);}
#endif
r je(r j0,i z1,X o9){r n0=B6(z1);r c1;switch(o9){case ke:c1=j0.xyz*n0.xyz;break;case le:c1=j0.xyz+n0.xyz-j0.xyz*n0.xyz;break;case me:{r C6=j0*n0;c1=2.0*mix(C6,j0+n0-C6-0.5,greaterThan(n0,T0(0.5)));break;}case ne:c1=min(j0.xyz,n0.xyz);break;case oe:c1=max(j0.xyz,n0.xyz);break;case pe:{z1.xyz=clamp(z1.xyz,T0(.0),z1.www);r qb=clamp(1.-j0,T0(.0),T0(1.))*z1.w;c1=mix(min(T0(1.),z1.xyz/qb),sign(z1.xyz),equal(qb,T0(.0)));break;}case re:{j0=clamp(j0,T0(.0),T0(1.));z1.xyz=clamp(z1.xyz,T0(.0),z1.www);if(z1.w==.0)z1.w=1.;r rb=z1.w-z1.xyz;c1=1.-mix(min(T0(1.),rb/(j0*z1.w)),sign(rb),equal(j0,T0(.0)));break;}case se:{r C6=j0*n0;c1=2.0*mix(C6,j0+n0-C6-0.5,greaterThan(j0,T0(0.5)));break;}case te:{for(int D0=0;D0<3;++D0){if(j0[D0]<=0.5)c1[D0]=(1.0-n0[D0]);else if(n0[D0]<=0.25)c1[D0]=((16.0*n0[D0]-12.0)*n0[D0]+3.0);else c1[D0]=(inversesqrt(n0[D0])-1.0);}c1=n0+n0*(2.0*j0-1.0)*c1;break;}case ue:c1=abs(n0.xyz-j0.xyz);break;case ve:c1=j0.xyz+n0.xyz-2.*j0.xyz*n0.xyz;break;
#ifdef ENABLE_HSL_BLEND_MODES
case we:if(ENABLE_HSL_BLEND_MODES){j0.xyz=clamp(j0.xyz,T0(.0),T0(1.));c1=ob(j0.xyz,n0.xyz,n0.xyz);}break;case xe:if(ENABLE_HSL_BLEND_MODES){j0.xyz=clamp(j0.xyz,T0(.0),T0(1.));c1=ob(n0.xyz,j0.xyz,n0.xyz);}break;case ye:if(ENABLE_HSL_BLEND_MODES){j0.xyz=clamp(j0.xyz,T0(.0),T0(1.));c1=j9(j0.xyz,n0.xyz);}break;case ze:if(ENABLE_HSL_BLEND_MODES){j0.xyz=clamp(j0.xyz,T0(.0),T0(1.));c1=j9(n0.xyz,j0.xyz);}break;
#endif
}return c1;}e r Q4(r j0,i z1,X o9){r c1=je(j0,z1,o9);return mix(j0,c1,T0(z1.w));}
#endif
#endif
