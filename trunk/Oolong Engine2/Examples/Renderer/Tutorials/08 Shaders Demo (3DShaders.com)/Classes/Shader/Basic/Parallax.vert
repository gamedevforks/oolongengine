attribute highp vec4	a_Vertex;
attribute highp vec2	a_MultiTexCoord0;
attribute highp vec4	a_Normal;
attribute highp vec4	a_Tangent;
attribute highp vec4	a_Binormal;

uniform mediump mat4	u_ModelViewProjectionMatrix;
uniform mediump mat4	u_TrasposeInverseModelMatrix;
uniform mediump mat4	u_ModelMatrix;

uniform highp	vec3	myEye;

uniform  vec3	light_direction;

varying mediump vec2	v_TexCoord;
varying mediump vec3	v_light;
varying mediump vec3	v_view;

void main(void)
{
    gl_Position = u_ModelViewProjectionMatrix * a_Vertex;
	vec4 pos_world = u_ModelMatrix * a_Vertex;

	mat3 worldToTangentSpace;
	vec4 tw = u_TrasposeInverseModelMatrix * a_Tangent;
	vec4 nw = u_TrasposeInverseModelMatrix * a_Normal;
	vec4 bw = u_TrasposeInverseModelMatrix * a_Binormal;
	//vec4 bi = vec4(cross( a_Tangent.xyz, a_Normal.xyz ),0);
	//vec4 bw = u_TrasposeInverseModelMatrix * bi;
	worldToTangentSpace[0] = normalize(tw.xyz);
	worldToTangentSpace[1] = normalize(bw.xyz);
	worldToTangentSpace[2] = normalize(nw.xyz);
	
	v_TexCoord = a_MultiTexCoord0;
	v_light = worldToTangentSpace * normalize(-1.0 * light_direction);
	v_view = worldToTangentSpace * normalize(myEye - pos_world.xyz);
}
