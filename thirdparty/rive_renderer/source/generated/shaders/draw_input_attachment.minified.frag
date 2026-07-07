#ifdef FRAGMENT
layout(input_attachment_index=0,
#ifdef INPUT_ATTACHMENT_BINDING
binding=INPUT_ATTACHMENT_BINDING,
#else
binding=0,
#endif
set=E3)uniform lowp subpassInput Ah;layout(location=0)out i jb;void main(){jb=subpassLoad(Ah);}
#endif
