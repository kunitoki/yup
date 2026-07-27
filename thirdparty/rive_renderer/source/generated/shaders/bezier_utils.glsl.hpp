#pragma once

#include "bezier_utils.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char bezier_utils[] = R"===(#ifndef Hb
#define Hb g
#endif
#ifndef J6
#define J6 c
#endif
e float K9(c o,c b){float Le=dot(o,b);float Ib=dot(o,o)*dot(b,b);return(Ib==.0)?1.:clamp(Le*inversesqrt(Ib),-1.,1.);}e void Me(c w0,c x0,c E0,c I0,e1(c)A,e1(c)F,e1(c)d2){d2=x0-w0;c K6=E0-x0;c h8=I0-w0;F=K6-d2;A=-3.*K6+h8;}e Z L9(c w0,c x0,c E0,c I0){Z t;t[0]=(any(notEqual(w0,x0))?x0:any(notEqual(x0,E0))?E0:I0)-w0;t[1]=I0-(any(notEqual(I0,E0))?E0:any(notEqual(E0,x0))?x0:w0);return t;}e float Ne(c w0,c x0,c E0,c I0,float w1,float Oe){c A,F,d2;Me(w0,x0,E0,I0,A,F,d2);c L6=3.*(((A*w1)+2.*F)*w1+d2);float Jb=length(L6);if(Jb==.0){return.0;}L6*=1./Jb;float i8=2.*dot(A,L6);float M6=3.*(i8*w1+4.*dot(F,L6))*w1+6.*dot(d2,L6);float M9=min(w1,1.-w1);float Pe=(i8*M9*M9+M6)*M9;float Kb=min(Oe,Pe*.9999);float V2;if(i8==.0){V2=Kb/M6;}else{float H=1./i8;float b=M6*H,I1=-Kb*H;float N6=(-1./3.)*b,O6=.5*I1;float Lb=O6*O6-N6*N6*N6;if(Lb<.0){float j8=sqrt(N6);float h1=acos(O6/(j8*j8*j8));V2=-2.*j8*cos(h1*(1./3.)+(-A3*2./3.));}else{float A=pow(abs(O6)+sqrt(Lb),1./3.);if(O6<.0)A=-A;V2=A!=.0?A+N6/A:.0;}}V2=abs(V2);g t0011=w1+Hb(-V2,-V2,V2,V2);g Mb=(A.xyxy*t0011+2.*F.xyxy)*t0011+d2.xyxy;Z F2=L9(w0,x0,E0,I0);c Qe=t0011.x<1e-3?F2[0]:Mb.xy;c Re=t0011.z>1.-1e-3?F2[1]:Mb.zw;return acos(K9(Qe,Re));}e float k8(float o,float b){o=b<.0?-o:o;b=abs(b);return o>.0?(o<b?o/b:1.):.0;}float Se(c w0,c x0,c E0,c I0,e1(float)N9){c Nb=I0-w0;float Ob=length(I0-w0);if(Ob==.0){N9=.5;return.0;}c W2=J6(-Nb.y,Nb.x)/Ob;float Pb=dot(W2,E0-w0);float y4=dot(W2,x0-w0);float z4=y4-Pb;
#if 0
float o=3.*z4;float Qb=z4+y4;float I1=y4;float q2=sqrt(max(z4*z4+Pb*y4,.0));if(Qb<.0)q2=-q2;q2+=Qb;c P6=J6(k8(q2,o),k8(I1,q2));c T5=3.*(P6*(P6*(P6*z4-(y4+z4))+y4));T5=abs(T5);N9=T5.x>T5.y?P6.x:P6.y;return max(T5.x,T5.y);
#else
float Rb=3.*z4;float F=-y4-z4;float d2=y4;float t=.5;for(int D0=0;D0<3;++D0){float Sb=Rb*t;t=k8(Sb*t-d2,2.*(Sb+F));}N9=t;return abs(t*(t*(t*Rb+3.*F)+3.*d2));
#endif
}
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive