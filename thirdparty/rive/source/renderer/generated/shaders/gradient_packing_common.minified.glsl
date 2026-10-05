#ifdef VERTEX
f e Y9(c l0,Y k9,c m2,float Uh,c ke,float y){e x2;x2.w=y;c le=M0(k9,l0)+m2;float Vh=ke.x;if(Vh>0.9){x2.z=2.0;}else{x2.z=ke.y;}if(Uh==float(uc)){x2.x=le.x;x2.y=0.0;}else{x2.z=-x2.z;x2.xy=le;}return x2;}
#endif
#ifdef FRAGMENT
f c Cc(e x2){float t=x2.z>0.0?x2.x:length(x2.xy);t=clamp(t,0.0,1.0);float me=abs(x2.z);float x=me>1.0?(1.0-1.0/Ka)*t+(0.5/Ka):(1.0/Ka)*t+me;float Wh=x2.w;return c(x,Wh);}
#endif
