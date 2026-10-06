#ifdef FRAGMENT
layout(input_attachment_index=0,binding=K2,set=C3) uniform lowp subpassInputMS E9;layout(location=0) out i bc;void main(){bc=(subpassLoad(E9,0)+subpassLoad(E9,1)+subpassLoad(E9,2)+subpassLoad(E9,3))*.25;}
#endif
