attribute highp vec4	myVertex;
attribute highp vec4	myColor;

uniform mediump mat4	myPMVMatrix;

varying mediump vec4 v_color;

void main(void)
{
	v_color = myColor;
    gl_Position = myPMVMatrix * myVertex;
}
 