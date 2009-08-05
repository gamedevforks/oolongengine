attribute highp vec4	a_Vertex;
attribute highp vec2	a_MultiTexCoord0;
attribute highp vec4	a_Normal;
attribute highp vec4	a_Tangent;

uniform mediump mat4	u_ModelViewProjectionMatrix;
uniform mediump mat4	u_TrasposeInverseModelMatrix;
uniform mediump mat4	u_ModelMatrix;



uniform highp	vec3	myEye;

uniform  vec3	light_direction;
uniform  vec3	light_position;

varying mediump vec2	v_TexCoord;
varying mediump vec3	v_normal;
varying mediump vec3	v_light;
varying mediump vec3	v_view;

varying vec3  v_MCPosition;

void main(void)
{
    gl_Position = u_ModelViewProjectionMatrix * a_Vertex;
	
	vec4 pos_world = u_ModelMatrix * a_Vertex;

	vec4 normal = u_TrasposeInverseModelMatrix * a_Normal;
	v_normal = normalize( normal.xyz );

	v_TexCoord = a_MultiTexCoord0;
	//v_light = -1.0 * light_direction;
	v_light = light_position - pos_world.xyz;
	v_view = myEye - pos_world.xyz;
	
	v_MCPosition      = vec3 (a_Vertex);
}
