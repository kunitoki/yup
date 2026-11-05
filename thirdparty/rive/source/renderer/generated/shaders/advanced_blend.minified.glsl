#ifdef FRAGMENT
#ifdef ENABLE_KHR_BLEND
layout(
#ifdef ENABLE_HSL_BLEND_MODES
blend_support_all_equations
#else
blend_support_multiply,blend_support_screen,blend_support_overlay,blend_support_darken,blend_support_lighten,blend_support_colordodge,blend_support_colorburn,blend_support_hardlight,blend_support_softlight,blend_support_difference,blend_support_exclusion
#endif
) out;
#endif
#ifdef ENABLE_ADVANCED_BLEND
#ifdef ENABLE_HSL_BLEND_MODES
d gd(v Y1){return dot(Y1,e1(.30,.59,.11));}v sa(v hd,v ta){d ua=gd(ta);v va=hd-gd(hd);F id=V2(ua,1.0-ua)/max(V2(wa),V2(-E3(va),n6(va)));d eg=min(B0(1.0),min(id.x,id.y));return va*eg+ua;}v jd(v M8,v kd,v ta){float fg=n6(kd)-E3(kd);M8-=E3(M8);float gg=n6(M8);float W2=fg/max(wa,gg);return sa(M8*W2,ta);}
#endif
v hg(v w0,i B1,O F3){v C0=o6(B1);v m1;switch(F3){case ig:m1=w0.xyz*C0.xyz;break;case jg:m1=w0.xyz+C0.xyz-w0.xyz*C0.xyz;break;case kg:{v l7=w0*C0;m1=2.0*mix(l7,w0+C0-l7-0.5,greaterThan(C0,e1(0.5)));break;}case lg:m1=min(w0.xyz,C0.xyz);break;case mg:m1=max(w0.xyz,C0.xyz);break;case ng:{B1.xyz=clamp(B1.xyz,e1(.0),B1.www);v ld=clamp(1.-w0,e1(.0),e1(1.))*B1.w;m1=mix(min(e1(1.),B1.xyz/ld),sign(B1.xyz),equal(ld,e1(.0)));break;}case pg:{w0=clamp(w0,e1(.0),e1(1.));B1.xyz=clamp(B1.xyz,e1(.0),B1.www);if(B1.w==.0) B1.w=1.;v md=B1.w-B1.xyz;m1=1.-mix(min(e1(1.),md/(w0*B1.w)),sign(md),equal(w0,e1(.0)));break;}case qg:{v l7=w0*C0;m1=2.0*mix(l7,w0+C0-l7-0.5,greaterThan(w0,e1(0.5)));break;}case rg:{for(int P0=0;P0<3;++P0){if(w0[P0]<=0.5) m1[P0]=(1.0-C0[P0]);else if(C0[P0]<=0.25) m1[P0]=((16.0*C0[P0]-12.0)*C0[P0]+3.0);else m1[P0]=(inversesqrt(C0[P0])-1.0);}m1=C0+C0*(2.0*w0-1.0)*m1;break;}case sg:m1=abs(C0.xyz-w0.xyz);break;case tg:m1=w0.xyz+C0.xyz-2.*w0.xyz*C0.xyz;break;
#ifdef ENABLE_HSL_BLEND_MODES
case ug:if(ENABLE_HSL_BLEND_MODES){w0.xyz=clamp(w0.xyz,e1(.0),e1(1.));m1=jd(w0.xyz,C0.xyz,C0.xyz);}break;case vg:if(ENABLE_HSL_BLEND_MODES){w0.xyz=clamp(w0.xyz,e1(.0),e1(1.));m1=jd(C0.xyz,w0.xyz,C0.xyz);}break;case wg:if(ENABLE_HSL_BLEND_MODES){w0.xyz=clamp(w0.xyz,e1(.0),e1(1.));m1=sa(w0.xyz,C0.xyz);}break;case xg:if(ENABLE_HSL_BLEND_MODES){w0.xyz=clamp(w0.xyz,e1(.0),e1(1.));m1=sa(C0.xyz,w0.xyz);}break;
#endif
}return m1;}f v Y4(v w0,i B1,O F3){v m1=hg(w0,B1,F3);return mix(w0,m1,e1(B1.w));}
#endif
#endif
