precision mediump float;

varying vec2	v_TexCoord;
varying vec3	v_normal;
varying vec3	v_light;
varying vec3	v_view;

uniform sampler2D		s_texture;
uniform sampler2D		s_normalmap;
uniform  float	material_shininess;

uniform  vec4	ambient_ic;
uniform  vec4	diffuse_ic;
uniform  vec4	specular_ic;

void main(void)
{
	vec2 uv = v_TexCoord * 0.5;
	uv = uv - (0.5 * floor(uv/0.5));
	
	vec4 nx = texture2D(s_normalmap, uv) * 2.0;
	vec3 n = nx.xyz - vec3( 1.0, 1.0, 1.0 );
	n = n * vec3( -1, 1, 1 );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	
	uv = uv + vec2( 0.5, 0 );
	float height = texture2D(s_normalmap, uv).x;
	
	vec3 view = normalize(v_view);
	float shifty = -((view.x * height) * 0.0175) - 0.0;
	float shiftx = -((view.y * height) * 0.0175) - 0.0;
	uv = v_TexCoord - (1.0 * floor(v_TexCoord/1.0));
	uv = uv * 0.5;	
	vec2 newUV = vec2( uv.x + shiftx, uv.y + shifty );
	
	float xx = v_TexCoord.x - (1.0 * floor(v_TexCoord.x/1.0));
	vec4 color = texture2D(s_texture, newUV); // + vec4(xx,0,0,1);
	
	gl_FragColor = color * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic)); 
}
