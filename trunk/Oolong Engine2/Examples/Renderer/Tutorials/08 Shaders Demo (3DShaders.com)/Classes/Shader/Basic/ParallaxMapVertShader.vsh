attribute highp vec4	myVertex;
attribute highp vec4	myNormal;
attribute highp vec2	myTextureCoord;
attribute highp vec3	myTangent;

uniform mediump mat4	myPMVMatrix;
uniform mediump mat4	u_TrasposeInverseModelMatrix;
uniform mediump mat4	u_ModelMatrix;

uniform highp	vec3	myEye;

uniform  vec3	light_direction;

varying  vec2	v_textureCoord;
varying  vec3	v_light;
varying  vec3	v_view;

void main(void)
{
    gl_Position = myPMVMatrix * myVertex;
	vec4 pos_world = u_ModelMatrix * myVertex;

	mat3 worldToTangentSpace;
	vec4 tw = u_TrasposeInverseModelMatrix * vec4(myTangent,0);
	vec3 bi = cross( myTangent, myNormal.xyz );
	vec4 bw = u_TrasposeInverseModelMatrix * vec4(bi,0);
	vec4 nw = u_TrasposeInverseModelMatrix * myNormal;
	worldToTangentSpace[0] = normalize(tw.xyz);
	worldToTangentSpace[1] = normalize(bw.xyz);
	worldToTangentSpace[2] = normalize(nw.xyz);
	
	v_textureCoord = myTextureCoord;
	v_light = worldToTangentSpace * normalize(-1.0 * light_direction);
	v_view = worldToTangentSpace * normalize(myEye - pos_world.xyz);
}
