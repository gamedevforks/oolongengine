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
	vec3 BrickColor = vec3(0.7, 0.1, 0.0);
	vec3 MortarColor = vec3(0.85, 0.86, 0.84);
	vec2 BrickSize = vec2(0.30, 0.15);
	vec2 BrickPct = vec2(0.90, 0.85);


	vec3 n = normalize( v_normal );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	

	vec3 color;
	vec2 position;
	vec2 useBrick;

	position = v_MCPosition.xy / BrickSize;
	
	if (fract(position.y * 0.5) > 0.5)
		position.x += 0.5;

	position = fract(position);

	useBrick = step(position, BrickPct);

	color    = mix(MortarColor, BrickColor, useBrick.x * useBrick.y);
		
	gl_FragColor = vec4(color,1.0) * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic)); 
}
