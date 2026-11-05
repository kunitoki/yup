#ifdef FRAGMENT
layout(input_attachment_index=0,binding=B2,set=N3) uniform lowp subpassInputMS ra;layout(location=0) out i ed;void main(){ed=(subpassLoad(ra,0)+subpassLoad(ra,1)+subpassLoad(ra,2)+subpassLoad(ra,3))*.25;}
#endif
