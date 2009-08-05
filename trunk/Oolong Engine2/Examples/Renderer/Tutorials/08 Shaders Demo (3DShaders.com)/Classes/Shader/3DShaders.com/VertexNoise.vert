attribute highp vec4	a_Vertex;
attribute highp vec2	a_MultiTexCoord0;
attribute highp vec4	a_Normal;
attribute highp vec4	a_Tangent;

uniform mediump mat4	u_ModelViewProjectionMatrix;
uniform mediump mat4	u_TrasposeInverseModelMatrix;
uniform mediump mat4	u_ModelMatrix;

uniform sampler2D		s_normalmap;

uniform highp	vec3	myEye;

uniform  vec3	light_direction;
uniform  vec3	light_position;

uniform mediump vec3	u_offset;

varying mediump vec2	v_TexCoord;
varying mediump vec3	v_normal;
varying mediump vec3	v_light;
varying mediump vec3	v_view;

varying vec3  v_MCPosition;

void main(void)
{
	vec2 nxy = a_MultiTexCoord0 + u_offset.xy;
	nxy = nxy - (1.0 * floor(nxy/1.0));
	nxy = (nxy * 0.5) + vec2( 0.0, 0.5 );

	vec4 noise = texture2D(s_normalmap, nxy);

	vec3 vertex = a_Vertex.xyz * (0.7+(noise.xyz * 0.6));
    gl_Position = u_ModelViewProjectionMatrix * vec4(vertex,1.0);
	
	vec4 pos_world = u_ModelMatrix * a_Vertex;

	vec4 normal = u_TrasposeInverseModelMatrix * a_Normal;
	v_normal = normalize( normal.xyz );

	v_TexCoord = a_MultiTexCoord0;
	//v_light = -1.0 * light_direction;
	v_light = light_position - pos_world.xyz;
	v_view = myEye - pos_world.xyz;
	
	v_MCPosition      = vec3 (a_Vertex);
}
