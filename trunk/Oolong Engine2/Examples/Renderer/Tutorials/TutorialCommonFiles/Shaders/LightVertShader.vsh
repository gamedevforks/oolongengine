attribute highp vec4	myVertex;
attribute highp vec4	myNormal;
attribute highp vec2	myTextureCoord;

uniform mediump mat4	myPMVMatrix;
uniform mediump mat4	myTIMMatrix;
uniform mediump mat4	myModelMatrix;

uniform highp	vec3	myEye;

uniform  vec3	light_direction;

varying  vec2	v_textureCoord;
varying  vec3	v_normal;
varying  vec3	v_light;
varying  vec3	v_view;

void main(void)
{
    gl_Position = myPMVMatrix * myVertex;
	vec4 pos_world = myModelMatrix * myVertex;

	vec4 normal = myTIMMatrix * myNormal;
	v_normal = normalize( normal.xyz );

	v_textureCoord = myTextureCoord;
	v_light = -1.0 * light_direction;
	v_view = myEye - pos_world.xyz;
}
