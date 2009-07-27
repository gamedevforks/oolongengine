attribute highp vec4	myVertex;
attribute highp vec4	myNormal;
attribute highp vec2	myTextureCoord;
attribute highp vec3	myTangent;

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

	mat3 worldToTangentSpace;
	vec4 tw = myTIMMatrix * vec4(myTangent,0);
	vec3 bi = cross( myTangent, myNormal.xyz );
	vec4 bw = myTIMMatrix * vec4(bi,0);
	vec4 nw = myTIMMatrix * myNormal;
	worldToTangentSpace[0] = tw.xyz;
	worldToTangentSpace[1] = bw.xyz;
	worldToTangentSpace[2] = nw.xyz;
	
	v_textureCoord = myTextureCoord;
	v_light = worldToTangentSpace * (-1.0 * light_direction);
	v_view = worldToTangentSpace * (myEye - pos_world.xyz);
}
