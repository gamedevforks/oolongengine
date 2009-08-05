precision mediump float;

uniform mediump vec4 u_Color;

varying vec3	v_normal;
varying vec3	v_light;
varying vec3	v_view;

uniform  vec4	ambient_ic;
uniform  vec4	diffuse_ic;
uniform  vec4	specular_ic;

uniform  float	material_shininess;

varying vec3  v_MCPosition;

void main(void)
{

	vec3 n = normalize( v_normal );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	
	gl_FragColor = u_Color * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic)); 
}
