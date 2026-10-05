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
d dc(v O1){return dot(O1,W0(.30,.59,.11));}v F9(v ec,v G9){d H9=dc(G9);v I9=ec-dc(ec);C fc=H2(H9,1.0-H9)/max(H2(J9),H2(-v3(I9),Y5(I9)));d cf=min(H0(1.0),min(fc.x,fc.y));return I9*cf+H9;}v gc(v f8,v hc,v G9){float df=Y5(hc)-v3(hc);f8-=v3(f8);float ef=Y5(f8);float I2=df/max(J9,ef);return F9(f8*I2,G9);}
#endif
v ff(v r0,i G1,Q a4){v x0=Q6(G1);v g1;switch(a4){case gf:g1=r0.xyz*x0.xyz;break;case hf:g1=r0.xyz+x0.xyz-r0.xyz*x0.xyz;break;case jf:{v R6=r0*x0;g1=2.0*mix(R6,r0+x0-R6-0.5,greaterThan(x0,W0(0.5)));break;}case kf:g1=min(r0.xyz,x0.xyz);break;case lf:g1=max(r0.xyz,x0.xyz);break;case mf:{G1.xyz=clamp(G1.xyz,W0(.0),G1.www);v ic=clamp(1.-r0,W0(.0),W0(1.))*G1.w;g1=mix(min(W0(1.),G1.xyz/ic),sign(G1.xyz),equal(ic,W0(.0)));break;}case of:{r0=clamp(r0,W0(.0),W0(1.));G1.xyz=clamp(G1.xyz,W0(.0),G1.www);if(G1.w==.0) G1.w=1.;v jc=G1.w-G1.xyz;g1=1.-mix(min(W0(1.),jc/(r0*G1.w)),sign(jc),equal(r0,W0(.0)));break;}case pf:{v R6=r0*x0;g1=2.0*mix(R6,r0+x0-R6-0.5,greaterThan(r0,W0(0.5)));break;}case qf:{for(int L0=0;L0<3;++L0){if(r0[L0]<=0.5) g1[L0]=(1.0-x0[L0]);else if(x0[L0]<=0.25) g1[L0]=((16.0*x0[L0]-12.0)*x0[L0]+3.0);else g1[L0]=(inversesqrt(x0[L0])-1.0);}g1=x0+x0*(2.0*r0-1.0)*g1;break;}case rf:g1=abs(x0.xyz-r0.xyz);break;case sf:g1=r0.xyz+x0.xyz-2.*r0.xyz*x0.xyz;break;
#ifdef ENABLE_HSL_BLEND_MODES
case tf:if(ENABLE_HSL_BLEND_MODES){r0.xyz=clamp(r0.xyz,W0(.0),W0(1.));g1=gc(r0.xyz,x0.xyz,x0.xyz);}break;case uf:if(ENABLE_HSL_BLEND_MODES){r0.xyz=clamp(r0.xyz,W0(.0),W0(1.));g1=gc(x0.xyz,r0.xyz,x0.xyz);}break;case vf:if(ENABLE_HSL_BLEND_MODES){r0.xyz=clamp(r0.xyz,W0(.0),W0(1.));g1=F9(r0.xyz,x0.xyz);}break;case wf:if(ENABLE_HSL_BLEND_MODES){r0.xyz=clamp(r0.xyz,W0(.0),W0(1.));g1=F9(x0.xyz,r0.xyz);}break;
#endif
}return g1;}f v h5(v r0,i G1,Q a4){v g1=ff(r0,G1,a4);return mix(r0,g1,W0(G1.w));}
#endif
#endif
