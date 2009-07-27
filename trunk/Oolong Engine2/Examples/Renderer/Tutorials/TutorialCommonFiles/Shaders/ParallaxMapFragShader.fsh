precision mediump float;

varying  vec2	v_textureCoord;
varying  vec3	v_light;
varying  vec3	v_view;

uniform sampler2D		s_texture;
uniform sampler2D		s_normalmap;
uniform  float	material_shininess;

uniform  vec4	ambient_ic;
uniform  vec4	diffuse_ic;
uniform  vec4	specular_ic;

void main(void)
{
	vec4 nx = texture2D(s_normalmap, (v_textureCoord * 0.5)) * 2.0;
	vec3 n = nx.xyz - vec3( 1.0, 1.0, 1.0 );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float height = texture2D(s_normalmap, (v_textureCoord * 0.5)+vec2(0.5,0.0)).x;
	height = (height * 0.06)-0.03;
	vec2 newUV = v_textureCoord - (height * normalize(v_view.xz));
	
	vec4 color = texture2D(s_texture, newUV);
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	
	gl_FragColor = color * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic)); 
}
