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

uniform  vec3	u_offset;

void main(void)
{
	// Constants
	const float C_PI    = 3.1415;
	const float C_2PI   = 2.0 * C_PI;
	const float C_2PI_I = 1.0 / (2.0 * C_PI);
	const float C_PI_2  = C_PI / 2.0;

	float StartRad = u_offset.x * 10.0;
	vec2  Freq = vec2(4.0, 4.0);
	vec2  Amplitude = vec2(0.05, 0.05);

	vec3 n = normalize( v_normal );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	
	vec2 uv = v_TexCoord; // - (1.0 * floor(v_TexCoord/1.0));
	
	vec2  perturb;
    float rad;
    vec4  color;

    // Compute a perturbation factor for the x-direction
    rad = (uv.x + uv.y - 1.0 + StartRad) * Freq.x;

    // Wrap to -2.0*PI, 2*PI
    rad = rad * C_2PI_I;
    rad = fract(rad);
    rad = rad * C_2PI;

    // Center in -PI, PI
    if (rad >  C_PI) rad = rad - C_2PI;
    if (rad < -C_PI) rad = rad + C_2PI;

    // Center in -PI/2, PI/2
    if (rad >  C_PI_2) rad =  C_PI - rad;
    if (rad < -C_PI_2) rad = -C_PI - rad;

    perturb.x  = (rad - (rad * rad * rad / 6.0)) * Amplitude.x;

    // Now compute a perturbation factor for the y-direction
    rad = (uv.x - uv.y + StartRad) * Freq.y;

    // Wrap to -2*PI, 2*PI
    rad = rad * C_2PI_I;
    rad = fract(rad);
    rad = rad * C_2PI;

    // Center in -PI, PI
    if (rad >  C_PI) rad = rad - C_2PI;
    if (rad < -C_PI) rad = rad + C_2PI;

    // Center in -PI/2, PI/2
    if (rad >  C_PI_2) rad =  C_PI - rad;
    if (rad < -C_PI_2) rad = -C_PI - rad;

    perturb.y  = (rad - (rad * rad * rad / 6.0)) * Amplitude.y;

    //color = texture2D(WobbleTex, perturb + gl_TexCoord[0].st);
	
	
	uv = uv + perturb;
	uv = uv - (1.0 * floor(uv/1.0));
	uv = uv * 0.5;
	
	gl_FragColor = texture2D(s_texture, uv) * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic)); 
}
