#ifdef FRAGMENT
layout(input_attachment_index=0,binding=P2,set=B3)uniform lowp subpassInputMS i9;layout(location=0)out i eb;void main(){eb=(subpassLoad(i9,0)+subpassLoad(i9,1)+subpassLoad(i9,2)+subpassLoad(i9,3))*.25;}
#endif
