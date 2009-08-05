precision mediump float;

uniform mediump vec4 u_Color;

varying mediump vec2	v_TexCoord;
varying vec3	v_normal;
varying vec3	v_light;
varying vec3	v_view;

uniform  vec4	ambient_ic;
uniform  vec4	diffuse_ic;
uniform  vec4	specular_ic;

uniform  float	material_shininess;

uniform float spacing;
uniform float transparency;

uniform sampler2D		s_texture;
uniform sampler2D		s_normalmap;

void main(void)
{
	vec3 n = normalize( v_normal );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	
	vec2 uv = v_TexCoord - (1.0 * floor(v_TexCoord/1.0));
	uv = (uv * 0.5) + vec2( 0.5, 0 );
	vec4 color = texture2D(s_texture, uv) * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic));

	uv = uv + vec2( 0, 0.5 );	
	float furValue = texture2D(s_normalmap, uv*vec2(spacing,spacing)).x;
	 
	gl_FragColor = vec4( color.xyz, transparency * furValue ); 
}
