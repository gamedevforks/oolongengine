attribute highp vec4	a_Vertex;
attribute highp vec2	a_MultiTexCoord0;
attribute highp vec4	a_Normal;
attribute highp vec4	a_Tangent;

uniform mediump mat4	u_ModelViewProjectionMatrix;
uniform mediump mat4	u_ModelViewMatrix;
uniform mediump mat4	u_TrasposeInverseModelMatrix;
uniform mediump mat4	u_ModelMatrix;



uniform highp	vec3	myEye;

uniform  vec3	light_direction;
uniform  vec3	light_position;

varying mediump vec2	v_TexCoord;
varying mediump vec3	v_normal;
varying mediump vec3	v_light;
varying mediump vec3	v_view;

varying float v_LightIntensity;
varying vec3  v_MCPosition;

void main(void)
{
	float SpecularContribution = 0.4;
    float diffusecontribution  = 1.0 - SpecularContribution;
	
	// compute the vertex position in eye coordinates
    vec3  ecPosition           = vec3(u_ModelViewProjectionMatrix * a_Vertex);
	
	// compute the transformed normal
    vec3  tnorm                = normalize(vec3(u_TrasposeInverseModelMatrix * a_Normal));
	
	// compute a vector from the model to the light position
    vec3  lightVec             = normalize(light_position - ecPosition);
	
	// compute the reflection vector
    vec3  reflectVec           = reflect(-lightVec, tnorm);
	
	// compute a unit vector in direction of viewing position
    vec3  viewVec              = normalize(myEye - ecPosition);
	
	// calculate amount of diffuse light based on normal and light angle
    float diffuse              = max(dot(lightVec, tnorm), 0.0);
    float spec                 = 0.0;
	
	// if there is diffuse lighting, calculate specular
    if(diffuse > 0.0)
	{
		spec = max(dot(reflectVec, viewVec), 0.0);
		spec = pow(spec, 16.0);
	}
	
    // add up the light sources, since this is a varying (global) it will pass to frag shader     
    v_LightIntensity  = diffusecontribution * diffuse * 1.5 +
                          SpecularContribution * spec;
						
	// the varying variable MCPosition will be used by the fragment shader to determine where
    //    in model space the current pixel is                      
    v_MCPosition      = vec3 (a_Vertex);					    

    // send vertex information
    gl_Position     = u_ModelViewProjectionMatrix * a_Vertex;




									
	vec4 pos_world = u_ModelMatrix * a_Vertex;

	vec4 normal = u_TrasposeInverseModelMatrix * a_Normal;
	v_normal = normalize( normal.xyz );

	v_TexCoord = a_MultiTexCoord0;
	v_light = -1.0 * light_direction;
	v_view = myEye - pos_world.xyz;
}
