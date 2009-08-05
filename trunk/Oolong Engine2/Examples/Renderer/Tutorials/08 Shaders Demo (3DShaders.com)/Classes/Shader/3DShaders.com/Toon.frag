//
// Fragment shader for cartoon-style shading
//
// Author: Philip Rideout
//
// Copyright (c) 2005-2006 3Dlabs Inc. Ltd.
//
// See 3Dlabs-License.txt for license information
//
precision mediump float;

varying vec3 v_Normal;

void main (void)
{
	vec3 DiffuseColor = vec3(0.0, 0.25, 1.0);
	vec3 PhongColor = vec3(0.75, 0.75, 1.0);
	float Edge = 0.5;
	float Phong = 0.98;

	vec3 n = normalize( v_Normal );
	
	vec3 color = DiffuseColor;
	float f = dot(vec3(0,0,1),n);
	if (abs(f) < Edge)
		color = vec3(0);
	if (f > Phong)
		color = PhongColor;

	gl_FragColor = vec4(color, 1);
}
