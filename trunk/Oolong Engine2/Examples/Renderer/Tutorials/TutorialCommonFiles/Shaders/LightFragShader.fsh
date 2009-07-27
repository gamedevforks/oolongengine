precision mediump float;

varying  vec2	v_textureCoord;
varying  vec3	v_normal;
varying  vec3	v_light;
varying  vec3	v_view;

uniform sampler2D		s_texture;
uniform  float	material_shininess;

uniform  vec4	ambient_ic;
uniform  vec4	diffuse_ic;
uniform  vec4	specular_ic;

void main(void)
{
	vec3 n = normalize( v_normal );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	
	gl_FragColor = texture2D(s_texture, v_textureCoord) * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic)); 
}
