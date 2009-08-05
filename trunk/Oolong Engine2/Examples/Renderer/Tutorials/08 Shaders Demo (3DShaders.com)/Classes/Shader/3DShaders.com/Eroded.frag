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

uniform sampler2D		s_texture;
uniform sampler2D		s_normalmap;

uniform mediump vec3	u_offset;

void main(void)
{
	vec3 n = normalize( v_normal );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	
    vec4 noisevec;
    vec3 color;
    float intensity;

    //noisevec = texture3D(sampler3d, 1.2 * (vec3 (0.5) + Position));
	vec2 nxy = v_TexCoord;
	nxy = nxy - (1.0 * floor(nxy/1.0));
	nxy = (nxy * 0.5) + vec2( 0.0, 0.5 );

	noisevec = texture2D(s_normalmap, nxy);
	intensity = noisevec.x;
	
    if (intensity <= 0.5+(u_offset.y/3.9)) 
		discard;


	vec2 uv = v_TexCoord - (1.0 * floor(v_TexCoord/1.0));
	uv = uv * 0.5;
	vec4 cx = texture2D(s_texture, uv);
	gl_FragColor = cx * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic)); 
}
