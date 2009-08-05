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
	vec3 SkyColor = vec3(0.0, 0.0, 0.8);
	vec3 CloudColor = vec3(0.8, 0.8, 0.8);


	
    vec4 noisevec;
    float intensity;


	vec2 nxy = v_TexCoord;
	nxy.y -= u_offset.x * 2.0;
	nxy = nxy - (1.0 * floor(nxy/1.0));
	nxy = (nxy * 0.5) + vec2( 0.0, 0.5 );

	noisevec = texture2D(s_normalmap, nxy);
	intensity = noisevec.x;
	

	vec3 color   = mix(SkyColor, CloudColor, intensity);
	gl_FragColor = vec4(color,1); 
}
