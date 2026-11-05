#ifdef FRAGMENT
layout(input_attachment_index=0,
#ifdef INPUT_ATTACHMENT_BINDING
binding=INPUT_ATTACHMENT_BINDING,
#else
binding=0,
#endif
set=N3) uniform lowp subpassInput tk;layout(location=0) out i ed;void main(){ed=subpassLoad(tk);}
#endif
