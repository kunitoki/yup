#ifdef FRAGMENT
layout(input_attachment_index=0,
#ifdef INPUT_ATTACHMENT_BINDING
binding=INPUT_ATTACHMENT_BINDING,
#else
binding=0,
#endif
set=C3) uniform lowp subpassInput cj;layout(location=0) out i bc;void main(){bc=subpassLoad(cj);}
#endif
