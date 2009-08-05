precision mediump float;

uniform mediump vec4 u_Color;

varying vec3	v_normal;
varying vec3	v_light;
varying vec3	v_view;

uniform  vec4	ambient_ic;
uniform  vec4	diffuse_ic;
uniform  vec4	specular_ic;

uniform  float	material_shininess;

uniform sampler2D		s_normalmap;

varying vec3  v_MCPosition;
varying mediump vec2	v_TexCoord;

uniform  vec3	u_offset;

void main(void)
{

	vec3 n = normalize( v_normal );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	
	vec2 nxy = u_offset.xy + v_TexCoord;
	nxy = nxy - (1.0 * floor(nxy/1.0));
	nxy = (nxy * 0.5) + vec2( 0.0, 0.5 );

	vec4 noise = texture2D(s_normalmap, nxy);
	vec4 color = vec4(0.2, 0.8, 0.4, 1.0);

	gl_FragColor = color * noise * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic)); 
}
