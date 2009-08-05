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
	vec3 Spacing = vec3( 0.3, 0.3, 0.25 );
	float DotSize = 0.13;
	vec3 ModelColor = vec3( 0.75, 0.2, 0.1 );
	vec3 PolkaDotColor = vec3( 1.0, 1.0, 1.0 );
	
	float insidesphere, sphereradius, scaledpointlength;
	vec3 scaledpoint;

	vec3 n = normalize( v_normal );
	vec3 l = normalize( v_light );
	vec3 h = normalize( l + normalize( v_view ) );
	
	float diff = dot( n, l );
	float spec = pow( dot(n,h), material_shininess );
	

	// Scale the coordinate system
	// The following line of code is not yet implemented in current drivers:
	// mcpos = mod(Spacing, MCposition);
	// We will use a workaround found below for now
	scaledpoint       = v_MCPosition - (Spacing * floor(v_MCPosition/Spacing));
	
	// Bring the scaledpoint vector into the center of the scaled coordinate system
	scaledpoint       = scaledpoint - Spacing/2.0;

	// Find the length of the scaledpoint vector and compare it to the dotsize
	scaledpointlength = length(scaledpoint);
	insidesphere      = step(scaledpointlength,DotSize);
   
	// Determine final output color before lighting
	vec4 finalcolor        = vec4(mix(ModelColor, PolkaDotColor, insidesphere), 1.0);
	
	gl_FragColor = finalcolor * (ambient_ic + (diff * diffuse_ic) + (spec * specular_ic)); 
}
