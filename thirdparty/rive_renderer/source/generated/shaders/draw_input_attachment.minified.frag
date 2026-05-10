#ifdef FRAGMENT
layout(input_attachment_index=0,
#ifdef INPUT_ATTACHMENT_BINDING
binding=INPUT_ATTACHMENT_BINDING,
#else
binding=0,
#endif
set=B3)uniform lowp subpassInput sh;layout(location=0)out i eb;void main(){eb=subpassLoad(sh);}
#endif
