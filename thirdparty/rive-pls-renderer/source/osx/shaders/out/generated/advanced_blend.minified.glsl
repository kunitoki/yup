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
h d5(O f0){return min(min(f0.x,f0.y),f0.z);}h w6(O f0){return max(max(f0.x,f0.y),f0.z);}h e5(O f0){return dot(f0,j0(.30,.59,.11));}h x6(O f0){return w6(f0)-d5(f0);}O A8(O f){h g2=e5(f);h y6=d5(f);h z6=w6(f);if(y6<.0)f=g2+((f-g2)*g2)/(g2-y6);if(z6>1.)f=g2+((f-g2)*(1.-g2))/(z6-g2);return f;}O f5(O h3,O g5){h B8=e5(h3);h C8=e5(g5);h h5=C8-B8;O f=h3+j0(h5,h5,h5);return A8(f);}O A6(O h3,O D8,O g5){h E8=d5(h3);h B6=x6(h3);h F8=x6(D8);O f;if(B6>.0){f=(h3-E8)*F8/B6;}else{f=j0(0,0,0);}return f5(f,g5);}
#endif
#ifdef ENABLE_HSL_BLEND_MODES
i D3(i q,i n,M C6)
#else
i E3(i q,i n,M C6)
#endif
{O V=j0(0,0,0);switch(C6){case G8:V=q.xyz*n.xyz;break;case H8:V=q.xyz+n.xyz-q.xyz*n.xyz;break;case I8:{for(int k=0;k<3;++k){if(n[k]<=.5)V[k]=2.*q[k]*n[k];else V[k]=1.-2.*(1.-q[k])*(1.-n[k]);}break;}case J8:V=min(q.xyz,n.xyz);break;case K8:V=max(q.xyz,n.xyz);break;case L8:V=mix(min(n.xyz/(1.-q.xyz),j0(1,1,1)),j0(0,0,0),lessThanEqual(n.xyz,j0(0,0,0)));break;case M8:V=mix(1.-min((1.-n.xyz)/q.xyz,1.),j0(1,1,1),greaterThanEqual(n.xyz,j0(1,1,1)));break;case N8:{for(int k=0;k<3;++k){if(q[k]<=.5)V[k]=2.*q[k]*n[k];else V[k]=1.-2.*(1.-q[k])*(1.-n[k]);}break;}case O8:{for(int k=0;k<3;++k){if(q[k]<=0.5)V[k]=n[k]-(1.-2.*q[k])*n[k]*(1.-n[k]);else if(n[k]<=.25)V[k]=n[k]+(2.*q[k]-1.)*n[k]*((16.*n[k]-12.)*n[k]+3.);else V[k]=n[k]+(2.*q[k]-1.)*(sqrt(n[k])-n[k]);}break;}case P8:V=abs(n.xyz-q.xyz);break;case Q8:V=q.xyz+n.xyz-2.*q.xyz*n.xyz;break;
#ifdef ENABLE_HSL_BLEND_MODES
case R8:q.xyz=clamp(q.xyz,j0(0,0,0),j0(1,1,1));V=A6(q.xyz,n.xyz,n.xyz);break;case S8:q.xyz=clamp(q.xyz,j0(0,0,0),j0(1,1,1));V=A6(n.xyz,q.xyz,n.xyz);break;case T8:q.xyz=clamp(q.xyz,j0(0,0,0),j0(1,1,1));V=f5(q.xyz,n.xyz);break;case U8:q.xyz=clamp(q.xyz,j0(0,0,0),j0(1,1,1));V=f5(n.xyz,q.xyz);break;
#endif
}O F3=j0(q.w*n.w,q.w*(1.-n.w),(1.-q.w)*n.w);return h0(i5(V,1,q.xyz,1,n.xyz,1),F3);}
#endif
