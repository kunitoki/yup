#ifdef VERTEX
void main(){gl_Position.x=(gl_VertexID&1)==0?-1.:1.;gl_Position.y=(gl_VertexID&2)==0?-1.:1.;gl_Position.z=0.;gl_Position.w=1.;}
#endif
