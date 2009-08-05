attribute highp vec4	a_Vertex;
attribute highp vec2	a_MultiTexCoord0;

uniform mediump mat4	u_ModelViewProjectionMatrix;

varying mediump vec2	v_TexCoord;

void main(void)
{
    gl_Position = u_ModelViewProjectionMatrix * a_Vertex;
	v_TexCoord = a_MultiTexCoord0;
}
