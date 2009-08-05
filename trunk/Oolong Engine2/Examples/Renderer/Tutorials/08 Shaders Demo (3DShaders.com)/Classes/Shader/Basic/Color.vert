attribute highp vec4	a_Vertex;

uniform mediump mat4	u_ModelViewProjectionMatrix;

void main(void)
{
    gl_Position = u_ModelViewProjectionMatrix * a_Vertex;
}
