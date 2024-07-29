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
h m5(q g0){return min(min(g0.x,g0.y),g0.z);}h F6(q g0){return max(max(g0.x,g0.y),g0.z);}h n5(q g0){return dot(g0,c0(.30,.59,.11));}h G6(q g0){return F6(g0)-m5(g0);}q R8(q f){h j2=n5(f);h H6=m5(f);h I6=F6(f);if(H6<.0)f=j2+((f-j2)*j2)/(j2-H6);if(I6>1.)f=j2+((f-j2)*(1.-j2))/(I6-j2);return f;}q o5(q n3,q p5){h S8=n5(n3);h T8=n5(p5);h q5=T8-S8;q f=n3+c0(q5,q5,q5);return R8(f);}q J6(q n3,q U8,q p5){h V8=m5(n3);h K6=G6(n3);h W8=G6(U8);q f;if(K6>.0){f=(n3-V8)*W8/K6;}else{f=c0(0,0,0);}return o5(f,p5);}
#endif
#ifdef ENABLE_HSL_BLEND_MODES
j J3(j r,j o,N L6)
#else
j K3(j r,j o,N L6)
#endif
{q h0=c0(0,0,0);switch(L6){case X8:h0=r.xyz*o.xyz;break;case Y8:h0=r.xyz+o.xyz-r.xyz*o.xyz;break;case Z8:{for(int l=0;l<3;++l){if(o[l]<=.5)h0[l]=2.*r[l]*o[l];else h0[l]=1.-2.*(1.-r[l])*(1.-o[l]);}break;}case a9:h0=min(r.xyz,o.xyz);break;case c9:h0=max(r.xyz,o.xyz);break;case d9:h0=mix(min(o.xyz/(1.-r.xyz),c0(1,1,1)),c0(0,0,0),lessThanEqual(o.xyz,c0(0,0,0)));break;case e9:h0=mix(1.-min((1.-o.xyz)/r.xyz,1.),c0(1,1,1),greaterThanEqual(o.xyz,c0(1,1,1)));break;case f9:{for(int l=0;l<3;++l){if(r[l]<=.5)h0[l]=2.*r[l]*o[l];else h0[l]=1.-2.*(1.-r[l])*(1.-o[l]);}break;}case g9:{for(int l=0;l<3;++l){if(r[l]<=0.5)h0[l]=o[l]-(1.-2.*r[l])*o[l]*(1.-o[l]);else if(o[l]<=.25)h0[l]=o[l]+(2.*r[l]-1.)*o[l]*((16.*o[l]-12.)*o[l]+3.);else h0[l]=o[l]+(2.*r[l]-1.)*(sqrt(o[l])-o[l]);}break;}case h9:h0=abs(o.xyz-r.xyz);break;case i9:h0=r.xyz+o.xyz-2.*r.xyz*o.xyz;break;
#ifdef ENABLE_HSL_BLEND_MODES
case j9:r.xyz=clamp(r.xyz,c0(0,0,0),c0(1,1,1));h0=J6(r.xyz,o.xyz,o.xyz);break;case k9:r.xyz=clamp(r.xyz,c0(0,0,0),c0(1,1,1));h0=J6(o.xyz,r.xyz,o.xyz);break;case l9:r.xyz=clamp(r.xyz,c0(0,0,0),c0(1,1,1));h0=o5(r.xyz,o.xyz);break;case m9:r.xyz=clamp(r.xyz,c0(0,0,0),c0(1,1,1));h0=o5(o.xyz,r.xyz);break;
#endif
}q L3=c0(r.w*o.w,r.w*(1.-o.w),(1.-r.w)*o.w);return l0(r5(h0,1,r.xyz,1,o.xyz,1),L3);}
#endif
