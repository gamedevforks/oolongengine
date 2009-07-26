attribute highp vec4	myVertex;
attribute highp vec2	myTexCoord;

uniform mediump mat4	myPMVMatrix;

varying mediump vec2	v_textureCoord;

void main(void)
{
    gl_Position = myPMVMatrix * myVertex;
	v_textureCoord = myTexCoord;
}
